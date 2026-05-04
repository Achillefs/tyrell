# Glossary

This file mirrors `docs/SPEC.md` §4 "Ubiquitous Language" and is the source of
truth for terminology used in code. **Renames in code require a glossary update
in the same commit** (spec §10).

When this glossary and the spec disagree, the spec wins; reconcile by updating
this file in the same commit that fixes the discrepancy.

| Term | Meaning |
|---|---|
| **Section** | One of the three voicings: Choir, Strings, Vocoder. A complete signal path from divider output to its own audio bus. |
| **TopOctaveDivider** (TOD) | Master oscillator + 12 integer dividers producing the highest chromatic octave (C8…B8). Reference clock for all pitched sound. |
| **OctaveDivider** | Flip-flop chain (÷2, ÷4, ÷8…) producing lower octaves from a TOD output. |
| **KeyGate** | Per-key on/off switch with attack/release shaping. Sits between the divider matrix and the Section sum bus. One KeyGate per key (49 keys / 4 octaves on the MkII). |
| **Paraphonic** | Polyphony architecture in which all sounding keys share a single voicing path downstream of the KeyGates. *Not* "polyphonic." Reserved term. |
| **Ensemble** | The BBD-based three-line chorus effect. Always "Ensemble," never "Chorus." Defeatable on Choir; **always engaged on Strings**. |
| **BbdLine** | A single bucket-brigade-modeled variable delay line. The Ensemble contains three. |
| **ChoirFilterBank** | The pool of 7 fixed bandpass filters in the Choir section, from which 4 are selected to produce a given `ChoirVariant`. |
| **ChoirVariant** | One of 4 selectable choir voicings the MkII offers, expressed via the front-panel Male / Female switches. Catalog TBD pending service-manual reading. |
| **Vibrato** | Section-level pitch modulation, implemented as LFO into the TOD master clock frequency. |
| **Bender** | Pitch bend source from MIDI. Routes to TOD master clock alongside Vibrato. |
| **SynthesisEngine** | The top-level domain object. Owns the TOD, all KeyGates, all enabled Sections, and the Ensemble. |
| **ReferenceCapture** | An audio recording of the author's VP-330 MkII paired with the MIDI that produced it, used as ground truth. Lives in a private companion repo at `$VP330_CAPTURES_DIR`. |
| **ListeningReference** | A commercially released recording known to feature the VP-330, used for ear-training and aesthetic calibration. Not a test fixture. |
| **Voice** | **Banned.** Use KeyGate, Section, or Note depending on what you mean. |

Code naming: CamelCase for types, snake_case for variables and functions,
`kCamelCase` for constants.
