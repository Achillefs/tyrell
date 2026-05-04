#!/usr/bin/env python3
"""Generate tests/golden/baselines/silence-1s.wav: 48 kHz / 24-bit / stereo / 1 s of zeros."""

import argparse
import struct
from pathlib import Path


def write_wav_24bit_silence(path: Path, sample_rate: int, duration_seconds: float) -> int:
    channels = 2
    bits_per_sample = 24
    bytes_per_sample = bits_per_sample // 8
    num_frames = int(round(sample_rate * duration_seconds))
    block_align = channels * bytes_per_sample
    byte_rate = sample_rate * block_align
    data_size = num_frames * block_align

    header = (
        b"RIFF"
        + struct.pack("<I", 36 + data_size)
        + b"WAVE"
        + b"fmt "
        + struct.pack("<IHHIIHH", 16, 1, channels, sample_rate, byte_rate, block_align, bits_per_sample)
        + b"data"
        + struct.pack("<I", data_size)
    )

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(header)
        chunk = bytes(block_align * 1024)
        remaining = data_size
        while remaining > 0:
            n = min(len(chunk), remaining)
            f.write(chunk[:n])
            remaining -= n

    return data_size + len(header)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--sample-rate", type=int, default=48000)
    parser.add_argument("--duration", type=float, default=1.0)
    args = parser.parse_args()

    total = write_wav_24bit_silence(args.output, args.sample_rate, args.duration)
    print(f"wrote {args.output} ({total} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
