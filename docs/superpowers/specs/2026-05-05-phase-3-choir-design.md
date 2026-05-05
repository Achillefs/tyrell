# Phase 3 — Choir Section Design

**Date:** 2026-05-05
**Status:** Approved — ready for implementation planning
**Author:** Achilles Charmpilas

---

## 1. Goal

Implement the VP-330 MkII Choir section: keyboard-split paraphonic buses, 7-band filter bank per zone, 4 independently switchable voice weightings, and section-level vibrato via LFO → TOD master clock.

This phase delivers the first recognisable VP-330 sound. Ensemble (Phase 4) is not in scope here.

---

## 2. Hardware Architecture (reference)

The MkII Choir section works as follows:

1. The 49-key keyboard (C2–C6, MIDI 36–84) is split at **C4 (MIDI 60)** into two independent paraphonic zones:
   - **Lower zone**: MIDI 36–59 (C2–B3)
   - **Upper zone**: MIDI 60–84 (C4–C6)
2. Each zone has its own summing amplifier — active KeyGates in that zone are summed into a single mono bus.
3. Each zone bus is fed through **7 fixed bandpass filter chains** (always powered). Each filter chain is a pair of cascaded 2-pole bandpass stages (4th-order total).
4. The front panel has **4 independent switches** — two per zone:
   - Lower: **Lower Male 8'**, **Lower Male 4'**
   - Upper: **Upper Male 8'**, **Upper Female 4'**
5. Each switch, when active, takes a weighted sum of 4 of the 7 filter outputs and routes it to the section output bus. Multiple switches in the same zone sum additively (louder). If no switch is active in a zone, that zone is silent.
6. **Vibrato** is a front-panel LFO that modulates the TOD master VCO clock frequency, affecting pitch for both zones equally.

---

## 3. Signal Flow

```
MIDI → SynthesisEngine
         │
         ├── Vibrato (Lfo → TOD master clock offset each render block)
         │
         ├── MkIIKeyboard
         │     ├── TopOctaveDivider ← modulated master clock
         │     ├── 12x OctaveDividers
         │     └── 49x KeyGates
         │          │
         │          ├── render_zones():
         │          │     lower zone sum (MIDI 36–59)
         │          │     upper zone sum (MIDI 60–84)
         │
         └── ChoirSection
               ├── ChoirFilterBank (lower) — 7 BpCascade chains, always running
               ├── ChoirFilterBank (upper) — 7 BpCascade chains, always running
               └── 4 switch booleans → weighted sums → stereo out (mono-duplicated)
```

`SynthesisEngine` owns two scratch buffers (`lower_zone_`, `upper_zone_`) and coordinates:
1. `vibrato_.tick()` → `keyboard_.tod().set_master_clock_hz(...)`
2. `keyboard_.render_zones(lower_zone_, upper_zone_, frames)`
3. `choir_.process(lower_zone_, upper_zone_, left, right, frames)`

The `Section` abstract base class is **deferred to Phase 7** (YAGNI — no second section yet to define the interface against).

---

## 4. New Domain Components

### 4.1 `BpCascade` (`domain/include/vp330/section/BpCascade.h`)

A pair of cascaded 2nd-order bandpass biquad filters (4th-order total).

- Constructed from `(float f0_1, float q1, float f0_2, float q2, int sample_rate)`.
- Uses the Audio EQ Cookbook **constant-peak-gain BP form** so the gain at each center frequency is 0 dB regardless of Q.
- `void process(const float* in, float* out, std::size_t frames)` — stateful, in-place-safe.
- L1: coefficient correctness and pole-stability at construction. L2: measured gain at center frequency within ±1 dB of 0 dB; −3 dB bandwidth matches `f0/Q ± 10%`.

### 4.2 `ChoirFilterBank` (`domain/include/vp330/section/ChoirFilterBank.h`)

Owns 7 `BpCascade` instances initialised from `ChoirConstants.h`. Always processes all 7 chains.

```cpp
class ChoirFilterBank {
public:
  explicit ChoirFilterBank(int sample_rate);
  void process(const float* in, std::size_t frames);   // fills internal scratch
  const float* output(int filter_idx) const;           // view into scratch[filter_idx]
private:
  std::array<BpCascade, 7> filters_;
  std::array<std::vector<float>, 7> scratch_;          // resized at construction
};
```

L1: voice-to-filter mapping (each voice references exactly 4 filter indices with correct rational weights). L2: per-filter frequency response; per-voice composite spectral peaks.

### 4.3 `ChoirSwitch` (`domain/include/vp330/section/ChoirSwitch.h`)

```cpp
enum class ChoirSwitch { LowerMale8, LowerMale4, UpperMale8, UpperFemale4 };
```

### 4.4 `ChoirSection` (`domain/include/vp330/section/ChoirSection.h`)

```cpp
class ChoirSection {
public:
  explicit ChoirSection(int sample_rate);
  void set_switch(ChoirSwitch sw, bool on);
  void process(const float* lower, const float* upper,
               float* left, float* right, std::size_t frames);
private:
  ChoirFilterBank lower_bank_, upper_bank_;
  std::array<bool, 4> switches_{};  // all off by default
};
```

`SynthesisEngine` defaults `UpperMale8` to on so the instrument produces sound without user interaction.

L2: all switches off → silence; single active switch → spectral peaks at expected center frequencies; both lower switches active → louder than one alone.

### 4.5 `Lfo` (`domain/include/vp330/modulation/Lfo.h`)

Sinusoidal LFO.

```cpp
class Lfo {
public:
  explicit Lfo(int sample_rate);
  void set_rate(Hertz rate);
  void set_depth(float depth_0_to_1);
  float tick();   // returns sample in [-1, 1] scaled by depth
};
```

L1: phase increment math; output bounds; wraparound continuity. L2: frequency accuracy (measured zero-crossing period within ±1% of `1/rate`).

### 4.6 `Vibrato` (`domain/include/vp330/modulation/Vibrato.h`)

Wraps `Lfo`, outputs a `Hertz` value to pass to `TopOctaveDivider::set_master_clock_hz()`.

- `tick()` returns `kMasterClockHz + lfo_.tick() * kMaxClockOffset`.
- `kMaxClockOffset` = 50 Hz placeholder (≈±6¢ at master clock frequency). **Calibrate against capture #4 before Phase 3 closes.**
- At zero depth: always returns `kMasterClockHz`.

L2: with `Lfo` at 5 Hz and moderate depth, the keyboard fundamental oscillates in pitch at 5 Hz; peak deviation within the expected ¢ range.

### 4.7 Updates to existing types

| Type | Change |
|---|---|
| `TopOctaveDivider` | Add `set_master_clock_hz(Hertz)` — updates all 12 divider step sizes immediately, no phase reset. |
| `KeyGate` | Add `set_attack_seconds(double)` / `set_release_seconds(double)` — takes effect on next gate event; does not interrupt an in-progress ramp. |
| `MkIIKeyboard` | Add `render_zones(float* lower, float* upper, std::size_t frames)`. Add `set_attack_seconds` / `set_release_seconds` propagating to all 49 KeyGates. Existing `render()` reimplemented as a sum of both zone buses. |
| `SynthesisEngine` | Owns `ChoirSection` + `Vibrato` + two zone scratch buffers. Exposes `set_choir_switch`, `set_vibrato_rate`, `set_vibrato_depth`, `set_attack_seconds`, `set_release_seconds`. |

---

## 5. Constants (`ChoirConstants.h`)

All PD-derived values stored as `constexpr` in `domain/include/vp330/section/ChoirConstants.h`. Calibration during L4 changes only this file.

```cpp
namespace vp330::mkii {

struct BpCascadeParams { float f0_1, q1, f0_2, q2; };

// Source: PureData circuit analysis by community contributor.
// Calibrate against author's MkII captures at L4; update values here if
// spectral peaks diverge.
inline constexpr std::array<BpCascadeParams, 7> kChoirFilters{{
    {175.f, 6.79f,  216.f,  6.83f},  // F0
    {209.f, 6.79f,  257.f,  6.83f},  // F1
    {559.f, 6.96f,  671.f,  6.83f},  // F2 — present in all 4 voices
    {845.f, 7.00f, 1007.f,  6.83f},  // F3 — present in all 4 voices
    {1202.f, 6.81f, 1473.f, 6.83f},  // F4
    {2532.f, 6.83f, 3098.f, 6.83f},  // F5
    {2974.f, 6.78f, 3660.f, 6.83f},  // F6
}};

struct VoiceWeights { std::array<int,4> filters; std::array<float,4> weights; };

// Array index MUST match ChoirSwitch enum value:
//   0 = LowerMale8, 1 = LowerMale4, 2 = UpperMale8, 3 = UpperFemale4
// ChoirSection::process() indexes kVoiceWeights by static_cast<int>(ChoirSwitch).
inline constexpr std::array<VoiceWeights, 4> kVoiceWeights{{
    {{0,2,3,5}, {0.3125f, 1.f, 1.f, 0.3125f}},        // LowerMale8
    {{1,2,3,5}, {0.125f,  1.f, 1.f, 0.3125f}},         // LowerMale4
    {{1,2,3,5}, {0.1875f, 1.f, 1.f, 0.3125f}},         // UpperMale8
    {{2,3,4,6}, {0.1875f, 1.f, 0.6875f, 0.1875f}},     // UpperFemale4
}};

} // namespace vp330::mkii
```

---

## 6. Test Strategy

### L1 — Unit (`tests/unit/section/`, `tests/unit/modulation/`)

| Test file | Covers |
|---|---|
| `BpCascadeTest.cpp` | Coefficients: gain at center = 0 dB ±0.1 dB; poles inside unit circle; gain at 10× center well below −3 dB. |
| `ChoirFilterBankTest.cpp` | Voice mapping: each voice references exactly 4 filter indices with correct rational weights. |
| `ChoirSectionTest.cpp` | All switches off: `process()` output is all-zeros. |
| `LfoTest.cpp` | Phase increment; output in `[−1, 1]`; full cycle length matches `sr / rate`; no discontinuity at wraparound. |
| `VibratoTest.cpp` | Zero depth → always `kMasterClockHz`; non-zero depth bounded within `[kMasterClockHz − kMaxClockOffset, kMasterClockHz + kMaxClockOffset]`. |
| `TopOctaveDividerTest.cpp` (update) | `set_master_clock_hz()` changes output frequency without phase reset. |
| `KeyGateTest.cpp` (update) | `set_attack_seconds()` takes effect on next gate_on; does not interrupt active ramp. |

### L2 — Characterization (`tests/characterization/section/`, `tests/characterization/modulation/`)

| Test file | Contract |
|---|---|
| `BpCascadeCharTest.cpp` | Swept-sine: peak gain within ±1 dB of 0 dB; −3 dB bandwidth = `f0/Q ± 10%`. |
| `ChoirFilterBankCharTest.cpp` | Per-voice: spectral energy concentrated at the 4 chosen bands; no contribution from inactive filter indices. One test per voice. |
| `ChoirSectionCharTest.cpp` | Single active switch → spectral peaks at expected frequencies; both lower switches active → higher RMS than one alone. |
| `VibratoCharTest.cpp` | Lfo at 5 Hz, moderate depth: keyboard fundamental oscillates at 5 Hz; peak deviation within expected ¢ range. |

### L3 — Golden render update

Update `single-c4-1s` baseline: `SynthesisEngine` now defaults `UpperMale8` on. The new baseline WAV reflects the choir-filtered output. Commit message: `golden: update single-c4-1s — choir section engaged (Upper Male 8')`.

### L4 — Reference comparison (phase boundary)

A/B each switch combination separately against hardware captures (chromatic scale, ensemble off). Calibrate `kChoirFilters` and `kVoiceWeights` in `ChoirConstants.h` if spectral peaks diverge. Author signs off perceptually on all 4 voices before the phase closes.

**Captures needed before L4 close:**
- Each of the 4 switches in isolation (chromatic scale, ensemble off) — if not already in the session notes.
- Capture #4 (single C4, vibrato sweep) for `kMaxClockOffset` calibration.
- Capture #7 (attack/release sweep) for `KeyGate` parameter range calibration.

---

## 7. File Layout

```text
domain/include/vp330/section/
    ChoirConstants.h
    BpCascade.h
    ChoirFilterBank.h
    ChoirSection.h
    ChoirSwitch.h

domain/include/vp330/modulation/
    Lfo.h
    Vibrato.h

domain/src/section/
    BpCascade.cpp
    ChoirFilterBank.cpp
    ChoirSection.cpp

domain/src/modulation/
    Lfo.cpp
    Vibrato.cpp

tests/unit/section/
    BpCascadeTest.cpp
    ChoirFilterBankTest.cpp
    ChoirSectionTest.cpp

tests/unit/modulation/
    LfoTest.cpp
    VibratoTest.cpp

tests/characterization/section/
    BpCascadeCharTest.cpp
    ChoirFilterBankCharTest.cpp
    ChoirSectionCharTest.cpp

tests/characterization/modulation/
    VibratoCharTest.cpp

docs/
    choir-filter-constants.md   ← provenance of PD values; deviation log updated at L4
```

No new third-party dependencies. CMake: add new source files to `vp330_domain`; add new test files to `vp330_tests`.
