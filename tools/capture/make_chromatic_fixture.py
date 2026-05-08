#!/usr/bin/env python3
"""Generate a 49-note chromatic MIDI fixture covering the VP-330 MkII keyboard
(C2..C6 = MIDI 36..84). Each note holds for ~1.1 s with a 0.2 s rest before
the next, giving the unit time to release before the next attack.

Output: $VP330_CAPTURES_DIR/fixtures/chromatic-49-keys.mid
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

import mido


NOTE_LOW = 36   # C2 — VP-330 MkII lowest key
NOTE_HIGH = 84  # C6 — VP-330 MkII highest key
NOTE_HOLD_SECONDS = 1.1
NOTE_REST_SECONDS = 0.2
TICKS_PER_BEAT = 480
BEAT_SECONDS = 0.5  # 120 BPM


def seconds_to_ticks(seconds: float) -> int:
    return int(round(seconds / BEAT_SECONDS * TICKS_PER_BEAT))


def main() -> int:
    captures_dir = os.environ.get("VP330_CAPTURES_DIR")
    if not captures_dir:
        sys.exit("error: VP330_CAPTURES_DIR is not set.")

    out_dir = Path(captures_dir) / "fixtures"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / "chromatic-49-keys.mid"

    mid = mido.MidiFile(ticks_per_beat=TICKS_PER_BEAT)
    track = mido.MidiTrack()
    mid.tracks.append(track)

    track.append(mido.MetaMessage("set_tempo", tempo=mido.bpm2tempo(120), time=0))

    hold_ticks = seconds_to_ticks(NOTE_HOLD_SECONDS)
    rest_ticks = seconds_to_ticks(NOTE_REST_SECONDS)

    for note in range(NOTE_LOW, NOTE_HIGH + 1):
        track.append(mido.Message("note_on", note=note, velocity=100, time=0))
        track.append(mido.Message("note_off", note=note, velocity=0, time=hold_ticks))
        # Rest before next note (delta_time on the next note_on).
        if note < NOTE_HIGH:
            # Gap is added as time on the next note_on; here it's a no-op meta.
            track.append(mido.MetaMessage("text", text="rest", time=rest_ticks))

    mid.save(out_path)

    total_seconds = (NOTE_HIGH - NOTE_LOW + 1) * NOTE_HOLD_SECONDS + (NOTE_HIGH - NOTE_LOW) * NOTE_REST_SECONDS
    print(f"wrote {out_path} ({NOTE_HIGH - NOTE_LOW + 1} notes, ~{total_seconds:.1f} s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
