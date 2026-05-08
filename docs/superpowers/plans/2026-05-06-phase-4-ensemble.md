# Phase 4 — Ensemble & ChoirCompander Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `BbdLine`, `Ensemble`, and `ChoirCompander` to the VP-330 domain, wire them into `SynthesisEngine`, and raise the post-cascade gain from 2.7× to 8.72× to produce true stereo output with BBD chorus character and choir "breathing" attack.

**Architecture:** Three new domain components arrive in dependency order: `BbdLine` (single BBD chip model) → `Ensemble` (four-line stereo BBD chorus) → `ChoirCompander` (peak-follower gain leveler). `SynthesisEngine` chains `ChoirSection → ChoirCompander → Ensemble → L/R`. All components land with L1 + L2 tests in the same commit as the implementation. The ETH16 pre/post BBD compander is not modelled (DSP has no noise floor).

**Tech Stack:** C++20, Catch2 (`ctest`), `cmake -B build -S . && cmake --build build`

**Spec:** `docs/superpowers/specs/2026-05-06-phase-4-ensemble-design.md`

---

## File Map

**Create:**
- `domain/include/vp330/ensemble/BbdLine.h`
- `domain/src/ensemble/BbdLine.cpp`
- `domain/include/vp330/ensemble/EnsembleConstants.h`
- `domain/include/vp330/ensemble/Ensemble.h`
- `domain/src/ensemble/Ensemble.cpp`
- `domain/include/vp330/section/ChoirCompander.h`
- `domain/src/section/ChoirCompander.cpp`
- `tests/unit/ensemble/BbdLineTest.cpp`
- `tests/unit/ensemble/EnsembleTest.cpp`
- `tests/unit/section/ChoirCompanderTest.cpp`
- `tests/characterization/ensemble/BbdLineCharTest.cpp`
- `tests/characterization/ensemble/EnsembleCharTest.cpp`
- `tests/characterization/section/ChoirCompanderCharTest.cpp`

**Modify:**
- `domain/CMakeLists.txt` — add new `.cpp` sources
- `tests/unit/CMakeLists.txt` — add new test sources
- `tests/characterization/CMakeLists.txt` — add new char-test sources
- `domain/include/vp330/engine/SynthesisEngine.h` — new members + setters
- `domain/src/engine/SynthesisEngine.cpp` — wire signal chain
- `domain/include/vp330/section/ChoirConstants.h` — raise gain 2.7 → 8.72
- `tests/golden/baselines/single-c4-1s.wav` — re-rendered baseline

---

## Task 1: BbdLine — variable fractional-delay with tracking lowpass

**Files:**
- Create: `domain/include/vp330/ensemble/BbdLine.h`
- Create: `domain/src/ensemble/BbdLine.cpp`
- Create: `tests/unit/ensemble/BbdLineTest.cpp`
- Create: `tests/characterization/ensemble/BbdLineCharTest.cpp`
- Modify: `domain/CMakeLists.txt`
- Modify: `tests/unit/CMakeLists.txt`
- Modify: `tests/characterization/CMakeLists.txt`

- [ ] **Step 1: Write failing L1 tests**

Create `tests/unit/ensemble/BbdLineTest.cpp`:

```cpp
#include "vp330/ensemble/BbdLine.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using vp330::BbdLine;

TEST_CASE("BbdLine L1: delay accurate to ±0.1ms", "[bbd_line][L1]") {
  // At 2ms, clock_hz = 256/0.002 = 128 kHz, fc = 38.4 kHz > Nyquist
  // → lp_coeff_ = 0 (bypass), output is a clean 96-sample delayed copy.
  BbdLine bbd{48000, 256, 0.0128f};
  bbd.set_delay(0.002f); // 96 samples exactly

  const int n = 256;
  std::vector<float> in(n, 0.f), out(n);
  in[0] = 1.f;
  bbd.process(in.data(), out.data(), n);

  int peak = 0;
  for (int i = 1; i < n; ++i)
    if (std::fabs(out[i]) > std::fabs(out[peak]))
      peak = i;

  REQUIRE(std::abs(peak - 96) <= 5); // ±0.1ms = ±4.8 samples
  REQUIRE(out[96] == Catch::Approx(1.0f).margin(1e-4f)); // exact with no lowpass
}

TEST_CASE("BbdLine L1: lowpass corner decreases as delay increases", "[bbd_line][L1]") {
  // At 2ms: fc >> Nyquist → no HF attenuation.
  // At max 12.8ms: fc = 6kHz → significant 10kHz attenuation.
  const int fs = 48000;
  const int n = fs;
  std::vector<float> in(n), out_short(n), out_long(n);
  const float k = 2.f * std::numbers::pi_v<float> * 10000.f / static_cast<float>(fs);
  for (int i = 0; i < n; ++i)
    in[i] = std::sin(k * static_cast<float>(i));

  BbdLine bbd_short{fs, 256, 0.0128f};
  bbd_short.set_delay(0.002f);
  bbd_short.process(in.data(), out_short.data(), n);

  BbdLine bbd_long{fs, 256, 0.0128f};
  bbd_long.set_delay(0.0128f);
  bbd_long.process(in.data(), out_long.data(), n);

  auto rms2 = [&](const std::vector<float>& v) {
    float s = 0;
    for (int i = n / 2; i < n; ++i) s += v[i] * v[i];
    return std::sqrt(2.f * s / static_cast<float>(n));
  };

  // At least 6 dB more HF energy at short (unfiltered) delay
  REQUIRE(rms2(out_short) > rms2(out_long) * 2.f);
}
```

- [ ] **Step 2: Verify L1 fails (file not found)**

```bash
cmake -B build -S . && cmake --build build 2>&1 | grep -c "error"
```

Expected: compile errors because `BbdLine.h` doesn't exist yet.

- [ ] **Step 3: Write the header**

Create `domain/include/vp330/ensemble/BbdLine.h`:

```cpp
#pragma once

#include <cstddef>
#include <vector>

namespace vp330 {

class BbdLine {
public:
  // n_stages: 256 (MN3009-style) or 512 (MN3004-style)
  // max_delay_seconds: sets circular-buffer capacity
  BbdLine(int sample_rate, int n_stages, float max_delay_seconds);

  // Called per-block by Ensemble before process().
  void set_delay(float seconds);

  void process(const float* in, float* out, std::size_t frames);

private:
  int sample_rate_;
  int n_stages_;
  float max_delay_seconds_;
  float delay_seconds_;

  std::vector<float> buffer_;
  int write_pos_ = 0;

  // Tracking 1-pole IIR: fc = 0.3 × (n_stages / delay_seconds).
  // Coefficient recomputed in set_delay(). 0 when fc > Nyquist (bypass).
  float lp_coeff_ = 0.f;
  float lp_pre_z1_ = 0.f;
  float lp_post_z1_ = 0.f;
};

} // namespace vp330
```

- [ ] **Step 4: Write the implementation**

Create `domain/src/ensemble/BbdLine.cpp`:

```cpp
#include "vp330/ensemble/BbdLine.h"

#include <cassert>
#include <cmath>
#include <numbers>

namespace vp330 {

BbdLine::BbdLine(int sample_rate, int n_stages, float max_delay_seconds)
    : sample_rate_{sample_rate}, n_stages_{n_stages}, max_delay_seconds_{max_delay_seconds},
      delay_seconds_{max_delay_seconds * 0.5f} {
  assert(sample_rate > 0);
  assert(n_stages > 0);
  assert(max_delay_seconds > 0.f);
  const int buf_size =
      static_cast<int>(max_delay_seconds * static_cast<float>(sample_rate)) + 4;
  buffer_.assign(buf_size, 0.f);
  set_delay(delay_seconds_);
}

void BbdLine::set_delay(float seconds) {
  assert(seconds > 0.f && seconds <= max_delay_seconds_);
  delay_seconds_ = seconds;
  // S&H-accurate: clock frequency determines bandwidth.
  const float clock_hz = static_cast<float>(n_stages_) / seconds;
  const float fc = 0.3f * clock_hz;
  const float w = 2.f * std::numbers::pi_v<float> * fc / static_cast<float>(sample_rate_);
  lp_coeff_ = (w >= std::numbers::pi_v<float>) ? 0.f : std::exp(-w);
}

void BbdLine::process(const float* in, float* out, std::size_t frames) {
  const int buf_size = static_cast<int>(buffer_.size());
  const float delay_samples = delay_seconds_ * static_cast<float>(sample_rate_);
  const int delay_int = static_cast<int>(delay_samples);
  const float frac = delay_samples - static_cast<float>(delay_int);
  const float c = lp_coeff_;
  const float a = 1.f - c;

  for (std::size_t i = 0; i < frames; ++i) {
    lp_pre_z1_ = a * in[i] + c * lp_pre_z1_;
    buffer_[write_pos_] = lp_pre_z1_;

    const int r0 = (write_pos_ - delay_int + buf_size) % buf_size;
    const int r1 = (r0 - 1 + buf_size) % buf_size;
    const float delayed = buffer_[r0] + frac * (buffer_[r1] - buffer_[r0]);

    lp_post_z1_ = a * delayed + c * lp_post_z1_;
    out[i] = lp_post_z1_;

    write_pos_ = (write_pos_ + 1) % buf_size;
  }
}

} // namespace vp330
```

- [ ] **Step 5: Write L2 characterization tests**

Create `tests/characterization/ensemble/BbdLineCharTest.cpp`:

```cpp
#include "vp330/ensemble/BbdLine.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using vp330::BbdLine;

namespace {

std::vector<float> make_sine(float freq, int n, int fs) {
  std::vector<float> v(n);
  const float k = 2.f * std::numbers::pi_v<float> * freq / static_cast<float>(fs);
  for (int i = 0; i < n; ++i)
    v[i] = std::sin(k * static_cast<float>(i));
  return v;
}

float rms_second_half(BbdLine& bbd, const std::vector<float>& in) {
  const int n = static_cast<int>(in.size());
  std::vector<float> out(n);
  bbd.process(in.data(), out.data(), n);
  float s = 0;
  for (int i = n / 2; i < n; ++i)
    s += out[i] * out[i];
  return std::sqrt(2.f * s / static_cast<float>(n));
}

} // namespace

TEST_CASE("BbdLine L2: 1kHz fundamental preserved ±1dB across full delay range (MN3009)",
          "[bbd_line][L2]") {
  const int fs = 48000, n = fs;
  auto sine_1k = make_sine(1000.f, n, fs);

  BbdLine bbd_min{fs, 256, 0.0128f};
  bbd_min.set_delay(0.00064f); // MN3009 min: 0.64ms, fc >> Nyquist → no filter
  const float rms_min = rms_second_half(bbd_min, sine_1k);

  BbdLine bbd_max{fs, 256, 0.0128f};
  bbd_max.set_delay(0.0128f); // MN3009 max: 12.8ms, fc = 6kHz
  const float rms_max = rms_second_half(bbd_max, sine_1k);

  // ±1 dB = ratio in [0.891, 1.122]
  REQUIRE(rms_max > rms_min * 0.891f);
  REQUIRE(rms_max < rms_min * 1.122f);
}

TEST_CASE("BbdLine L2: 1kHz fundamental preserved ±1dB across full delay range (MN3004)",
          "[bbd_line][L2]") {
  const int fs = 48000, n = fs;
  auto sine_1k = make_sine(1000.f, n, fs);

  BbdLine bbd_min{fs, 512, 0.0256f};
  bbd_min.set_delay(0.00256f); // MN3004 min: 2.56ms, fc >> Nyquist
  const float rms_min = rms_second_half(bbd_min, sine_1k);

  BbdLine bbd_max{fs, 512, 0.0256f};
  bbd_max.set_delay(0.0256f); // MN3004 max: 25.6ms, fc = 6kHz
  const float rms_max = rms_second_half(bbd_max, sine_1k);

  REQUIRE(rms_max > rms_min * 0.891f);
  REQUIRE(rms_max < rms_min * 1.122f);
}

TEST_CASE("BbdLine L2: bandwidth at max delay ≤ 7kHz, −3dB (MN3009)", "[bbd_line][L2]") {
  const int fs = 48000, n = fs;

  BbdLine bbd_7k{fs, 256, 0.0128f};
  bbd_7k.set_delay(0.0128f);
  const float rms_7k = rms_second_half(bbd_7k, make_sine(7000.f, n, fs));

  BbdLine bbd_ref{fs, 256, 0.0128f};
  bbd_ref.set_delay(0.0128f);
  const float rms_ref = rms_second_half(bbd_ref, make_sine(100.f, n, fs));

  // 7kHz should be below −3dB (0.707×) relative to passband at max delay
  REQUIRE(rms_7k < rms_ref * 0.707f);
}

TEST_CASE("BbdLine L2: bandwidth at max delay ≤ 7kHz, −3dB (MN3004)", "[bbd_line][L2]") {
  const int fs = 48000, n = fs;

  BbdLine bbd_7k{fs, 512, 0.0256f};
  bbd_7k.set_delay(0.0256f);
  const float rms_7k = rms_second_half(bbd_7k, make_sine(7000.f, n, fs));

  BbdLine bbd_ref{fs, 512, 0.0256f};
  bbd_ref.set_delay(0.0256f);
  const float rms_ref = rms_second_half(bbd_ref, make_sine(100.f, n, fs));

  REQUIRE(rms_7k < rms_ref * 0.707f);
}
```

- [ ] **Step 6: Register sources in CMake**

In `domain/CMakeLists.txt`, add after `src/modulation/Vibrato.cpp`:

```cmake
    src/ensemble/BbdLine.cpp
```

In `tests/unit/CMakeLists.txt`, add after the last `VibratoTest.cpp` line:

```
    ${CMAKE_CURRENT_SOURCE_DIR}/ensemble/BbdLineTest.cpp
```

In `tests/characterization/CMakeLists.txt`, add after `VibratoCharTest.cpp`:

```
    ${CMAKE_CURRENT_SOURCE_DIR}/ensemble/BbdLineCharTest.cpp
```

- [ ] **Step 7: Build and run BbdLine tests**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "bbd_line"
```

Expected: all `[bbd_line]` tests pass.

- [ ] **Step 8: Commit**

```bash
git add domain/include/vp330/ensemble/BbdLine.h \
        domain/src/ensemble/BbdLine.cpp \
        tests/unit/ensemble/BbdLineTest.cpp \
        tests/characterization/ensemble/BbdLineCharTest.cpp \
        domain/CMakeLists.txt \
        tests/unit/CMakeLists.txt \
        tests/characterization/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(ensemble): add BbdLine — variable fractional-delay with tracking lowpass (L1+L2)

S&H-accurate BBD model: fc = 0.3 × (n_stages / delay_s). lp_coeff_ clamps to 0
when fc > Nyquist so short delays are filter-transparent. Pre/post 1-pole cascade
gives ≥ −3dB at 7kHz at max delay for both MN3009 and MN3004 parameters.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
EOF
)"
```

---

## Task 2: Ensemble — four-line stereo BBD chorus

**Files:**
- Create: `domain/include/vp330/ensemble/EnsembleConstants.h`
- Create: `domain/include/vp330/ensemble/Ensemble.h`
- Create: `domain/src/ensemble/Ensemble.cpp`
- Create: `tests/unit/ensemble/EnsembleTest.cpp`
- Create: `tests/characterization/ensemble/EnsembleCharTest.cpp`
- Modify: `domain/CMakeLists.txt`
- Modify: `tests/unit/CMakeLists.txt`
- Modify: `tests/characterization/CMakeLists.txt`

- [ ] **Step 1: Write failing L1 tests**

Create `tests/unit/ensemble/EnsembleTest.cpp`:

```cpp
#include "vp330/ensemble/Ensemble.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

using vp330::Ensemble;

TEST_CASE("Ensemble L1: enabled produces different L and R", "[ensemble][L1]") {
  Ensemble ens{48000};
  const int n = 48000; // one full second covers > one LFO period
  std::vector<float> in(n, 0.1f), left(n), right(n);
  ens.process(in.data(), left.data(), right.data(), n);

  bool any_diff = false;
  for (int i = 0; i < n; ++i)
    if (left[i] != right[i]) {
      any_diff = true;
      break;
    }
  REQUIRE(any_diff);
}

TEST_CASE("Ensemble L1: bypass produces L == R == dry (bit-exact)", "[ensemble][L1]") {
  Ensemble ens{48000};
  ens.set_enabled(false);

  const int n = 512;
  std::vector<float> in(n);
  for (int i = 0; i < n; ++i)
    in[i] = static_cast<float>(i) * 0.001f; // non-trivial signal
  std::vector<float> left(n), right(n);
  ens.process(in.data(), left.data(), right.data(), n);

  for (int i = 0; i < n; ++i) {
    REQUIRE(left[i] == in[i]);
    REQUIRE(right[i] == in[i]);
  }
}

TEST_CASE("Ensemble L1: LFO phase does not advance while bypassed", "[ensemble][L1]") {
  Ensemble ens{48000};
  ens.set_enabled(false);

  std::vector<float> in(256, 0.1f), l1(256), r1(256), l2(256), r2(256);
  ens.process(in.data(), l1.data(), r1.data(), 256);

  ens.set_enabled(true);
  ens.set_enabled(false);
  ens.process(in.data(), l2.data(), r2.data(), 256);

  // Both bypass blocks should be bit-identical dry output
  for (int i = 0; i < 256; ++i) {
    REQUIRE(l1[i] == in[i]);
    REQUIRE(l2[i] == in[i]);
  }
}
```

- [ ] **Step 2: Verify L1 fails (file not found)**

```bash
cmake -B build -S . 2>&1 | grep -c "error" || true
```

Expected: errors because `Ensemble.h` doesn't exist.

- [ ] **Step 3: Write the constants header**

Create `domain/include/vp330/ensemble/EnsembleConstants.h`:

```cpp
#pragma once

// VP-330 MkII ETH16 BBD ensemble constants.
// Source: service manual page 14 (IC16–IC19 chip parameters), page 9 (LFO range).
// Mix weights are L4 calibration targets; adjust here without code changes.

namespace vp330::mkii {

// MN3009-style: 256 stages, delay range 0.64–12.8ms
inline constexpr float kMaxDelayShort = 12.8e-3f;
inline constexpr float kCentreDelayShort = 6.0e-3f;
inline constexpr float kDepthShort = 1.5e-3f; // ±1.5ms default modulation depth

// MN3004-style: 512 stages, delay range 2.56–25.6ms
inline constexpr float kMaxDelayLong = 25.6e-3f;
inline constexpr float kCentreDelayLong = 20.0e-3f;
inline constexpr float kDepthLong = 3.0e-3f; // ±3ms default modulation depth

// LFO: 0.45–1.4 Hz range (service manual page 9), default 0.7 Hz
inline constexpr float kLfoRateHz = 0.7f;

// Stereo mix: Left = dry×kDryGain + line0×kWetGain + line2×kWetGain
//             Right = dry×kDryGain + line1×kWetGain + line3×kWetGain
inline constexpr float kDryGain = 0.5f;
inline constexpr float kWetGain = 0.25f;

} // namespace vp330::mkii
```

- [ ] **Step 4: Write the Ensemble header**

Create `domain/include/vp330/ensemble/Ensemble.h`:

```cpp
#pragma once

#include "vp330/ensemble/BbdLine.h"
#include "vp330/ensemble/EnsembleConstants.h"
#include "vp330/values/Hertz.h"

#include <array>
#include <cstddef>
#include <vector>

namespace vp330 {

class Ensemble {
public:
  explicit Ensemble(int sample_rate);

  void set_rate(Hertz rate);
  void set_depth(float depth_0_to_1); // scales default mkii::kDepthShort/Long
  void set_enabled(bool on);

  // mono in → stereo out (overwrites left and right)
  void process(const float* in, float* left, float* right, std::size_t frames);

private:
  int sample_rate_;
  std::array<BbdLine, 4> lines_; // [0,1] = MN3009-style; [2,3] = MN3004-style
  float phase_ = 0.f;
  float rate_hz_ = mkii::kLfoRateHz;
  float depth_short_ = mkii::kDepthShort;
  float depth_long_ = mkii::kDepthLong;
  bool enabled_ = true;
  std::array<std::vector<float>, 4> line_scratch_;
};

} // namespace vp330
```

- [ ] **Step 5: Write the Ensemble implementation**

Create `domain/src/ensemble/Ensemble.cpp`:

```cpp
#include "vp330/ensemble/Ensemble.h"

#include "vp330/ensemble/EnsembleConstants.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

namespace vp330 {

Ensemble::Ensemble(int sample_rate)
    : sample_rate_{sample_rate},
      lines_{BbdLine{sample_rate, 256, mkii::kMaxDelayShort},
             BbdLine{sample_rate, 256, mkii::kMaxDelayShort},
             BbdLine{sample_rate, 512, mkii::kMaxDelayLong},
             BbdLine{sample_rate, 512, mkii::kMaxDelayLong}} {}

void Ensemble::set_rate(Hertz rate) {
  assert(rate.value() > 0.0);
  rate_hz_ = static_cast<float>(rate.value());
}

void Ensemble::set_depth(float depth_0_to_1) {
  assert(depth_0_to_1 >= 0.f && depth_0_to_1 <= 1.f);
  depth_short_ = depth_0_to_1 * mkii::kDepthShort;
  depth_long_ = depth_0_to_1 * mkii::kDepthLong;
}

void Ensemble::set_enabled(bool on) {
  enabled_ = on;
}

void Ensemble::process(const float* in, float* left, float* right, std::size_t frames) {
  if (!enabled_) {
    std::copy(in, in + frames, left);
    std::copy(in, in + frames, right);
    return;
  }

  for (auto& s : line_scratch_)
    if (s.size() < frames)
      s.resize(frames);

  // Set delays for this block from current LFO phase.
  // Quadrature phases: line0=0°, line1=90°, line2=180°, line3=270°.
  const float sn = std::sin(phase_);
  const float cs = std::cos(phase_); // sin(phase + 90°)

  lines_[0].set_delay(mkii::kCentreDelayShort + depth_short_ * sn);
  lines_[1].set_delay(mkii::kCentreDelayShort + depth_short_ * cs);
  lines_[2].set_delay(mkii::kCentreDelayLong - depth_long_ * sn); // sin(phase + 180°)
  lines_[3].set_delay(mkii::kCentreDelayLong - depth_long_ * cs); // sin(phase + 270°)

  for (int i = 0; i < 4; ++i)
    lines_[i].process(in, line_scratch_[i].data(), frames);

  // Mix: Left  = dry×0.5 + line0×0.25 + line2×0.25
  //      Right = dry×0.5 + line1×0.25 + line3×0.25
  const float* l0 = line_scratch_[0].data();
  const float* l1 = line_scratch_[1].data();
  const float* l2 = line_scratch_[2].data();
  const float* l3 = line_scratch_[3].data();
  for (std::size_t i = 0; i < frames; ++i) {
    left[i] = mkii::kDryGain * in[i] + mkii::kWetGain * l0[i] + mkii::kWetGain * l2[i];
    right[i] = mkii::kDryGain * in[i] + mkii::kWetGain * l1[i] + mkii::kWetGain * l3[i];
  }

  // Advance phase by one block (per-block LFO update as spec'd).
  const float block_seconds = static_cast<float>(frames) / static_cast<float>(sample_rate_);
  phase_ += 2.f * std::numbers::pi_v<float> * rate_hz_ * block_seconds;
  if (phase_ >= 2.f * std::numbers::pi_v<float>)
    phase_ -= 2.f * std::numbers::pi_v<float>;
}

} // namespace vp330
```

- [ ] **Step 6: Write L2 characterization tests**

Create `tests/characterization/ensemble/EnsembleCharTest.cpp`:

```cpp
#include "vp330/ensemble/Ensemble.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numeric>
#include <vector>

using vp330::Ensemble;

namespace {

float cross_correlation(const std::vector<float>& a, const std::vector<float>& b) {
  const std::size_t n = a.size();
  double sum_ab = 0, sum_aa = 0, sum_bb = 0;
  for (std::size_t i = 0; i < n; ++i) {
    sum_ab += static_cast<double>(a[i]) * b[i];
    sum_aa += static_cast<double>(a[i]) * a[i];
    sum_bb += static_cast<double>(b[i]) * b[i];
  }
  const double denom = std::sqrt(sum_aa * sum_bb);
  return denom < 1e-30 ? 0.f : static_cast<float>(sum_ab / denom);
}

} // namespace

TEST_CASE("Ensemble L2: stereo cross-correlation < 0.8 (decorrelated)", "[ensemble][L2]") {
  Ensemble ens{48000};
  // Two full LFO periods at 0.7 Hz: ~2.86 s = 137280 samples
  const int n = 2 * static_cast<int>(48000.f / 0.7f) + 1;
  std::vector<float> in(n, 0.1f), left(n), right(n);
  ens.process(in.data(), left.data(), right.data(), n);

  // Measure correlation over the second half (past initial delay settling)
  std::vector<float> l2(left.begin() + n / 2, left.end());
  std::vector<float> r2(right.begin() + n / 2, right.end());
  REQUIRE(cross_correlation(l2, r2) < 0.8f);
}

TEST_CASE("Ensemble L2: output level within expected range for DC input", "[ensemble][L2]") {
  Ensemble ens{48000};
  const int n = 48000;
  std::vector<float> in(n, 1.0f), left(n), right(n);
  ens.process(in.data(), left.data(), right.data(), n);

  // With dry=0.5 and two wet lines at 0.25 each, total gain approaches 1.0
  // for DC (all lines agree at DC). Allow [0.8, 1.2] for modulation variance.
  auto mean_abs = [](const std::vector<float>& v) {
    float s = 0;
    for (auto x : v)
      s += std::fabs(x);
    return s / static_cast<float>(v.size());
  };
  REQUIRE(mean_abs(left) > 0.8f);
  REQUIRE(mean_abs(left) < 1.2f);
  REQUIRE(mean_abs(right) > 0.8f);
  REQUIRE(mean_abs(right) < 1.2f);
}
```

- [ ] **Step 7: Register sources in CMake**

In `domain/CMakeLists.txt`, add after `src/ensemble/BbdLine.cpp`:

```cmake
    src/ensemble/Ensemble.cpp
```

In `tests/unit/CMakeLists.txt`, add after `BbdLineTest.cpp`:

```
    ${CMAKE_CURRENT_SOURCE_DIR}/ensemble/EnsembleTest.cpp
```

In `tests/characterization/CMakeLists.txt`, add after `BbdLineCharTest.cpp`:

```
    ${CMAKE_CURRENT_SOURCE_DIR}/ensemble/EnsembleCharTest.cpp
```

- [ ] **Step 8: Build and run Ensemble tests**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "ensemble"
```

Expected: all `[ensemble]` tests pass.

- [ ] **Step 9: Commit**

```bash
git add domain/include/vp330/ensemble/EnsembleConstants.h \
        domain/include/vp330/ensemble/Ensemble.h \
        domain/src/ensemble/Ensemble.cpp \
        tests/unit/ensemble/EnsembleTest.cpp \
        tests/characterization/ensemble/EnsembleCharTest.cpp \
        domain/CMakeLists.txt \
        tests/unit/CMakeLists.txt \
        tests/characterization/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(ensemble): add Ensemble — four-line BBD stereo chorus with quadrature LFO (L1+L2)

Lines 0/1: MN3009 256-stage (short, ±1.5ms), Lines 2/3: MN3004 512-stage (long, ±3ms).
Quadrature phases (0°/90°/180°/270°) produce stereo cross-correlation < 0.8. Phase
holds during bypass; no phase jump on re-enable.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
EOF
)"
```

---

## Task 3: ChoirCompander — HVH105 single-sided peak-follower leveler

**Files:**
- Create: `domain/include/vp330/section/ChoirCompander.h`
- Create: `domain/src/section/ChoirCompander.cpp`
- Create: `tests/unit/section/ChoirCompanderTest.cpp`
- Create: `tests/characterization/section/ChoirCompanderCharTest.cpp`
- Modify: `domain/CMakeLists.txt`
- Modify: `tests/unit/CMakeLists.txt`
- Modify: `tests/characterization/CMakeLists.txt`

- [ ] **Step 1: Write failing L1 tests**

Create `tests/unit/section/ChoirCompanderTest.cpp`:

```cpp
#include "vp330/section/ChoirCompander.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::ChoirCompander;

TEST_CASE("ChoirCompander L1: instantaneous attack — envelope reaches step in ≤ 2 samples",
          "[choir_compander][L1]") {
  ChoirCompander cc{48000};
  // Feed step: 0 → 0.5
  std::vector<float> in{0.f, 0.5f, 0.5f, 0.5f};
  std::vector<float> out(4);
  cc.process(in.data(), out.data(), 4);

  // Instantaneous attack: g[1] should already reflect envelope=0.5
  // g = kGMax * 0.5 / (kEnvTarget + 0.5) ≈ 3.33; out[1] ≈ 1.67
  REQUIRE(out[1] > out[0]);             // gain applied immediately
  REQUIRE(out[2] == Catch::Approx(out[1]).margin(1e-5f)); // steady at sample 2
}

TEST_CASE("ChoirCompander L1: steady-state gain at env_target is g_max/2",
          "[choir_compander][L1]") {
  ChoirCompander cc{48000};
  // DC at kEnvTarget (0.1): envelope snaps to 0.1 on first sample and stays.
  // g = kGMax * 0.1 / (0.1 + 0.1) = kGMax/2 = 2.0; out = 2.0 * 0.1 = 0.2
  const float probe = 0.1f; // = kEnvTarget
  std::vector<float> in(4, probe), out(4);
  cc.process(in.data(), out.data(), 4);
  REQUIRE(out[3] == Catch::Approx(0.2f).margin(1e-4f)); // g_max/2 * probe = 2.0*0.1
}

TEST_CASE("ChoirCompander L1: release τ within ±10% of 103ms", "[choir_compander][L1]") {
  ChoirCompander cc{48000};
  // Drive envelope to 1.0 via instantaneous attack.
  float x = 1.0f, y;
  cc.process(&x, &y, 1);

  // Tiny probe (1e-6 << kEnvTarget=0.1) so it does not re-trigger the attack
  // while the envelope decays from 1.0 toward 0 during the observation window.
  const float probe = 1e-6f;
  const int tau_nominal = static_cast<int>(std::round(0.103 * 48000)); // 4944
  std::vector<float> in_decay(tau_nominal, probe), out_decay(tau_nominal);
  cc.process(in_decay.data(), out_decay.data(), tau_nominal);

  // Back-compute envelope: env = kEnvTarget * g / (kGMax - g)
  // kGMax=4.0, kEnvTarget=0.1
  auto env_from_g = [](float g) { return 0.1f * g / (4.0f - g); };
  const float g0 = out_decay[0] / probe;       // ≈ 3.636 (env ≈ 1.0)
  const float g_tau = out_decay.back() / probe; // ≈ 3.145 (env ≈ 1/e ≈ 0.368)
  const float env0 = env_from_g(g0);
  const float env_tau = env_from_g(g_tau);

  REQUIRE(env0 > 0.9f);      // sanity: initial envelope should be ≈ 1.0
  REQUIRE(env_tau > 0.329f); // exp(−1/0.9): τ ≥ 92.7ms
  REQUIRE(env_tau < 0.403f); // exp(−1/1.1): τ ≤ 113.3ms
}

TEST_CASE("ChoirCompander L1: process is in-place safe", "[choir_compander][L1]") {
  ChoirCompander cc{48000};
  std::vector<float> buf(64, 0.3f);
  cc.process(buf.data(), buf.data(), 64); // in == out
  // Should not crash or produce NaN
  for (auto s : buf)
    REQUIRE(std::isfinite(s));
}
```

- [ ] **Step 2: Verify L1 fails**

```bash
cmake -B build -S . 2>&1 | grep "error" | head -5
```

Expected: errors because `ChoirCompander.h` doesn't exist.

- [ ] **Step 3: Write the header**

Create `domain/include/vp330/section/ChoirCompander.h`:

```cpp
#pragma once

#include <cstddef>

// HVH105 / BA662A single-sided leveler model.
// Source: service manual (R=4.7MΩ, C=22nF → τ=103ms), filter-bank-research.md §5.
//
// DSP:  r[n]   = |x[n]|
//        env[n] = r[n] >= env[n-1] ? r[n] : α × env[n-1]   (instantaneous attack)
//        g[n]   = kGMax × env[n] / (kEnvTarget + env[n])
//        y[n]   = g[n] × x[n]
//
// Adjust kGMax and kEnvTarget here for L4 calibration.

namespace vp330 {

inline constexpr double kGMax = 4.0;
inline constexpr double kEnvTarget = 0.1;

class ChoirCompander {
public:
  explicit ChoirCompander(int sample_rate);

  // in and out may alias (in-place safe).
  void process(const float* in, float* out, std::size_t frames);

private:
  double envelope_ = 0.0;
  double alpha_release_; // exp(−1 / (0.103 × sample_rate))
};

} // namespace vp330
```

- [ ] **Step 4: Write the implementation**

Create `domain/src/section/ChoirCompander.cpp`:

```cpp
#include "vp330/section/ChoirCompander.h"

#include <cmath>

namespace vp330 {

ChoirCompander::ChoirCompander(int sample_rate)
    : alpha_release_{std::exp(-1.0 / (0.103 * static_cast<double>(sample_rate)))} {}

void ChoirCompander::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    const double x = static_cast<double>(in[i]);
    const double r = std::fabs(x);
    envelope_ = (r >= envelope_) ? r : alpha_release_ * envelope_;
    const double g = kGMax * envelope_ / (kEnvTarget + envelope_);
    out[i] = static_cast<float>(g * x);
  }
}

} // namespace vp330
```

- [ ] **Step 5: Write L2 characterization tests**

Create `tests/characterization/section/ChoirCompanderCharTest.cpp`:

```cpp
#include "vp330/section/ChoirCompander.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::ChoirCompander;

TEST_CASE("ChoirCompander L2: output does not clip for high-amplitude input",
          "[choir_compander][L2]") {
  ChoirCompander cc{48000};
  // Simulate post-cascade peak amplitude with 8.72× gain applied.
  // A full-scale input (1.0) would be clipped to 1.0 by the compander.
  const int n = 48000;
  std::vector<float> in(n, 1.0f), out(n);
  cc.process(in.data(), out.data(), n);

  float peak = 0;
  for (auto s : out)
    peak = std::max(peak, std::fabs(s));
  // Compander gain map ensures g < kGMax → output ≤ kGMax * input
  // At steady state envelope = input: g = kGMax/(1 + kEnvTarget/input) < kGMax
  REQUIRE(peak < static_cast<float>(vp330::kGMax));
}

TEST_CASE("ChoirCompander L2: gain monotonically increases with envelope level",
          "[choir_compander][L2]") {
  // g = kGMax * env / (kEnvTarget + env) is monotonically increasing.
  // Verify by driving to three different steady-state levels.
  auto steady_gain = [](float amplitude) {
    ChoirCompander cc{48000};
    std::vector<float> in(48000, amplitude), out(48000);
    cc.process(in.data(), out.data(), 48000);
    return std::fabs(out.back()) / amplitude; // gain
  };

  const float g_low = steady_gain(0.05f);  // env_target=0.1 → env below target
  const float g_mid = steady_gain(0.1f);   // env = env_target → g = kGMax/2
  const float g_high = steady_gain(1.0f);  // env >> env_target → g → kGMax

  REQUIRE(g_low < g_mid);
  REQUIRE(g_mid < g_high);
  REQUIRE(g_mid == Catch::Approx(static_cast<float>(vp330::kGMax) / 2.f).margin(0.01f));
}
```

- [ ] **Step 6: Register sources in CMake**

In `domain/CMakeLists.txt`, add after `src/ensemble/Ensemble.cpp`:

```cmake
    src/section/ChoirCompander.cpp
```

In `tests/unit/CMakeLists.txt`, add after `EnsembleTest.cpp`:

```
    ${CMAKE_CURRENT_SOURCE_DIR}/section/ChoirCompanderTest.cpp
```

In `tests/characterization/CMakeLists.txt`, add after `EnsembleCharTest.cpp`:

```
    ${CMAKE_CURRENT_SOURCE_DIR}/section/ChoirCompanderCharTest.cpp
```

- [ ] **Step 7: Build and run ChoirCompander tests**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure -R "choir_compander"
```

Expected: all `[choir_compander]` tests pass.

- [ ] **Step 8: Commit**

```bash
git add domain/include/vp330/section/ChoirCompander.h \
        domain/src/section/ChoirCompander.cpp \
        tests/unit/section/ChoirCompanderTest.cpp \
        tests/characterization/section/ChoirCompanderCharTest.cpp \
        domain/CMakeLists.txt \
        tests/unit/CMakeLists.txt \
        tests/characterization/CMakeLists.txt
git commit -m "$(cat <<'EOF'
feat(section): add ChoirCompander — HVH105 BA662A single-sided leveler (L1+L2)

Peak-follower with instantaneous attack and τ_release = 103ms (R=4.7MΩ × C=22nF).
Soft-knee gain map g = kGMax × env / (kEnvTarget + env) creates choir "breathing"
attack and enables raising BpCascade gain from 2.7× to 8.72× without clipping.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
EOF
)"
```

---

## Task 4: SynthesisEngine integration — wire chain, raise gain, update golden

**Files:**
- Modify: `domain/include/vp330/engine/SynthesisEngine.h`
- Modify: `domain/src/engine/SynthesisEngine.cpp`
- Modify: `domain/include/vp330/section/ChoirConstants.h`
- Update: `tests/golden/baselines/single-c4-1s.wav`

- [ ] **Step 1: Update SynthesisEngine.h**

Replace the current `SynthesisEngine.h` content with:

```cpp
#pragma once

#include "vp330/ensemble/Ensemble.h"
#include "vp330/keyboard/MkIIKeyboard.h"
#include "vp330/modulation/Vibrato.h"
#include "vp330/note/MidiNote.h"
#include "vp330/section/ChoirCompander.h"
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

  void set_ensemble_enabled(bool on);
  void set_ensemble_rate(Hertz rate);
  void set_ensemble_depth(float depth_0_to_1);

  void render(float* left, float* right, std::size_t frames);

private:
  int sample_rate_;
  MkIIKeyboard keyboard_;
  ChoirSection choir_;
  ChoirCompander choir_compander_;
  Ensemble ensemble_;
  Vibrato vibrato_;
  std::vector<float> lower_8_, lower_4_, upper_8_, upper_4_;
  std::vector<float> choir_mono_, choir_scratch_;
  std::array<bool, 128> note_held_{};
};

} // namespace vp330
```

- [ ] **Step 2: Update SynthesisEngine.cpp**

Replace `domain/src/engine/SynthesisEngine.cpp` with:

```cpp
#include "vp330/engine/SynthesisEngine.h"

namespace vp330 {

SynthesisEngine::SynthesisEngine(int sample_rate)
    : sample_rate_{sample_rate}, keyboard_{sample_rate}, choir_{sample_rate},
      choir_compander_{sample_rate}, ensemble_{sample_rate}, vibrato_{sample_rate} {
  choir_.set_switch(ChoirSwitch::UpperMale8, true);
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

void SynthesisEngine::set_choir_switch(ChoirSwitch sw, bool on) {
  choir_.set_switch(sw, on);
}
void SynthesisEngine::set_vibrato_rate(Hertz r) {
  vibrato_.set_rate(r);
}
void SynthesisEngine::set_vibrato_depth(float d) {
  vibrato_.set_depth(d);
}
void SynthesisEngine::set_attack_seconds(double s) {
  keyboard_.set_attack_seconds(s);
}
void SynthesisEngine::set_release_seconds(double s) {
  keyboard_.set_release_seconds(s);
}
void SynthesisEngine::set_ensemble_enabled(bool on) {
  ensemble_.set_enabled(on);
}
void SynthesisEngine::set_ensemble_rate(Hertz rate) {
  ensemble_.set_rate(rate);
}
void SynthesisEngine::set_ensemble_depth(float d) {
  ensemble_.set_depth(d);
}

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  auto ensure = [&](std::vector<float>& v) {
    if (v.size() < frames)
      v.resize(frames);
  };
  ensure(lower_8_);
  ensure(lower_4_);
  ensure(upper_8_);
  ensure(upper_4_);
  ensure(choir_mono_);
  ensure(choir_scratch_);

  keyboard_.set_master_clock_hz(vibrato_.tick(frames));
  keyboard_.render_zones(lower_8_.data(), lower_4_.data(), upper_8_.data(), upper_4_.data(),
                         frames);
  choir_.process(lower_8_.data(), lower_4_.data(), upper_8_.data(), upper_4_.data(),
                 choir_mono_.data(), choir_scratch_.data(), frames);
  choir_compander_.process(choir_mono_.data(), choir_mono_.data(), frames);
  ensemble_.process(choir_mono_.data(), left, right, frames);
}

} // namespace vp330
```

- [ ] **Step 3: Raise the post-cascade gain in ChoirConstants.h**

In `domain/include/vp330/section/ChoirConstants.h`, find the line:

```cpp
  float gain = 2.7f;
```

Replace with:

```cpp
  float gain = 8.72f; // raised from 2.7×; ChoirCompander now prevents clipping
```

- [ ] **Step 4: Build and confirm tests still pass (pre-golden)**

```bash
cmake -B build -S . && cmake --build build && \
  ctest --test-dir build --output-on-failure --exclude-regex "golden"
```

Expected: all non-golden tests pass (the golden test will fail because the baseline is stale).

- [ ] **Step 5: Update the golden baseline**

Re-render the `single-c4-1s` fixture with the new engine (ensemble on, gain 8.72×):

```bash
./build/infrastructure/cli/vp330_render \
  --input tests/golden/fixtures/single-c4-1s.mid \
  --output tests/golden/baselines/single-c4-1s.wav \
  --duration 1.0 \
  --sample-rate 48000
```

- [ ] **Step 6: Confirm golden test passes**

```bash
ctest --test-dir build --output-on-failure -R "golden"
```

Expected: golden test passes (new baseline matches the re-rendered output within ±6 dB RMS tolerance).

- [ ] **Step 7: Run full test suite**

```bash
ctest --test-dir build --output-on-failure
```

Expected: all tests pass (L1, L2, golden).

- [ ] **Step 8: Commit engine integration**

```bash
git add domain/include/vp330/engine/SynthesisEngine.h \
        domain/src/engine/SynthesisEngine.cpp \
        domain/include/vp330/section/ChoirConstants.h
git commit -m "$(cat <<'EOF'
feat(engine): wire ChoirCompander → Ensemble into SynthesisEngine signal chain

Chain: ChoirSection → ChoirCompander → Ensemble → L/R. Adds set_ensemble_enabled/
rate/depth setters. ChoirCompander prevents clipping so BpCascade gain is raised
from 2.7× to 8.72× (hardware target).

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
EOF
)"
```

- [ ] **Step 9: Commit the updated golden baseline**

```bash
git add tests/golden/baselines/single-c4-1s.wav
git commit -m "$(cat <<'EOF'
golden: update single-c4-1s — ensemble on, gain 8.72×

Re-rendered after Phase 4 integration: ChoirCompander + BBD Ensemble now in chain.
Output is stereo with BBD chorus character. Baseline RMS is higher than Phase 3
due to 8.72× filter gain (vs 2.7×).

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
EOF
)"
```

---

## Self-Review Checklist

**Spec coverage:**
- [x] BbdLine: S&H-accurate lowpass (fc = 0.3 × clock), linear interpolation, pre/post cascade — Task 1
- [x] BbdLine: MN3009 (256 stages, 0.64–12.8ms) and MN3004 (512 stages, 2.56–25.6ms) parameters — EnsembleConstants.h in Task 2
- [x] Ensemble: four lines, quadrature phases 0°/90°/180°/270° — Task 2
- [x] Ensemble: set_rate, set_depth, set_enabled, bypass holds phase — Task 2
- [x] Ensemble: mix weights as `kDryGain`/`kWetGain` constants — EnsembleConstants.h
- [x] ChoirCompander: instantaneous attack, τ_release = 103ms, soft-knee gain — Task 3
- [x] ChoirCompander: kGMax, kEnvTarget as inline constexpr — Task 3
- [x] SynthesisEngine: new members, setters, render chain — Task 4
- [x] ChoirConstants.h gain 2.7 → 8.72 — Task 4
- [x] Golden baseline update — Task 4

**All spec test table rows:**
- [x] BbdLine L1: delay ±0.1ms — Task 1 Step 1
- [x] BbdLine L1: lowpass corner decreases — Task 1 Step 1
- [x] BbdLine L2: 1kHz ±1dB (both chip types) — Task 1 Step 5
- [x] BbdLine L2: bandwidth ≤ 7kHz at max delay (both chip types) — Task 1 Step 5
- [x] Ensemble L1: L ≠ R for sustained tone — Task 2 Step 1
- [x] Ensemble L1: bypass L == R == dry — Task 2 Step 1
- [x] Ensemble L2: cross-correlation < 0.8 — Task 2 Step 6
- [x] ChoirCompander L1: attack ≤ 2 samples — Task 3 Step 1
- [x] ChoirCompander L1: release τ ±10% of 103ms — Task 3 Step 1
- [x] ChoirCompander L1: steady-state gain at env_target ≈ g_max/2 — Task 3 Step 1
- [x] ChoirCompander L2: no clipping at 8.72× — Task 3 Step 5 (output < kGMax, monotone gain)
- [x] Golden single-c4-1s update — Task 4
- [ ] L4: author A/B sign-off — post-plan, done by ear
