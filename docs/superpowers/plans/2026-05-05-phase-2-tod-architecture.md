# Phase 2 Implementation Plan — TopOctaveDivider Architecture

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace Phase 1's naïve-square-per-note oscillator with the actual VP-330 MkII keyboard architecture: a continuously-running `TopOctaveDivider` (12 master-clock-divided pitch-class oscillators producing C8…B8), `OctaveDivider` flip-flop chains producing the lower octaves, and one `KeyGate` per key gating divider taps with attack/release shaping. The Phase-2 acceptance state is a 49-key keyboard whose every key produces a tuned, attack/release-shaped square wave with the *intrinsic top-octave-division detuning* that the original hardware exhibits. End state sounds like a cheap organ — that is the correct outcome (spec §9 Phase 2).

**Architecture:**
- `TopOctaveDivider` is a phase-accumulator-at-host-sample-rate that *emulates* a master-clock + 12 integer dividers. Output frequency for pitch class *p* is `master_clock_hz / (2 * divider_ratio[p])`. Modelling the divider math this way preserves the intrinsic ±~0.5–1 ¢ deviation from equal temperament that is part of the VP-330's character (spec §9 Phase 2 L2 note), without paying for 2 MHz internal counters.
- `OctaveDivider` is a chain of ÷2 stages fed by one TOD pitch-class output. Implemented as a phase accumulator that toggles every *N* periods of its input, producing 50%-duty-cycle squares at *f_in / 2^k* for octave-down k.
- `KeyGate` is a per-key state machine — `Idle / Attacking / Sustain / Releasing` — that multiplies an incoming square stream by an envelope sample-by-sample. Linear or exponential ramp (see Task 5).
- `MkIIKeyboard` is a new domain object that wires a `TopOctaveDivider`, the 12 `OctaveDivider` chains, and 49 `KeyGate` instances together. It is what `SynthesisEngine` now calls into instead of the Phase-1 active-notes table.
- `SynthesisEngine` keeps its public API (`note_on`, `note_off`, `render`) but its body is rewritten to drive the `MkIIKeyboard`. The Phase-1 `active_` and `phase_` arrays go away.
- The existing L3 golden test (`single-c4-1s.mid`) keeps passing — but the assertion shifts from a generic ~261.63 Hz to *exactly the master/(2·divider) frequency for the MkII's C-divider*, which will deviate from 261.63 Hz by a fraction of a cent. This is the load-bearing demonstration that intrinsic detuning is being preserved.

**Tech Stack:** Same as Phase 1. C++20, CMake ≥ 3.22, JUCE 8.0.12 (untouched in this phase — the JUCE adapter calls the same `SynthesisEngine` API), Catch2 v3.14.0, rapidcheck (used for the divider-math property tests). No new third-party dependencies. No additions to `THIRD_PARTY.md` expected.

**Spec sections implemented:**
- §3 (domain isolation preserved — TOD/divider/KeyGate are pure C++; no JUCE/libsndfile inside `domain/`).
- §4 (every new type matches the glossary: `TopOctaveDivider`, `OctaveDivider`, `KeyGate`, `SynthesisEngine`. No new vocabulary introduced.).
- §7 L1 + L2 (each component ships with both a unit test and, where the spec demands one, a characterization contract).
- §9 Phase 2 (this plan's scope).
- §10 (one concept per commit; service-manual numbers are pinned in Task 1 with author confirmation before any code consumes them).
- §11 (resolves the "Service manual specifics — TOD master frequency, exact divider ratios" open question for the TOD axis. ChoirFilterBank/Variant numbers stay deferred to Phase 3; ensemble + vibrato numbers stay deferred.).

**What is *not* in scope (confirm if uncertain):**
- Vibrato, bender, choir variants, ensemble — all explicitly later phases.
- A new `tests/characterization/` tree. Phase 2 introduces L2 tests; per spec §7 they live at `tests/characterization/`. This plan creates that directory in Task 1 and registers it with the test runner; if you'd prefer to keep all tests under `tests/unit/` until Phase 3 forces the split, flag it before Task 1.
- L4 listening sign-off. Hardware-gated; tracked here as Task 10 but expected to land async after the recording session.

---

## File Structure (target end-state)

```
domain/
├── CMakeLists.txt                                  [edited Tasks 2,3,4,6,7]
├── include/vp330/
│   ├── tod/
│   │   ├── MkIIConstants.h                         [Task 1]
│   │   ├── TopOctaveDivider.h                      [Task 2]
│   │   └── OctaveDivider.h                         [Task 3]
│   ├── keygate/
│   │   └── KeyGate.h                               [Tasks 4, 5]
│   ├── keyboard/
│   │   └── MkIIKeyboard.h                          [Task 6]
│   └── engine/
│       └── SynthesisEngine.h                       [edited Task 7]
└── src/
    ├── tod/
    │   ├── TopOctaveDivider.cpp                    [Task 2]
    │   └── OctaveDivider.cpp                       [Task 3]
    ├── keygate/
    │   └── KeyGate.cpp                             [Tasks 4, 5]
    ├── keyboard/
    │   └── MkIIKeyboard.cpp                        [Task 6]
    └── engine/
        └── SynthesisEngine.cpp                     [edited Task 7]

docs/
└── tod-intrinsic-detuning.md                       [Task 8]

tests/
├── CMakeLists.txt                                  [edited Task 0]
├── characterization/
│   ├── CMakeLists.txt                              [Task 0]
│   └── keygate/
│       └── KeyGateEnvelopeTest.cpp                 [Task 5]
├── unit/
│   ├── CMakeLists.txt                              [edited Tasks 2,3,4,6]
│   ├── tod/
│   │   ├── TopOctaveDividerTest.cpp                [Task 2]
│   │   └── OctaveDividerTest.cpp                   [Task 3]
│   ├── keygate/
│   │   └── KeyGateTest.cpp                         [Task 4]
│   └── keyboard/
│       ├── MkIIKeyboardTest.cpp                    [Task 6]
│       └── MkIIKeyboardTuningTest.cpp              [Task 8]
└── golden/
    └── golden_test.cpp                             [edited Task 9]
```

---

## Task 1: Pin MkII service-manual numbers (author input required)

**Files:**
- Create: `domain/include/vp330/tod/MkIIConstants.h`

This is the spec-§11 "manual-reading session" for the TOD axis. **No subsequent task can compile until this header exists with real numbers.** The author owns the lookup; the agent's role is to scaffold the header, list the unknowns, and stop.

- [ ] **Step 1: Create the constants header with explicit `// AUTHOR: …` markers**

```cpp
// domain/include/vp330/tod/MkIIConstants.h
#pragma once

#include "vp330/note/MidiNote.h"
#include "vp330/values/Hertz.h"

#include <array>

namespace vp330::mkii {

// MkII service-manual values for the Top Octave Divider chip. Replace each
// AUTHOR-marked value with the manual's number. Do not commit this file with
// any AUTHOR marker remaining.

// AUTHOR: TOD master clock frequency from the MkII service manual (the
// crystal/RC frequency feeding the TOS chip's clock input).
inline constexpr Hertz kMasterClockHz{0.0}; // AUTHOR

// AUTHOR: Integer divider ratio for each chromatic pitch class, in MIDI-class
// order (C, C#, D, D#, E, F, F#, G, G#, A, A#, B). One divider per pitch
// class, producing the C8..B8 octave when divided into kMasterClockHz.
// Convention: f_pitchclass = kMasterClockHz / (2 * kDividerRatios[i]).
inline constexpr std::array<int, 12> kDividerRatios{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // AUTHOR: 12 values
};

// MkII keyboard span — 49 keys, 4 octaves. Confirm the lowest note from the
// manual; common range is C2 (MidiNote 36) to C6 (MidiNote 84) inclusive.
inline constexpr MidiNote kKeyboardLowestNote{36}; // AUTHOR: confirm
inline constexpr int kKeyCount = 49;               // MkII fact, do not edit

} // namespace vp330::mkii
```

- [ ] **Step 2: Author fills in the AUTHOR-marked values from the MkII service manual.**

The agent halts here. The author replies in chat with either (a) the four numbers/arrays filled in directly, or (b) a confirmed copy of the service-manual page. When the values are pinned, edit the header to use them and remove every `// AUTHOR` comment.

- [ ] **Step 3: Verify the pinned values produce a sane top octave**

Open a REPL or quick scratch script. For each `i` in 0..11, compute `kMasterClockHz / (2 * kDividerRatios[i])` and check it lies within ±2 ¢ of equal temperament for that pitch class in octave 8 (C8 = 4186.01 Hz, C#8 = 4434.92, …, B8 = 7902.13). A TOS chip is expected to land within ~±1 ¢ of ET; ±2 ¢ is the sanity bound. If any pitch class is outside ±2 ¢, the manual was misread — stop and re-confirm with the author before continuing.

- [ ] **Step 4: Commit**

```bash
git add domain/include/vp330/tod/MkIIConstants.h
git commit -m "vp330: pin MkII TOD master clock and divider ratios from service manual

Service-manual values for the Top Octave Synthesizer master clock and the
12 chromatic dividers feeding C8..B8. Resolves part of spec §11 (TOD axis
only; choir filter / ensemble / vibrato numbers remain deferred).
"
```

---

## Task 0 (preparatory, runs before Task 2): Add the characterization test target

**Files:**
- Create: `tests/characterization/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

`tests/characterization/` does not exist yet; Phase 2 introduces L2 tests and the directory must be wired into the runner before any L2 test can be added. Pure plumbing — no test code yet.

- [ ] **Step 1: Create `tests/characterization/CMakeLists.txt`**

```cmake
# Characterization (L2) tests. See SPEC.md §7 — L2 verifies frequency response,
# time-domain behavior, stability, and property-based invariants. Each new DSP
# component lands its L2 contract here in the same patch as the component.
target_include_directories(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(vp330_tests PRIVATE
    # populated by Task 5
)
```

- [ ] **Step 2: Register the directory in the parent test list**

Edit `tests/CMakeLists.txt` — add `add_subdirectory(characterization)` directly after `add_subdirectory(unit)`.

- [ ] **Step 3: Build to verify nothing broke**

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build green, all existing tests pass, no new ones introduced yet.

- [ ] **Step 4: Commit**

```bash
git add tests/characterization/CMakeLists.txt tests/CMakeLists.txt
git commit -m "tests: scaffold characterization (L2) test target

Phase 2 introduces L2 tests; wire tests/characterization/ into vp330_tests so
subsequent commits can drop L2 sources directly into target_sources.
"
```

---

## Task 2: `TopOctaveDivider` — master clock + 12 dividers (L1)

**Files:**
- Create: `domain/include/vp330/tod/TopOctaveDivider.h`
- Create: `domain/src/tod/TopOctaveDivider.cpp`
- Test: `tests/unit/tod/TopOctaveDividerTest.cpp`
- Modify: `domain/CMakeLists.txt`, `tests/unit/CMakeLists.txt`

The TOD generates 12 continuous square-wave streams. It is parametric — it accepts a master clock and 12 divider ratios at construction. Tests use synthetic ratios so the L1 contract is independent of Task 1's MkII-specific numbers.

- [ ] **Step 1: Write the failing test for the public API and divider math**

```cpp
// tests/unit/tod/TopOctaveDividerTest.cpp
#include "vp330/tod/TopOctaveDivider.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>

using vp330::Hertz;
using vp330::TopOctaveDivider;

TEST_CASE("TopOctaveDivider: pitch-class frequency = master / (2 * ratio)", "[tod]") {
  // Master clock 1.000 MHz, divider 250 → 1e6 / 500 = 2000 Hz.
  std::array<int, 12> ratios{250, 200, 150, 125, 100, 80, 60, 50, 40, 30, 25, 20};
  TopOctaveDivider tod{Hertz{1.0e6}, ratios, /*sample_rate=*/48000};

  REQUIRE(tod.pitch_class_frequency(0).value() == Catch::Approx(2000.0).margin(1e-9));
  REQUIRE(tod.pitch_class_frequency(11).value() == Catch::Approx(25000.0).margin(1e-9));
}

TEST_CASE("TopOctaveDivider: produces 50%-duty squares at the divided frequencies", "[tod]") {
  // Single pitch class: master 96 kHz, ratio 2 → expected 24 kHz. At a 48 kHz
  // sample rate that's exactly Nyquist; pick something audible.
  // Master 12 kHz, ratio 50 → 120 Hz.
  std::array<int, 12> ratios{};
  ratios.fill(50);
  TopOctaveDivider tod{Hertz{12000.0}, ratios, /*sample_rate=*/48000};

  // Render 1 second of pitch class 0; count zero crossings.
  std::vector<float> buf(48000);
  tod.render_pitch_class(0, buf.data(), buf.size());

  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i) {
    const bool prev_pos = buf[i - 1] >= 0.0f;
    const bool curr_pos = buf[i] >= 0.0f;
    if (prev_pos != curr_pos) ++crossings;
  }
  // 120 Hz square → 240 sign changes/sec.
  REQUIRE(crossings == 240);
}

TEST_CASE("TopOctaveDivider: amplitude is ±1.0 (50% duty)", "[tod]") {
  std::array<int, 12> ratios{};
  ratios.fill(100);
  TopOctaveDivider tod{Hertz{48000.0}, ratios, /*sample_rate=*/48000};

  std::vector<float> buf(1024);
  tod.render_pitch_class(3, buf.data(), buf.size());

  bool saw_pos = false, saw_neg = false;
  for (auto s : buf) {
    REQUIRE((s == 1.0f || s == -1.0f));
    if (s > 0) saw_pos = true;
    if (s < 0) saw_neg = true;
  }
  REQUIRE(saw_pos);
  REQUIRE(saw_neg);
}

TEST_CASE("TopOctaveDivider: continuous phase across render calls", "[tod]") {
  // Calling render_pitch_class twice should produce the same waveform as one
  // call covering both blocks. The TOD does not reset on render boundary.
  std::array<int, 12> ratios{};
  ratios.fill(100);
  TopOctaveDivider tod_a{Hertz{48000.0}, ratios, 48000};
  TopOctaveDivider tod_b{Hertz{48000.0}, ratios, 48000};

  std::vector<float> one_shot(2048);
  tod_a.render_pitch_class(0, one_shot.data(), one_shot.size());

  std::vector<float> two_shot(2048);
  tod_b.render_pitch_class(0, two_shot.data(), 1024);
  tod_b.render_pitch_class(0, two_shot.data() + 1024, 1024);

  for (std::size_t i = 0; i < one_shot.size(); ++i)
    REQUIRE(one_shot[i] == two_shot[i]);
}
```

- [ ] **Step 2: Wire the test source and build to verify it fails**

Edit `tests/unit/CMakeLists.txt` — add `${CMAKE_CURRENT_SOURCE_DIR}/tod/TopOctaveDividerTest.cpp` to the `target_sources` list.

```bash
cmake --build build 2>&1 | tail -20
```

Expected: compile error — `vp330/tod/TopOctaveDivider.h` not found.

- [ ] **Step 3: Write the header**

```cpp
// domain/include/vp330/tod/TopOctaveDivider.h
#pragma once

#include "vp330/values/Hertz.h"

#include <array>
#include <cstddef>

namespace vp330 {

class TopOctaveDivider {
public:
  TopOctaveDivider(Hertz master_clock, std::array<int, 12> divider_ratios, int sample_rate);

  Hertz pitch_class_frequency(int pitch_class) const;

  // Append `frames` samples of the requested pitch class's square output to
  // `out`. Output is ±1.0 50%-duty. State persists across calls.
  void render_pitch_class(int pitch_class, float* out, std::size_t frames);

private:
  Hertz master_clock_;
  std::array<int, 12> divider_ratios_;
  int sample_rate_;
  std::array<double, 12> phase_{};
};

} // namespace vp330
```

- [ ] **Step 4: Write the implementation**

```cpp
// domain/src/tod/TopOctaveDivider.cpp
#include "vp330/tod/TopOctaveDivider.h"

#include <cassert>

namespace vp330 {

TopOctaveDivider::TopOctaveDivider(Hertz master_clock,
                                   std::array<int, 12> divider_ratios,
                                   int sample_rate)
    : master_clock_{master_clock},
      divider_ratios_{divider_ratios},
      sample_rate_{sample_rate} {}

Hertz TopOctaveDivider::pitch_class_frequency(int pitch_class) const {
  assert(pitch_class >= 0 && pitch_class < 12);
  const double ratio = static_cast<double>(divider_ratios_[pitch_class]);
  return Hertz{master_clock_.value() / (2.0 * ratio)};
}

void TopOctaveDivider::render_pitch_class(int pitch_class, float* out, std::size_t frames) {
  assert(pitch_class >= 0 && pitch_class < 12);
  const double freq = pitch_class_frequency(pitch_class).value();
  const double inc = freq / static_cast<double>(sample_rate_);
  double& p = phase_[pitch_class];
  for (std::size_t i = 0; i < frames; ++i) {
    out[i] = (p < 0.5 ? 1.0f : -1.0f);
    p += inc;
    if (p >= 1.0) p -= 1.0;
  }
}

} // namespace vp330
```

- [ ] **Step 5: Wire the source in `domain/CMakeLists.txt`**

Add `src/tod/TopOctaveDivider.cpp` to the `vp330_domain` source list.

- [ ] **Step 6: Build and run**

```bash
cmake --build build
ctest --test-dir build -R TopOctaveDivider --output-on-failure
```

Expected: 4 test cases pass.

- [ ] **Step 7: Run the domain-isolation grep check locally**

```bash
grep -rE "JuceHeader|juce::|libsndfile|sndfile.h|alsa" domain/ && echo FAIL || echo OK
```

Expected: `OK`.

- [ ] **Step 8: Commit**

```bash
git add domain/include/vp330/tod/TopOctaveDivider.h \
        domain/src/tod/TopOctaveDivider.cpp \
        domain/CMakeLists.txt \
        tests/unit/tod/TopOctaveDividerTest.cpp \
        tests/unit/CMakeLists.txt
git commit -m "vp330: add TopOctaveDivider with parametric divider math (L1)

Master-clock + 12 integer-divider model emitting 50%-duty squares per
pitch class with continuous phase across render calls. Parametric
constructor; MkII calibration is supplied by callers.
"
```

---

## Task 3: `OctaveDivider` — flip-flop chain (L1)

**Files:**
- Create: `domain/include/vp330/tod/OctaveDivider.h`
- Create: `domain/src/tod/OctaveDivider.cpp`
- Test: `tests/unit/tod/OctaveDividerTest.cpp`
- Modify: `domain/CMakeLists.txt`, `tests/unit/CMakeLists.txt`

A divider chain is "given input frequency f, produce f/2, f/4, f/8, …" via flip-flops. We model it the same way as the TOD: a phase accumulator at host sample rate, exact frequency. Octave-down k = phase increment scaled by 1/2^k.

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/tod/OctaveDividerTest.cpp
#include "vp330/tod/OctaveDivider.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

using vp330::Hertz;
using vp330::OctaveDivider;

TEST_CASE("OctaveDivider: octave 0 passes input frequency through", "[octave-divider]") {
  OctaveDivider div{Hertz{2000.0}, /*sample_rate=*/48000};
  std::vector<float> buf(48000);
  div.render(/*octave_down=*/0, buf.data(), buf.size());

  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i)
    if ((buf[i - 1] >= 0.0f) != (buf[i] >= 0.0f)) ++crossings;
  REQUIRE(crossings == 4000); // 2000 Hz → 4000 sign changes/sec
}

TEST_CASE("OctaveDivider: each octave-down halves the frequency", "[octave-divider]") {
  OctaveDivider div{Hertz{2000.0}, 48000};
  std::vector<float> buf(48000);

  for (int k : {1, 2, 3, 4}) {
    std::ranges::fill(buf, 0.0f);
    div.render(k, buf.data(), buf.size());
    std::size_t crossings = 0;
    for (std::size_t i = 1; i < buf.size(); ++i)
      if ((buf[i - 1] >= 0.0f) != (buf[i] >= 0.0f)) ++crossings;
    const std::size_t expected = static_cast<std::size_t>(2000.0 / (1 << k) * 2.0);
    REQUIRE(crossings == expected);
  }
}

TEST_CASE("OctaveDivider: each octave maintains independent phase", "[octave-divider]") {
  OctaveDivider div{Hertz{2000.0}, 48000};
  std::vector<float> a(1024), b(1024);

  div.render(0, a.data(), a.size());
  div.render(0, b.data(), b.size());
  // Continuous phase: there exists a position where a[end-1] differs in sign
  // from b[0] only by the natural waveform progression — simpler check: render
  // the same 2048 in one shot and compare.
  OctaveDivider one_shot{Hertz{2000.0}, 48000};
  std::vector<float> full(2048);
  one_shot.render(0, full.data(), full.size());
  for (std::size_t i = 0; i < 1024; ++i) {
    REQUIRE(full[i] == a[i]);
    REQUIRE(full[i + 1024] == b[i]);
  }
}
```

- [ ] **Step 2: Wire the test source, build to verify failure**

Edit `tests/unit/CMakeLists.txt` — add `${CMAKE_CURRENT_SOURCE_DIR}/tod/OctaveDividerTest.cpp`.

```bash
cmake --build build 2>&1 | tail -10
```

Expected: header-not-found.

- [ ] **Step 3: Write header and implementation**

```cpp
// domain/include/vp330/tod/OctaveDivider.h
#pragma once

#include "vp330/values/Hertz.h"

#include <array>
#include <cstddef>

namespace vp330 {

class OctaveDivider {
public:
  static constexpr int kMaxOctavesDown = 6; // covers 4-octave keyboard with margin

  OctaveDivider(Hertz input_frequency, int sample_rate);

  // Append `frames` samples of the (input / 2^octave_down) square to `out`.
  void render(int octave_down, float* out, std::size_t frames);

private:
  Hertz input_frequency_;
  int sample_rate_;
  std::array<double, kMaxOctavesDown + 1> phase_{};
};

} // namespace vp330
```

```cpp
// domain/src/tod/OctaveDivider.cpp
#include "vp330/tod/OctaveDivider.h"

#include <cassert>

namespace vp330 {

OctaveDivider::OctaveDivider(Hertz input_frequency, int sample_rate)
    : input_frequency_{input_frequency}, sample_rate_{sample_rate} {}

void OctaveDivider::render(int octave_down, float* out, std::size_t frames) {
  assert(octave_down >= 0 && octave_down <= kMaxOctavesDown);
  const double freq = input_frequency_.value() / static_cast<double>(1 << octave_down);
  const double inc = freq / static_cast<double>(sample_rate_);
  double& p = phase_[octave_down];
  for (std::size_t i = 0; i < frames; ++i) {
    out[i] = (p < 0.5 ? 1.0f : -1.0f);
    p += inc;
    if (p >= 1.0) p -= 1.0;
  }
}

} // namespace vp330
```

- [ ] **Step 4: Wire source in `domain/CMakeLists.txt`, build, run**

```bash
cmake --build build
ctest --test-dir build -R OctaveDivider --output-on-failure
```

Expected: 3 test cases pass.

- [ ] **Step 5: Commit**

```bash
git add domain/include/vp330/tod/OctaveDivider.h \
        domain/src/tod/OctaveDivider.cpp \
        domain/CMakeLists.txt \
        tests/unit/tod/OctaveDividerTest.cpp \
        tests/unit/CMakeLists.txt
git commit -m "vp330: add OctaveDivider chain (L1)

Per-octave phase-accumulator emulating the flip-flop chain that takes
each TOD output down to lower octaves. Independent phase per octave-down
index so the same divider object can drive multiple keys.
"
```

---

## Task 4: `KeyGate` state machine (L1)

**Files:**
- Create: `domain/include/vp330/keygate/KeyGate.h`
- Create: `domain/src/keygate/KeyGate.cpp`
- Test: `tests/unit/keygate/KeyGateTest.cpp`
- Modify: `domain/CMakeLists.txt`, `tests/unit/CMakeLists.txt`

This task is the *state-machine* L1 — does the gate transition correctly through Idle → Attacking → Sustain → Releasing → Idle on `gate_on()` / `gate_off()`. Envelope shape and click suppression are Task 5's L2 concerns.

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/keygate/KeyGateTest.cpp
#include "vp330/keygate/KeyGate.h"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using vp330::KeyGate;

namespace {

KeyGate make_gate(int sample_rate = 48000) {
  // 1 ms attack, 50 ms release: short enough that tests finish quickly,
  // long enough that single-sample steps cannot leap straight to Sustain.
  return KeyGate{sample_rate, /*attack_seconds=*/0.001, /*release_seconds=*/0.05};
}

} // namespace

TEST_CASE("KeyGate: starts Idle", "[keygate]") {
  auto g = make_gate();
  REQUIRE(g.state() == KeyGate::State::Idle);
}

TEST_CASE("KeyGate: gate_on transitions Idle → Attacking", "[keygate]") {
  auto g = make_gate();
  g.gate_on();
  REQUIRE(g.state() == KeyGate::State::Attacking);
}

TEST_CASE("KeyGate: reaches Sustain after attack-time samples", "[keygate]") {
  auto g = make_gate(48000);
  g.gate_on();

  // 1 ms @ 48 kHz = 48 samples. Process exactly that many.
  std::vector<float> sig(48, 1.0f), out(48);
  g.process(sig.data(), out.data(), out.size());
  REQUIRE(g.state() == KeyGate::State::Sustain);
}

TEST_CASE("KeyGate: gate_off from Sustain transitions to Releasing", "[keygate]") {
  auto g = make_gate();
  g.gate_on();
  std::vector<float> sig(48, 1.0f), out(48);
  g.process(sig.data(), out.data(), out.size()); // → Sustain
  g.gate_off();
  REQUIRE(g.state() == KeyGate::State::Releasing);
}

TEST_CASE("KeyGate: returns to Idle after release-time samples", "[keygate]") {
  auto g = make_gate(48000);
  g.gate_on();
  std::vector<float> warm(48, 1.0f), warm_out(48);
  g.process(warm.data(), warm_out.data(), warm.size());
  g.gate_off();

  // 50 ms @ 48 kHz = 2400 samples.
  std::vector<float> sig(2400, 1.0f), out(2400);
  g.process(sig.data(), out.data(), out.size());
  REQUIRE(g.state() == KeyGate::State::Idle);
}

TEST_CASE("KeyGate: gate_on during Releasing returns to Attacking", "[keygate]") {
  auto g = make_gate();
  g.gate_on();
  std::vector<float> sig(48, 1.0f), out(48);
  g.process(sig.data(), out.data(), out.size());
  g.gate_off();
  REQUIRE(g.state() == KeyGate::State::Releasing);
  g.gate_on();
  REQUIRE(g.state() == KeyGate::State::Attacking);
}

TEST_CASE("KeyGate: idle output is zero regardless of input", "[keygate]") {
  auto g = make_gate();
  std::vector<float> sig(64, 1.0f), out(64, 999.0f);
  g.process(sig.data(), out.data(), out.size());
  for (auto s : out) REQUIRE(s == 0.0f);
}
```

- [ ] **Step 2: Wire and verify failure**

Edit `tests/unit/CMakeLists.txt` — add `${CMAKE_CURRENT_SOURCE_DIR}/keygate/KeyGateTest.cpp`.

```bash
cmake --build build 2>&1 | tail -10
```

Expected: header-not-found.

- [ ] **Step 3: Write the header**

```cpp
// domain/include/vp330/keygate/KeyGate.h
#pragma once

#include <cstddef>

namespace vp330 {

class KeyGate {
public:
  enum class State { Idle, Attacking, Sustain, Releasing };

  KeyGate(int sample_rate, double attack_seconds, double release_seconds);

  void gate_on();
  void gate_off();

  State state() const { return state_; }

  // Multiply each input sample by the current envelope value, advancing
  // state. `in` and `out` may alias.
  void process(const float* in, float* out, std::size_t frames);

private:
  void advance_one_sample();

  State state_ = State::Idle;
  double envelope_ = 0.0;
  double attack_step_;
  double release_step_;
};

} // namespace vp330
```

- [ ] **Step 4: Write the implementation**

```cpp
// domain/src/keygate/KeyGate.cpp
#include "vp330/keygate/KeyGate.h"

namespace vp330 {

KeyGate::KeyGate(int sample_rate, double attack_seconds, double release_seconds)
    : attack_step_{1.0 / (attack_seconds * sample_rate)},
      release_step_{1.0 / (release_seconds * sample_rate)} {}

void KeyGate::gate_on() {
  state_ = State::Attacking;
}

void KeyGate::gate_off() {
  if (state_ != State::Idle) state_ = State::Releasing;
}

void KeyGate::advance_one_sample() {
  switch (state_) {
  case State::Idle:
    envelope_ = 0.0;
    break;
  case State::Attacking:
    envelope_ += attack_step_;
    if (envelope_ >= 1.0) {
      envelope_ = 1.0;
      state_ = State::Sustain;
    }
    break;
  case State::Sustain:
    envelope_ = 1.0;
    break;
  case State::Releasing:
    envelope_ -= release_step_;
    if (envelope_ <= 0.0) {
      envelope_ = 0.0;
      state_ = State::Idle;
    }
    break;
  }
}

void KeyGate::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    out[i] = in[i] * static_cast<float>(envelope_);
    advance_one_sample();
  }
}

} // namespace vp330
```

- [ ] **Step 5: Wire source in `domain/CMakeLists.txt`, build, run**

```bash
cmake --build build
ctest --test-dir build -R KeyGate --output-on-failure
```

Expected: 7 test cases pass.

- [ ] **Step 6: Commit**

```bash
git add domain/include/vp330/keygate/KeyGate.h \
        domain/src/keygate/KeyGate.cpp \
        domain/CMakeLists.txt \
        tests/unit/keygate/KeyGateTest.cpp \
        tests/unit/CMakeLists.txt
git commit -m "vp330: add KeyGate state machine (L1)

Idle/Attacking/Sustain/Releasing transitions with linear-ramp envelope.
Envelope shape and click-suppression contracts come in the L2 follow-up.
"
```

---

## Task 5: `KeyGate` envelope characterization (L2)

**Files:**
- Create: `tests/characterization/keygate/KeyGateEnvelopeTest.cpp`
- Modify: `tests/characterization/CMakeLists.txt`

L2 verifies the *envelope shape* and *click suppression*. Click suppression is the load-bearing one: a hard gate of an in-flight square produces an audible click at note-on. The attack ramp must reduce the click below an audible threshold (we use peak instantaneous transient < 0.05 of a full-scale square as a proxy; revisit by ear in Phase 2 listening session).

- [ ] **Step 1: Write the failing L2 contract**

```cpp
// tests/characterization/keygate/KeyGateEnvelopeTest.cpp
#include "vp330/keygate/KeyGate.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using vp330::KeyGate;

namespace {

constexpr int kSampleRate = 48000;

std::vector<float> dc_signal(std::size_t n, float value = 1.0f) {
  return std::vector<float>(n, value);
}

} // namespace

TEST_CASE("KeyGate L2: attack ramp is monotonic non-decreasing", "[keygate][L2]") {
  KeyGate gate{kSampleRate, /*attack=*/0.005, /*release=*/0.05}; // 5 ms attack
  gate.gate_on();
  auto in = dc_signal(kSampleRate / 100); // 10 ms
  std::vector<float> out(in.size());
  gate.process(in.data(), out.data(), out.size());
  for (std::size_t i = 1; i < out.size(); ++i)
    REQUIRE(out[i] >= out[i - 1]);
}

TEST_CASE("KeyGate L2: release ramp is monotonic non-increasing", "[keygate][L2]") {
  KeyGate gate{kSampleRate, 0.005, 0.05};
  gate.gate_on();
  auto warm = dc_signal(kSampleRate / 50); // settle to Sustain
  std::vector<float> warm_out(warm.size());
  gate.process(warm.data(), warm_out.data(), warm.size());

  gate.gate_off();
  auto in = dc_signal(kSampleRate / 10); // 100 ms ≥ release time
  std::vector<float> out(in.size());
  gate.process(in.data(), out.data(), out.size());
  for (std::size_t i = 1; i < out.size(); ++i)
    REQUIRE(out[i] <= out[i - 1]);
}

TEST_CASE("KeyGate L2: Sustain output equals input (unity gain)", "[keygate][L2]") {
  KeyGate gate{kSampleRate, 0.001, 0.05};
  gate.gate_on();
  auto warm = dc_signal(kSampleRate / 100); // 10 ms covers attack
  std::vector<float> warm_out(warm.size());
  gate.process(warm.data(), warm_out.data(), warm.size());

  auto in = dc_signal(1024, 0.7f);
  std::vector<float> out(1024);
  gate.process(in.data(), out.data(), out.size());
  for (auto s : out) REQUIRE(s == Catch::Approx(0.7f).margin(1e-6));
}

TEST_CASE("KeyGate L2: click suppression on note-on", "[keygate][L2]") {
  // With a 5 ms attack ramp at 48 kHz = 240 samples, and an in-flight square
  // that flips between ±1.0, the maximum sample-to-sample step in the gated
  // output equals (envelope-step * full-scale * 2) plus the within-square
  // transition * envelope. The hard click would be 2.0 (full-scale flip);
  // a 5 ms ramp limits the *first* sample to envelope ≈ 1/240 ≈ 0.0042, so
  // the worst-case |out[i] - out[i-1]| during attack is ≪ 0.05.
  KeyGate gate{kSampleRate, 0.005, 0.05};
  gate.gate_on();

  // Synthetic in-flight square: alternates ±1 every sample (worst-case
  // for click-causing transients).
  std::vector<float> sig(240);
  for (std::size_t i = 0; i < sig.size(); ++i)
    sig[i] = (i % 2 == 0) ? 1.0f : -1.0f;
  std::vector<float> out(sig.size());
  gate.process(sig.data(), out.data(), out.size());

  float max_step = 0.0f;
  for (std::size_t i = 1; i < out.size(); ++i)
    max_step = std::max(max_step, std::abs(out[i] - out[i - 1]));
  REQUIRE(max_step < 0.05f);
}

TEST_CASE("KeyGate L2: idempotent retrigger does not click on Attacking", "[keygate][L2]") {
  // Calling gate_on() while already Attacking should not jump the envelope.
  KeyGate gate{kSampleRate, 0.005, 0.05};
  gate.gate_on();

  std::vector<float> sig(120, 1.0f), out(120);
  gate.process(sig.data(), out.data(), out.size()); // half-attack
  const float mid = out.back();
  gate.gate_on();
  std::vector<float> sig2(1, 1.0f), out2(1);
  gate.process(sig2.data(), out2.data(), 1);
  REQUIRE(std::abs(out2[0] - mid) < 0.01f);
}
```

- [ ] **Step 2: Wire the source**

Edit `tests/characterization/CMakeLists.txt` — replace the placeholder comment with:

```cmake
target_sources(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/keygate/KeyGateEnvelopeTest.cpp
)
```

- [ ] **Step 3: Build and run**

```bash
cmake --build build
ctest --test-dir build -R "L2" --output-on-failure
```

Expected: 5 cases pass on the first try (the implementation from Task 4 already produces a linear ramp, which satisfies these L2 contracts). If a case fails, *that is the L2 telling you Task 4 was under-specified* — fix `KeyGate.cpp`, do not relax the contract.

- [ ] **Step 4: Commit**

```bash
git add tests/characterization/keygate/KeyGateEnvelopeTest.cpp \
        tests/characterization/CMakeLists.txt
git commit -m "vp330: KeyGate envelope L2 — monotonicity, unity sustain, click suppression

L2 contracts for KeyGate: attack and release are monotonic, Sustain is
unity gain, and the worst-case sample-to-sample step during attack stays
below the 0.05 click-audibility threshold. Retrigger during Attacking
does not jump the envelope.
"
```

---

## Task 6: `MkIIKeyboard` — wire TOD + dividers + KeyGates with the MkII constants

**Files:**
- Create: `domain/include/vp330/keyboard/MkIIKeyboard.h`
- Create: `domain/src/keyboard/MkIIKeyboard.cpp`
- Test: `tests/unit/keyboard/MkIIKeyboardTest.cpp`
- Modify: `domain/CMakeLists.txt`, `tests/unit/CMakeLists.txt`

Composes a `TopOctaveDivider`, twelve `OctaveDivider` chains (one per pitch class, fed by the corresponding TOD output), and `kKeyCount` `KeyGate` instances. `note_on(note)` resolves to a (pitch-class, octave-down, gate-index) triple and opens the right gate. `render` sums all open gates' contributions to the output. This task uses Task 1's `MkIIConstants.h`.

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/keyboard/MkIIKeyboardTest.cpp
#include "vp330/keyboard/MkIIKeyboard.h"
#include "vp330/note/MidiNote.h"
#include "vp330/tod/MkIIConstants.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

using vp330::MidiNote;
using vp330::MkIIKeyboard;

namespace {

double rms(const std::vector<float>& buf) {
  double a = 0.0;
  for (auto s : buf) a += static_cast<double>(s) * s;
  return std::sqrt(a / static_cast<double>(buf.size()));
}

} // namespace

TEST_CASE("MkIIKeyboard: silent with no keys held", "[keyboard]") {
  MkIIKeyboard kb{48000};
  std::vector<float> out(1024);
  kb.render(out.data(), out.size());
  for (auto s : out) REQUIRE(s == 0.0f);
}

TEST_CASE("MkIIKeyboard: notes outside the 49-key range are ignored", "[keyboard]") {
  MkIIKeyboard kb{48000};
  const auto below = MidiNote{vp330::mkii::kKeyboardLowestNote.value() - 1};
  const auto above = MidiNote{vp330::mkii::kKeyboardLowestNote.value() + vp330::mkii::kKeyCount};
  kb.note_on(below);
  kb.note_on(above);
  std::vector<float> out(1024);
  kb.render(out.data(), out.size());
  for (auto s : out) REQUIRE(s == 0.0f);
}

TEST_CASE("MkIIKeyboard: lowest key produces audible output", "[keyboard]") {
  MkIIKeyboard kb{48000};
  kb.note_on(vp330::mkii::kKeyboardLowestNote);

  // Skip attack window
  std::vector<float> warm(48000 / 100), warm_out(warm.size());
  kb.render(warm_out.data(), warm_out.size());

  std::vector<float> out(48000);
  kb.render(out.data(), out.size());
  REQUIRE(rms(out) > 0.05); // squarish — single-key RMS is around per-voice gain
}

TEST_CASE("MkIIKeyboard: paraphonic — two keys both audible", "[keyboard]") {
  MkIIKeyboard kb{48000};
  kb.note_on(vp330::mkii::kKeyboardLowestNote);
  kb.note_on(MidiNote{vp330::mkii::kKeyboardLowestNote.value() + 7}); // a fifth
  std::vector<float> warm(480), warm_out(480);
  kb.render(warm_out.data(), warm_out.size());

  std::vector<float> out(48000);
  kb.render(out.data(), out.size());
  REQUIRE(rms(out) > 0.07); // two summed squares > one
}
```

- [ ] **Step 2: Build to verify failure**

Wire `tests/unit/keyboard/MkIIKeyboardTest.cpp` in `tests/unit/CMakeLists.txt`. Build expects header-not-found.

- [ ] **Step 3: Write header**

```cpp
// domain/include/vp330/keyboard/MkIIKeyboard.h
#pragma once

#include "vp330/keygate/KeyGate.h"
#include "vp330/note/MidiNote.h"
#include "vp330/tod/MkIIConstants.h"
#include "vp330/tod/OctaveDivider.h"
#include "vp330/tod/TopOctaveDivider.h"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace vp330 {

class MkIIKeyboard {
public:
  explicit MkIIKeyboard(int sample_rate);

  void note_on(MidiNote note);
  void note_off(MidiNote note);

  // Render `frames` samples summed across every open KeyGate into `out`.
  // `out` is overwritten (not accumulated). Output is mono — caller dups.
  void render(float* out, std::size_t frames);

private:
  struct KeyTopology {
    int pitch_class;     // 0..11
    int octave_down;     // 0..3 typical for the MkII 49-key span
  };
  std::optional<KeyTopology> topology_for(MidiNote note) const;

  int sample_rate_;
  TopOctaveDivider tod_;
  std::array<OctaveDivider, 12> octave_dividers_; // one per pitch class
  std::vector<KeyGate> keygates_;                 // size = kKeyCount
};

} // namespace vp330
```

- [ ] **Step 4: Write implementation**

```cpp
// domain/src/keyboard/MkIIKeyboard.cpp
#include "vp330/keyboard/MkIIKeyboard.h"

#include <algorithm>
#include <vector>

namespace vp330 {

namespace {

std::array<OctaveDivider, 12> make_octave_dividers(const TopOctaveDivider& tod, int sr) {
  std::array<OctaveDivider, 12> out{
      OctaveDivider{tod.pitch_class_frequency(0), sr},
      OctaveDivider{tod.pitch_class_frequency(1), sr},
      OctaveDivider{tod.pitch_class_frequency(2), sr},
      OctaveDivider{tod.pitch_class_frequency(3), sr},
      OctaveDivider{tod.pitch_class_frequency(4), sr},
      OctaveDivider{tod.pitch_class_frequency(5), sr},
      OctaveDivider{tod.pitch_class_frequency(6), sr},
      OctaveDivider{tod.pitch_class_frequency(7), sr},
      OctaveDivider{tod.pitch_class_frequency(8), sr},
      OctaveDivider{tod.pitch_class_frequency(9), sr},
      OctaveDivider{tod.pitch_class_frequency(10), sr},
      OctaveDivider{tod.pitch_class_frequency(11), sr},
  };
  return out;
}

constexpr float kPerKeyGain = 0.05f; // mirrors Phase 1; revisit in Phase 3 section gain.

} // namespace

MkIIKeyboard::MkIIKeyboard(int sample_rate)
    : sample_rate_{sample_rate},
      tod_{mkii::kMasterClockHz, mkii::kDividerRatios, sample_rate},
      octave_dividers_{make_octave_dividers(tod_, sample_rate)} {
  keygates_.reserve(mkii::kKeyCount);
  for (int i = 0; i < mkii::kKeyCount; ++i)
    keygates_.emplace_back(sample_rate, /*attack=*/0.005, /*release=*/0.05);
}

std::optional<MkIIKeyboard::KeyTopology>
MkIIKeyboard::topology_for(MidiNote note) const {
  const int n = note.value() - mkii::kKeyboardLowestNote.value();
  if (n < 0 || n >= mkii::kKeyCount) return std::nullopt;
  // The TOS produces C8..B8 — pitches above the highest physical key.
  // For a key whose semitone offset from kKeyboardLowestNote is `n`, the
  // pitch class is `(kKeyboardLowestNote + n) mod 12` and the octave-down
  // count is the number of octaves between C8..B8 and the key's octave.
  const int absolute_midi = mkii::kKeyboardLowestNote.value() + n;
  const int pitch_class = absolute_midi % 12;
  // C8 is MIDI 108. The TOD produces the C8..B8 octave; octave-down for
  // a note equals 8 - octave_of(absolute_midi). For MIDI 60 (C4), octave 4
  // → octave_down = 4.
  const int octave = absolute_midi / 12 - 1; // MIDI octave: 60 = C4 → 4
  const int octave_down = 8 - octave;
  return KeyTopology{pitch_class, octave_down};
}

void MkIIKeyboard::note_on(MidiNote note) {
  const auto t = topology_for(note);
  if (!t) return;
  const int gate_idx = note.value() - mkii::kKeyboardLowestNote.value();
  keygates_[gate_idx].gate_on();
}

void MkIIKeyboard::note_off(MidiNote note) {
  const auto t = topology_for(note);
  if (!t) return;
  const int gate_idx = note.value() - mkii::kKeyboardLowestNote.value();
  keygates_[gate_idx].gate_off();
}

void MkIIKeyboard::render(float* out, std::size_t frames) {
  std::fill_n(out, frames, 0.0f);
  std::vector<float> scratch(frames);
  std::vector<float> gated(frames);

  for (int i = 0; i < mkii::kKeyCount; ++i) {
    if (keygates_[i].state() == KeyGate::State::Idle) continue;
    const auto note = MidiNote{mkii::kKeyboardLowestNote.value() + i};
    const auto t = topology_for(note);
    octave_dividers_[t->pitch_class].render(t->octave_down, scratch.data(), frames);
    keygates_[i].process(scratch.data(), gated.data(), frames);
    for (std::size_t f = 0; f < frames; ++f)
      out[f] += gated[f] * kPerKeyGain;
  }
}

} // namespace vp330
```

> **Per-key state contention warning:** every key sharing a pitch class draws from the *same* `OctaveDivider` instance, so two keys an octave apart on the same pitch class will pull two octaves from one divider. That is correct for VP-330 paraphony; the divider already keeps independent phase per octave-down index. But two keys at the *same* pitch class and *same* octave (impossible on a 49-key keyboard) would call `render(k, …)` twice in the same block on the same divider, which would advance phase too far. The 49-key range guarantees uniqueness; do not refactor the divider to share by pitch-class-only without revisiting this.

- [ ] **Step 5: Wire source, build, run**

```bash
cmake --build build
ctest --test-dir build -R MkIIKeyboard --output-on-failure
```

Expected: 4 test cases pass.

- [ ] **Step 6: Commit**

```bash
git add domain/include/vp330/keyboard/MkIIKeyboard.h \
        domain/src/keyboard/MkIIKeyboard.cpp \
        domain/CMakeLists.txt \
        tests/unit/keyboard/MkIIKeyboardTest.cpp \
        tests/unit/CMakeLists.txt
git commit -m "vp330: add MkIIKeyboard composing TOD + dividers + 49 KeyGates

Wires the parametric TOD/divider/KeyGate components with the MkII
service-manual constants. note_on/off resolve to (pitch-class,
octave-down) pairs via the keyboard topology; render sums every open
gate's divider tap.
"
```

---

## Task 7: Rewire `SynthesisEngine` through `MkIIKeyboard`

**Files:**
- Modify: `domain/include/vp330/engine/SynthesisEngine.h`
- Modify: `domain/src/engine/SynthesisEngine.cpp`
- Modify: `tests/unit/engine/SynthesisEngineTest.cpp`

The Phase-1 engine kept a per-MIDI-note `active_` and `phase_` array. Replace both with a `MkIIKeyboard` member. The public API (`note_on`, `note_off`, `is_note_active`, `render`) stays — the JUCE adapter and the CLI need no changes.

- [ ] **Step 1: Update the existing `SynthesisEngineTest.cpp` expectations**

The Phase-1 test asserts `f == Approx(261.63).margin(2.0)` and `rms == Approx(0.05).margin(0.005)`. The new C4 frequency is `mkii::kMasterClockHz / (2 * mkii::kDividerRatios[0]) / 16` (C4 is octave-down 4 from C8; phase-accumulator output is master/(2N), so 4 octaves down is master/(2N)/16). After Task 1's manual-pinning, this should land within ±2 ¢ of 261.63 Hz. Keep the 2 Hz margin — it covers the intrinsic detuning. RMS stays ~0.05.

The first three tests (`stores its sample rate`, `tracks active notes via note_on / note_off`, `independent state per note`) keep passing as-is — `is_note_active` becomes "is this key's gate non-Idle." Update the engine to expose this via the keyboard.

```cpp
// Add to tests/unit/engine/SynthesisEngineTest.cpp — append:
TEST_CASE("SynthesisEngine: note outside MkII keyboard range is silently ignored", "[engine]") {
  SynthesisEngine engine{48000};
  engine.note_on(MidiNote{0}); // far below the 49-key range
  REQUIRE_FALSE(engine.is_note_active(MidiNote{0}));
}
```

- [ ] **Step 2: Build to confirm the existing tests still pass after the engine swap**

Pre-edit: run the suite and note current pass count.

```bash
ctest --test-dir build --output-on-failure
```

- [ ] **Step 3: Rewrite `SynthesisEngine.h`**

```cpp
#pragma once

#include "vp330/keyboard/MkIIKeyboard.h"
#include "vp330/note/MidiNote.h"

#include <cstddef>

namespace vp330 {

class SynthesisEngine {
public:
  explicit SynthesisEngine(int sample_rate);

  int sample_rate() const { return sample_rate_; }

  void note_on(MidiNote note);
  void note_off(MidiNote note);
  bool is_note_active(MidiNote note) const;

  void render(float* left, float* right, std::size_t frames);

private:
  int sample_rate_;
  MkIIKeyboard keyboard_;
  std::array<bool, 128> note_held_{};
};

} // namespace vp330
```

- [ ] **Step 4: Rewrite `SynthesisEngine.cpp`**

```cpp
#include "vp330/engine/SynthesisEngine.h"

#include <vector>

namespace vp330 {

SynthesisEngine::SynthesisEngine(int sample_rate)
    : sample_rate_{sample_rate}, keyboard_{sample_rate} {}

void SynthesisEngine::note_on(MidiNote note) {
  keyboard_.note_on(note);
  note_held_[static_cast<std::size_t>(note.value())] = true;
}

void SynthesisEngine::note_off(MidiNote note) {
  keyboard_.note_off(note);
  note_held_[static_cast<std::size_t>(note.value())] = false;
}

bool SynthesisEngine::is_note_active(MidiNote note) const {
  return note_held_[static_cast<std::size_t>(note.value())];
}

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  std::vector<float> mono(frames);
  keyboard_.render(mono.data(), frames);
  for (std::size_t i = 0; i < frames; ++i) {
    left[i] = mono[i];
    right[i] = mono[i];
  }
}

} // namespace vp330
```

> **Note:** `is_note_active` returns the user's logical hold state, not the gate state. A held note that has finished its release stage is still "active" by this definition — that matches Phase 1's behavior and keeps the existing tests honest. If Phase 3 needs to distinguish "audibly producing sound" from "user has finger down" we'll add a separate accessor.

- [ ] **Step 5: Build and run the full suite**

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: every prior test passes, the new `note_outside_range` case passes, the C4 frequency case passes within the 2 Hz margin.

- [ ] **Step 6: Run the JUCE plugin smoke test**

The JUCE adapter has not changed — but verify it still loads.

```bash
cmake --build build --target VP330_Standalone
./build/VP330_artefacts/Standalone/VP330.app/Contents/MacOS/VP330 &
sleep 2
killall VP330 || true
```

Expected: process starts, no crash. (Optional but recommended; skip if the standalone target name differs on Linux.)

- [ ] **Step 7: Commit**

```bash
git add domain/include/vp330/engine/SynthesisEngine.h \
        domain/src/engine/SynthesisEngine.cpp \
        tests/unit/engine/SynthesisEngineTest.cpp
git commit -m "vp330: rewire SynthesisEngine through MkIIKeyboard

Engine no longer maintains its own per-note phase table; it owns a
MkIIKeyboard and delegates note events plus rendering. Public API
unchanged — JUCE adapter and CLI need no modification.
"
```

---

## Task 8: L2 — keyboard-wide tuning + intrinsic-detuning documentation

**Files:**
- Create: `tests/unit/keyboard/MkIIKeyboardTuningTest.cpp`
- Create: `docs/tod-intrinsic-detuning.md`
- Modify: `tests/unit/CMakeLists.txt`

Spec §9 Phase 2: "tuning across the full keyboard within ±1 cent of equal temperament (and document the deviations from equal temperament that are *intrinsic* to integer top-octave division — these are part of the VP-330's character and we keep them)".

Implementation: render each of the 49 keys, measure fundamental via zero-crossing rate, compare to ET. Assert `|cents_deviation| < 1.0`. Then a separate doc enumerates the per-pitch-class intrinsic deviation as a function of the chosen divider ratios (computed from Task 1's constants).

- [ ] **Step 1: Write the tuning test**

```cpp
// tests/unit/keyboard/MkIIKeyboardTuningTest.cpp
#include "vp330/keyboard/MkIIKeyboard.h"
#include "vp330/note/MidiNote.h"
#include "vp330/tod/MkIIConstants.h"
#include "vp330/values/Pitch.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::MidiNote;
using vp330::MkIIKeyboard;

namespace {

double measure_frequency_hz(MkIIKeyboard& kb, MidiNote n, int sample_rate) {
  kb.note_on(n);
  // Skip attack
  std::vector<float> warm(sample_rate / 100), warm_buf(warm.size());
  kb.render(warm_buf.data(), warm_buf.size());
  // 1 second window
  std::vector<float> buf(sample_rate);
  kb.render(buf.data(), buf.size());
  kb.note_off(n);

  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i)
    if ((buf[i - 1] >= 0.0f) != (buf[i] >= 0.0f)) ++crossings;
  return static_cast<double>(crossings) / 2.0;
}

double cents_deviation(double measured_hz, double et_hz) {
  return 1200.0 * std::log2(measured_hz / et_hz);
}

} // namespace

TEST_CASE("MkIIKeyboard: every key tunes within ±1 cent of ET", "[keyboard][L2]") {
  constexpr int sr = 48000;
  for (int i = 0; i < vp330::mkii::kKeyCount; ++i) {
    MkIIKeyboard kb{sr};
    const auto note = MidiNote{vp330::mkii::kKeyboardLowestNote.value() + i};
    const double measured = measure_frequency_hz(kb, note, sr);
    const double et = vp330::Pitch::from_midi_note(note).to_hertz().value();
    const double cents = cents_deviation(measured, et);
    INFO("key " << note.value() << " measured " << measured << " ET " << et);
    REQUIRE(std::abs(cents) < 1.0);
  }
}
```

- [ ] **Step 2: Wire and run**

Edit `tests/unit/CMakeLists.txt` — add `${CMAKE_CURRENT_SOURCE_DIR}/keyboard/MkIIKeyboardTuningTest.cpp`.

```bash
cmake --build build
ctest --test-dir build -R MkIIKeyboardTuning --output-on-failure
```

Expected: pass for every key. **If a pitch class fails the ±1 ¢ assertion, that means Task 1's pinned divider ratio for that class falls outside the bound — re-confirm the manual reading with the author. Do not relax the test before that conversation.**

- [ ] **Step 3: Generate the intrinsic-detuning document**

`docs/tod-intrinsic-detuning.md` — a one-table doc enumerating each pitch class's `(divider_ratio, master/(2*ratio), nearest ET in octave 8, cents deviation)`. Compute by hand from Task 1's pinned numbers; this is reference material, not auto-generated. Include a one-paragraph "why we keep it" preamble citing spec §9 Phase 2.

```markdown
# TOD Intrinsic Detuning

The Roland VP-330 MkII's pitch comes from a Top Octave Synthesizer chip:
a master clock (~2 MHz, see `MkIIConstants.h`) integer-divided to produce
the 12 pitches of the C8 octave, then chains of ÷2 flip-flops to produce
the lower octaves. Because the dividers are integers, no choice of master
clock can land all 12 pitches exactly on equal temperament. The result is
a small, fixed, pitch-class-dependent detuning that is *part of the
instrument's character*. Spec §9 Phase 2 commits to preserving it.

| Pitch class | Divider | f_8 = master/(2·N) (Hz) | ET C8..B8 (Hz) | Cents |
|-------------|---------|--------------------------|----------------|-------|
| C  (0) | … | … | 4186.01 | … |
| C# (1) | … | … | 4434.92 | … |
…
```

Fill the table from the pinned Task 1 numbers.

- [ ] **Step 4: Commit**

```bash
git add tests/unit/keyboard/MkIIKeyboardTuningTest.cpp \
        tests/unit/CMakeLists.txt \
        docs/tod-intrinsic-detuning.md
git commit -m "vp330: L2 keyboard tuning ±1¢ + document intrinsic TOD detuning

Per-key ET-deviation contract for the MkII keyboard. Companion doc
enumerates the pitch-class-specific deviations introduced by integer
top-octave division — these are kept as VP-330 character per spec §9.
"
```

---

## Task 9: Update the L3 single-c4 golden test threshold

**Files:**
- Modify: `tests/golden/golden_test.cpp`

The Phase-1 golden asserted "fundamental within 2 Hz of 261.63". The new C4 frequency comes from `master / (2 * divider[0]) / 16` and may differ from 261.63 by a fraction of a cent. Tighten the assertion to the actual MkII frequency (computed from constants), with a tight margin (±0.05 Hz) — this is the *test* that catches future regressions in the TOD math.

- [ ] **Step 1: Modify the existing golden test**

Compute the expected fundamental at the top of the file:

```cpp
#include "vp330/tod/MkIIConstants.h"
…
constexpr double kMkIIC4Hz =
    vp330::mkii::kMasterClockHz.value() / (2.0 * vp330::mkii::kDividerRatios[0]) / 16.0;
```

Then update the relevant `REQUIRE(f == Approx(...).margin(...))` to use `kMkIIC4Hz` with a `0.05` Hz margin.

- [ ] **Step 2: Run the golden**

```bash
ctest --test-dir build -R golden --output-on-failure
```

Expected: pass with the tighter margin.

- [ ] **Step 3: Commit**

```bash
git add tests/golden/golden_test.cpp
git commit -m "tests/golden: tighten single-c4 fundamental to MkII TOD frequency

The Phase-1 ±2 Hz tolerance covered any approximate C4 oscillator. With
TOD wiring landed, the engine produces master/(2·divider)/16 exactly —
tighten the assertion to ±0.05 Hz around that computed value to make
this golden a meaningful regression gate.
"
```

---

## Task 10 (deferred, hardware-gated): L4 listening checkpoint vs. capture #1

**Files:**
- None in this repo. Author runs the listening session against `$VP330_CAPTURES_DIR/sessions/<…>/capture-01-chromatic-no-choir.wav`.

Per spec §9 Phase 2: "sounds like a cheap organ. That's correct. A/B vs. capture #1 should show same fundamentals, very different timbre."

- [ ] **Step 1: Render the L4 fixture**

Use `vp330_render` to produce the same MIDI file used for capture-01 through the plugin engine.

```bash
./build/vp330_render \
  --input "$VP330_CAPTURES_DIR/sessions/<session>/capture-01.mid" \
  --output /tmp/vp330-l4-phase2.wav \
  --sample-rate 48000
```

- [ ] **Step 2: A/B by ear**

Author listens to `/tmp/vp330-l4-phase2.wav` against the captured WAV. Pass criterion: same pitch envelope, recognizably different timbre (no choir filtering yet, raw square-wave divider sum). A spectral-distance number is informational; the author's ear is the gate.

- [ ] **Step 3: If pass, append a one-line entry to the spec change-log**

Edit `docs/SPEC.md` §12 with a row noting Phase 2 L4 sign-off. If fail, file the discrepancy and *do not tag*.

---

## Task 11: Tag `phase-2`

**Pre-condition:** Tasks 1–9 merged, Task 10 author-signed-off.

- [ ] **Step 1: Mark the Phase 2 deliverables checked in `docs/SPEC.md`**

Edit §9 Phase 2 — flip every `[ ]` to `[x]` once the corresponding task is complete. Update the `**Status:**` line at the top of the spec to "Phase 2 complete; Phase 3 not yet begun."

- [ ] **Step 2: Append a Phase-2 row to the §12 change-log**

Same format as the existing rows. Note: TOD architecture, KeyGate, MkIIKeyboard, intrinsic-detuning doc; no new third-party dependencies; service-manual numbers pinned for the TOD axis only.

- [ ] **Step 3: Commit, tag, push**

```bash
git add docs/SPEC.md
git commit -m "docs: mark Phase 2 complete (TOD architecture)"
git tag phase-2
git push origin main --tags
```

---

## Self-Review

**Spec coverage:**
- §9 Phase 2 deliverable 1 (TopOctaveDivider master clock + 12 dividers, L1) → Task 2.
- §9 Phase 2 deliverable 2 (OctaveDivider chains, L1) → Task 3.
- §9 Phase 2 deliverable 3 (KeyGate with attack/release, L1 + L2) → Tasks 4, 5.
- §9 Phase 2 deliverable 4 (SynthesisEngine rewired) → Task 7.
- §9 Phase 2 deliverable 5 (L2 tuning ±1 ¢ + intrinsic-detuning doc) → Task 8.
- §9 Phase 2 deliverable 6 (L4 checkpoint) → Task 10.
- §9 Phase 2 deliverable 7 (tag phase-2) → Task 11.
- §11 (TOD master clock + divider ratios pending manual reading) → Task 1.
- §3 (domain isolation) → Task 2 step 7 grep check; the same check runs in CI on every push.
- §7 L1 → Tasks 2/3/4/6/8 unit tests.
- §7 L2 → Tasks 5, 8 (envelope + tuning).

**Placeholder scan:** Every code step contains complete code. Two intentionally-blank slots: (a) `MkIIConstants.h`'s AUTHOR markers in Task 1, which exist *because* the values come from the author; (b) `tod-intrinsic-detuning.md` table cells, filled from Task 1's numbers in Task 8 step 3. Both are author-input slots, not unspecified plan steps.

**Type consistency:** `KeyGate::process(const float*, float*, std::size_t)` is the same shape used in Tasks 4, 5, 6. `OctaveDivider::render(int, float*, std::size_t)` matches Tasks 3, 6. `TopOctaveDivider::render_pitch_class(int, float*, std::size_t)` is consumed only inside `MkIIKeyboard` (Task 6) — no public callers. `MidiNote::value()` returns `int`, matching every test. `Hertz` is constructed via `Hertz{double}` everywhere.
