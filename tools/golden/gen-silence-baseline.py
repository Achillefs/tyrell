#!/usr/bin/env python3
"""Generate tests/golden/baselines/silence-1s.wav: 48 kHz / 24-bit / stereo / 1 s of zeros."""

import argparse
import struct
from pathlib import Path


def write_wav_24bit_silence(path: Path, sample_rate: int, duration_seconds: float) -> int:
    """
    Create a 24-bit stereo WAV file of silence at the given sample rate and duration.
    
    Parameters:
        path (Path): Output file path; parent directories will be created if necessary.
        sample_rate (int): Sample rate in Hertz.
        duration_seconds (float): Duration of the silence in seconds.
    
    Returns:
        total_bytes (int): Total number of bytes written to the file, including the WAV header.
    """
    if sample_rate <= 0:
        raise ValueError(f"sample_rate must be > 0 (got {sample_rate})")
    if duration_seconds <= 0:
        raise ValueError(f"duration_seconds must be > 0 (got {duration_seconds})")

    channels = 2
    bits_per_sample = 24
    bytes_per_sample = bits_per_sample // 8
    num_frames = round(sample_rate * duration_seconds)
    block_align = channels * bytes_per_sample
    byte_rate = sample_rate * block_align
    data_size = num_frames * block_align

    # RIFF/WAV chunk sizes are unsigned 32-bit; fail fast with a clear
    # message if the requested duration overflows the format's limit.
    if data_size > 0xFFFFFFFF or 36 + data_size > 0xFFFFFFFF:
        raise ValueError(
            f"WAV data chunk too large for RIFF (data_size={data_size}); "
            f"reduce --duration or --sample-rate"
        )

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
    """
    Parse command-line arguments and generate a 24-bit stereo WAV file of silence, then print the total bytes written.
    
    Accepts these CLI options: `--output` (Path, required), `--sample-rate` (int, default 48000), and `--duration` (float, default 1.0). The function writes the file and reports the number of bytes written.
    
    Returns:
        int: Exit status code (0 indicates success).
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--sample-rate", type=int, default=48000)
    parser.add_argument("--duration", type=float, default=1.0)
    args = parser.parse_args()
    if args.sample_rate <= 0:
        parser.error("--sample-rate must be > 0")
    if args.duration <= 0:
        parser.error("--duration must be > 0")

    total = write_wav_24bit_silence(args.output, args.sample_rate, args.duration)
    print(f"wrote {args.output} ({total} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
