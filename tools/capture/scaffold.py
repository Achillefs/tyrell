#!/usr/bin/env python3
"""Create a new VP-330 capture session directory under $VP330_CAPTURES_DIR/sessions."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import sys
from pathlib import Path


SLUG_RE = re.compile(r"^[a-z0-9][a-z0-9-]*$")


def manifest_template(session_date: str, session_name: str) -> dict:
    """
    Create a manifest dictionary populated with session metadata and default capture settings.
    
    Parameters:
        session_date (str): Session date in ISO format `YYYY-MM-DD`.
        session_name (str): Session slug/name to record in the manifest.
    
    Returns:
        dict: Manifest with keys:
            - `revision` (str)
            - `session_date` (str)
            - `session_name` (str)
            - `midi_input` (str)
            - `audio_output` (str)
            - `audio` (dict): contains `sample_rate` (int), `bit_depth` (int), `channels` (int), `duration_seconds` (float or None)
            - `instrument_settings` (dict): contains instrument-related fields and toggles
            - `signal_chain` (list): ordered signal chain descriptions
            - `room_conditions` (str)
            - `anomalies` (list)
    """
    return {
        "revision": "MkII",
        "session_date": session_date,
        "session_name": session_name,
        "midi_input": "input.mid",
        "audio_output": "output.wav",
        "audio": {
            "sample_rate": 48000,
            "bit_depth": 24,
            "channels": 2,
            "duration_seconds": None,
        },
        "instrument_settings": {
            "section": "choir",
            "choir_variant": None,
            "male_switch": False,
            "female_switch": False,
            "ensemble_on": False,
            "vibrato_rate": None,
            "vibrato_depth": None,
            "tone": None,
            "balance": None,
        },
        "signal_chain": ["TODO: describe the signal chain"],
        "room_conditions": "",
        "anomalies": [],
    }


def resolve_captures_dir() -> Path:
    """
    Resolve the VP-330 captures root directory from the VP330_CAPTURES_DIR environment variable.
    
    Expands a leading `~` and verifies that the resulting path exists. If the environment
    variable is unset or the resolved path does not exist, the process exits with an
    error message.
    
    Returns:
        Path: The resolved captures directory path.
    
    Raises:
        SystemExit: If `VP330_CAPTURES_DIR` is not set or the resolved path does not exist.
    """
    raw = os.environ.get("VP330_CAPTURES_DIR")
    if not raw:
        sys.exit("error: $VP330_CAPTURES_DIR is not set. Point it at your captures repo.")
    p = Path(raw).expanduser()
    if not p.exists():
        sys.exit(f"error: $VP330_CAPTURES_DIR does not exist: {p}")
    return p


def main() -> int:
    """
    Create a new VP-330 capture session directory and populate it with a manifest and notes.
    
    Creates the session directory at "$VP330_CAPTURES_DIR/sessions/{YYYY-MM-DD}-{name}", where `name` is validated against the slug pattern (lowercase letters, digits, hyphens). Writes a pretty-printed `manifest.json` from the manifest template and a `notes.md` with a header and placeholder text, then prints the created path and next steps.
    
    Returns:
        int: Exit code `0` on success.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True, help="Session slug, e.g. choir-chromatic-variant-A-ensemble-off")
    parser.add_argument("--date", default=None, help="Session date YYYY-MM-DD (defaults to today)")
    args = parser.parse_args()

    if not SLUG_RE.match(args.name):
        sys.exit(f"error: --name must match {SLUG_RE.pattern} (lowercase, digits, hyphens)")

    session_date = args.date or dt.date.today().isoformat()
    try:
        dt.date.fromisoformat(session_date)
    except ValueError:
        sys.exit(f"error: --date must be ISO 8601 (YYYY-MM-DD), got {session_date!r}")

    captures = resolve_captures_dir()
    session_dir = captures / "sessions" / f"{session_date}-{args.name}"
    if session_dir.exists():
        sys.exit(f"error: session already exists: {session_dir}")

    session_dir.mkdir(parents=True)
    (session_dir / "manifest.json").write_text(
        json.dumps(manifest_template(session_date, args.name), indent=2) + "\n"
    )
    (session_dir / "notes.md").write_text(f"# {args.name}\n\n_(free-form observations from the session)_\n")

    print(f"Created: {session_dir}")
    print("Next: drop input.mid, record output.wav (or use driver.py), edit manifest.json + notes.md.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
