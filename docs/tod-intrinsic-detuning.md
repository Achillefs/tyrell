# TOD Intrinsic Detuning

The Roland VP-330 MkII's pitch comes from a General Instrument **AY-3-0214**
top-octave-synthesizer chip: a Master VCO clocked nominally at 2.000272 MHz
fed into 12 fixed integer dividers, producing the C8…B8 octave. Lower octaves
come from chains of ÷2 flip-flop dividers driving the 49 keys C2…C6.

Because the dividers are integers, no choice of master clock can land all 12
pitches exactly on equal temperament. The result is a small, fixed,
pitch-class-dependent deviation that is *part of the instrument's character*.
Spec §9 Phase 2 commits to preserving it.

The table below is computed from `domain/include/vp330/tod/MkIIConstants.h`.
The C8 column is `master_clock / divider`; the cents column is the deviation
from the equal-temperament reference frequency (A4 = 440 Hz). The same
deviations apply at every octave (the OctaveDivider's ÷2 chain is exact in
the model).

| Pitch class | Divider | f₈ = master/N (Hz) | ET C8…B8 (Hz) | Cents from ET |
|-------------|---------|--------------------|---------------|---------------|
| C  (0)  | 478 | 4185.51 | 4186.01 | −0.21 |
| C# (1)  | 451 | 4435.19 | 4434.92 | +0.10 |
| D  (2)  | 426 | 4695.47 | 4698.64 | −1.17 |
| D# (3)  | 402 | 4975.80 | 4978.03 | −0.78 |
| E  (4)  | 379 | 5277.76 | 5274.04 | +1.22 |
| F  (5)  | 358 | 5587.35 | 5587.65 | −0.09 |
| F# (6)  | 338 | 5917.96 | 5919.91 | −0.57 |
| G  (7)  | 319 | 6271.70 | 6271.93 | −0.06 |
| G# (8)  | 301 | 6645.42 | 6644.88 | +0.14 |
| A  (9)  | 284 | 7043.21 | 7040.00 | +0.79 |
| A# (10) | 268 | 7464.45 | 7458.62 | +1.35 |
| B  (11) | 253 | 7906.21 | 7902.13 | +0.89 |

Maximum absolute deviation: **+1.35 ¢ at A#** (and −1.17 ¢ at D, +1.22 ¢ at
E). All other pitch classes are within ±1 ¢ of equal temperament.

The L2 test in `tests/unit/keyboard/MkIIKeyboardTuningTest.cpp` enforces two
contracts:
1. Every key's measured frequency matches `master / (divider × 2^octave_down)`
   within ±0.1 ¢ — guards the keyboard topology and divider chain.
2. Every key tunes within ±1.5 ¢ of ET — guards the master clock and divider
   choice without forbidding the intrinsic deviations above.

The front-panel TUNE control (±50 ¢) trims the Master VCO away from
2.000272 MHz, shifting all pitches by the same cent amount; this does not
affect the per-pitch-class deviations relative to each other.
