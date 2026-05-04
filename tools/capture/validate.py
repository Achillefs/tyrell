#!/usr/bin/env python3
"""Validate every session under $VP330_CAPTURES_DIR/sessions/."""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path

import jsonschema
import mido
import soundfile as sf


SCHEMA_PATH = Path(__file__).parent / "schema.json"


def resolve_captures_dir(arg: str | None) -> Path:
    """
    Resolve and validate the captures root directory.
    
    If `arg` is provided use it; otherwise read `VP330_CAPTURES_DIR` from the environment. Exits the process with an error message if neither is set or if the resolved path does not exist.
    
    Parameters:
        arg (str | None): Optional path provided via the CLI `--root` argument.
    
    Returns:
        Path: An expanded Path object pointing to an existing captures root directory.
    """
    raw = arg or os.environ.get("VP330_CAPTURES_DIR")
    if not raw:
        sys.exit("error: pass --root or set $VP330_CAPTURES_DIR.")
    p = Path(raw).expanduser()
    if not p.exists():
        sys.exit(f"error: captures root does not exist: {p}")
    if not p.is_dir():
        sys.exit(f"error: captures root is not a directory: {p}")
    return p


def _safe_session_subpath(session_dir: Path, name: str) -> tuple[Path | None, str | None]:
    """Resolve `session_dir / name` and ensure it stays inside `session_dir`.

    Returns (path, None) on success, (None, error_message) on rejection.
    """
    if not name:
        return None, "name is empty"
    candidate = Path(name)
    if candidate.is_absolute() or any(part == ".." for part in candidate.parts):
        return None, f"path escapes session dir: {name}"
    resolved_root = session_dir.resolve()
    resolved = (session_dir / name).resolve()
    try:
        resolved.relative_to(resolved_root)
    except ValueError:
        return None, f"path escapes session dir: {name}"
    return resolved, None


_SUBTYPE_TO_BIT_DEPTH = {
    "PCM_24": 24,
    "PCM_32": 32,
    "FLOAT": "float32",
    "DOUBLE": "float64",
}


def midi_duration_seconds(path: Path) -> float:
    """
    Compute the duration of a MIDI file in seconds.
    
    Returns:
        duration_seconds (float): Duration of the MIDI file in seconds.
    """
    mid = mido.MidiFile(str(path))
    return float(mid.length)


def validate_session(session_dir: Path, schema: dict) -> list[str]:
    """
    Validate a session directory's manifest, MIDI file, and audio file against the provided JSON schema.
    
    Performs these checks and collects human-readable issue strings:
    - Manifest presence and JSON validity (returns ["missing manifest.json"] or a JSON parse error).
    - Schema validation of the manifest (appends schema error messages with JSON path).
    - Existence and parseability of the MIDI input file.
    - Existence and readable metadata of the audio output file.
    - Audio properties vs. manifest/audio expectations: sample rate (must be >= 48000), acceptable subtype (PCM_24, PCM_32, FLOAT, DOUBLE), channel count, declared duration tolerance (±0.5s), and that audio is not significantly shorter than MIDI (0.1s tolerance).
    
    Parameters:
        session_dir (Path): Path to the session directory containing manifest.json and media files.
        schema (dict): JSON Schema used to validate the manifest contents.
    
    Returns:
        list[str]: A list of issue messages found during validation. An empty list indicates no issues.
    """
    issues: list[str] = []
    manifest_path = session_dir / "manifest.json"

    if not manifest_path.exists():
        return ["missing manifest.json"]

    try:
        manifest = json.loads(manifest_path.read_text())
    except json.JSONDecodeError as e:
        return [f"manifest.json is not valid JSON: {e}"]

    try:
        jsonschema.validate(manifest, schema)
    except jsonschema.ValidationError as e:
        issues.append(f"schema: {e.message} (at {'/'.join(str(p) for p in e.absolute_path)})")

    midi_name = manifest.get("midi_input", "input.mid")
    audio_name = manifest.get("audio_output", "output.wav")
    midi_path, midi_err = _safe_session_subpath(session_dir, str(midi_name) if isinstance(midi_name, str) else "")
    audio_path, audio_err = _safe_session_subpath(session_dir, str(audio_name) if isinstance(audio_name, str) else "")
    if midi_err:
        issues.append(f"midi_input: {midi_err}")
    if audio_err:
        issues.append(f"audio_output: {audio_err}")

    midi_dur: float | None = None
    if midi_path is None:
        pass  # already reported via midi_err
    elif not midi_path.exists():
        issues.append(f"missing midi_input file: {midi_name}")
    else:
        try:
            midi_dur = midi_duration_seconds(midi_path)
        except Exception as e:
            issues.append(f"midi parse failed: {e}")

    if audio_path is None:
        return issues
    if not audio_path.exists():
        issues.append(f"missing audio_output file: {audio_name}")
        return issues

    try:
        info = sf.info(str(audio_path))
    except Exception as e:
        issues.append(f"audio open failed: {e}")
        return issues

    # The manifest may have failed schema validation above; treat fields
    # defensively so a malformed manifest still produces a useful report
    # instead of a Python traceback.
    audio_meta_raw = manifest.get("audio", {})
    audio_meta = audio_meta_raw if isinstance(audio_meta_raw, dict) else {}
    if not isinstance(audio_meta_raw, dict):
        issues.append("schema: audio must be an object")

    if info.samplerate < 48000:
        issues.append(f"audio sample_rate {info.samplerate} < 48000")
    if info.subtype not in {"PCM_24", "PCM_32", "FLOAT", "DOUBLE"}:
        issues.append(f"audio bit depth too low: {info.subtype} (need PCM_24+ or FLOAT)")

    declared_sr = audio_meta.get("sample_rate")
    if isinstance(declared_sr, int) and info.samplerate != declared_sr:
        issues.append(f"audio sample_rate {info.samplerate} != manifest {declared_sr}")

    declared_bd = audio_meta.get("bit_depth")
    actual_bd = _SUBTYPE_TO_BIT_DEPTH.get(info.subtype)
    if declared_bd is not None and actual_bd is not None and actual_bd != declared_bd:
        issues.append(f"audio bit_depth {actual_bd!r} != manifest {declared_bd!r}")

    declared_channels = audio_meta.get("channels")
    if isinstance(declared_channels, int) and info.channels != declared_channels:
        issues.append(f"audio channels {info.channels} != manifest {declared_channels}")

    actual_duration = info.frames / info.samplerate
    declared_duration = audio_meta.get("duration_seconds")
    # duration_seconds may be null for unrecorded sessions; skip the check
    # then. After a schema failure it may also be a non-numeric type, in
    # which case flag it explicitly rather than crashing on abs(...).
    if isinstance(declared_duration, (int, float)) and not isinstance(declared_duration, bool):
        if abs(actual_duration - float(declared_duration)) > 0.5:
            issues.append(
                f"audio duration {actual_duration:.3f}s vs declared {float(declared_duration):.3f}s "
                f"(>0.5s tolerance)"
            )
    elif declared_duration is not None:
        issues.append(
            f"audio duration_seconds has unsupported type: {type(declared_duration).__name__}"
        )

    if midi_dur is not None and actual_duration < midi_dur - 0.1:
        issues.append(
            f"audio duration {actual_duration:.3f}s is shorter than midi {midi_dur:.3f}s"
        )

    return issues


def main() -> int:
    """
    Validate each session directory under the captures root, print per-session results, and exit with a status summarizing validation.
    
    Reads the JSON schema from SCHEMA_PATH, resolves the captures root from the --root argument or the VP330_CAPTURES_DIR environment variable, and iterates the directories in <root>/sessions. For each session it prints "OK" when validation passes or "FAIL" followed by a list of issues when validation fails, then prints a final summary line.
    
    Returns:
        int: 0 if all sessions are valid, 1 if any session failed validation.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=None, help="captures root (defaults to $VP330_CAPTURES_DIR)")
    args = parser.parse_args()

    root = resolve_captures_dir(args.root)
    sessions_root = root / "sessions"
    if not sessions_root.exists():
        print(f"no sessions/ under {root} — nothing to validate")
        return 0

    schema = json.loads(SCHEMA_PATH.read_text())

    session_dirs = sorted(p for p in sessions_root.iterdir() if p.is_dir())
    if not session_dirs:
        print(f"no sessions in {sessions_root}")
        return 0

    failures = 0
    for d in session_dirs:
        issues = validate_session(d, schema)
        if issues:
            failures += 1
            print(f"FAIL: {d.name}")
            for i in issues:
                print(f"  - {i}")
        else:
            print(f"OK:   {d.name}")

    print()
    print(f"{len(session_dirs) - failures}/{len(session_dirs)} sessions valid")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
