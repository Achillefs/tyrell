#pragma once

#include "vp330/note/MidiNote.h"
#include "vp330/values/Hertz.h"

#include <array>

// MkII service-manual values for the Top Octave Divider chip.
//
// The VP-330 MkII uses a General Instrument **AY-3-0214** ("chromatic divider"
// in the service-manual semiconductor list, page 5). The chip is a 12-output
// top-octave synthesizer driven by a tunable Master VCO (page 6 labels test
// point TP1A "Master VCO Out / Top octave VCO out (C)"). The front-panel
// TUNE control gives ±50¢ trim.
//
// The MkII service notes do not include the actual Master VCO schematic page,
// so the exact crystal/RC values are not pinned by the manual. The values
// below are the canonical AY-3-0214 nominal master clock and the canonical
// divider ratios published in the chip's datasheet — the standard 12-note
// TOS series, twice the divider counts of the simpler 12-note TOS family
// (SAJ110 / MK50240 / S2562). With these values the chip's outputs are within
// ±1.5 ¢ of equal temperament across the C8…B8 octave; the small deviations
// at D, E, A# (~1.2–1.4 ¢) are intrinsic to integer top-octave division and
// part of the VP-330's character (preserved per spec §9 Phase 2).
//
// Keyboard range MIDI 36 (C2) through 84 (C6) is confirmed both by the
// service notes (page 1 spec sheet: "49 keys, C-C") and by the CHD
// Elektroservis VP-330 MIDI interface (which retrofits the same instrument
// and documents the receive range as MIDI 36–84).
//
// L4 listening checkpoint (Phase 2 close) verifies these values against the
// author's MkII; if a divergence shows up, edit here.

namespace vp330::mkii {

// AY-3-0214 nominal Master VCO clock frequency.
inline constexpr Hertz kMasterClockHz{2000272.0};

// AY-3-0214 chromatic divider counts in MIDI-class order (C, C#, D, D#, E, F,
// F#, G, G#, A, A#, B). Output frequency for pitch class p is
// kMasterClockHz / kDividerRatios[p] (see TopOctaveDivider).
inline constexpr std::array<int, 12> kDividerRatios{
    478, // C
    451, // C#
    426, // D
    402, // D#
    379, // E
    358, // F
    338, // F#
    319, // G
    301, // G#
    284, // A
    268, // A#
    253, // B
};

// 49 keys, four octaves, C2 to C6. Lowest note feeds the deepest tap of the
// OctaveDivider chain (5 ÷2 stages below the AY-3-0214's C8 output).
inline constexpr MidiNote kKeyboardLowestNote{36}; // C2
inline constexpr int kKeyCount = 49;

} // namespace vp330::mkii
