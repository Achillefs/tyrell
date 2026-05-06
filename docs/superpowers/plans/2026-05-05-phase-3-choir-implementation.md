# Phase 3 — Choir Section Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the VP-330 MkII Choir section — split paraphonic keyboard buses, 7-band BP filter bank per zone, 4 independent voice switches, LFO vibrato, and exposed attack/release parameters.

**Architecture:** `MkIIKeyboard` gains `render_zones()` that splits the 49 keys at C4 (MIDI 60) into lower/upper mono buses. `ChoirSection` owns two `ChoirFilterBank` instances (one per zone), always runs all 7 filter chains, and sums the weighted outputs of active switches. `SynthesisEngine` owns `ChoirSection` + `Vibrato`, feeds them from the split buses each render block. Filter coefficients are PD-derived and stored as `constexpr` in `ChoirConstants.h` for easy L4 calibration.

**Tech Stack:** C++20, Catch2 v3, Audio EQ Cookbook biquad BP filter form (constant 0 dB peak gain).

---

### Task 1: ChoirConstants.h + ChoirSwitch.h

**Files:**
- Create: `domain/include/vp330/section/ChoirConstants.h`
- Create: `domain/include/vp330/section/ChoirSwitch.h`

- [ ] **Step 1: Create ChoirSwitch.h**

```cpp
// domain/include/vp330/section/ChoirSwitch.h
#pragma once

namespace vp330 {

// The four front-panel switches of the MkII Choir section.
// Array index in kVoiceWeights MUST match static_cast<int>(ChoirSwitch).
enum class ChoirSwitch { LowerMale8 = 0, LowerMale4 = 1, UpperMale8 = 2, UpperFemale4 = 3 };

} // namespace vp330
```

- [ ] **Step 2: Create ChoirConstants.h**

```cpp
// domain/include/vp330/section/ChoirConstants.h
#pragma once

#include <array>

// PD-derived VP-330 MkII Choir filter constants.
// Source: community circuit analysis; calibrate against author's MkII at L4.
// To recalibrate: update kChoirFilters / kVoiceWeights here only.

namespace vp330::mkii {

struct BpCascadeParams {
  float f0_1, q1; // first 2-pole stage
  float f0_2, q2; // second 2-pole stage
};

inline constexpr std::array<BpCascadeParams, 7> kChoirFilters{{
    {175.f, 6.79f,  216.f,  6.83f}, // F0
    {209.f, 6.79f,  257.f,  6.83f}, // F1
    {559.f, 6.96f,  671.f,  6.83f}, // F2 — present in all 4 voices
    {845.f, 7.00f, 1007.f,  6.83f}, // F3 — present in all 4 voices
    {1202.f, 6.81f, 1473.f, 6.83f}, // F4
    {2532.f, 6.83f, 3098.f, 6.83f}, // F5
    {2974.f, 6.78f, 3660.f, 6.83f}, // F6
}};

struct VoiceWeights {
  std::array<int, 4> filters;
  std::array<float, 4> weights;
};

// Order MUST match ChoirSwitch enum: LowerMale8=0, LowerMale4=1, UpperMale8=2, UpperFemale4=3.
inline constexpr std::array<VoiceWeights, 4> kVoiceWeights{{
    {{0, 2, 3, 5}, {0.3125f, 1.f, 1.f, 0.3125f}}, // LowerMale8
    {{1, 2, 3, 5}, {0.125f,  1.f, 1.f, 0.3125f}}, // LowerMale4
    {{1, 2, 3, 5}, {0.1875f, 1.f, 1.f, 0.3125f}}, // UpperMale8
    {{2, 3, 4, 6}, {0.1875f, 1.f, 0.6875f, 0.1875f}}, // UpperFemale4
}};

inline constexpr double kVibratoMaxClockOffsetHz = 50.0; // calibrate vs capture #4

} // namespace vp330::mkii
```

- [ ] **Step 3: Commit**

```bash
git add domain/include/vp330/section/ChoirSwitch.h \
        domain/include/vp330/section/ChoirConstants.h
git commit -m "feat(section): add ChoirSwitch enum and ChoirConstants (PD-derived filter params)"
```

---

### Task 2: BpCascade — cascaded 2-pole bandpass biquad pair

**Files:**
- Create: `domain/include/vp330/section/BpCascade.h`
- Create: `domain/src/section/BpCascade.cpp`
- Create: `tests/unit/section/BpCascadeTest.cpp`
- Create: `tests/characterization/section/BpCascadeCharTest.cpp`
- Modify: `domain/CMakeLists.txt`
- Modify: `tests/unit/CMakeLists.txt`
- Modify: `tests/characterization/CMakeLists.txt`

- [ ] **Step 1: Write failing L1 tests**

```cpp
// tests/unit/section/BpCascadeTest.cpp
#include "vp330/section/BpCascade.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::BpCascade;

TEST_CASE("BpCascade: rejects DC input after settling", "[bp_cascade][L1]") {
  BpCascade bp{175.f, 6.79f, 216.f, 6.83f, 48000};
  std::vector<float> in(2000, 1.0f), out(2000);
  bp.process(in.data(), out.data(), 2000);
  // Bandpass filter must drive DC to zero; last 200 samples should be negligible.
  for (int i = 1800; i < 2000; ++i)
    REQUIRE(std::abs(out[i]) < 0.01f);
}

TEST_CASE("BpCascade: BIBO stable at resonance", "[bp_cascade][L1]") {
  BpCascade bp{175.f, 6.79f, 216.f, 6.83f, 48000};
  std::vector<float> in(10000), out(10000);
  for (int i = 0; i < 10000; ++i)
    in[i] = std::sin(2.f * 3.14159265f * 190.f * i / 48000.f);
  bp.process(in.data(), out.data(), 10000);
  for (auto s : out) {
    REQUIRE(std::isfinite(s));
    REQUIRE(std::abs(s) < 10.f); // bounded output for bounded input
  }
}

TEST_CASE("BpCascade: reset zeroes output on silence", "[bp_cascade][L1]") {
  BpCascade bp{175.f, 6.79f, 216.f, 6.83f, 48000};
  // Drive filter into a non-zero state.
  std::vector<float> in(500, 1.0f), out(500);
  bp.process(in.data(), out.data(), 500);
  bp.reset();
  // After reset, silence in → silence out.
  std::vector<float> zeros(100, 0.0f), zeros_out(100);
  bp.process(zeros.data(), zeros_out.data(), 100);
  for (auto s : zeros_out)
    REQUIRE(s == 0.0f);
}
```

- [ ] **Step 2: Run tests — expect compilation failure (BpCascade not yet defined)**

```bash
cmake -B build -S . && cmake --build build 2>&1 | head -20
```

Expected: error about missing `vp330/section/BpCascade.h`.

- [ ] **Step 3: Create BpCascade.h**

```cpp
// domain/include/vp330/section/BpCascade.h
#pragma once

#include <cstddef>

namespace vp330 {

// Pair of cascaded 2nd-order bandpass biquad filters (4th-order total).
// Uses Audio EQ Cookbook constant-0-dB-peak-gain BP form.
class BpCascade {
public:
  BpCascade(float f0_1, float q1, float f0_2, float q2, int sample_rate);

  void process(const float* in, float* out, std::size_t frames);
  void reset(); // zero all filter state

private:
  struct Stage {
    float b0, b2, a1, a2;        // b1 = 0 for BP
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    float tick(float x) noexcept;
    void reset() noexcept { x1 = x2 = y1 = y2 = 0.f; }
  };

  static Stage make_stage(float f0, float q, int sample_rate);

  Stage stage1_, stage2_;
};

} // namespace vp330
```

- [ ] **Step 4: Create BpCascade.cpp**

```cpp
// domain/src/section/BpCascade.cpp
#include "vp330/section/BpCascade.h"

#include <cmath>

namespace vp330 {

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

BpCascade::Stage BpCascade::make_stage(float f0, float q, int sample_rate) {
  const float w0    = 2.f * kPi * f0 / static_cast<float>(sample_rate);
  const float sinw0 = std::sin(w0);
  const float cosw0 = std::cos(w0);
  const float alpha = sinw0 / (2.f * q);
  const float a0    = 1.f + alpha;
  // Constant 0 dB peak-gain BP: b0 = sin(w0)/2, b1=0, b2=-sin(w0)/2, normalised by a0.
  return Stage{
      .b0 =  sinw0 / (2.f * a0),
      .b2 = -sinw0 / (2.f * a0),
      .a1 = -2.f * cosw0 / a0,
      .a2 = (1.f - alpha) / a0,
  };
}

BpCascade::BpCascade(float f0_1, float q1, float f0_2, float q2, int sample_rate)
    : stage1_{make_stage(f0_1, q1, sample_rate)},
      stage2_{make_stage(f0_2, q2, sample_rate)} {}

float BpCascade::Stage::tick(float x) noexcept {
  const float y = b0 * x + b2 * x2 - a1 * y1 - a2 * y2;
  x2 = x1; x1 = x;
  y2 = y1; y1 = y;
  return y;
}

void BpCascade::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i)
    out[i] = stage2_.tick(stage1_.tick(in[i]));
}

void BpCascade::reset() {
  stage1_.reset();
  stage2_.reset();
}

} // namespace vp330
```

- [ ] **Step 5: Update domain/CMakeLists.txt — add BpCascade.cpp**

In `domain/CMakeLists.txt`, add `src/section/BpCascade.cpp` to the `add_library` source list:

```cmake
add_library(vp330_domain STATIC
    src/engine/SynthesisEngine.cpp
    src/tod/TopOctaveDivider.cpp
    src/tod/OctaveDivider.cpp
    src/keygate/KeyGate.cpp
    src/keyboard/MkIIKeyboard.cpp
    src/section/BpCascade.cpp
)
```

- [ ] **Step 6: Update tests/unit/CMakeLists.txt — add BpCascadeTest.cpp**

Append to `target_sources(vp330_tests PRIVATE …)` in `tests/unit/CMakeLists.txt`:

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/section/BpCascadeTest.cpp
```

- [ ] **Step 7: Build and run L1 tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[bp_cascade\]\[L1\]"
```

Expected: 3 tests PASS.

- [ ] **Step 8: Write L2 characterisation test**

```cpp
// tests/characterization/section/BpCascadeCharTest.cpp
#include "vp330/section/BpCascade.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::BpCascade;

namespace {

float rms_at(BpCascade& bp, float freq, int sample_rate = 48000) {
  bp.reset();
  const int n = sample_rate;
  std::vector<float> in(n), out(n);
  const float k = 2.f * 3.14159265f * freq / static_cast<float>(sample_rate);
  for (int i = 0; i < n; ++i) in[i] = std::sin(k * i);
  bp.process(in.data(), out.data(), n);
  // Measure second half (settled state).
  float sum = 0;
  for (int i = n / 2; i < n; ++i) sum += out[i] * out[i];
  return std::sqrt(2.f * sum / static_cast<float>(n));
}

} // namespace

TEST_CASE("BpCascade L2: passes signal near centre, rejects extremes", "[bp_cascade][L2]") {
  // F0: stages at 175 Hz and 216 Hz — centre region ~190 Hz.
  BpCascade bp{175.f, 6.79f, 216.f, 6.83f, 48000};

  const float rms_centre = rms_at(bp, 190.f);
  const float rms_lo     = rms_at(bp, 30.f);
  const float rms_hi     = rms_at(bp, 5000.f);

  // At least 10 dB more gain in the passband than at the extremes.
  REQUIRE(rms_centre > rms_lo  * 3.f);
  REQUIRE(rms_centre > rms_hi  * 3.f);
}

TEST_CASE("BpCascade L2: higher-frequency filter (F5) centred around 2700 Hz", "[bp_cascade][L2]") {
  BpCascade bp{2532.f, 6.83f, 3098.f, 6.83f, 48000};

  const float rms_centre = rms_at(bp, 2700.f);
  const float rms_lo     = rms_at(bp, 400.f);
  const float rms_hi     = rms_at(bp, 12000.f);

  REQUIRE(rms_centre > rms_lo * 3.f);
  REQUIRE(rms_centre > rms_hi * 3.f);
}
```

- [ ] **Step 9: Update tests/characterization/CMakeLists.txt**

Append to `target_sources(vp330_tests PRIVATE …)`:

```cmake
    ${CMAKE_CURRENT_SOURCE_DIR}/section/BpCascadeCharTest.cpp
```

- [ ] **Step 10: Build and run L2 tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[bp_cascade\]\[L2\]"
```

Expected: 2 tests PASS.

- [ ] **Step 11: Commit**

```bash
git add domain/include/vp330/section/BpCascade.h \
        domain/src/section/BpCascade.cpp \
        tests/unit/section/BpCascadeTest.cpp \
        tests/characterization/section/BpCascadeCharTest.cpp \
        domain/CMakeLists.txt \
        tests/unit/CMakeLists.txt \
        tests/characterization/CMakeLists.txt
git commit -m "feat(section): add BpCascade — cascaded 2-pole BP biquad pair with L1+L2 tests"
```

---

### Task 3: ChoirFilterBank — 7-chain filter bank per zone

**Files:**
- Create: `domain/include/vp330/section/ChoirFilterBank.h`
- Create: `domain/src/section/ChoirFilterBank.cpp`
- Create: `tests/unit/section/ChoirFilterBankTest.cpp`
- Create: `tests/characterization/section/ChoirFilterBankCharTest.cpp`
- Modify: `domain/CMakeLists.txt`
- Modify: `tests/unit/CMakeLists.txt`
- Modify: `tests/characterization/CMakeLists.txt`

- [ ] **Step 1: Write failing L1 tests**

```cpp
// tests/unit/section/ChoirFilterBankTest.cpp
#include "vp330/section/ChoirConstants.h"
#include "vp330/section/ChoirSwitch.h"
#include "vp330/section/ChoirFilterBank.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using vp330::ChoirSwitch;
using namespace vp330::mkii;

// Verify the PD-derived voice weight tables before any DSP runs.
TEST_CASE("ChoirFilterBank: LowerMale8 uses filters 0,2,3,5 with correct weights", "[choir_fb][L1]") {
  const auto& v = kVoiceWeights[static_cast<int>(ChoirSwitch::LowerMale8)];
  REQUIRE(v.filters == std::array<int,4>{0, 2, 3, 5});
  REQUIRE(v.weights[0] == Catch::Approx(0.3125f));
  REQUIRE(v.weights[1] == Catch::Approx(1.f));
  REQUIRE(v.weights[2] == Catch::Approx(1.f));
  REQUIRE(v.weights[3] == Catch::Approx(0.3125f));
}

TEST_CASE("ChoirFilterBank: LowerMale4 uses filters 1,2,3,5 with correct weights", "[choir_fb][L1]") {
  const auto& v = kVoiceWeights[static_cast<int>(ChoirSwitch::LowerMale4)];
  REQUIRE(v.filters == std::array<int,4>{1, 2, 3, 5});
  REQUIRE(v.weights[0] == Catch::Approx(0.125f));
}

TEST_CASE("ChoirFilterBank: UpperMale8 uses filters 1,2,3,5 with correct weights", "[choir_fb][L1]") {
  const auto& v = kVoiceWeights[static_cast<int>(ChoirSwitch::UpperMale8)];
  REQUIRE(v.filters == std::array<int,4>{1, 2, 3, 5});
  REQUIRE(v.weights[0] == Catch::Approx(0.1875f));
}

TEST_CASE("ChoirFilterBank: UpperFemale4 uses filters 2,3,4,6 with correct weights", "[choir_fb][L1]") {
  const auto& v = kVoiceWeights[static_cast<int>(ChoirSwitch::UpperFemale4)];
  REQUIRE(v.filters == std::array<int,4>{2, 3, 4, 6});
  REQUIRE(v.weights[0] == Catch::Approx(0.1875f));
  REQUIRE(v.weights[2] == Catch::Approx(0.6875f));
  REQUIRE(v.weights[3] == Catch::Approx(0.1875f));
}

TEST_CASE("ChoirFilterBank: silence in → silence out for all 7 outputs", "[choir_fb][L1]") {
  vp330::ChoirFilterBank bank{48000};
  std::vector<float> in(256, 0.0f);
  bank.process(in.data(), 256);
  for (int i = 0; i < 7; ++i)
    for (std::size_t f = 0; f < 256; ++f)
      REQUIRE(bank.output(i)[f] == 0.0f);
}
```

- [ ] **Step 2: Run — expect compilation failure**

```bash
cmake -B build -S . && cmake --build build 2>&1 | head -20
```

Expected: missing `vp330/section/ChoirFilterBank.h`.

- [ ] **Step 3: Create ChoirFilterBank.h**

```cpp
// domain/include/vp330/section/ChoirFilterBank.h
#pragma once

#include "vp330/section/BpCascade.h"

#include <array>
#include <cstddef>
#include <vector>

namespace vp330 {

// Holds 7 BpCascade chains initialised from kChoirFilters.
// Call process() once per render block, then read per-filter output via output().
class ChoirFilterBank {
public:
  explicit ChoirFilterBank(int sample_rate);

  void process(const float* in, std::size_t frames);
  const float* output(int filter_idx) const;
  void reset(); // zero all filter state and scratch

private:
  std::array<BpCascade, 7> filters_;
  std::array<std::vector<float>, 7> scratch_;
};

} // namespace vp330
```

- [ ] **Step 4: Create ChoirFilterBank.cpp**

```cpp
// domain/src/section/ChoirFilterBank.cpp
#include "vp330/section/ChoirFilterBank.h"
#include "vp330/section/ChoirConstants.h"

namespace vp330 {

namespace {

std::array<BpCascade, 7> make_filters(int sr) {
  const auto& p = mkii::kChoirFilters;
  return {
    BpCascade{p[0].f0_1, p[0].q1, p[0].f0_2, p[0].q2, sr},
    BpCascade{p[1].f0_1, p[1].q1, p[1].f0_2, p[1].q2, sr},
    BpCascade{p[2].f0_1, p[2].q1, p[2].f0_2, p[2].q2, sr},
    BpCascade{p[3].f0_1, p[3].q1, p[3].f0_2, p[3].q2, sr},
    BpCascade{p[4].f0_1, p[4].q1, p[4].f0_2, p[4].q2, sr},
    BpCascade{p[5].f0_1, p[5].q1, p[5].f0_2, p[5].q2, sr},
    BpCascade{p[6].f0_1, p[6].q1, p[6].f0_2, p[6].q2, sr},
  };
}

} // namespace

ChoirFilterBank::ChoirFilterBank(int sample_rate)
    : filters_{make_filters(sample_rate)} {}

void ChoirFilterBank::process(const float* in, std::size_t frames) {
  for (int i = 0; i < 7; ++i) {
    if (scratch_[i].size() < frames) scratch_[i].resize(frames);
    filters_[i].process(in, scratch_[i].data(), frames);
  }
}

const float* ChoirFilterBank::output(int filter_idx) const {
  return scratch_[filter_idx].data();
}

void ChoirFilterBank::reset() {
  for (int i = 0; i < 7; ++i) {
    filters_[i].reset();
    std::fill(scratch_[i].begin(), scratch_[i].end(), 0.f);
  }
}

} // namespace vp330
```

- [ ] **Step 5: Update domain/CMakeLists.txt**

Add `src/section/ChoirFilterBank.cpp` to the source list.

- [ ] **Step 6: Update tests/unit/CMakeLists.txt**

Append `${CMAKE_CURRENT_SOURCE_DIR}/section/ChoirFilterBankTest.cpp`.

- [ ] **Step 7: Build and run L1 tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[choir_fb\]\[L1\]"
```

Expected: 5 tests PASS.

- [ ] **Step 8: Write L2 characterisation test**

```cpp
// tests/characterization/section/ChoirFilterBankCharTest.cpp
#include "vp330/section/ChoirConstants.h"
#include "vp330/section/ChoirFilterBank.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::ChoirFilterBank;

namespace {

// Feed a sine at `freq` for 1 s, return settled RMS of filter `idx`.
float filter_rms(ChoirFilterBank& bank, int idx, float freq) {
  bank.reset();
  const int n = 48000;
  std::vector<float> in(n);
  const float k = 2.f * 3.14159265f * freq / 48000.f;
  for (int i = 0; i < n; ++i) in[i] = std::sin(k * i);
  bank.process(in.data(), n);
  const float* out = bank.output(idx);
  float sum = 0;
  for (int i = n / 2; i < n; ++i) sum += out[i] * out[i];
  return std::sqrt(2.f * sum / static_cast<float>(n));
}

} // namespace

TEST_CASE("ChoirFilterBank L2: F2 (559/671 Hz) passes 600 Hz, rejects 100 Hz", "[choir_fb][L2]") {
  ChoirFilterBank bank{48000};
  REQUIRE(filter_rms(bank, 2, 600.f) > filter_rms(bank, 2, 100.f) * 3.f);
}

TEST_CASE("ChoirFilterBank L2: F5 (2532/3098 Hz) passes 2700 Hz, rejects 500 Hz", "[choir_fb][L2]") {
  ChoirFilterBank bank{48000};
  REQUIRE(filter_rms(bank, 5, 2700.f) > filter_rms(bank, 5, 500.f) * 3.f);
}

TEST_CASE("ChoirFilterBank L2: F0 (175/216 Hz) passes 190 Hz, rejects 2000 Hz", "[choir_fb][L2]") {
  ChoirFilterBank bank{48000};
  REQUIRE(filter_rms(bank, 0, 190.f) > filter_rms(bank, 0, 2000.f) * 3.f);
}
```

- [ ] **Step 9: Update tests/characterization/CMakeLists.txt**

Append `${CMAKE_CURRENT_SOURCE_DIR}/section/ChoirFilterBankCharTest.cpp`.

- [ ] **Step 10: Build and run L2 tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[choir_fb\]\[L2\]"
```

Expected: 3 tests PASS.

- [ ] **Step 11: Commit**

```bash
git add domain/include/vp330/section/ChoirFilterBank.h \
        domain/src/section/ChoirFilterBank.cpp \
        tests/unit/section/ChoirFilterBankTest.cpp \
        tests/characterization/section/ChoirFilterBankCharTest.cpp \
        domain/CMakeLists.txt tests/unit/CMakeLists.txt \
        tests/characterization/CMakeLists.txt
git commit -m "feat(section): add ChoirFilterBank — 7-chain BP bank with L1+L2 tests"
```

---

### Task 4: ChoirSection — zone processor with switch control

**Files:**
- Create: `domain/include/vp330/section/ChoirSection.h`
- Create: `domain/src/section/ChoirSection.cpp`
- Create: `tests/unit/section/ChoirSectionTest.cpp`
- Create: `tests/characterization/section/ChoirSectionCharTest.cpp`
- Modify: `domain/CMakeLists.txt`, `tests/unit/CMakeLists.txt`, `tests/characterization/CMakeLists.txt`

- [ ] **Step 1: Write failing L1 tests**

```cpp
// tests/unit/section/ChoirSectionTest.cpp
#include "vp330/section/ChoirSection.h"
#include "vp330/section/ChoirSwitch.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using vp330::ChoirSection;
using vp330::ChoirSwitch;

TEST_CASE("ChoirSection: all switches off → silence", "[choir_section][L1]") {
  ChoirSection cs{48000};
  std::vector<float> lower(256, 0.5f), upper(256, 0.5f);
  std::vector<float> left(256), right(256);
  cs.process(lower.data(), upper.data(), left.data(), right.data(), 256);
  for (auto s : left)  REQUIRE(s == 0.0f);
  for (auto s : right) REQUIRE(s == 0.0f);
}

TEST_CASE("ChoirSection: lower zone silence when only upper switch is on", "[choir_section][L1]") {
  ChoirSection cs{48000};
  cs.set_switch(ChoirSwitch::UpperMale8, true);
  // Feed signal in lower zone only, upper zone silent.
  std::vector<float> lower(256, 0.5f), upper(256, 0.0f);
  std::vector<float> left(256), right(256);
  cs.process(lower.data(), upper.data(), left.data(), right.data(), 256);
  // Output should be silence (upper switch active but upper input is zero).
  float max_out = 0;
  for (auto s : left) max_out = std::max(max_out, std::abs(s));
  REQUIRE(max_out < 1e-6f);
}

TEST_CASE("ChoirSection: left and right outputs are equal (mono duplication)", "[choir_section][L1]") {
  ChoirSection cs{48000};
  cs.set_switch(ChoirSwitch::UpperMale8, true);
  std::vector<float> lower(256, 0.0f), upper(256, 0.1f);
  std::vector<float> left(256), right(256);
  // Settle the filter first.
  for (int i = 0; i < 10; ++i)
    cs.process(lower.data(), upper.data(), left.data(), right.data(), 256);
  for (std::size_t i = 0; i < 256; ++i)
    REQUIRE(left[i] == right[i]);
}
```

- [ ] **Step 2: Run — expect compilation failure**

```bash
cmake -B build -S . && cmake --build build 2>&1 | head -20
```

- [ ] **Step 3: Create ChoirSection.h**

```cpp
// domain/include/vp330/section/ChoirSection.h
#pragma once

#include "vp330/section/ChoirFilterBank.h"
#include "vp330/section/ChoirSwitch.h"

#include <array>
#include <cstddef>

namespace vp330 {

class ChoirSection {
public:
  explicit ChoirSection(int sample_rate);

  void set_switch(ChoirSwitch sw, bool on);
  bool get_switch(ChoirSwitch sw) const;

  void process(const float* lower, const float* upper,
               float* left, float* right, std::size_t frames);

private:
  ChoirFilterBank lower_bank_, upper_bank_;
  std::array<bool, 4> switches_{}; // all off by default
};

} // namespace vp330
```

- [ ] **Step 4: Create ChoirSection.cpp**

```cpp
// domain/src/section/ChoirSection.cpp
#include "vp330/section/ChoirSection.h"
#include "vp330/section/ChoirConstants.h"

#include <algorithm>

namespace vp330 {

ChoirSection::ChoirSection(int sample_rate)
    : lower_bank_{sample_rate}, upper_bank_{sample_rate} {}

void ChoirSection::set_switch(ChoirSwitch sw, bool on) {
  switches_[static_cast<int>(sw)] = on;
}

bool ChoirSection::get_switch(ChoirSwitch sw) const {
  return switches_[static_cast<int>(sw)];
}

void ChoirSection::process(const float* lower, const float* upper,
                           float* left, float* right, std::size_t frames) {
  std::fill_n(left,  frames, 0.f);
  std::fill_n(right, frames, 0.f);

  lower_bank_.process(lower, frames);
  upper_bank_.process(upper, frames);

  // Switches 0,1 use the lower bank; switches 2,3 use the upper bank.
  const ChoirFilterBank* banks[4] = {
    &lower_bank_, &lower_bank_, &upper_bank_, &upper_bank_
  };

  for (int sw = 0; sw < 4; ++sw) {
    if (!switches_[sw]) continue;
    const auto& v    = mkii::kVoiceWeights[sw];
    const auto* bank = banks[sw];
    for (std::size_t f = 0; f < frames; ++f) {
      float s = 0.f;
      for (int i = 0; i < 4; ++i)
        s += v.weights[i] * bank->output(v.filters[i])[f];
      left[f]  += s;
      right[f] += s;
    }
  }
}

} // namespace vp330
```

- [ ] **Step 5: Update CMakeLists files** (add `src/section/ChoirSection.cpp` to domain, add test files to unit and characterization).

- [ ] **Step 6: Build and run L1 tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[choir_section\]\[L1\]"
```

Expected: 3 tests PASS.

- [ ] **Step 7: Write L2 characterisation test**

```cpp
// tests/characterization/section/ChoirSectionCharTest.cpp
#include "vp330/section/ChoirSection.h"
#include "vp330/section/ChoirSwitch.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::ChoirSection;
using vp330::ChoirSwitch;

namespace {

double rms(const std::vector<float>& v, int start) {
  double s = 0;
  for (int i = start; i < static_cast<int>(v.size()); ++i) s += v[i] * v[i];
  return std::sqrt(s / (v.size() - start));
}

} // namespace

TEST_CASE("ChoirSection L2: UpperMale8 produces non-silent output from upper zone", "[choir_section][L2]") {
  ChoirSection cs{48000};
  cs.set_switch(ChoirSwitch::UpperMale8, true);

  const int n = 48000;
  std::vector<float> lower(n, 0.f), upper(n), left(n), right(n);
  // Square wave at ~261 Hz (C4 region)
  for (int i = 0; i < n; ++i) upper[i] = (i % 184 < 92) ? 0.05f : -0.05f;
  cs.process(lower.data(), upper.data(), left.data(), right.data(), n);

  REQUIRE(rms(left, n / 2) > 1e-5); // choir section producing output
}

TEST_CASE("ChoirSection L2: both lower switches sum louder than one alone", "[choir_section][L2]") {
  ChoirSection cs1{48000}, cs2{48000};
  cs1.set_switch(ChoirSwitch::LowerMale8, true);
  cs2.set_switch(ChoirSwitch::LowerMale8, true);
  cs2.set_switch(ChoirSwitch::LowerMale4, true);

  const int n = 48000;
  std::vector<float> lower(n), upper(n, 0.f), l1(n), r1(n), l2(n), r2(n);
  for (int i = 0; i < n; ++i) lower[i] = (i % 100 < 50) ? 0.05f : -0.05f;

  cs1.process(lower.data(), upper.data(), l1.data(), r1.data(), n);
  cs2.process(lower.data(), upper.data(), l2.data(), r2.data(), n);

  REQUIRE(rms(l2, n / 2) > rms(l1, n / 2));
}
```

- [ ] **Step 8: Update characterization CMakeLists — add ChoirSectionCharTest.cpp, build and run**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[choir_section\]\[L2\]"
```

Expected: 2 tests PASS.

- [ ] **Step 9: Commit**

```bash
git add domain/include/vp330/section/ChoirSection.h \
        domain/src/section/ChoirSection.cpp \
        tests/unit/section/ChoirSectionTest.cpp \
        tests/characterization/section/ChoirSectionCharTest.cpp \
        domain/CMakeLists.txt tests/unit/CMakeLists.txt \
        tests/characterization/CMakeLists.txt
git commit -m "feat(section): add ChoirSection — zone split processor with L1+L2 tests"
```

---

### Task 5: Lfo — sinusoidal LFO

**Files:**
- Create: `domain/include/vp330/modulation/Lfo.h`
- Create: `domain/src/modulation/Lfo.cpp`
- Create: `tests/unit/modulation/LfoTest.cpp`
- Modify: `domain/CMakeLists.txt`, `tests/unit/CMakeLists.txt`

- [ ] **Step 1: Write failing L1 tests**

```cpp
// tests/unit/modulation/LfoTest.cpp
#include "vp330/modulation/Lfo.h"
#include "vp330/values/Hertz.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using vp330::Hertz;
using vp330::Lfo;

TEST_CASE("Lfo: zero depth produces all-zero output", "[lfo][L1]") {
  Lfo lfo{48000};
  lfo.set_rate(Hertz{5.0});
  lfo.set_depth(0.0f);
  for (int i = 0; i < 200; ++i)
    REQUIRE(lfo.tick() == 0.0f);
}

TEST_CASE("Lfo: output bounded to [-1, 1] at unity depth", "[lfo][L1]") {
  Lfo lfo{48000};
  lfo.set_rate(Hertz{440.0});
  lfo.set_depth(1.0f);
  for (int i = 0; i < 48000; ++i) {
    const float s = lfo.tick();
    REQUIRE(s >= -1.0f);
    REQUIRE(s <=  1.0f);
  }
}

TEST_CASE("Lfo: full cycle in sr/rate ticks", "[lfo][L1]") {
  // 8 Hz at 48 kHz → 6000 samples per cycle.
  Lfo lfo{48000};
  lfo.set_rate(Hertz{8.0});
  lfo.set_depth(1.0f);
  const float first = lfo.tick();
  for (int i = 1; i < 6000; ++i) lfo.tick();
  const float after_cycle = lfo.tick(); // sample 6000 ≈ sample 0
  REQUIRE(after_cycle == Catch::Approx(first).margin(0.01f));
}

TEST_CASE("Lfo: advance(N) produces the same phase as N tick()s", "[lfo][L1]") {
  Lfo a{48000}, b{48000};
  a.set_rate(Hertz{5.0}); b.set_rate(Hertz{5.0});
  a.set_depth(1.0f);      b.set_depth(1.0f);
  for (int i = 0; i < 256; ++i) a.tick();
  b.advance(256);
  REQUIRE(b.value() == Catch::Approx(a.value()).margin(1e-5f));
}
```

- [ ] **Step 2: Run — expect compilation failure**

```bash
cmake -B build -S . && cmake --build build 2>&1 | head -20
```

- [ ] **Step 3: Create Lfo.h**

```cpp
// domain/include/vp330/modulation/Lfo.h
#pragma once

#include "vp330/values/Hertz.h"

namespace vp330 {

class Lfo {
public:
  explicit Lfo(int sample_rate);

  void set_rate(Hertz rate);
  void set_depth(float depth_0_to_1);

  float tick();            // advance one sample, return scaled output
  void  advance(int n);    // advance n samples without computing output
  float value() const;     // current output without advancing

private:
  double phase_     = 0.0;
  double phase_inc_ = 0.0;
  float  depth_     = 0.0f;
  int    sample_rate_;
};

} // namespace vp330
```

- [ ] **Step 4: Create Lfo.cpp**

```cpp
// domain/src/modulation/Lfo.cpp
#include "vp330/modulation/Lfo.h"

#include <cmath>

namespace vp330 {

namespace {
constexpr double kTwoPi = 6.283185307179586;
}

Lfo::Lfo(int sample_rate) : sample_rate_{sample_rate} {}

void Lfo::set_rate(Hertz rate) {
  phase_inc_ = kTwoPi * rate.value() / static_cast<double>(sample_rate_);
}

void Lfo::set_depth(float depth) { depth_ = depth; }

float Lfo::value() const {
  return depth_ * static_cast<float>(std::sin(phase_));
}

float Lfo::tick() {
  const float v = value();
  phase_ += phase_inc_;
  if (phase_ >= kTwoPi) phase_ -= kTwoPi;
  return v;
}

void Lfo::advance(int n) {
  phase_ = std::fmod(phase_ + phase_inc_ * n, kTwoPi);
}

} // namespace vp330
```

- [ ] **Step 5: Update CMakeLists files** (add `src/modulation/Lfo.cpp` to domain; add `LfoTest.cpp` to unit tests).

- [ ] **Step 6: Build and run L1 tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[lfo\]\[L1\]"
```

Expected: 4 tests PASS.

- [ ] **Step 7: Commit**

```bash
git add domain/include/vp330/modulation/Lfo.h \
        domain/src/modulation/Lfo.cpp \
        tests/unit/modulation/LfoTest.cpp \
        domain/CMakeLists.txt tests/unit/CMakeLists.txt
git commit -m "feat(modulation): add Lfo — sinusoidal LFO with L1 tests"
```

---

### Task 6: Vibrato — LFO-modulated TOD master clock

**Files:**
- Create: `domain/include/vp330/modulation/Vibrato.h`
- Create: `domain/src/modulation/Vibrato.cpp`
- Create: `tests/unit/modulation/VibratoTest.cpp`
- Create: `tests/characterization/modulation/VibratoCharTest.cpp`
- Modify: `domain/CMakeLists.txt`, `tests/unit/CMakeLists.txt`, `tests/characterization/CMakeLists.txt`

- [ ] **Step 1: Write failing L1 tests**

```cpp
// tests/unit/modulation/VibratoTest.cpp
#include "vp330/modulation/Vibrato.h"
#include "vp330/tod/MkIIConstants.h"
#include "vp330/values/Hertz.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using vp330::Hertz;
using vp330::Vibrato;

TEST_CASE("Vibrato: zero depth always returns master clock", "[vibrato][L1]") {
  Vibrato v{48000};
  v.set_depth(0.0f);
  v.set_rate(Hertz{5.0});
  for (int i = 0; i < 100; ++i)
    REQUIRE(v.tick(256).value() ==
            Catch::Approx(vp330::mkii::kMasterClockHz.value()).margin(1.0));
}

TEST_CASE("Vibrato: at full depth stays within max offset of master clock", "[vibrato][L1]") {
  Vibrato v{48000};
  v.set_depth(1.0f);
  v.set_rate(Hertz{5.0});
  const double base  = vp330::mkii::kMasterClockHz.value();
  const double limit = vp330::mkii::kVibratoMaxClockOffsetHz;
  for (int i = 0; i < 500; ++i) {
    const double hz = v.tick(256).value();
    REQUIRE(hz >= base - limit);
    REQUIRE(hz <= base + limit);
  }
}
```

- [ ] **Step 2: Run — expect compilation failure**

```bash
cmake -B build -S . && cmake --build build 2>&1 | head -20
```

- [ ] **Step 3: Create Vibrato.h**

```cpp
// domain/include/vp330/modulation/Vibrato.h
#pragma once

#include "vp330/modulation/Lfo.h"
#include "vp330/values/Hertz.h"

#include <cstddef>

namespace vp330 {

class Vibrato {
public:
  explicit Vibrato(int sample_rate);

  void set_rate(Hertz rate);
  void set_depth(float depth_0_to_1);

  // Advance LFO by `frames` samples and return modulated master clock for the block.
  Hertz tick(std::size_t frames = 1);

private:
  Lfo lfo_;
};

} // namespace vp330
```

- [ ] **Step 4: Create Vibrato.cpp**

```cpp
// domain/src/modulation/Vibrato.cpp
#include "vp330/modulation/Vibrato.h"
#include "vp330/tod/MkIIConstants.h"

namespace vp330 {

// Calibrate kVibratoMaxClockOffsetHz in ChoirConstants.h against capture #4.
static constexpr double kMaxOffset = mkii::kVibratoMaxClockOffsetHz;

Vibrato::Vibrato(int sample_rate) : lfo_{sample_rate} {}

void Vibrato::set_rate(Hertz rate)         { lfo_.set_rate(rate); }
void Vibrato::set_depth(float d)           { lfo_.set_depth(d); }

Hertz Vibrato::tick(std::size_t frames) {
  lfo_.advance(static_cast<int>(frames));
  return Hertz{mkii::kMasterClockHz.value() +
               static_cast<double>(lfo_.value()) * kMaxOffset};
}

} // namespace vp330
```

- [ ] **Step 5: Update CMakeLists files** (add `src/modulation/Vibrato.cpp`; add test files).

- [ ] **Step 6: Build and run L1 tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[vibrato\]\[L1\]"
```

Expected: 2 tests PASS.

- [ ] **Step 7: Write L2 characterisation test**

```cpp
// tests/characterization/modulation/VibratoCharTest.cpp
#include "vp330/modulation/Vibrato.h"
#include "vp330/tod/MkIIConstants.h"
#include "vp330/values/Hertz.h"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using vp330::Hertz;
using vp330::Vibrato;

TEST_CASE("Vibrato L2: clock oscillates at the set LFO rate", "[vibrato][L2]") {
  Vibrato v{48000};
  v.set_rate(Hertz{5.0});
  v.set_depth(0.5f);

  // Sample once per 256-sample block over 1 second ≈ 187 blocks.
  const int block = 256;
  const int n_blocks = 48000 / block;
  std::vector<double> clocks(n_blocks);
  for (int i = 0; i < n_blocks; ++i)
    clocks[i] = v.tick(block).value();

  // Count sign changes of clock relative to its mean.
  double mean = 0;
  for (auto c : clocks) mean += c;
  mean /= n_blocks;

  int crossings = 0;
  for (int i = 1; i < n_blocks; ++i) {
    const bool prev = (clocks[i - 1] - mean) >= 0;
    const bool curr = (clocks[i]     - mean) >= 0;
    if (prev != curr) ++crossings;
  }
  // 5 Hz × 2 sign-changes/cycle × 1 s ≈ 10 crossings.
  REQUIRE(crossings >= 8);
  REQUIRE(crossings <= 12);
}
```

- [ ] **Step 8: Update characterization CMakeLists — add VibratoCharTest.cpp, build and run**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[vibrato\]\[L2\]"
```

Expected: 1 test PASS.

- [ ] **Step 9: Commit**

```bash
git add domain/include/vp330/modulation/Vibrato.h \
        domain/src/modulation/Vibrato.cpp \
        tests/unit/modulation/VibratoTest.cpp \
        tests/characterization/modulation/VibratoCharTest.cpp \
        domain/CMakeLists.txt tests/unit/CMakeLists.txt \
        tests/characterization/CMakeLists.txt
git commit -m "feat(modulation): add Vibrato — LFO-modulated master clock with L1+L2 tests"
```

---

### Task 7: OctaveDivider — add set_input_frequency

**Files:**
- Modify: `domain/include/vp330/tod/OctaveDivider.h`
- Modify: `domain/src/tod/OctaveDivider.cpp`
- Modify: `tests/unit/tod/OctaveDividerTest.cpp`

- [ ] **Step 1: Add failing L1 test to OctaveDividerTest.cpp**

Append to `tests/unit/tod/OctaveDividerTest.cpp`:

```cpp
TEST_CASE("OctaveDivider: set_input_frequency changes output zero-crossing rate", "[octave-divider][L1]") {
  OctaveDivider div{Hertz{2000.0}, 48000};

  // After changing to 4000 Hz, zero crossings per second should double.
  div.set_input_frequency(Hertz{4000.0});
  std::vector<float> buf(48000);
  div.render(/*octave_down=*/0, buf.data(), buf.size());

  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i)
    if ((buf[i - 1] >= 0.f) != (buf[i] >= 0.f)) ++crossings;
  REQUIRE(crossings == 8000); // 4000 Hz × 2 crossings/cycle × 1 s
}
```

- [ ] **Step 2: Run — expect failure**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[octave-divider\]\[L1\]"
```

Expected: new test FAIL (method not found).

- [ ] **Step 3: Add set_input_frequency to OctaveDivider.h**

Add to the public section of `OctaveDivider`:

```cpp
void set_input_frequency(Hertz freq);
```

- [ ] **Step 4: Implement in OctaveDivider.cpp**

```cpp
void OctaveDivider::set_input_frequency(Hertz freq) {
  input_frequency_ = freq;
}
```

- [ ] **Step 5: Build and run — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[octave-divider\]"
```

Expected: all OctaveDivider tests PASS.

- [ ] **Step 6: Commit**

```bash
git add domain/include/vp330/tod/OctaveDivider.h \
        domain/src/tod/OctaveDivider.cpp \
        tests/unit/tod/OctaveDividerTest.cpp
git commit -m "feat(tod): OctaveDivider::set_input_frequency for vibrato support"
```

---

### Task 8: TopOctaveDivider — add set_master_clock_hz

**Files:**
- Modify: `domain/include/vp330/tod/TopOctaveDivider.h`
- Modify: `domain/src/tod/TopOctaveDivider.cpp`
- Modify: `tests/unit/tod/TopOctaveDividerTest.cpp`

- [ ] **Step 1: Add failing L1 test to TopOctaveDividerTest.cpp**

Append to `tests/unit/tod/TopOctaveDividerTest.cpp`:

```cpp
TEST_CASE("TopOctaveDivider: set_master_clock_hz updates pitch_class_frequency", "[tod][L1]") {
  std::array<int, 12> ratios{}; ratios.fill(100);
  TopOctaveDivider tod{Hertz{10000.0}, ratios, 48000};
  REQUIRE(tod.pitch_class_frequency(0).value() == Catch::Approx(100.0).margin(1e-9));

  tod.set_master_clock_hz(Hertz{20000.0});
  REQUIRE(tod.pitch_class_frequency(0).value() == Catch::Approx(200.0).margin(1e-9));
}
```

- [ ] **Step 2: Run — expect failure**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[tod\]\[L1\]"
```

Expected: new test FAIL.

- [ ] **Step 3: Add set_master_clock_hz to TopOctaveDivider.h**

Add to public section:

```cpp
void set_master_clock_hz(Hertz hz);
```

- [ ] **Step 4: Implement in TopOctaveDivider.cpp**

```cpp
void TopOctaveDivider::set_master_clock_hz(Hertz hz) {
  master_clock_ = hz;
}
```

- [ ] **Step 5: Build and run all TOD tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[tod\]"
```

- [ ] **Step 6: Commit**

```bash
git add domain/include/vp330/tod/TopOctaveDivider.h \
        domain/src/tod/TopOctaveDivider.cpp \
        tests/unit/tod/TopOctaveDividerTest.cpp
git commit -m "feat(tod): TopOctaveDivider::set_master_clock_hz for vibrato support"
```

---

### Task 9: KeyGate — add attack/release setters

**Files:**
- Modify: `domain/include/vp330/keygate/KeyGate.h`
- Modify: `domain/src/keygate/KeyGate.cpp`
- Modify: `tests/unit/keygate/KeyGateTest.cpp`

- [ ] **Step 1: Add failing L1 tests to KeyGateTest.cpp**

Append to `tests/unit/keygate/KeyGateTest.cpp`:

```cpp
TEST_CASE("KeyGate: set_attack_seconds takes effect on next gate_on", "[keygate][L1]") {
  KeyGate gate{48000, /*attack=*/0.001, /*release=*/0.05}; // 1 ms = 48 samples

  gate.set_attack_seconds(0.002); // 2 ms = 96 samples
  gate.gate_on();

  std::vector<float> sig(48, 1.0f), out(48);
  gate.process(sig.data(), out.data(), 48);
  REQUIRE(gate.state() == KeyGate::State::Attacking); // not yet done at old 48-sample boundary

  gate.process(sig.data(), out.data(), 48);
  REQUIRE(gate.state() == KeyGate::State::Sustain);   // done at new 96-sample boundary
}

TEST_CASE("KeyGate: set_release_seconds takes effect on next gate_off", "[keygate][L1]") {
  KeyGate gate{48000, 0.001, /*release=*/0.05}; // 50 ms = 2400 samples
  gate.gate_on();
  std::vector<float> warm(48, 1.f), wo(48);
  gate.process(warm.data(), wo.data(), 48);

  gate.set_release_seconds(0.01); // 10 ms = 480 samples
  gate.gate_off();

  std::vector<float> sig(480, 1.f), out(480);
  gate.process(sig.data(), out.data(), 480);
  REQUIRE(gate.state() == KeyGate::State::Idle);
}
```

- [ ] **Step 2: Run — expect failure**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[keygate\]\[L1\]"
```

Expected: two new tests FAIL (methods not found).

- [ ] **Step 3: Update KeyGate.h**

Add `sample_rate_` member and the two setters:

```cpp
// In the public section:
void set_attack_seconds(double seconds);
void set_release_seconds(double seconds);

// In the private section (new member):
int sample_rate_;
```

Full updated private section:
```cpp
private:
  void advance_one_sample();

  int sample_rate_;          // stored for set_*_seconds()
  int attack_samples_;
  int release_samples_;
  State state_ = State::Idle;
  double envelope_ = 0.0;
  int phase_index_ = 0;
```

- [ ] **Step 4: Implement in KeyGate.cpp**

Update the constructor to store `sample_rate_`:
```cpp
KeyGate::KeyGate(int sample_rate, double attack_seconds, double release_seconds)
    : sample_rate_{sample_rate},
      attack_samples_{samples_for(attack_seconds, sample_rate)},
      release_samples_{samples_for(release_seconds, sample_rate)} {}
```

Add the two setters (before `advance_one_sample`):
```cpp
void KeyGate::set_attack_seconds(double seconds) {
  attack_samples_ = samples_for(seconds, sample_rate_);
}

void KeyGate::set_release_seconds(double seconds) {
  release_samples_ = samples_for(seconds, sample_rate_);
}
```

- [ ] **Step 5: Build and run all KeyGate tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[keygate\]"
```

Expected: all KeyGate tests PASS.

- [ ] **Step 6: Commit**

```bash
git add domain/include/vp330/keygate/KeyGate.h \
        domain/src/keygate/KeyGate.cpp \
        tests/unit/keygate/KeyGateTest.cpp
git commit -m "feat(keygate): add set_attack/release_seconds for front-panel control"
```

---

### Task 10: MkIIKeyboard — render_zones, set_master_clock_hz, attack/release

**Files:**
- Modify: `domain/include/vp330/tod/MkIIConstants.h`
- Modify: `domain/include/vp330/keyboard/MkIIKeyboard.h`
- Modify: `domain/src/keyboard/MkIIKeyboard.cpp`

- [ ] **Step 1: Add kSplitNote to MkIIConstants.h**

Add after `kKeyboardLowestNote`:

```cpp
inline constexpr MidiNote kSplitNote{60}; // C4 — upper zone starts at this MIDI note
```

- [ ] **Step 2: Update MkIIKeyboard.h**

Add to the public section:

```cpp
void render_zones(float* lower, float* upper, std::size_t frames);
void set_master_clock_hz(vp330::Hertz hz);
void set_attack_seconds(double seconds);
void set_release_seconds(double seconds);
```

`render()` will be reimplemented to call `render_zones` internally (keeps existing callers working):

```cpp
void render(float* out, std::size_t frames); // kept; sums both zones
```

- [ ] **Step 3: Implement in MkIIKeyboard.cpp**

Replace the existing `render()` implementation and add the new methods:

```cpp
void MkIIKeyboard::set_master_clock_hz(Hertz hz) {
  tod_.set_master_clock_hz(hz);
  for (int p = 0; p < 12; ++p)
    octave_dividers_[p].set_input_frequency(tod_.pitch_class_frequency(p));
}

void MkIIKeyboard::set_attack_seconds(double s) {
  for (auto& g : keygates_) g.set_attack_seconds(s);
}

void MkIIKeyboard::set_release_seconds(double s) {
  for (auto& g : keygates_) g.set_release_seconds(s);
}

void MkIIKeyboard::render_zones(float* lower, float* upper, std::size_t frames) {
  std::fill_n(lower, frames, 0.f);
  std::fill_n(upper, frames, 0.f);
  std::vector<float> scratch(frames), gated(frames);

  for (int i = 0; i < mkii::kKeyCount; ++i) {
    if (keygates_[i].state() == KeyGate::State::Idle) continue;
    const auto note = MidiNote{mkii::kKeyboardLowestNote.value() + i};
    const auto t = topology_for(note);
    octave_dividers_[t->pitch_class].render(t->octave_down, scratch.data(), frames);
    keygates_[i].process(scratch.data(), gated.data(), frames);
    const int midi = mkii::kKeyboardLowestNote.value() + i;
    float* zone = (midi < mkii::kSplitNote.value()) ? lower : upper;
    for (std::size_t f = 0; f < frames; ++f)
      zone[f] += gated[f] * kPerKeyGain;
  }
}

void MkIIKeyboard::render(float* out, std::size_t frames) {
  std::vector<float> lower(frames), upper(frames);
  render_zones(lower.data(), upper.data(), frames);
  for (std::size_t i = 0; i < frames; ++i)
    out[i] = lower[i] + upper[i];
}
```

- [ ] **Step 4: Build and run all existing tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure
```

Expected: all existing tests PASS (render() reimplemented as zone sum, same output).

- [ ] **Step 5: Commit**

```bash
git add domain/include/vp330/tod/MkIIConstants.h \
        domain/include/vp330/keyboard/MkIIKeyboard.h \
        domain/src/keyboard/MkIIKeyboard.cpp
git commit -m "feat(keyboard): MkIIKeyboard::render_zones, set_master_clock_hz, attack/release setters"
```

---

### Task 11: SynthesisEngine — wire ChoirSection and Vibrato

**Files:**
- Modify: `domain/include/vp330/engine/SynthesisEngine.h`
- Modify: `domain/src/engine/SynthesisEngine.cpp`

- [ ] **Step 1: Update SynthesisEngine.h**

```cpp
// domain/include/vp330/engine/SynthesisEngine.h
#pragma once

#include "vp330/keyboard/MkIIKeyboard.h"
#include "vp330/modulation/Vibrato.h"
#include "vp330/note/MidiNote.h"
#include "vp330/section/ChoirSection.h"
#include "vp330/section/ChoirSwitch.h"
#include "vp330/values/Hertz.h"

#include <array>
#include <cstddef>
#include <vector>

namespace vp330 {

class SynthesisEngine {
public:
  explicit SynthesisEngine(int sample_rate);

  int sample_rate() const { return sample_rate_; }

  void note_on(MidiNote note);
  void note_off(MidiNote note);
  bool is_note_active(MidiNote note) const;

  void set_choir_switch(ChoirSwitch sw, bool on);
  void set_vibrato_rate(Hertz rate);
  void set_vibrato_depth(float depth_0_to_1);
  void set_attack_seconds(double seconds);
  void set_release_seconds(double seconds);

  void render(float* left, float* right, std::size_t frames);

private:
  int sample_rate_;
  MkIIKeyboard keyboard_;
  ChoirSection  choir_;
  Vibrato       vibrato_;
  std::vector<float> lower_zone_, upper_zone_;
  std::array<bool, 128> note_held_{};
};

} // namespace vp330
```

- [ ] **Step 2: Update SynthesisEngine.cpp**

```cpp
// domain/src/engine/SynthesisEngine.cpp
#include "vp330/engine/SynthesisEngine.h"

namespace vp330 {

SynthesisEngine::SynthesisEngine(int sample_rate)
    : sample_rate_{sample_rate},
      keyboard_{sample_rate},
      choir_{sample_rate},
      vibrato_{sample_rate} {
  choir_.set_switch(ChoirSwitch::UpperMale8, true); // default: upper male 8' on
}

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

void SynthesisEngine::set_choir_switch(ChoirSwitch sw, bool on) { choir_.set_switch(sw, on); }
void SynthesisEngine::set_vibrato_rate(Hertz r)   { vibrato_.set_rate(r); }
void SynthesisEngine::set_vibrato_depth(float d)  { vibrato_.set_depth(d); }
void SynthesisEngine::set_attack_seconds(double s) { keyboard_.set_attack_seconds(s); }
void SynthesisEngine::set_release_seconds(double s){ keyboard_.set_release_seconds(s); }

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  if (lower_zone_.size() < frames) lower_zone_.resize(frames);
  if (upper_zone_.size() < frames) upper_zone_.resize(frames);

  keyboard_.set_master_clock_hz(vibrato_.tick(frames));
  keyboard_.render_zones(lower_zone_.data(), upper_zone_.data(), frames);
  choir_.process(lower_zone_.data(), upper_zone_.data(), left, right, frames);
}

} // namespace vp330
```

- [ ] **Step 3: Build and run all tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure
```

Expected: all tests PASS. The existing golden test will now fail because the rendered output is choir-filtered — that is fixed in Task 12.

- [ ] **Step 4: Commit**

```bash
git add domain/include/vp330/engine/SynthesisEngine.h \
        domain/src/engine/SynthesisEngine.cpp
git commit -m "feat(engine): wire ChoirSection and Vibrato into SynthesisEngine"
```

---

### Task 12: Update golden test for choir output

**Files:**
- Modify: `tests/golden/golden_test.cpp`

The single-C4 golden test currently checks for a square-wave fundamental. Choir filtering turns this into a multi-harmonic bandpass signal — the zero-crossing frequency check no longer makes sense. Update the assertions to verify that the choir section produces a non-silent, non-clipping output.

- [ ] **Step 1: Update the test case in golden_test.cpp**

Replace the `"golden: single C4 renders the MkII C-divider frequency"` test case with:

```cpp
TEST_CASE("golden: single C4 choir section produces non-silent bounded output", "[golden]") {
  // Phase 3: SynthesisEngine now routes through ChoirSection (UpperMale8 on by
  // default). The output is a bandpass-filtered square wave — no longer a clean
  // fundamental, so zero-crossing frequency is not a meaningful check. Verify
  // that the choir section is engaged (non-silent) and not clipping.
  const auto wav = render_fixture("single-c4-1s.mid", 48000, 1.0);
  REQUIRE(wav.sample_rate == 48000);
  REQUIRE(wav.channels    == 2);

  const auto left = left_channel(wav);
  const auto level = rms(left);
  INFO("RMS: " << level);

  REQUIRE(level > 1e-4);           // choir section is producing sound
  REQUIRE(max_abs_sample(wav) < 1.0f); // no clipping
}
```

- [ ] **Step 2: Build and run golden tests — expect pass**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "\[golden\]"
```

Expected: both golden tests PASS (`walking skeleton` unchanged; `single C4` now passes with new assertions).

- [ ] **Step 3: Run the full test suite — all green**

```bash
ctest --test-dir build --output-on-failure
```

Expected: all tests PASS.

- [ ] **Step 4: Commit**

```bash
git add tests/golden/golden_test.cpp
git commit -m "golden: update single-c4-1s — choir section engaged (Upper Male 8')"
```

---

### Task 13: choir-filter-constants.md — provenance doc

**Files:**
- Create: `docs/choir-filter-constants.md`

- [ ] **Step 1: Create the doc**

```markdown
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
```

- [ ] **Step 2: Commit**

```bash
git add docs/choir-filter-constants.md
git commit -m "docs: add choir-filter-constants provenance and calibration log"
```
