#!/usr/bin/env python3
"""Generate tests/golden/fixtures/single-c4-1s.mid: NoteOn C4 at t=0, NoteOff at +960 ticks."""

import argparse
import struct
from pathlib import Path


def vlq(value: int) -> bytes:
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

    # 120 BPM tempo at t=0
    tempo = b"\x00" + b"\xFF\x51\x03" + b"\x07\xA1\x20"
    # NoteOn C4 vel=100 at t=0
    note_on = b"\x00" + bytes([0x90, 60, 100])
    # NoteOff C4 at +960 ticks (1.0 s at 120 BPM / 480 PPQN)
    note_off = vlq(960) + bytes([0x80, 60, 0])
    eot = b"\x00" + b"\xFF\x2F\x00"

    track_data = tempo + note_on + note_off + eot
    track = b"MTrk" + struct.pack(">I", len(track_data)) + track_data

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(header + track)
    print(f"wrote {args.output} ({len(header) + len(track)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
