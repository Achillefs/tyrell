# Choir Filter Constants — Provenance and Calibration Log

## Source

`kChoirFilters` and `kVoiceWeights` in `domain/include/vp330/section/ChoirConstants.h`
were derived by a community contributor who circuit-analysed the VP-330 Choir section
in Pure Data. The 7 filter bands are each modelled as two cascaded 2nd-order bandpass
stages (4th-order total). The voice weights are exact rational values (multiples of 1/16)
consistent with resistor-ladder mixing networks on the original hardware.

## Calibration Status

| Date       | Action | Notes |
|------------|--------|-------|
| 2026-05-05 | Initial values from PD analysis | Pending L4 comparison vs author's MkII |

## L4 Calibration Procedure

1. Record each switch combination in isolation (chromatic scale, ensemble off) on the MkII.
2. Run `vp330_render` with the same MIDI; compare spectrograms.
3. If a filter band's spectral peak position deviates by more than ±10%:
   - Adjust the corresponding `f0_1` / `f0_2` in `kChoirFilters`.
   - Commit with message: `calibrate: adjust ChoirConstants filter F<n> — <observation>`.
4. If a voice balance sounds wrong: adjust `kVoiceWeights`.
5. Re-run `ctest` to verify L2 characterisation tests still pass after adjustment.

## kVibratoMaxClockOffsetHz

Current value: 50.0 Hz (≈ ±6 ¢ on the master clock).
Calibrate against **capture #4** (single C4, vibrato rate min→max sweep).
Update `kVibratoMaxClockOffsetHz` in `ChoirConstants.h` and record here.
