#!/usr/bin/env python3
"""Generate tests/golden/fixtures/silence-1s.mid: a 1-second MIDI file with no notes.

Header: format 0, 1 track, 480 ticks per quarter note (PPQN).
Tempo: 500_000 µs/quarter = 120 BPM → 1 quarter = 0.5 s, so 1.0 s = 2 quarters = 960 ticks.
"""

import argparse
import struct
from pathlib import Path


def variable_length_quantity(value: int) -> bytes:
    """
    Encode a non-negative integer as a MIDI Variable Length Quantity (VLQ).
    
    Parameters:
        value (int): The non-negative integer to encode as a VLQ (0 encodes to a single 0x00 byte).
    
    Returns:
        bytes: The VLQ byte sequence where each byte contains 7 bits of the value and the high bit
        (0x80) set on all bytes except the final one.
    """
    if value == 0:
        return b"\x00"
    parts = []
    while value:
        parts.append(value & 0x7F)
        value >>= 7
    parts.reverse()
    return bytes((p | 0x80) for p in parts[:-1]) + bytes([parts[-1]])


def main() -> int:
    """
    Create a 1-second silent MIDI fixture file at the CLI-specified output path.
    
    Writes a minimal format-0 MIDI file (PPQN=480) containing a tempo meta-event (120 BPM) and an end-of-track meta-event. The output directory is created if needed; the function writes the MIDI bytes to the provided --output path and prints a message with the path and total byte count.
    
    Returns:
        int: `0` on success.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, 480)
    tempo_event = b"\x00\xFF\x51\x03" + b"\x07\xA1\x20"
    end_of_track = variable_length_quantity(960) + b"\xFF\x2F\x00"
    track_data = tempo_event + end_of_track
    track = b"MTrk" + struct.pack(">I", len(track_data)) + track_data

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(header + track)
    print(f"wrote {args.output} ({len(header) + len(track)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
