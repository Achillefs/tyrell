# Phase 1 Implementation Plan — Skeleton Engine

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** End-to-end MIDI → audio in both adapters: load a `.mid` in the CLI, render naïve-square paraphony, write a `.wav`; load the JUCE plugin in a host, play notes, hear naïve-square paraphony. Validated by a golden test that renders `single-c4-1s.mid` and asserts a fundamental at ~261.6 Hz with a sane RMS. Phase 1 is the spec-§9 deliverable list, no more.

**Architecture:** Domain core gains `Hertz`, `MidiNote`, `Pitch` value types; `MidiEvent` POD; and the `MidiSource`, `AudioSink`, `Clock` port interfaces. The new `SynthesisEngine` exposes a small direct API (`note_on`, `note_off`, `render`) — Phase 2 will rewire it through the `MidiSource` port once `KeyGate`/`TopOctaveDivider` arrive. The naïve oscillator is intentionally trivial: a per-MIDI-note phase counter producing a square at the equal-temperament frequency for that note, summed across active notes, mono dup'd to stereo. The CLI gets an in-tree minimal Standard MIDI File reader (`infrastructure/cli/SmfReader`) — no new third-party dependencies — and replaces the silence renderer with a real engine pump. The JUCE plugin's `processBlock` iterates the `MidiBuffer`, applies events to the engine, and renders the audio block.

**Tech Stack:** Same as Phase 0. C++20, CMake ≥ 3.22, JUCE 8.0.12, Catch2 v3.14.0, rapidcheck, libsndfile. No new dependencies.

**Spec sections implemented:**
- §3 (architecture invariants — domain isolation preserved by linking SmfReader only into adapter targets)
- §4 (vocabulary — every name matches the glossary; no `Voice`)
- §7 L1 + L3 (every value-type and engine-state addition has a unit test; the new golden test asserts engine output)
- §9 Phase 1 (this plan's scope)
- §10 (one concept per commit; ask before deps)

---

## File Structure (target end-state)

```
domain/
├── CMakeLists.txt                                  [edited Tasks 2,7]
├── include/vp330/
│   ├── values/
│   │   ├── Hertz.h                                 [Task 2]
│   │   └── Pitch.h                                 [Task 4]
│   ├── note/
│   │   └── MidiNote.h                              [Task 3]
│   ├── ports/
│   │   ├── MidiSource.h                            [Task 6]
│   │   ├── AudioSink.h                             [Task 6]
│   │   └── Clock.h                                 [Task 6]
│   └── engine/
│       ├── MidiEvent.h                             [Task 5]
│       └── SynthesisEngine.h                       [Task 7; edited Task 8]
└── src/
    └── engine/
        └── SynthesisEngine.cpp                     [Task 7; edited Task 8]
                                                     (stub.cpp removed in Task 7)

infrastructure/
├── cli/
│   ├── CMakeLists.txt                              [edited Task 9]
│   ├── SmfReader.h                                 [Task 9]
│   ├── SmfReader.cpp                               [Task 9]
│   └── render_main.cpp                             [edited Task 10]
└── juce/
    ├── PluginProcessor.h                           [edited Task 11]
    └── PluginProcessor.cpp                         [edited Task 11]

tests/
├── CMakeLists.txt                                  [edited Task 1]
├── unit/
│   ├── CMakeLists.txt                              [Task 1]
│   ├── values/
│   │   ├── HertzTest.cpp                           [Task 2]
│   │   └── PitchTest.cpp                           [Task 4]
│   ├── note/
│   │   └── MidiNoteTest.cpp                        [Task 3]
│   ├── engine/
│   │   ├── MidiEventTest.cpp                       [Task 5]
│   │   └── SynthesisEngineTest.cpp                 [Task 7; edited Task 8]
│   └── cli/
│       └── SmfReaderTest.cpp                       [Task 9]
└── golden/
    ├── CMakeLists.txt                              [edited Task 12]
    ├── golden_test.cpp                             [edited Task 12]
    └── fixtures/
        └── single-c4-1s.mid                        [Task 12, generated]

tools/
└── golden/
    └── gen-single-c4-fixture.py                    [Task 12]
```

`tests/golden/baselines/` does **not** receive a `single-c4-1s.wav`. The Phase 1 golden test makes a procedural assertion (fundamental + RMS), not a byte-comparison against a frozen baseline. The naïve square is going to be rewritten in Phase 2; freezing its output would be churn. Phase 2+ may introduce locked-in WAV baselines for stable components.

**Bootstrap step (before Task 1):**

- [ ] **Commit this plan**

```bash
cd /Users/ac/dev/tyrell
git add docs/superpowers/plans/2026-05-04-phase-1-skeleton-engine.md
git commit -m "$(cat <<'EOF'
docs: add phase 1 implementation plan

Implements spec §9 Phase 1 (skeleton engine: value types, ports, naïve square
paraphony, CLI + JUCE adapters, golden test for single-c4-1s.mid) as 13
ordered commits.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 1: Wire `tests/unit/` into the build

**Files:**
- Create: `tests/unit/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt:12` (add `add_subdirectory(unit)` before `catch_discover_tests`)
- Remove: `tests/unit/.gitkeep` (no longer needed once a real test is added in Task 2; keep for now to avoid an empty commit in this task)

Phase 0 left `tests/unit/` as just `.gitkeep`. We need a CMakeLists that lets later tasks `target_sources` test files into `vp330_tests`.

- [ ] **Step 1: Create `tests/unit/CMakeLists.txt`**

```cmake
# Each subsequent task adds its test source via target_sources(vp330_tests PRIVATE …)
# from a sub-CMakeLists or directly here. Keep this file as the single hook.
target_include_directories(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

- [ ] **Step 2: Wire it into `tests/CMakeLists.txt`**

Edit `tests/CMakeLists.txt`. Insert `add_subdirectory(unit)` between `add_subdirectory(golden)` and `catch_discover_tests(vp330_tests)`:

```cmake
add_executable(vp330_tests)

target_link_libraries(vp330_tests PRIVATE
    vp330_domain
    Catch2::Catch2WithMain
    rapidcheck
    rapidcheck_catch
)

target_compile_features(vp330_tests PRIVATE cxx_std_20)

add_subdirectory(golden)
add_subdirectory(unit)

catch_discover_tests(vp330_tests)
```

- [ ] **Step 3: Configure and build to confirm nothing broke**

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure
```

Expected: configure succeeds; `vp330_tests` builds (still only the silence golden test); `ctest` runs the existing walking-skeleton test green.

- [ ] **Step 4: Commit**

```bash
git add tests/unit/CMakeLists.txt tests/CMakeLists.txt
git commit -m "$(cat <<'EOF'
build: wire tests/unit subdir into vp330_tests

Empty CMakeLists hook; subsequent Phase 1 tasks add their unit tests via
target_sources(vp330_tests PRIVATE …). No behavior change; ctest still
runs the silence golden test only.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `ctest --test-dir build` is green; `tests/unit/CMakeLists.txt` exists.

---

## Task 2: `Hertz` value type

**Files:**
- Create: `domain/include/vp330/values/Hertz.h`
- Create: `tests/unit/values/HertzTest.cpp`
- Modify: `tests/unit/CMakeLists.txt` (add the test source)

**Design notes:**
- Wraps a `double` with a unit tag. No implicit conversion to `double` (spec §3.2).
- Operators: `+ Hertz`, `- Hertz`, `* double` (commutative), `/ double`, `==`, `<=>`. No `Hertz * Hertz` (Hz² has no use here).
- Header-only, all `constexpr`-friendly.

- [ ] **Step 1: Write the failing test**

Create `tests/unit/values/HertzTest.cpp`:

```cpp
#include "vp330/values/Hertz.h"

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

using vp330::Hertz;

TEST_CASE("Hertz preserves its constructor argument", "[hertz]") {
  constexpr Hertz f{440.0};
  STATIC_REQUIRE(f.value() == 440.0);
}

TEST_CASE("Hertz equality compares values", "[hertz]") {
  REQUIRE(Hertz{440.0} == Hertz{440.0});
  REQUIRE(Hertz{440.0} != Hertz{441.0});
}

TEST_CASE("Hertz adds and subtracts as Hertz", "[hertz]") {
  REQUIRE((Hertz{100.0} + Hertz{50.0}) == Hertz{150.0});
  REQUIRE((Hertz{100.0} - Hertz{50.0}) == Hertz{50.0});
}

TEST_CASE("Hertz scales by a scalar in either order", "[hertz]") {
  REQUIRE((Hertz{440.0} * 2.0) == Hertz{880.0});
  REQUIRE((2.0 * Hertz{440.0}) == Hertz{880.0});
  REQUIRE((Hertz{440.0} / 2.0) == Hertz{220.0});
}

TEST_CASE("Hertz orders by value", "[hertz]") {
  REQUIRE(Hertz{100.0} < Hertz{200.0});
  REQUIRE(Hertz{200.0} > Hertz{100.0});
  REQUIRE(Hertz{100.0} <= Hertz{100.0});
}

TEST_CASE("Hertz forbids implicit conversion to double", "[hertz]") {
  STATIC_REQUIRE(!std::is_convertible_v<Hertz, double>);
  STATIC_REQUIRE(!std::is_convertible_v<double, Hertz>);
}
```

- [ ] **Step 2: Wire the test into the build**

Edit `tests/unit/CMakeLists.txt`:

```cmake
target_include_directories(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/values/HertzTest.cpp
)
```

- [ ] **Step 3: Run to confirm it fails**

```bash
cmake --build build --target vp330_tests
```

Expected: compile error — `vp330/values/Hertz.h: No such file or directory`.

- [ ] **Step 4: Implement `Hertz`**

Create `domain/include/vp330/values/Hertz.h`:

```cpp
#pragma once

#include <compare>

namespace vp330 {

class Hertz {
 public:
  constexpr explicit Hertz(double hz) : value_{hz} {}

  constexpr double value() const { return value_; }

  constexpr bool operator==(const Hertz&) const = default;
  constexpr auto operator<=>(const Hertz&) const = default;

 private:
  double value_;
};

constexpr Hertz operator+(Hertz a, Hertz b) { return Hertz{a.value() + b.value()}; }
constexpr Hertz operator-(Hertz a, Hertz b) { return Hertz{a.value() - b.value()}; }
constexpr Hertz operator*(Hertz a, double s) { return Hertz{a.value() * s}; }
constexpr Hertz operator*(double s, Hertz a) { return Hertz{a.value() * s}; }
constexpr Hertz operator/(Hertz a, double s) { return Hertz{a.value() / s}; }

} // namespace vp330
```

- [ ] **Step 5: Build and run; confirm green**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R hertz
```

Expected: 6 assertions across 6 test cases pass.

- [ ] **Step 6: Verify domain isolation invariant still holds**

```bash
grep -rE '#include[[:space:]]*<(juce_|sndfile|alsa)' domain/ || echo "OK: domain/ is isolated."
```

Expected: `OK: domain/ is isolated.`

- [ ] **Step 7: Commit**

```bash
git add domain/include/vp330/values/Hertz.h tests/unit/values/HertzTest.cpp tests/unit/CMakeLists.txt
git commit -m "$(cat <<'EOF'
domain: add Hertz value type

Strongly-typed double wrapper; no implicit conversion to scalar (spec §3.2).
Operators: +/- Hertz, * scalar (both orders), / scalar, == and <=>.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** all hertz tests pass; domain isolation grep is clean.

---

## Task 3: `MidiNote` value type

**Files:**
- Create: `domain/include/vp330/note/MidiNote.h`
- Create: `tests/unit/note/MidiNoteTest.cpp`
- Modify: `tests/unit/CMakeLists.txt`

**Design notes:**
- Integer 0..127. Out-of-range construction is a programming error.
- `explicit MidiNote(int)` asserts in-range; `MidiNote::try_from_int(int) → std::optional<MidiNote>` is the safe entry point.
- Storage is `std::int8_t` to make the engine's per-note arrays cheap if they ever pack a note number. Accessor returns `int`.

- [ ] **Step 1: Write the failing test**

Create `tests/unit/note/MidiNoteTest.cpp`:

```cpp
#include "vp330/note/MidiNote.h"

#include <catch2/catch_test_macros.hpp>

using vp330::MidiNote;

TEST_CASE("MidiNote: try_from_int accepts 0..127", "[midi-note]") {
  REQUIRE(MidiNote::try_from_int(0).has_value());
  REQUIRE(MidiNote::try_from_int(60).has_value());
  REQUIRE(MidiNote::try_from_int(127).has_value());
}

TEST_CASE("MidiNote: try_from_int rejects out of range", "[midi-note]") {
  REQUIRE_FALSE(MidiNote::try_from_int(-1).has_value());
  REQUIRE_FALSE(MidiNote::try_from_int(128).has_value());
  REQUIRE_FALSE(MidiNote::try_from_int(1000).has_value());
}

TEST_CASE("MidiNote: value() round-trips the constructor", "[midi-note]") {
  REQUIRE(MidiNote::try_from_int(60)->value() == 60);
  REQUIRE(MidiNote::try_from_int(0)->value() == 0);
  REQUIRE(MidiNote::try_from_int(127)->value() == 127);
}

TEST_CASE("MidiNote: equality and ordering", "[midi-note]") {
  REQUIRE(*MidiNote::try_from_int(60) == *MidiNote::try_from_int(60));
  REQUIRE(*MidiNote::try_from_int(60) != *MidiNote::try_from_int(61));
  REQUIRE(*MidiNote::try_from_int(60) < *MidiNote::try_from_int(61));
}

TEST_CASE("MidiNote: well-known constants", "[midi-note]") {
  REQUIRE(vp330::kMidiC4.value() == 60);
  REQUIRE(vp330::kMidiA4.value() == 69);
}
```

- [ ] **Step 2: Add the test to `tests/unit/CMakeLists.txt`**

Append to the `target_sources(vp330_tests PRIVATE …)` list:

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/note/MidiNoteTest.cpp
```

- [ ] **Step 3: Run to confirm it fails**

```bash
cmake --build build --target vp330_tests
```

Expected: compile error — `vp330/note/MidiNote.h: No such file or directory`.

- [ ] **Step 4: Implement `MidiNote`**

Create `domain/include/vp330/note/MidiNote.h`:

```cpp
#pragma once

#include <cassert>
#include <compare>
#include <cstdint>
#include <optional>

namespace vp330 {

class MidiNote {
 public:
  static constexpr std::optional<MidiNote> try_from_int(int v) {
    if (v < 0 || v > 127) return std::nullopt;
    return MidiNote{v};
  }

  constexpr explicit MidiNote(int v) : value_{static_cast<std::int8_t>(v)} {
    assert(v >= 0 && v <= 127);
  }

  constexpr int value() const { return value_; }

  constexpr bool operator==(const MidiNote&) const = default;
  constexpr auto operator<=>(const MidiNote&) const = default;

 private:
  std::int8_t value_;
};

inline constexpr MidiNote kMidiC4{60};
inline constexpr MidiNote kMidiA4{69};

} // namespace vp330
```

- [ ] **Step 5: Build and run; confirm green**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R midi-note
```

Expected: 5 cases pass.

- [ ] **Step 6: Commit**

```bash
git add domain/include/vp330/note/MidiNote.h tests/unit/note/MidiNoteTest.cpp tests/unit/CMakeLists.txt
git commit -m "$(cat <<'EOF'
domain: add MidiNote value type

Integer wrapper for MIDI note numbers 0..127. try_from_int returns optional;
the explicit ctor asserts in debug. kMidiC4 (60) and kMidiA4 (69) constants.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** all midi-note tests pass; no out-of-range construction path silently succeeds.

---

## Task 4: `Pitch` value type + MidiNote↔Hertz conversion

**Files:**
- Create: `domain/include/vp330/values/Pitch.h`
- Create: `tests/unit/values/PitchTest.cpp`
- Modify: `tests/unit/CMakeLists.txt`

**Design notes:**
- `Pitch` is a fractional-semitone wrapper over `double`. Lets future bender / vibrato code carry continuous pitch without abusing `MidiNote`.
- `Pitch::from_midi_note(MidiNote)` and `Pitch::from_hertz(Hertz, Hertz a4)`.
- `Pitch::to_hertz(Hertz a4 = 440)` and `Pitch::semitones()`.
- A4 reference defaults to 440 Hz. The MkII isn't pinned to 440, so a future task can tweak the default — but Phase 1 ships A4=440.

- [ ] **Step 1: Write the failing test**

Create `tests/unit/values/PitchTest.cpp`:

```cpp
#include "vp330/values/Pitch.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using vp330::Hertz;
using vp330::MidiNote;
using vp330::Pitch;

TEST_CASE("Pitch::from_midi_note carries the integer semitone", "[pitch]") {
  REQUIRE(Pitch::from_midi_note(*MidiNote::try_from_int(60)).semitones() == 60.0);
  REQUIRE(Pitch::from_midi_note(*MidiNote::try_from_int(69)).semitones() == 69.0);
}

TEST_CASE("Pitch::to_hertz: A4 round-trip", "[pitch]") {
  const auto a4 = Pitch::from_midi_note(*MidiNote::try_from_int(69)).to_hertz();
  REQUIRE(a4.value() == Approx(440.0).margin(1e-9));
}

TEST_CASE("Pitch::to_hertz: octaves halve / double", "[pitch]") {
  const auto a3 = Pitch::from_midi_note(*MidiNote::try_from_int(57)).to_hertz();
  const auto a5 = Pitch::from_midi_note(*MidiNote::try_from_int(81)).to_hertz();
  REQUIRE(a3.value() == Approx(220.0).margin(1e-9));
  REQUIRE(a5.value() == Approx(880.0).margin(1e-9));
}

TEST_CASE("Pitch::to_hertz: middle C (C4)", "[pitch]") {
  const auto c4 = Pitch::from_midi_note(vp330::kMidiC4).to_hertz();
  // 12-TET C4 with A4=440: 440 * 2^((60-69)/12) ≈ 261.6255653...
  REQUIRE(c4.value() == Approx(261.6255653005986).margin(1e-9));
}

TEST_CASE("Pitch::from_hertz inverts to_hertz", "[pitch]") {
  const auto p = Pitch::from_hertz(Hertz{261.6255653005986});
  REQUIRE(p.semitones() == Approx(60.0).margin(1e-9));
}

TEST_CASE("Pitch: equality and ordering", "[pitch]") {
  REQUIRE(Pitch{60.0} == Pitch{60.0});
  REQUIRE(Pitch{60.0} < Pitch{60.5});
}

TEST_CASE("Pitch: forbids implicit conversion to double", "[pitch]") {
  STATIC_REQUIRE(!std::is_convertible_v<Pitch, double>);
  STATIC_REQUIRE(!std::is_convertible_v<double, Pitch>);
}
```

- [ ] **Step 2: Add the test to `tests/unit/CMakeLists.txt`**

Append:

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/values/PitchTest.cpp
```

- [ ] **Step 3: Run to confirm it fails**

```bash
cmake --build build --target vp330_tests
```

Expected: compile error — `vp330/values/Pitch.h: No such file or directory`.

- [ ] **Step 4: Implement `Pitch`**

Create `domain/include/vp330/values/Pitch.h`:

```cpp
#pragma once

#include "vp330/note/MidiNote.h"
#include "vp330/values/Hertz.h"

#include <cmath>
#include <compare>

namespace vp330 {

class Pitch {
 public:
  static constexpr Pitch from_midi_note(MidiNote n) {
    return Pitch{static_cast<double>(n.value())};
  }

  static Pitch from_hertz(Hertz f, Hertz a4 = Hertz{440.0}) {
    return Pitch{69.0 + 12.0 * std::log2(f.value() / a4.value())};
  }

  constexpr explicit Pitch(double semitones) : semitones_{semitones} {}

  constexpr double semitones() const { return semitones_; }

  Hertz to_hertz(Hertz a4 = Hertz{440.0}) const {
    return a4 * std::pow(2.0, (semitones_ - 69.0) / 12.0);
  }

  constexpr bool operator==(const Pitch&) const = default;
  constexpr auto operator<=>(const Pitch&) const = default;

 private:
  double semitones_;
};

} // namespace vp330
```

- [ ] **Step 5: Build and run; confirm green**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R pitch
```

Expected: 7 cases pass; the C4-to-Hertz check is the value the Phase 1 golden test will assert against.

- [ ] **Step 6: Commit**

```bash
git add domain/include/vp330/values/Pitch.h tests/unit/values/PitchTest.cpp tests/unit/CMakeLists.txt
git commit -m "$(cat <<'EOF'
domain: add Pitch value type with MidiNote↔Hertz conversion

Fractional-semitone wrapper. from_midi_note / from_hertz factories;
to_hertz uses a4=440 Hz default, equal-temperament. Inverse of from_hertz
is exact at semitone integers; C4 ≈ 261.6256 Hz pinned in tests.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** Pitch round-trips MidiNote at integer semitones; A4 → 440.000 within 1e-9.

---

## Task 5: `MidiEvent` POD

**Files:**
- Create: `domain/include/vp330/engine/MidiEvent.h`
- Create: `tests/unit/engine/MidiEventTest.cpp`
- Modify: `tests/unit/CMakeLists.txt`

**Design notes:**
- POD struct with a `Kind` enum. Phase 1 only needs `NoteOn` and `NoteOff`; pitch-bend / CC arrive in later phases.
- `velocity` is meaningful only on `NoteOn`. The Phase 1 engine ignores it (square at unity); we still carry it for the JUCE adapter and future shaping.
- `sample_offset` is the offset within the current render block. The JUCE adapter sets this when iterating `MidiBuffer`; the CLI sets it when expanding tempo-quantized event times into sample indices.

- [ ] **Step 1: Write the failing test**

Create `tests/unit/engine/MidiEventTest.cpp`:

```cpp
#include "vp330/engine/MidiEvent.h"

#include <catch2/catch_test_macros.hpp>

using vp330::MidiEvent;
using vp330::MidiNote;

TEST_CASE("MidiEvent: aggregate-initialisable", "[midi-event]") {
  const auto note = *MidiNote::try_from_int(60);
  MidiEvent ev{MidiEvent::Kind::NoteOn, note, 100, 0};
  REQUIRE(ev.kind == MidiEvent::Kind::NoteOn);
  REQUIRE(ev.note == note);
  REQUIRE(ev.velocity == 100);
  REQUIRE(ev.sample_offset == 0);
}

TEST_CASE("MidiEvent: NoteOff carries 0 velocity convention", "[midi-event]") {
  const auto note = *MidiNote::try_from_int(60);
  MidiEvent ev{MidiEvent::Kind::NoteOff, note, 0, 128};
  REQUIRE(ev.kind == MidiEvent::Kind::NoteOff);
  REQUIRE(ev.sample_offset == 128);
}
```

- [ ] **Step 2: Add the test to `tests/unit/CMakeLists.txt`**

Append:

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/MidiEventTest.cpp
```

- [ ] **Step 3: Run to confirm it fails**

```bash
cmake --build build --target vp330_tests
```

Expected: `vp330/engine/MidiEvent.h: No such file or directory`.

- [ ] **Step 4: Implement `MidiEvent`**

Create `domain/include/vp330/engine/MidiEvent.h`:

```cpp
#pragma once

#include "vp330/note/MidiNote.h"

namespace vp330 {

struct MidiEvent {
  enum class Kind { NoteOn, NoteOff };

  Kind kind;
  MidiNote note;
  int velocity;       // 0..127; NoteOff convention is 0
  int sample_offset;  // offset within the current render block
};

} // namespace vp330
```

- [ ] **Step 5: Build and run; confirm green**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R midi-event
```

Expected: both cases pass.

- [ ] **Step 6: Commit**

```bash
git add domain/include/vp330/engine/MidiEvent.h tests/unit/engine/MidiEventTest.cpp tests/unit/CMakeLists.txt
git commit -m "$(cat <<'EOF'
domain: add MidiEvent POD

NoteOn / NoteOff with note, velocity, sample_offset. Phase 1 scope; pitch
bend / CC will land alongside the bender and parameter store.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** test green.

---

## Task 6: `MidiSource`, `AudioSink`, `Clock` ports

**Files:**
- Create: `domain/include/vp330/ports/MidiSource.h`
- Create: `domain/include/vp330/ports/AudioSink.h`
- Create: `domain/include/vp330/ports/Clock.h`

**Design notes:**
- These are abstract interfaces; no implementation lives in `domain/`.
- The Phase 1 engine **does not consume them** at its API surface; adapters call `engine.note_on/note_off/render` directly. The ports exist because spec §9 lists them as Phase 1 deliverables, and Phase 2 will rewire the engine to consume `MidiSource` once event ordering across blocks matters.
- Header-only; no test files (an empty interface has no behaviour to test, and instantiating a mock derives no Phase 1 value).

- [ ] **Step 1: Create `MidiSource.h`**

```cpp
#pragma once

#include "vp330/engine/MidiEvent.h"

namespace vp330 {

class MidiSource {
 public:
  virtual ~MidiSource() = default;

  // Pulls the next event for the current block. Returns false when no further
  // events fit in the current block; the caller advances the block boundary
  // before calling again. The returned event's sample_offset is in the
  // current-block local frame.
  virtual bool next(MidiEvent& out) = 0;
};

} // namespace vp330
```

- [ ] **Step 2: Create `AudioSink.h`**

```cpp
#pragma once

#include <cstddef>

namespace vp330 {

class AudioSink {
 public:
  virtual ~AudioSink() = default;

  // Receives a deinterleaved stereo block.
  virtual void write(const float* left, const float* right, std::size_t frames) = 0;
};

} // namespace vp330
```

- [ ] **Step 3: Create `Clock.h`**

```cpp
#pragma once

namespace vp330 {

class Clock {
 public:
  virtual ~Clock() = default;
  virtual int sample_rate() const = 0;
};

} // namespace vp330
```

- [ ] **Step 4: Confirm domain still builds and isolation invariant holds**

```bash
cmake --build build --target vp330_domain
grep -rE '#include[[:space:]]*<(juce_|sndfile|alsa)' domain/ || echo "OK: domain/ is isolated."
```

Expected: build succeeds; isolation OK.

- [ ] **Step 5: Commit**

```bash
git add domain/include/vp330/ports/MidiSource.h \
        domain/include/vp330/ports/AudioSink.h \
        domain/include/vp330/ports/Clock.h
git commit -m "$(cat <<'EOF'
domain: add MidiSource, AudioSink, Clock port interfaces

Spec §9 Phase 1 deliverable. Pure abstract; the Phase 1 engine does not
consume them at its API surface (adapters call engine directly). Phase 2
will rewire the engine through MidiSource once cross-block event ordering
matters.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** ports compile in isolation; domain isolation grep clean.

---

## Task 7: `SynthesisEngine` skeleton — note state

**Files:**
- Create: `domain/include/vp330/engine/SynthesisEngine.h`
- Create: `domain/src/engine/SynthesisEngine.cpp`
- Remove: `domain/src/stub.cpp` (placeholder anchor TU is no longer needed)
- Modify: `domain/CMakeLists.txt` (replace `stub.cpp` with `src/engine/SynthesisEngine.cpp`)
- Create: `tests/unit/engine/SynthesisEngineTest.cpp`
- Modify: `tests/unit/CMakeLists.txt`

**Design notes:**
- Phase 1 engine API is direct: `note_on`, `note_off`, `render`. Internal state: 128 active flags + 128 phase counters.
- This task sets up the skeleton with state only — `render` exists but writes zeros. Task 8 fills in the square oscillator. Splitting these isolates "the state machine works" from "the audio comes out at the right pitch."
- `render` writes into caller-owned deinterleaved stereo buffers (`float* left`, `float* right`). Matches the JUCE plugin's `processBlock` model; CLI will allocate its own.

- [ ] **Step 1: Write the failing test**

Create `tests/unit/engine/SynthesisEngineTest.cpp`:

```cpp
#include "vp330/engine/SynthesisEngine.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using vp330::MidiNote;
using vp330::SynthesisEngine;

TEST_CASE("SynthesisEngine: stores its sample rate", "[engine]") {
  SynthesisEngine engine{48000};
  REQUIRE(engine.sample_rate() == 48000);
}

TEST_CASE("SynthesisEngine: tracks active notes via note_on / note_off", "[engine]") {
  SynthesisEngine engine{48000};
  const auto c4 = vp330::kMidiC4;

  REQUIRE_FALSE(engine.is_note_active(c4));

  engine.note_on(c4);
  REQUIRE(engine.is_note_active(c4));

  engine.note_off(c4);
  REQUIRE_FALSE(engine.is_note_active(c4));
}

TEST_CASE("SynthesisEngine: independent state per note", "[engine]") {
  SynthesisEngine engine{48000};
  const auto c4 = vp330::kMidiC4;
  const auto a4 = vp330::kMidiA4;

  engine.note_on(c4);
  engine.note_on(a4);
  REQUIRE(engine.is_note_active(c4));
  REQUIRE(engine.is_note_active(a4));

  engine.note_off(c4);
  REQUIRE_FALSE(engine.is_note_active(c4));
  REQUIRE(engine.is_note_active(a4));
}

TEST_CASE("SynthesisEngine: render returns silence with no notes active", "[engine]") {
  SynthesisEngine engine{48000};
  std::vector<float> left(128, 0.5f);
  std::vector<float> right(128, 0.5f);

  engine.render(left.data(), right.data(), left.size());

  for (auto v : left) REQUIRE(v == 0.0f);
  for (auto v : right) REQUIRE(v == 0.0f);
}
```

- [ ] **Step 2: Add the test to `tests/unit/CMakeLists.txt`**

Append:

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/SynthesisEngineTest.cpp
```

- [ ] **Step 3: Run to confirm it fails**

```bash
cmake --build build --target vp330_tests
```

Expected: `vp330/engine/SynthesisEngine.h: No such file or directory`.

- [ ] **Step 4: Implement the header**

Create `domain/include/vp330/engine/SynthesisEngine.h`:

```cpp
#pragma once

#include "vp330/note/MidiNote.h"

#include <array>
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
  std::array<bool, 128> active_{};
  std::array<double, 128> phase_{};
};

} // namespace vp330
```

- [ ] **Step 5: Implement the source**

Create `domain/src/engine/SynthesisEngine.cpp`:

```cpp
#include "vp330/engine/SynthesisEngine.h"

#include <algorithm>

namespace vp330 {

SynthesisEngine::SynthesisEngine(int sample_rate) : sample_rate_{sample_rate} {}

void SynthesisEngine::note_on(MidiNote note) {
  const auto idx = static_cast<std::size_t>(note.value());
  active_[idx] = true;
  phase_[idx] = 0.0;
}

void SynthesisEngine::note_off(MidiNote note) {
  active_[static_cast<std::size_t>(note.value())] = false;
}

bool SynthesisEngine::is_note_active(MidiNote note) const {
  return active_[static_cast<std::size_t>(note.value())];
}

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  // Task 8 fills this in. For now, write silence so the state-machine tests
  // can verify is_note_active without depending on audio output.
  std::fill_n(left, frames, 0.0f);
  std::fill_n(right, frames, 0.0f);
}

} // namespace vp330
```

- [ ] **Step 6: Update `domain/CMakeLists.txt`**

Replace the source list. Edit `domain/CMakeLists.txt`:

```cmake
add_library(vp330_domain STATIC
    src/engine/SynthesisEngine.cpp
)

target_include_directories(vp330_domain
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(vp330_domain PUBLIC cxx_std_20)
```

- [ ] **Step 7: Remove the stub TU**

```bash
git rm domain/src/stub.cpp
```

- [ ] **Step 8: Build and run; confirm green**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R engine
```

Expected: 4 cases pass.

- [ ] **Step 9: Verify domain isolation**

```bash
grep -rE '#include[[:space:]]*<(juce_|sndfile|alsa)' domain/ || echo "OK: domain/ is isolated."
```

Expected: clean.

- [ ] **Step 10: Commit**

```bash
git add domain/include/vp330/engine/SynthesisEngine.h \
        domain/src/engine/SynthesisEngine.cpp \
        domain/CMakeLists.txt \
        domain/src/stub.cpp \
        tests/unit/engine/SynthesisEngineTest.cpp \
        tests/unit/CMakeLists.txt
git commit -m "$(cat <<'EOF'
domain: SynthesisEngine skeleton with note-state machine

128-slot active set + per-note phase counter; note_on / note_off /
is_note_active. render() writes silence; Task 8 fills it in with the
naïve square oscillator. Removes the placeholder stub.cpp anchor.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** state-machine tests pass; render is a documented no-op until Task 8.

---

## Task 8: `SynthesisEngine::render` — naïve square per active note

**Files:**
- Modify: `domain/src/engine/SynthesisEngine.cpp` (replace the silence body in `render`)
- Modify: `tests/unit/engine/SynthesisEngineTest.cpp` (add audio-output cases)

**Design notes:**
- Per render frame: for each active note, produce `+1` if its phase is `< 0.5`, else `-1`; advance phase by `freq / sample_rate`; wrap to `[0, 1)`. Sum across active notes; scale by a per-voice gain to keep amplitude reasonable; mono dup'd to L/R.
- Per-voice gain = `0.05`. Magic number for Phase 1; will be replaced by proper section-level gain once `KeyGate` and `Section` arrive in Phases 2–3.
- Phase is reset at `note_on` (Task 7). Subsequent re-attacks of the same note start at zero phase. A real VP-330 doesn't do this — but the TOD architecture in Phase 2 makes the question moot (one continuous oscillator per pitch class with gates).
- L1 verifies a single note produces a square at the expected fundamental via zero-crossing rate over a long enough buffer to be statistically stable.

- [ ] **Step 1: Add the failing audio tests**

Append to `tests/unit/engine/SynthesisEngineTest.cpp`:

```cpp
#include <cmath>

namespace {

// Returns the dominant fundamental in Hz estimated by counting zero crossings
// of a long enough buffer. For a square the count is 2*frequency*duration
// (two crossings per cycle). Tolerance is dominated by integer counting.
double zero_crossing_frequency(const std::vector<float>& buf, int sample_rate) {
  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i) {
    const bool prev_pos = buf[i - 1] >= 0.0f;
    const bool curr_pos = buf[i]     >= 0.0f;
    if (prev_pos != curr_pos) ++crossings;
  }
  const double seconds = static_cast<double>(buf.size()) / sample_rate;
  return static_cast<double>(crossings) / (2.0 * seconds);
}

double rms(const std::vector<float>& buf) {
  double acc = 0.0;
  for (auto s : buf) acc += static_cast<double>(s) * s;
  return std::sqrt(acc / static_cast<double>(buf.size()));
}

} // namespace

TEST_CASE("SynthesisEngine: single C4 renders a square at ~261.63 Hz", "[engine][audio]") {
  constexpr int sample_rate = 48000;
  constexpr std::size_t frames = sample_rate;  // 1.0 s of audio
  SynthesisEngine engine{sample_rate};
  engine.note_on(vp330::kMidiC4);

  std::vector<float> left(frames), right(frames);
  engine.render(left.data(), right.data(), frames);

  const double f = zero_crossing_frequency(left, sample_rate);
  REQUIRE(f == Catch::Approx(261.63).margin(2.0));

  // Square at this gain (0.05) and unity peak: rms ≈ 0.05.
  REQUIRE(rms(left) == Catch::Approx(0.05).margin(0.005));

  // Mono dup: L == R sample-for-sample.
  for (std::size_t i = 0; i < frames; ++i) REQUIRE(left[i] == right[i]);
}

TEST_CASE("SynthesisEngine: silence after note_off", "[engine][audio]") {
  constexpr int sample_rate = 48000;
  SynthesisEngine engine{sample_rate};
  engine.note_on(vp330::kMidiC4);
  engine.note_off(vp330::kMidiC4);

  std::vector<float> left(1024), right(1024);
  engine.render(left.data(), right.data(), left.size());

  REQUIRE(rms(left) == 0.0);
}
```

You also need `#include <catch2/catch_approx.hpp>` near the top — if it isn't already there, add it after the other catch include.

- [ ] **Step 2: Run to confirm the new cases fail**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R "engine.*audio"
```

Expected: zero-crossing test fails (currently 0 Hz, expected 261.63); RMS test fails (currently 0.0, expected ≈0.05).

- [ ] **Step 3: Replace `render` with the naïve oscillator**

Edit `domain/src/engine/SynthesisEngine.cpp`. Replace the body of `render`:

```cpp
#include "vp330/engine/SynthesisEngine.h"

#include "vp330/values/Pitch.h"

#include <cstddef>

namespace vp330 {

namespace {
constexpr float kPerVoiceGain = 0.05f;  // Phase 1 placeholder; replaced in Phase 3.
}

SynthesisEngine::SynthesisEngine(int sample_rate) : sample_rate_{sample_rate} {}

void SynthesisEngine::note_on(MidiNote note) {
  const auto idx = static_cast<std::size_t>(note.value());
  active_[idx] = true;
  phase_[idx] = 0.0;
}

void SynthesisEngine::note_off(MidiNote note) {
  active_[static_cast<std::size_t>(note.value())] = false;
}

bool SynthesisEngine::is_note_active(MidiNote note) const {
  return active_[static_cast<std::size_t>(note.value())];
}

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  const double sr = static_cast<double>(sample_rate_);
  for (std::size_t i = 0; i < frames; ++i) {
    float sum = 0.0f;
    for (int n = 0; n < 128; ++n) {
      const auto idx = static_cast<std::size_t>(n);
      if (!active_[idx]) continue;
      const auto note = MidiNote{n};
      const double freq = Pitch::from_midi_note(note).to_hertz().value();
      const double inc = freq / sr;
      auto& p = phase_[idx];
      sum += (p < 0.5 ? 1.0f : -1.0f);
      p += inc;
      if (p >= 1.0) p -= 1.0;
    }
    const float out = sum * kPerVoiceGain;
    left[i] = out;
    right[i] = out;
  }
}

} // namespace vp330
```

(The `algorithm` include is no longer needed — `std::fill_n` is gone — so drop it from the file's includes.)

- [ ] **Step 4: Build and run; confirm green**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R engine
```

Expected: all engine tests pass, including the audio cases. The `single C4 renders a square at ~261.63 Hz` test is the canary for the Phase 1 golden test in Task 12.

- [ ] **Step 5: Commit**

```bash
git add domain/src/engine/SynthesisEngine.cpp tests/unit/engine/SynthesisEngineTest.cpp
git commit -m "$(cat <<'EOF'
engine: naïve square per active note, summed and mono-dup'd

Phase 1 oscillator: per-MIDI-note phase counter producing ±1 at the
equal-temperament frequency for that note, summed across active notes
and scaled by a placeholder per-voice gain (0.05). Phase 2 replaces this
with the TopOctaveDivider + KeyGate path.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** zero-crossing rate of a single C4 render is within ±2 Hz of 261.63; RMS within ±0.005 of 0.05.

---

## Task 9: `SmfReader` in `infrastructure/cli`

**Files:**
- Create: `infrastructure/cli/SmfReader.h`
- Create: `infrastructure/cli/SmfReader.cpp`
- Modify: `infrastructure/cli/CMakeLists.txt` (extract SmfReader into a small static lib so tests can link it)
- Create: `tests/unit/cli/SmfReaderTest.cpp`
- Modify: `tests/unit/CMakeLists.txt`

**Design notes:**
- Minimal in-tree Standard MIDI File reader. Handles format-0 and format-1 files (Phase 1 fixtures are format 0; format 1 single-track is folded by reading both tracks into one timeline). Handles tempo meta events, NoteOn/NoteOff (with the velocity-0 NoteOn → NoteOff convention), running status, and skips all other channel/meta/SysEx events.
- Returns events with absolute time in seconds (computed from PPQN + accumulated tempo). The CLI converts seconds to sample offsets per render block.
- Lives under `infrastructure/cli/` because it's an adapter — domain stays pure-stdlib. Extracted into a static lib `vp330_cli_smf` so `vp330_render` and `vp330_tests` both link it.
- Throws `std::runtime_error` on malformed input. We assert the file is well-formed enough; a real DAW export goes through this code path so it has to be more than a toy.

- [ ] **Step 1: Write the failing test**

Create `tests/unit/cli/SmfReaderTest.cpp`:

```cpp
#include "SmfReader.h"

#include "vp330/engine/MidiEvent.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

using vp330::MidiEvent;
using vp330::cli::ParsedMidi;
using vp330::cli::read_smf;

namespace {

std::filesystem::path write_temp_midi(const std::vector<std::uint8_t>& bytes,
                                      const std::string& tag) {
  auto p = std::filesystem::temp_directory_path() / ("vp330_smf_" + tag + ".mid");
  std::ofstream f(p, std::ios::binary);
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return p;
}

// Format 0, PPQN 480, 120 BPM, NoteOn C4 vel=100 at t=0, NoteOff C4 at +960 ticks (1.0 s).
std::vector<std::uint8_t> simple_c4_smf() {
  return {
      'M','T','h','d', 0,0,0,6,    // chunk + size
      0,0,                          // format 0
      0,1,                          // 1 track
      0x01,0xE0,                    // PPQN 480
      'M','T','r','k', 0,0,0,19,    // track size = 19 bytes
      0x00, 0xFF,0x51,0x03, 0x07,0xA1,0x20,  // tempo 500000 µs/quarter (120 BPM)
      0x00, 0x90, 60, 100,                   // NoteOn  ch0 C4 vel=100
      0x87,0x40, 0x80, 60, 0,                // dt=960 ticks, NoteOff ch0 C4 vel=0
      0x00, 0xFF,0x2F,0x00,                  // EOT
  };
}

// Same as simple_c4_smf but the NoteOff is encoded as NoteOn vel=0 (running status).
std::vector<std::uint8_t> running_status_c4_smf() {
  return {
      'M','T','h','d', 0,0,0,6,
      0,0, 0,1, 0x01,0xE0,
      'M','T','r','k', 0,0,0,18,
      0x00, 0xFF,0x51,0x03, 0x07,0xA1,0x20,  // tempo
      0x00, 0x90, 60, 100,                   // NoteOn  ch0 C4 vel=100
      0x87,0x40, 60, 0,                      // running status: omit 0x90, NoteOn C4 vel=0 == NoteOff
      0x00, 0xFF,0x2F,0x00,                  // EOT
  };
}

} // namespace

TEST_CASE("SmfReader: parses simple format-0 NoteOn/NoteOff pair", "[smf]") {
  const auto path = write_temp_midi(simple_c4_smf(), "simple");
  const auto parsed = read_smf(path.string());

  REQUIRE(parsed.ppqn == 480);
  REQUIRE(parsed.events.size() == 2);

  REQUIRE(parsed.events[0].event.kind == MidiEvent::Kind::NoteOn);
  REQUIRE(parsed.events[0].event.note.value() == 60);
  REQUIRE(parsed.events[0].event.velocity == 100);
  REQUIRE(parsed.events[0].time_seconds == Catch::Approx(0.0).margin(1e-9));

  REQUIRE(parsed.events[1].event.kind == MidiEvent::Kind::NoteOff);
  REQUIRE(parsed.events[1].event.note.value() == 60);
  REQUIRE(parsed.events[1].time_seconds == Catch::Approx(1.0).margin(1e-9));
}

TEST_CASE("SmfReader: handles running status and NoteOn vel=0 → NoteOff", "[smf]") {
  const auto path = write_temp_midi(running_status_c4_smf(), "running");
  const auto parsed = read_smf(path.string());

  REQUIRE(parsed.events.size() == 2);
  REQUIRE(parsed.events[1].event.kind == MidiEvent::Kind::NoteOff);
  REQUIRE(parsed.events[1].time_seconds == Catch::Approx(1.0).margin(1e-9));
}

TEST_CASE("SmfReader: rejects missing MThd", "[smf]") {
  const std::vector<std::uint8_t> bad = {'X','Y','Z','Z'};
  const auto path = write_temp_midi(bad, "bad");
  REQUIRE_THROWS_AS(read_smf(path.string()), std::runtime_error);
}
```

- [ ] **Step 2: Wire the test into the build**

Append to `tests/unit/CMakeLists.txt`:

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/cli/SmfReaderTest.cpp
```

(`vp330_tests` will need to link `vp330_cli_smf`; we add that link in Step 5.)

- [ ] **Step 3: Run to confirm it fails**

```bash
cmake --build build --target vp330_tests
```

Expected: compile error — `SmfReader.h: No such file or directory`.

- [ ] **Step 4: Implement `SmfReader`**

Create `infrastructure/cli/SmfReader.h`:

```cpp
#pragma once

#include "vp330/engine/MidiEvent.h"

#include <string>
#include <vector>

namespace vp330::cli {

struct TimedMidiEvent {
  double time_seconds;
  vp330::MidiEvent event;
};

struct ParsedMidi {
  int ppqn = 0;
  std::vector<TimedMidiEvent> events;
};

ParsedMidi read_smf(const std::string& path);

} // namespace vp330::cli
```

Create `infrastructure/cli/SmfReader.cpp`:

```cpp
#include "SmfReader.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace vp330::cli {

namespace {

class Cursor {
 public:
  explicit Cursor(const std::vector<std::uint8_t>& bytes) : bytes_{bytes} {}

  std::uint8_t u8() {
    require(1);
    return bytes_[pos_++];
  }
  std::uint16_t u16_be() {
    const auto hi = u8();
    const auto lo = u8();
    return static_cast<std::uint16_t>((hi << 8) | lo);
  }
  std::uint32_t u32_be() {
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v = (v << 8) | u8();
    return v;
  }
  std::uint32_t vlq() {
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
      const auto b = u8();
      v = (v << 7) | (b & 0x7Fu);
      if ((b & 0x80u) == 0) return v;
    }
    throw std::runtime_error("SMF: VLQ exceeds 4 bytes");
  }
  void skip(std::size_t n) {
    require(n);
    pos_ += n;
  }
  std::uint8_t peek() {
    require(1);
    return bytes_[pos_];
  }
  std::size_t pos() const { return pos_; }

 private:
  void require(std::size_t n) const {
    if (pos_ + n > bytes_.size()) throw std::runtime_error("SMF: unexpected EOF");
  }

  const std::vector<std::uint8_t>& bytes_;
  std::size_t pos_ = 0;
};

std::vector<std::uint8_t> read_all(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("SMF: cannot open " + path);
  return {std::istreambuf_iterator<char>(f), {}};
}

bool match_tag(Cursor& c, const char (&tag)[5]) {
  for (int i = 0; i < 4; ++i) {
    if (c.u8() != static_cast<std::uint8_t>(tag[i])) return false;
  }
  return true;
}

} // namespace

ParsedMidi read_smf(const std::string& path) {
  const auto bytes = read_all(path);
  Cursor c{bytes};

  if (!match_tag(c, "MThd")) throw std::runtime_error("SMF: missing MThd");
  if (c.u32_be() != 6) throw std::runtime_error("SMF: bad MThd size");
  const auto format = c.u16_be();
  const auto ntracks = c.u16_be();
  const auto division = c.u16_be();
  if (format > 1) throw std::runtime_error("SMF: format >1 unsupported");
  if (division & 0x8000u) throw std::runtime_error("SMF: SMPTE timing unsupported");
  if (ntracks == 0) throw std::runtime_error("SMF: zero tracks");

  ParsedMidi out;
  out.ppqn = division;

  std::uint32_t us_per_quarter = 500'000;  // 120 BPM default

  for (std::uint16_t t = 0; t < ntracks; ++t) {
    if (!match_tag(c, "MTrk")) throw std::runtime_error("SMF: missing MTrk");
    const auto track_bytes = c.u32_be();
    const auto track_end = c.pos() + track_bytes;

    double seconds = 0.0;
    std::uint8_t running_status = 0;

    while (c.pos() < track_end) {
      const auto delta = c.vlq();
      const double seconds_per_tick =
          static_cast<double>(us_per_quarter) / 1'000'000.0 / static_cast<double>(out.ppqn);
      seconds += static_cast<double>(delta) * seconds_per_tick;

      std::uint8_t status = c.peek();
      if (status >= 0x80u) {
        running_status = status;
        (void)c.u8();  // consume status byte
      } else {
        if (running_status == 0) throw std::runtime_error("SMF: running status with no prior");
        status = running_status;
      }

      if (status == 0xFFu) {
        const auto meta_type = c.u8();
        const auto len = c.vlq();
        if (meta_type == 0x51u && len == 3) {
          const std::uint32_t a = c.u8();
          const std::uint32_t b = c.u8();
          const std::uint32_t cc = c.u8();
          us_per_quarter = (a << 16) | (b << 8) | cc;
        } else if (meta_type == 0x2Fu && len == 0) {
          break;
        } else {
          c.skip(len);
        }
      } else if (status == 0xF0u || status == 0xF7u) {
        const auto len = c.vlq();
        c.skip(len);
      } else {
        const auto kind = static_cast<std::uint8_t>(status & 0xF0u);
        if (kind == 0x90u || kind == 0x80u) {
          const auto note_byte = c.u8();
          const auto velocity = c.u8();
          const auto note_opt = vp330::MidiNote::try_from_int(note_byte);
          if (!note_opt) throw std::runtime_error("SMF: note out of range");
          const bool is_off = (kind == 0x80u) || (kind == 0x90u && velocity == 0);
          out.events.push_back({seconds, vp330::MidiEvent{
                                             is_off ? vp330::MidiEvent::Kind::NoteOff
                                                    : vp330::MidiEvent::Kind::NoteOn,
                                             *note_opt,
                                             velocity,
                                             0}});
        } else if (kind == 0xA0u || kind == 0xB0u || kind == 0xE0u) {
          (void)c.u8();
          (void)c.u8();
        } else if (kind == 0xC0u || kind == 0xD0u) {
          (void)c.u8();
        } else {
          throw std::runtime_error("SMF: unknown channel event");
        }
      }
    }
  }
  return out;
}

} // namespace vp330::cli
```

- [ ] **Step 5: Update `infrastructure/cli/CMakeLists.txt`**

Replace the file contents:

```cmake
add_library(vp330_cli_smf STATIC
    SmfReader.cpp
)

target_include_directories(vp330_cli_smf
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vp330_cli_smf PUBLIC
    vp330_domain
)

target_compile_features(vp330_cli_smf PUBLIC cxx_std_20)

add_executable(vp330_render
    render_main.cpp
)

target_link_libraries(vp330_render PRIVATE
    vp330_cli_smf
    PkgConfig::SNDFILE
)

target_compile_features(vp330_render PRIVATE cxx_std_20)
```

- [ ] **Step 6: Link `vp330_cli_smf` into `vp330_tests`**

Edit `tests/CMakeLists.txt`. Add after the existing `target_link_libraries`:

```cmake
target_link_libraries(vp330_tests PRIVATE
    vp330_cli_smf
)
```

(Or fold it into the main `target_link_libraries` block; either works.)

- [ ] **Step 7: Build and run**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R smf
```

Expected: 3 test cases pass.

- [ ] **Step 8: Verify domain isolation**

```bash
grep -rE '#include[[:space:]]*<(juce_|sndfile|alsa)' domain/ || echo "OK: domain/ is isolated."
```

Expected: clean. (`vp330_cli_smf` lives in `infrastructure/`, not `domain/`.)

- [ ] **Step 9: Commit**

```bash
git add infrastructure/cli/SmfReader.h \
        infrastructure/cli/SmfReader.cpp \
        infrastructure/cli/CMakeLists.txt \
        tests/CMakeLists.txt \
        tests/unit/cli/SmfReaderTest.cpp \
        tests/unit/CMakeLists.txt
git commit -m "$(cat <<'EOF'
cli: minimal in-tree Standard MIDI File reader

Format 0/1, single MTrk per track. Tempo meta, NoteOn/NoteOff (with
velocity-0 NoteOn → NoteOff convention), running status. Skips other
channel and meta events. Extracted into vp330_cli_smf static lib so
vp330_render and vp330_tests can both link it; domain isolation
preserved (this lives under infrastructure/, not domain/).

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** SmfReader tests pass; domain isolation grep clean.

---

## Task 10: CLI wires SmfReader → SynthesisEngine → libsndfile

**Files:**
- Modify: `infrastructure/cli/render_main.cpp` (replace silence loop with engine pump)
- Modify: `infrastructure/cli/CMakeLists.txt` (vp330_render now needs to link vp330_domain transitively via vp330_cli_smf — already done in Task 9)

**Design notes:**
- New flag semantics: `--input <path>` is now required if you want notes (it's still accepted as optional — without it, the CLI renders silence, which preserves the Phase 0 walking-skeleton golden test). This matters: the existing `silence-1s.mid` golden fixture must still render to silence.
- Render loop: build a sorted list of `(sample_index, MidiEvent)` from the SMF; iterate render frames in chunks (or per-sample is fine for Phase 1); apply each event when the sample index reaches its timestamp; call `engine.render` for the chunk; write the WAV.
- For simplicity and correctness: render a 1-sample-per-chunk loop in Phase 1. The CLI is offline; performance doesn't matter. (Optimisation can come when an L4 capture run shows it.)

- [ ] **Step 1: Replace `render_main.cpp`**

Overwrite `infrastructure/cli/render_main.cpp`:

```cpp
#include "SmfReader.h"

#include "vp330/engine/MidiEvent.h"
#include "vp330/engine/SynthesisEngine.h"

#include <sndfile.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

namespace {

struct Args {
  std::string input_midi;
  std::string output_wav;
  double duration_seconds = 1.0;
  int sample_rate = 48000;
};

bool parse_double(const char* str, double& out, const char* name) {
  char* end = nullptr;
  out = std::strtod(str, &end);
  if (end == str || *end != '\0' || !std::isfinite(out) || out <= 0.0) {
    std::fprintf(stderr, "invalid numeric value for %s: %s\n", name, str);
    return false;
  }
  return true;
}

bool parse_int(const char* str, int& out, const char* name) {
  char* end = nullptr;
  const long v = std::strtol(str, &end, 10);
  if (end == str || *end != '\0' || v <= 0 || v > 1'000'000'000) {
    std::fprintf(stderr, "invalid integer for %s: %s\n", name, str);
    return false;
  }
  out = static_cast<int>(v);
  return true;
}

bool parse_args(int argc, char** argv, Args& out) {
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto next = [&](const char* name) -> const char* {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "missing value for %s\n", name);
        return nullptr;
      }
      return argv[++i];
    };
    if (a == "--input") {
      if (auto v = next("--input")) out.input_midi = v;
      else return false;
    } else if (a == "--output") {
      if (auto v = next("--output")) out.output_wav = v;
      else return false;
    } else if (a == "--duration") {
      if (auto v = next("--duration")) {
        if (!parse_double(v, out.duration_seconds, "--duration")) return false;
      } else return false;
    } else if (a == "--sample-rate") {
      if (auto v = next("--sample-rate")) {
        if (!parse_int(v, out.sample_rate, "--sample-rate")) return false;
      } else return false;
    } else {
      std::fprintf(stderr, "unknown arg: %s\n", a.c_str());
      return false;
    }
  }
  if (out.output_wav.empty()) {
    std::fprintf(stderr, "--output is required\n");
    return false;
  }
  return true;
}

struct ScheduledEvent {
  std::size_t sample_index;
  vp330::MidiEvent event;
};

std::vector<ScheduledEvent> schedule(const vp330::cli::ParsedMidi& parsed, int sample_rate,
                                     std::size_t max_frames) {
  std::vector<ScheduledEvent> out;
  out.reserve(parsed.events.size());
  for (const auto& te : parsed.events) {
    const auto idx = static_cast<std::size_t>(
        std::llround(te.time_seconds * static_cast<double>(sample_rate)));
    if (idx >= max_frames) continue;
    out.push_back({idx, te.event});
  }
  std::stable_sort(out.begin(), out.end(),
                   [](const ScheduledEvent& a, const ScheduledEvent& b) {
                     return a.sample_index < b.sample_index;
                   });
  return out;
}

// Interleave deinterleaved L/R into a libsndfile-friendly buffer.
void interleave(const std::vector<float>& left, const std::vector<float>& right,
                std::vector<float>& out) {
  out.resize(left.size() * 2);
  for (std::size_t i = 0; i < left.size(); ++i) {
    out[2 * i] = left[i];
    out[2 * i + 1] = right[i];
  }
}

} // namespace

int main(int argc, char** argv) {
  Args args;
  if (!parse_args(argc, argv, args)) {
    std::fprintf(stderr, "usage: vp330_render --output <wav> [--input <mid>] "
                         "[--duration <sec>] [--sample-rate <hz>]\n");
    return 2;
  }

  const double frame_count_d = args.duration_seconds * static_cast<double>(args.sample_rate);
  if (!std::isfinite(frame_count_d) || frame_count_d <= 0.0 ||
      frame_count_d > static_cast<double>(std::numeric_limits<sf_count_t>::max())) {
    std::fprintf(stderr, "render too long: duration*sample_rate=%g\n", frame_count_d);
    return 2;
  }
  const auto frames = static_cast<std::size_t>(frame_count_d);

  vp330::cli::ParsedMidi parsed;
  if (!args.input_midi.empty()) {
    try {
      parsed = vp330::cli::read_smf(args.input_midi);
    } catch (const std::exception& e) {
      std::fprintf(stderr, "read_smf failed: %s\n", e.what());
      return 1;
    }
  }
  const auto events = schedule(parsed, args.sample_rate, frames);

  vp330::SynthesisEngine engine{args.sample_rate};

  SF_INFO info{};
  info.samplerate = args.sample_rate;
  info.channels = 2;
  info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

  SNDFILE* sf = sf_open(args.output_wav.c_str(), SFM_WRITE, &info);
  if (!sf) {
    std::fprintf(stderr, "sf_open failed: %s\n", sf_strerror(nullptr));
    return 1;
  }

  constexpr std::size_t kBlock = 1024;
  std::vector<float> left(kBlock), right(kBlock), interleaved;
  std::size_t produced = 0;
  std::size_t event_idx = 0;

  while (produced < frames) {
    const auto block = std::min<std::size_t>(kBlock, frames - produced);

    // Apply any events whose absolute sample index falls within this block.
    while (event_idx < events.size() && events[event_idx].sample_index < produced + block) {
      const auto& ev = events[event_idx].event;
      switch (ev.kind) {
        case vp330::MidiEvent::Kind::NoteOn:
          engine.note_on(ev.note);
          break;
        case vp330::MidiEvent::Kind::NoteOff:
          engine.note_off(ev.note);
          break;
      }
      ++event_idx;
    }

    left.assign(block, 0.0f);
    right.assign(block, 0.0f);
    engine.render(left.data(), right.data(), block);
    interleave(left, right, interleaved);

    const auto wrote = sf_writef_float(sf, interleaved.data(), static_cast<sf_count_t>(block));
    if (wrote != static_cast<sf_count_t>(block)) {
      std::fprintf(stderr, "sf_writef_float short write\n");
      sf_close(sf);
      return 1;
    }
    produced += block;
  }

  sf_close(sf);
  return 0;
}
```

- [ ] **Step 2: Build**

```bash
cmake --build build --target vp330_render
```

Expected: clean build.

- [ ] **Step 3: Smoke-test silence still renders silence**

```bash
build/infrastructure/cli/vp330_render \
  --input tests/golden/fixtures/silence-1s.mid \
  --output /tmp/vp330-silence-test.wav \
  --duration 1.0 --sample-rate 48000
```

Expected: exit 0; output WAV is silent.

- [ ] **Step 4: Re-run the existing silence golden test**

```bash
ctest --test-dir build --output-on-failure -R "silence renders to silence"
```

Expected: green. The Phase 0 walking-skeleton test still passes through the new render path.

- [ ] **Step 5: Commit**

```bash
git add infrastructure/cli/render_main.cpp
git commit -m "$(cat <<'EOF'
cli: render via SynthesisEngine, scheduling SMF events into blocks

Replaces the silence renderer. Reads --input via SmfReader, schedules
events at sample-accurate offsets, pumps SynthesisEngine in 1024-frame
blocks, writes 24-bit stereo WAV via libsndfile. With no --input the
renderer emits silence; the Phase 0 silence-1s.mid golden test still
passes through this path.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** silence golden still green; CLI handles both empty and note-bearing inputs.

---

## Task 11: JUCE plugin wires `MidiBuffer` → `SynthesisEngine`

**Files:**
- Modify: `infrastructure/juce/PluginProcessor.h` (own a `SynthesisEngine` instance)
- Modify: `infrastructure/juce/PluginProcessor.cpp` (process MIDI, render audio)

**Design notes:**
- Construct the engine lazily in `prepareToPlay` once the host's sample rate is known. Store as `std::unique_ptr<SynthesisEngine>`.
- `processBlock`: iterate `MidiBuffer` events in order; for each NoteOn (vel>0) call `engine.note_on`; for NoteOff or NoteOn vel=0 call `engine.note_off`. Then call `engine.render` into channel-0 (`left`) and channel-1 (`right`). Mono output: copy channel 0 into channel 1 — already what the engine does. With a mono bus, copy left into a discarded right buffer.
- We don't sub-divide the block at MIDI-event boundaries in Phase 1. The engine has no envelopes yet; a small (sub-buffer) latency on note transitions is inaudible at 48 kHz / 256-frame blocks. Phase 2's `KeyGate` gets exact-sample triggering.

- [ ] **Step 1: Update `PluginProcessor.h`**

Replace contents of `infrastructure/juce/PluginProcessor.h`:

```cpp
#pragma once

#include "vp330/engine/SynthesisEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

class VP330Processor : public juce::AudioProcessor {
 public:
  VP330Processor();
  ~VP330Processor() override = default;

  void prepareToPlay(double sample_rate, int samples_per_block) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

  juce::AudioProcessorEditor* createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  const juce::String getName() const override { return "VP330"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock&) override {}
  void setStateInformation(const void*, int) override {}

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

 private:
  std::unique_ptr<vp330::SynthesisEngine> engine_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VP330Processor)
};
```

- [ ] **Step 2: Update `PluginProcessor.cpp`**

Replace contents of `infrastructure/juce/PluginProcessor.cpp`:

```cpp
#include "PluginProcessor.h"

#include "vp330/engine/MidiEvent.h"
#include "vp330/note/MidiNote.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstddef>

VP330Processor::VP330Processor()
    : juce::AudioProcessor(
          BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
}

void VP330Processor::prepareToPlay(double sample_rate, int /*samples_per_block*/) {
  engine_ = std::make_unique<vp330::SynthesisEngine>(static_cast<int>(sample_rate));
}

void VP330Processor::releaseResources() {
  engine_.reset();
}

void VP330Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
  juce::ScopedNoDenormals no_denormals;

  if (!engine_) {
    buffer.clear();
    return;
  }

  for (const auto meta : midi) {
    const auto& msg = meta.getMessage();
    if (msg.isNoteOn()) {
      const auto note = vp330::MidiNote::try_from_int(msg.getNoteNumber());
      if (note) engine_->note_on(*note);
    } else if (msg.isNoteOff()) {
      const auto note = vp330::MidiNote::try_from_int(msg.getNoteNumber());
      if (note) engine_->note_off(*note);
    }
  }

  const auto num_samples = static_cast<std::size_t>(buffer.getNumSamples());
  auto* left = buffer.getWritePointer(0);
  if (buffer.getNumChannels() >= 2) {
    auto* right = buffer.getWritePointer(1);
    engine_->render(left, right, num_samples);
  } else {
    // Mono bus: render into left, discard the engine's right output.
    std::vector<float> right(num_samples, 0.0f);
    engine_->render(left, right.data(), num_samples);
  }
}

bool VP330Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto& out = layouts.getMainOutputChannelSet();
  return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new VP330Processor();
}
```

- [ ] **Step 3: Build**

```bash
cmake --build build --target VP330_Standalone
```

Expected: clean build (the standalone target is the lightest cross-format check).

- [ ] **Step 4: Smoke-test the standalone (manual)**

Launch the standalone, attach a MIDI keyboard or virtual MIDI source, play a note. Expected: a square at the played pitch comes out. (Skip if no MIDI source is at hand; the golden test in Task 12 covers the engine path through the CLI binary.)

- [ ] **Step 5: Run the test suite to confirm no regressions**

```bash
ctest --test-dir build --output-on-failure
```

Expected: all unit + golden tests green.

- [ ] **Step 6: Commit**

```bash
git add infrastructure/juce/PluginProcessor.h infrastructure/juce/PluginProcessor.cpp
git commit -m "$(cat <<'EOF'
juce: route MidiBuffer through SynthesisEngine in processBlock

Engine constructed in prepareToPlay using the host's sample rate; reset in
releaseResources. NoteOn / NoteOff events are dispatched directly; the
engine is rendered for the entire block (Phase 1 has no envelopes that
would care about sub-block timing — Phase 2's KeyGate handles that).

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** standalone builds; existing tests still green.

---

## Task 12: Golden test — render `single-c4-1s.mid`, assert fundamental + RMS

**Files:**
- Create: `tools/golden/gen-single-c4-fixture.py`
- Generate: `tests/golden/fixtures/single-c4-1s.mid` (committed; ~38 bytes, not LFS — small enough to keep inline)
- Modify: `tests/golden/CMakeLists.txt` (no change needed — fixtures are read by path, not compiled in)
- Modify: `tests/golden/golden_test.cpp` (add the new TEST_CASE)

**Design notes:**
- The fixture: format-0 SMF, PPQN 480, 120 BPM, NoteOn C4 vel=100 at t=0, NoteOff C4 at t=960 ticks (1.0 s).
- The test renders the fixture for 1.0 s through the same `render_fixture` helper Phase 0 introduced. Then it estimates the dominant frequency by zero-crossing rate over the rendered left channel and checks RMS.
- We assert tolerance ±2 Hz on fundamental and ±0.005 on RMS (around 0.05). These tolerances are generous — Phase 1 oscillator is ideal so the actual numbers will be much tighter, but we leave room for sample-edge counting differences.

- [ ] **Step 1: Create the fixture generator**

Create `tools/golden/gen-single-c4-fixture.py`:

```python
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
```

- [ ] **Step 2: Generate the fixture**

```bash
python3 tools/golden/gen-single-c4-fixture.py \
  --output tests/golden/fixtures/single-c4-1s.mid
```

Expected: prints `wrote tests/golden/fixtures/single-c4-1s.mid (XX bytes)` (XX in the high 30s).

- [ ] **Step 3: Sanity-check the fixture parses**

```bash
ls -l tests/golden/fixtures/single-c4-1s.mid
```

The file size will be in the 35–40 byte range. If you have `mido` installed, verify:

```bash
python3 -c "import mido; mf = mido.MidiFile('tests/golden/fixtures/single-c4-1s.mid'); [print(m) for m in mf]"
```

Expected: prints a tempo, a NoteOn, a NoteOff, and an EOT meta.

- [ ] **Step 4: Add the failing golden test**

Append to `tests/golden/golden_test.cpp`:

```cpp
#include <catch2/catch_approx.hpp>

namespace {

double zero_crossing_frequency(const std::vector<float>& buf, int sample_rate) {
  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i) {
    const bool prev_pos = buf[i - 1] >= 0.0f;
    const bool curr_pos = buf[i]     >= 0.0f;
    if (prev_pos != curr_pos) ++crossings;
  }
  const double seconds = static_cast<double>(buf.size()) / sample_rate;
  return static_cast<double>(crossings) / (2.0 * seconds);
}

double rms(const std::vector<float>& buf) {
  double acc = 0.0;
  for (auto s : buf) acc += static_cast<double>(s) * s;
  return std::sqrt(acc / static_cast<double>(buf.size()));
}

std::vector<float> left_channel(const vp330::test::Wav& w) {
  std::vector<float> out;
  out.reserve(w.frames);
  for (std::size_t i = 0; i < w.frames; ++i) out.push_back(w.samples[i * w.channels]);
  return out;
}

} // namespace

TEST_CASE("golden: single C4 renders a square at ~261.63 Hz, sane RMS", "[golden]") {
  const auto wav = vp330::test::render_fixture("single-c4-1s.mid", 48000, 1.0);
  REQUIRE(wav.sample_rate == 48000);
  REQUIRE(wav.channels == 2);

  const auto left = left_channel(wav);
  const auto f = zero_crossing_frequency(left, wav.sample_rate);
  INFO("estimated fundamental: " << f << " Hz");
  REQUIRE(f == Catch::Approx(261.63).margin(2.0));

  const auto level = rms(left);
  INFO("RMS: " << level);
  REQUIRE(level == Catch::Approx(0.05).margin(0.005));
}
```

You'll also need these headers added at the top of the file if not already present: `<vector>`, `<cmath>`. The `wav_io.h` already provides the `Wav` type.

- [ ] **Step 5: Run to confirm it fails or passes**

```bash
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure -R "single C4"
```

Expected: passes (the engine is already implemented; this is the L3 acceptance gate). If it fails, the failure should report the actual fundamental and RMS, which will tell you whether the issue is in the SMF reader, the scheduler, or the engine.

- [ ] **Step 6: Run the full suite to confirm no regressions**

```bash
ctest --test-dir build --output-on-failure
```

Expected: all unit + golden tests green.

- [ ] **Step 7: Commit**

```bash
git add tools/golden/gen-single-c4-fixture.py \
        tests/golden/fixtures/single-c4-1s.mid \
        tests/golden/golden_test.cpp
git commit -m "$(cat <<'EOF'
test: golden — single-c4-1s.mid → fundamental ≈ 261.63 Hz, RMS ≈ 0.05

L3 acceptance gate for spec §9 Phase 1: end-to-end MIDI → CLI → WAV with
the naïve square engine. Procedural assertion (zero-crossing rate + RMS)
rather than a frozen WAV baseline, since the engine is going to be
rewritten in Phase 2 and a frozen baseline would be churn.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** golden test green; both unit and golden suites green.

---

## Task 13: Glossary review + Phase 1 tag

**Files:**
- Possibly modify: `GLOSSARY.md` (only if a new domain term was introduced — Phase 1 didn't add any glossary-worthy terms)
- Possibly modify: `docs/SPEC.md` Phase 1 done-list (check the boxes)

**Design notes:**
- This task is the closing gate. Run through the glossary and check that nothing in `domain/` uses a term not listed there. `Hertz`, `MidiNote`, `Pitch`, `SynthesisEngine`, `MidiSource`, `AudioSink`, `Clock` — all present in the spec §4 / GLOSSARY.md. `MidiEvent` is new but it's a generic term that doesn't clash with anything; it's not really jargon. We do not introduce `Voice`, `Oscillator` (banned-or-substitutable), or anything similar.
- Tagging is non-trivial — `git tag` is reversible locally but can be awkward to retract once pushed. Per CLAUDE.md, confirm with the user before pushing the tag.

- [ ] **Step 1: Skim domain types for glossary coverage**

```bash
grep -rE 'class |struct |enum class' domain/include/ | sort
```

Expected output (after Phase 1):

```
domain/include/vp330/engine/MidiEvent.h:struct MidiEvent {
domain/include/vp330/engine/SynthesisEngine.h:class SynthesisEngine {
domain/include/vp330/note/MidiNote.h:class MidiNote {
domain/include/vp330/ports/AudioSink.h:class AudioSink {
domain/include/vp330/ports/Clock.h:class Clock {
domain/include/vp330/ports/MidiSource.h:class MidiSource {
domain/include/vp330/values/Hertz.h:class Hertz {
domain/include/vp330/values/Pitch.h:class Pitch {
```

Cross-reference each name against `GLOSSARY.md`. Any term in code but not in the glossary needs a glossary entry — `MidiEvent` is a candidate, though it's arguably a primitive of the MIDI vocabulary and not VP-330-specific.

- [ ] **Step 2: Decide whether `MidiEvent` warrants a glossary entry**

Discuss with the user. Default: add a one-line entry for completeness. Edit `GLOSSARY.md` if added; commit separately if so.

- [ ] **Step 3: Update `docs/SPEC.md` Phase 1 done-list**

Edit `docs/SPEC.md` § "Phase 1 — Skeleton Engine". Convert the unchecked boxes to checked:

```diff
-- [ ] `MidiNote`, `Hertz`, `Pitch` value types with full L1 coverage.
-- [ ] `MidiSource`, `AudioSink`, `Clock` ports defined.
-- [ ] `SynthesisEngine` skeleton: takes MIDI events, produces stereo audio. Internally just one-square-per-key, summed.
-- [ ] JUCE adapter: receives MIDI, calls engine, fills audio buffer. Plugin loads in a host.
-- [ ] CLI adapter: reads `.mid`, writes `.wav`. Used by golden tests.
-- [ ] Golden test: render `single-c4-1s.mid` → check fundamental at ~261.6 Hz, RMS in expected range.
-- [ ] Tag: `phase-1`.
+- [x] `MidiNote`, `Hertz`, `Pitch` value types with full L1 coverage.
+- [x] `MidiSource`, `AudioSink`, `Clock` ports defined.
+- [x] `SynthesisEngine` skeleton: takes MIDI events, produces stereo audio. Internally just one-square-per-key, summed.
+- [x] JUCE adapter: receives MIDI, calls engine, fills audio buffer. Plugin loads in a host.
+- [x] CLI adapter: reads `.mid`, writes `.wav`. Used by golden tests.
+- [x] Golden test: render `single-c4-1s.mid` → check fundamental at ~261.6 Hz, RMS in expected range.
+- [x] Tag: `phase-1`. (after this task)
```

- [ ] **Step 4: Add a Phase 1 entry to the spec change log**

Append to `docs/SPEC.md` §12 "Change Log":

```markdown
| 2026-05-04 | 1.5     | Phase 1 complete: skeleton engine ships. Hertz / MidiNote / Pitch value types, MidiSource / AudioSink / Clock ports, SynthesisEngine with naïve square per active note, in-tree SMF reader in the CLI, JUCE plugin wired through the engine, golden test for single-c4-1s.mid. No new dependencies; no glossary additions. |
```

Bump the version line at the top from 1.4 to 1.5.

- [ ] **Step 5: Final test sweep**

```bash
cmake --build build
ctest --test-dir build --output-on-failure
grep -rE '#include[[:space:]]*<(juce_|sndfile|alsa)' domain/ || echo "OK: domain/ is isolated."
clang-format --dry-run -Werror $(find domain infrastructure tests -name '*.cpp' -o -name '*.h')
```

Expected: clean build, all tests green, isolation OK, format clean.

- [ ] **Step 6: Commit doc updates**

```bash
git add docs/SPEC.md
# include GLOSSARY.md if you edited it
git commit -m "$(cat <<'EOF'
docs: tick Phase 1 deliverables; bump SPEC to v1.5

Phase 1 (skeleton engine) is complete. No glossary additions; no new
third-party dependencies. Change log updated.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

- [ ] **Step 7: Tag — confirm with user before pushing**

Local tag is fine without confirmation. Pushing requires user confirmation per CLAUDE.md.

```bash
git tag -a phase-1 -m "Phase 1: skeleton engine — value types, ports, naïve square paraphony, CLI + JUCE adapters, golden test."
```

Then ask the user before:

```bash
# git push origin phase-1   # await confirmation
```

**Acceptance:** spec doc updated; suite green; tag created locally.

---

## Self-review

**Spec coverage** (§9 Phase 1):
- `MidiNote`, `Hertz`, `Pitch` with L1 — Tasks 2, 3, 4. ✓
- `MidiSource`, `AudioSink`, `Clock` ports — Task 6. ✓
- `SynthesisEngine` skeleton with one-square-per-key — Tasks 7, 8. ✓
- JUCE adapter — Task 11. ✓
- CLI adapter (reads .mid, writes .wav) — Tasks 9, 10. ✓
- Golden test (single-c4-1s.mid → 261.6 Hz + RMS) — Task 12. ✓
- Tag `phase-1` — Task 13. ✓

**Spec discipline:**
- §3.1 domain isolation: SmfReader lives in `infrastructure/cli/`, not `domain/`. Verified by grep in Tasks 6, 9. ✓
- §3.3 L1 strict TDD: every domain pure addition (Hertz, MidiNote, Pitch, MidiEvent, engine state, engine audio) has a test-first step that confirms failure before implementation. ✓
- §4 ubiquitous language: no `Voice`. New domain types (`Hertz`, `MidiNote`, `Pitch`, `SynthesisEngine`, the three ports) all match the glossary. `MidiEvent` is generic MIDI; flagged in Task 13 for explicit user decision. ✓
- §10 one concept per commit: 13 commits across 13 tasks (plus the bootstrap commit for the plan and any glossary tweak). ✓

**Placeholder scan:** none. No "TBD", "implement later", "similar to Task N" — every task has its own complete code blocks.

**Type consistency:**
- `MidiNote` constructor is `explicit MidiNote(int)` everywhere it appears (Tasks 3, 5, 7, 8, 9, 10, 11).
- `SynthesisEngine::render(float* left, float* right, std::size_t frames)` signature is identical across the header (Task 7), CLI usage (Task 10), and JUCE usage (Task 11).
- `Pitch::to_hertz` returns `Hertz`; called from `SynthesisEngine::render` in Task 8 — matches.
- `vp330::cli::ParsedMidi` shape (`int ppqn`, `std::vector<TimedMidiEvent>`) declared in Task 9 and used in Task 10 — matches.

---

## Open questions for the user before execution

These are flagged here rather than left ambiguous in the plan. Confirm or override before kicking off Task 1.

1. **In-tree SMF reader vs. an external library.** The plan ships a ~150-line reader under `infrastructure/cli/`. Alternative: link JUCE into the CLI for `juce::MidiFile` (zero new third-party deps either way; JUCE is already pulled in for the plugin, but the CLI binary footprint grows). Bias: in-tree reader. Confirm or redirect.
2. **`Pitch` shape.** Plan ships a fractional-semitone wrapper (`Pitch{60.5}.to_hertz()`). Alternative: a thin alias / non-fractional bridge between MidiNote and Hertz. Bias: fractional, since bender / vibrato will need it. Confirm.
3. **Phase 1 engine API.** Plan exposes direct `note_on / note_off / render`; ports exist as headers but the engine doesn't consume them yet. Alternative: rewrite the engine to take a `MidiSource&` at render time now. Bias: direct API; defer port wiring to Phase 2 when block-spanning event timing matters. Confirm.
4. **Golden assertion: procedural vs. frozen WAV.** Plan asserts fundamental + RMS procedurally on the rendered output (no `single-c4-1s.wav` in `baselines/`). Alternative: render once, commit a frozen WAV via LFS. Bias: procedural — Phase 2 will replace the engine; freezing now is churn. Confirm.
5. **Glossary entry for `MidiEvent`.** Add or skip. Bias: skip; it's a generic MIDI primitive, not VP-330 jargon. Confirm.

---

**End of plan.**
