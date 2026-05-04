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
    """
    Select a MIDI output port name from the available system MIDI outputs.
    
    If `preselect` is provided, returns the first port whose name contains that substring (case-insensitive); if no match is found the function exits the process with an error. If `preselect` is not provided, prints the available ports with indices and prompts the user to enter an index, then returns the corresponding port name. If no output ports are available the function exits the process with an error.
    
    Parameters:
        preselect (str | None): Optional substring used to automatically choose a port by name.
    
    Returns:
        port_name (str): The selected MIDI output port name.
    """
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
    raw = input("Pick MIDI output index: ").strip()
    try:
        idx = int(raw)
        return names[idx]
    except (ValueError, IndexError):
        sys.exit(f"error: invalid MIDI output index {raw!r} (expected 0..{len(names) - 1})")


def list_and_pick_audio_in(preselect: str | None) -> int:
    """
    Select an audio input device index, optionally chosen by substring match.
    
    When `preselect` is provided, returns the index of the first input device whose name contains
    `preselect` case-insensitively. Without `preselect`, prints available input devices with their
    indices and prompts the user to enter an index.
    
    Parameters:
    	preselect (str | None): Optional case-insensitive substring to auto-select a device by name.
    
    Returns:
    	int: The selected audio input device index.
    
    Notes:
    	Exits the process with an error message if no input devices are available or if `preselect`
    	does not match any device.
    """
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
    raw = input("Pick audio input index: ").strip()
    valid_indices = {i for i, _ in inputs}
    try:
        idx = int(raw)
    except ValueError:
        sys.exit(f"error: invalid audio input index {raw!r}")
    if idx not in valid_indices:
        sys.exit(f"error: audio input index {idx} is not in {sorted(valid_indices)}")
    return idx


class RingRecorder:
    """Records to a list of np arrays via the sounddevice callback. Tracks live RMS."""

    def __init__(self, sample_rate: int, channels: int):
        """
        Initialize a RingRecorder that accumulates incoming audio blocks and maintains a running RMS level in dB.
        
        Parameters:
            sample_rate (int): Sampling rate in Hz used for the input stream.
            channels (int): Number of audio channels per frame.
        
        Initial state:
            - _chunks: empty list to store copied input blocks.
            - _lock: threading.Lock protecting _chunks.
            - _latest_rms_db: initial estimated RMS level set to -120.0 dB.
        """
        self.sample_rate = sample_rate
        self.channels = channels
        self._chunks: list[np.ndarray] = []
        self._lock = threading.Lock()
        self._latest_rms_db = -120.0

    def _callback(self, indata, frames, time_info, status):  # noqa: ANN001
        """
        SoundDevice input-stream callback: records the incoming block, updates the running RMS level in dB, and reports any stream status.
        
        Parameters:
            indata (numpy.ndarray): Incoming audio block shaped (frames, channels), dtype float32 or similar; this buffer is copied before storage.
            frames (int): Number of frames in `indata`.
            time_info (dict): Timing information provided by sounddevice (e.g., input/output timestamps).
            status (sounddevice.CallbackFlags): Stream status flags; if non-empty they are printed to stderr.
        
        Side effects:
            - Appends a copy of `indata` to the recorder's internal chunk list (thread-safe).
            - Updates the recorder's `_latest_rms_db` with the block's RMS converted to decibels.
            - Prints `status` to stderr when present.
        """
        if status:
            print(f"[audio status] {status}", file=sys.stderr)
        block = indata.copy()
        with self._lock:
            self._chunks.append(block)
        rms = float(np.sqrt(np.mean(block.astype(np.float32) ** 2) + 1e-30))
        self._latest_rms_db = 20.0 * np.log10(rms + 1e-30)

    def stream(self, device: int):
        """
        Create and return a configured sounddevice InputStream bound to this recorder.
        
        Parameters:
            device (int): Index or identifier of the audio input device to open.
        
        Returns:
            sd.InputStream: An InputStream configured with this recorder's sample rate, channel count, float32 dtype, and the internal callback that captures incoming audio.
        """
        return sd.InputStream(
            device=device,
            channels=self.channels,
            samplerate=self.sample_rate,
            dtype="float32",
            callback=self._callback,
        )

    @property
    def rms_db(self) -> float:
        """
        Current estimated RMS level expressed in decibels.
        
        Returns:
            float: Most recently computed RMS level in dB (higher = louder). Initialized to -120.0 before any audio has been processed.
        """
        return self._latest_rms_db

    def collect(self) -> np.ndarray:
        """
        Concatenate and return all captured audio chunks as a single NumPy array.
        
        If no audio has been captured yet, returns a zero-length array with shape (0, channels) and dtype float32.
        
        Returns:
            np.ndarray: Array of shape (N, channels) containing the recorded samples as float32, where N is the total number of frames (0 if no chunks).
        """
        with self._lock:
            if not self._chunks:
                return np.zeros((0, self.channels), dtype=np.float32)
            return np.concatenate(self._chunks, axis=0)


def play_midi(port_name: str, midi_path: Path) -> None:
    """
    Send all messages from a MIDI file to the specified MIDI output port, preserving the file's timing.
    
    Parameters:
        port_name (str): Name of the MIDI output port to open.
        midi_path (Path): Path to the MIDI file whose messages will be played.
    """
    midi = mido.MidiFile(str(midi_path))
    with mido.open_output(port_name) as port:
        for msg in midi.play():
            port.send(msg)


def wait_for_silence(rec: RingRecorder, *, tail_min: float, thresh_db: float,
                     window_s: float, max_tail: float) -> None:
    """Wait for `rec.rms_db` to stay below `thresh_db` for `window_s` seconds.

    Honors a mandatory `tail_min` minimum wait before silence detection begins,
    and a hard `max_tail` ceiling before returning unconditionally.
    """
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
    """
    Open the given file in the editor specified by the `EDITOR` environment variable, if one is configured and available.
    
    If `EDITOR` is unset or empty, this function does nothing. If the configured editor executable is not found, the function silently returns without raising an exception. The editor is invoked with the file path as its sole argument and the editor's exit status is not checked.
    
    Parameters:
        path (Path): Path to the file to open in the editor.
    """
    editor = os.environ.get("EDITOR")
    if not editor:
        return
    try:
        subprocess.run([editor, str(path)], check=False)
    except FileNotFoundError:
        pass


def main() -> int:
    """
    Run the capture session command-line interface and perform an end-to-end VP-330 capture.
    
    This function parses CLI arguments, validates inputs, creates a dated session directory, copies
    the provided MIDI file into the session, prompts or selects MIDI output and audio input devices,
    records audio while sending the MIDI performance, waits for a configured silence tail, writes
    the recorded audio and a JSON manifest into the session directory, creates session notes, and
    optionally opens the notes in the user's editor.
    
    Returns:
        int: Exit code (0 on success).
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument("--midi", required=True, type=Path, help="Fixture MIDI path")
    parser.add_argument("--midi-out", default=None)
    parser.add_argument("--audio-in", default=None)
    parser.add_argument("--sample-rate", type=int, default=48000)
    parser.add_argument("--channels", type=int, default=2, choices=[1, 2])
    parser.add_argument("--bit-depth", type=int, default=24, choices=[24, 32])
    parser.add_argument("--tail-min", type=float, default=5.0)
    parser.add_argument("--silence-thresh-db", type=float, default=-50.0)
    parser.add_argument("--silence-window", type=float, default=1.0)
    parser.add_argument("--max-tail", type=float, default=30.0)
    parser.add_argument("--no-editor", action="store_true")
    args = parser.parse_args()

    # Enforce schema-compatible bounds before we record so the resulting
    # session can't be rejected later by validate.py for reasons we could
    # have caught here.
    if args.sample_rate < 48000:
        parser.error("--sample-rate must be >= 48000 (schema minimum)")
    if args.tail_min < 0:
        parser.error("--tail-min must be >= 0")
    if args.silence_window <= 0:
        parser.error("--silence-window must be > 0")
    if args.max_tail <= 0:
        parser.error("--max-tail must be > 0")
    if args.max_tail < args.tail_min:
        parser.error("--max-tail must be >= --tail-min")

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
