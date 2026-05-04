#!/usr/bin/env python3
"""End-to-end VP-330 capture: send MIDI to the unit, record audio, write a session dir."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

import mido
import numpy as np
import sounddevice as sd
import soundfile as sf

# Re-use scaffold helpers without duplication.
HERE = Path(__file__).parent
sys.path.insert(0, str(HERE))
from scaffold import manifest_template, resolve_captures_dir, SLUG_RE  # noqa: E402


def list_and_pick_midi_out(preselect: str | None) -> str:
    names = mido.get_output_names()
    if not names:
        sys.exit("error: no MIDI output ports available.")
    if preselect:
        for n in names:
            if preselect.lower() in n.lower():
                return n
        sys.exit(f"error: --midi-out {preselect!r} did not match any of: {names}")
    print("MIDI output ports:")
    for i, n in enumerate(names):
        print(f"  [{i}] {n}")
    idx = int(input("Pick MIDI output index: ").strip())
    return names[idx]


def list_and_pick_audio_in(preselect: str | None) -> int:
    devices = sd.query_devices()
    inputs = [(i, d) for i, d in enumerate(devices) if d["max_input_channels"] > 0]
    if not inputs:
        sys.exit("error: no audio input devices available.")
    if preselect:
        for i, d in inputs:
            if preselect.lower() in d["name"].lower():
                return i
        sys.exit(f"error: --audio-in {preselect!r} did not match any input device.")
    print("Audio input devices:")
    for i, d in inputs:
        print(f"  [{i}] {d['name']} (max in: {d['max_input_channels']})")
    return int(input("Pick audio input index: ").strip())


class RingRecorder:
    """Records to a list of np arrays via the sounddevice callback. Tracks live RMS."""

    def __init__(self, sample_rate: int, channels: int):
        self.sample_rate = sample_rate
        self.channels = channels
        self._chunks: list[np.ndarray] = []
        self._lock = threading.Lock()
        self._latest_rms_db = -120.0

    def _callback(self, indata, frames, time_info, status):  # noqa: ANN001
        if status:
            print(f"[audio status] {status}", file=sys.stderr)
        block = indata.copy()
        with self._lock:
            self._chunks.append(block)
        rms = float(np.sqrt(np.mean(block.astype(np.float32) ** 2) + 1e-30))
        self._latest_rms_db = 20.0 * np.log10(rms + 1e-30)

    def stream(self, device: int):
        return sd.InputStream(
            device=device,
            channels=self.channels,
            samplerate=self.sample_rate,
            dtype="float32",
            callback=self._callback,
        )

    @property
    def rms_db(self) -> float:
        return self._latest_rms_db

    def collect(self) -> np.ndarray:
        with self._lock:
            if not self._chunks:
                return np.zeros((0, self.channels), dtype=np.float32)
            return np.concatenate(self._chunks, axis=0)


def play_midi(port_name: str, midi_path: Path) -> None:
    midi = mido.MidiFile(str(midi_path))
    with mido.open_output(port_name) as port:
        for msg in midi.play():
            port.send(msg)


def wait_for_silence(rec: RingRecorder, *, tail_min: float, thresh_db: float,
                     window_s: float, max_tail: float) -> None:
    start = time.monotonic()
    deadline = start + max_tail
    # Mandatory minimum tail.
    time.sleep(tail_min)
    # Then poll RMS; break when we've been below threshold for window_s seconds.
    below_since: float | None = None
    while time.monotonic() < deadline:
        if rec.rms_db < thresh_db:
            if below_since is None:
                below_since = time.monotonic()
            elif time.monotonic() - below_since >= window_s:
                return
        else:
            below_since = None
        time.sleep(0.05)


def open_editor(path: Path) -> None:
    editor = os.environ.get("EDITOR")
    if not editor:
        return
    try:
        subprocess.run([editor, str(path)], check=False)
    except FileNotFoundError:
        pass


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument("--midi", required=True, type=Path, help="Fixture MIDI path")
    parser.add_argument("--midi-out", default=None)
    parser.add_argument("--audio-in", default=None)
    parser.add_argument("--sample-rate", type=int, default=48000)
    parser.add_argument("--channels", type=int, default=2)
    parser.add_argument("--bit-depth", type=int, default=24, choices=[24, 32])
    parser.add_argument("--tail-min", type=float, default=5.0)
    parser.add_argument("--silence-thresh-db", type=float, default=-50.0)
    parser.add_argument("--silence-window", type=float, default=1.0)
    parser.add_argument("--max-tail", type=float, default=30.0)
    parser.add_argument("--no-editor", action="store_true")
    args = parser.parse_args()

    if not SLUG_RE.match(args.name):
        sys.exit(f"error: --name must match {SLUG_RE.pattern}")
    if not args.midi.exists():
        sys.exit(f"error: --midi does not exist: {args.midi}")

    captures = resolve_captures_dir()
    session_date = dt.date.today().isoformat()
    session_dir = captures / "sessions" / f"{session_date}-{args.name}"
    if session_dir.exists():
        sys.exit(f"error: session already exists: {session_dir}")
    session_dir.mkdir(parents=True)

    shutil.copyfile(args.midi, session_dir / "input.mid")

    midi_port = list_and_pick_midi_out(args.midi_out)
    audio_dev = list_and_pick_audio_in(args.audio_in)

    rec = RingRecorder(args.sample_rate, args.channels)
    print(f"Opening audio input #{audio_dev} @ {args.sample_rate} Hz / {args.channels} ch ...")
    with rec.stream(audio_dev):
        time.sleep(0.25)  # warmup
        print(f"Sending MIDI from {args.midi.name} → {midi_port} ...")
        play_midi(midi_port, args.midi)
        print(f"Tail: min {args.tail_min}s, then silence < {args.silence_thresh_db} dB for "
              f"{args.silence_window}s, hard cap {args.max_tail}s")
        wait_for_silence(
            rec,
            tail_min=args.tail_min,
            thresh_db=args.silence_thresh_db,
            window_s=args.silence_window,
            max_tail=args.max_tail,
        )

    audio = rec.collect()
    duration = float(audio.shape[0] / args.sample_rate)
    out_wav = session_dir / "output.wav"
    subtype = "PCM_24" if args.bit_depth == 24 else "PCM_32"
    sf.write(str(out_wav), audio, args.sample_rate, subtype=subtype)
    print(f"Wrote {out_wav} ({duration:.3f}s)")

    manifest = manifest_template(session_date, args.name)
    manifest["audio"] = {
        "sample_rate": args.sample_rate,
        "bit_depth": args.bit_depth,
        "channels": args.channels,
        "duration_seconds": round(duration, 3),
    }
    (session_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    notes = session_dir / "notes.md"
    notes.write_text(f"# {args.name}\n\n_(free-form observations from the session)_\n")

    print(f"\nSession ready: {session_dir}")
    print("Edit manifest.json to fill in instrument_settings, signal_chain, room_conditions.")

    if not args.no_editor:
        open_editor(notes)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
