#!/usr/bin/env python3
"""Generate tests/golden/fixtures/silence-1s.mid: a 1-second MIDI file with no notes."""

import argparse
import struct
from pathlib import Path


def variable_length_quantity(value: int) -> bytes:
    if value == 0:
        return b"\x00"
    parts = []
    while value:
        parts.append(value & 0x7F)
        value >>= 7
    parts.reverse()
    return bytes((p | 0x80) for p in parts[:-1]) + bytes([parts[-1]])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, 480)
    tempo_event = b"\x00\xFF\x51\x03" + b"\x07\xA1\x20"
    end_of_track = variable_length_quantity(1920) + b"\xFF\x2F\x00"
    track_data = tempo_event + end_of_track
    track = b"MTrk" + struct.pack(">I", len(track_data)) + track_data

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(header + track)
    print(f"wrote {args.output} ({len(header) + len(track)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
