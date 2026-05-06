# Phase 4 — Ensemble & ChoirCompander Implementation Design

## Goal

Add the VP-330 MkII BBD ensemble (ETH16 board) and the HVH105 single-sided leveler (ChoirCompander) to produce true stereo output with the characteristic "breathing" choir attack and BBD chorus character. This is the v1.0 gate: after this phase the instrument should pass L4 A/B against the author's reference unit.

## Architecture

```text
keyboard.render_zones()
  → choir.process()           [mono; left = right, as before]
  → ChoirCompander            [HVH105 leveler: mono → mono]
  → Ensemble                  [ETH16 BBD: mono → stereo]
  → left / right output
```

Three new domain components: `BbdLine`, `Ensemble`, `ChoirCompander`. `SynthesisEngine` wires them together via a `choir_mono_` scratch buffer. The ETH16 pre/post compander around the BBD chips is not modelled in DSP — it exists solely to hide BBD noise floor, which DSP does not have.

Alongside this, `BpCascadeParams::gain` is raised from 2.7× to 8.72× (the hardware target). The ChoirCompander prevents clipping that made 8.72× unsafe without it.

## Source authority

- VP-330 MkII Service Notes (Oct 25 1980, 2nd Edition), pages 14–15 (ETH16 board) and page 9 (VBH106 LFO waveforms).
- `docs/filter-bank-research.md` §5 (HV-330 compander analysis).

---

## Component 1: BbdLine

**File:** `domain/include/vp330/ensemble/BbdLine.h` / `domain/src/ensemble/BbdLine.cpp`

A single BBD chip modelled as a variable fractional delay with a 1-pole tracking lowpass applied both before and after the delay. The lowpass corner tracks the instantaneous BBD clock frequency:

```text
clock_hz  = n_stages / delay_seconds
fc_lowpass = 0.3 × clock_hz
```

This is the S&H-accurate (Approach B) property: longer delay → lower clock → lower bandwidth → darker timbre. At the minimum delays used here the corner exceeds Nyquist and is effectively absent; it only audibly engages towards the longer end of the LFO sweep.

**Parameters (from service manual page 14):**

| Chip model | Stages | Delay range | Centre delay |
|---|---|---|---|
| MN3009-style | 256 | 0.64 – 12.8 ms | ~6 ms |
| MN3004-style | 512 | 2.56 – 25.6 ms | ~20 ms |

**API:**

```cpp
class BbdLine {
public:
  // n_stages: 256 (MN3009) or 512 (MN3004)
  BbdLine(int sample_rate, int n_stages, float max_delay_seconds);
  // Called per-block by Ensemble before process().
  void set_delay(float seconds);
  void process(const float* in, float* out, std::size_t frames);
private:
  // Circular buffer, linear interpolation, tracking 1-pole IIR pre/post.
};
```

Fractional delay uses linear interpolation. The tracking lowpass already limits HF; the mild linear-interpolation rolloff is inaudible beneath it.

---

## Component 2: Ensemble

**File:** `domain/include/vp330/ensemble/Ensemble.h` / `domain/src/ensemble/Ensemble.cpp`

Four `BbdLine` instances driven by a single quadrature phase accumulator at the LFO rate. Lines 0 and 1 use MN3009 parameters (short); lines 2 and 3 use MN3004 parameters (long).

**Delay modulation:**

```text
φ     = 2π × rate_hz × t   (phase accumulator, per sample)
delay[0] = centre_short + depth × sin(φ + 0°)    → CH1
delay[1] = centre_short + depth × sin(φ + 90°)   → CH2
delay[2] = centre_long  + depth × sin(φ + 180°)  → CH1
delay[3] = centre_long  + depth × sin(φ + 270°)  → CH2
```

**Stereo mix:**

```text
Left  = dry × 0.5 + line0 × 0.25 + line2 × 0.25
Right = dry × 0.5 + line1 × 0.25 + line3 × 0.25
```

Mix gains are L4 calibration targets, not invariants. They are exposed as `kEnsembleMixWeights` constants in `EnsembleConstants.h` so they can be adjusted without code changes.

**LFO range (service manual page 9):** 0.45–1.4 Hz (T = 2.2 s to 0.7 s). Default centre: 0.7 Hz. Default depth: ±1.5 ms for short lines, ±3 ms for long lines (calibrated at L4).

**Bypass:** when `set_enabled(false)`, `process()` copies `in` to both `left` and `right` without touching the BBD lines or LFO phase (the LFO does not accumulate while bypassed — no phase jump on re-enable because the phase holds).

**API:**

```cpp
class Ensemble {
public:
  explicit Ensemble(int sample_rate);
  void set_rate(Hertz rate);
  void set_depth(float depth_0_to_1);  // scales default depths
  void set_enabled(bool on);
  // mono in → stereo out (overwrites left/right)
  void process(const float* in, float* left, float* right, std::size_t frames);
private:
  std::array<BbdLine, 4> lines_; // [0,1] = MN3009-style, [2,3] = MN3004-style
  float phase_ = 0.f;
  float rate_hz_ = 0.7f;
  float depth_short_ = 1.5e-3f;  // seconds
  float depth_long_  = 3.0e-3f;  // seconds
  bool enabled_ = true;
};
```

The existing `Lfo` class is not reused here. Ensemble drives its own phase accumulator so all four lines remain phase-coherent within each block.

---

## Component 3: ChoirCompander

**File:** `domain/include/vp330/section/ChoirCompander.h` / `domain/src/section/ChoirCompander.cpp`

Models the HVH105 BA662A single-sided leveler. This is not a noise-reduction compander; it is a peak-follower-driven gain stage whose expanding action on note attacks creates the choir "breathing" character. It also enables raising `BpCascadeParams::gain` to 8.72× by preventing post-filter clipping.

**DSP model (from `docs/filter-bank-research.md` §5, confirmed against service manual):**

```text
r[n]   = |x[n]|
env[n] = r[n] >= env[n-1] ? r[n] : α × env[n-1]
           (instantaneous attack; τ_release = 103 ms)
g[n]   = g_max × env[n] / (env_target + env[n])
y[n]   = g[n] × x[n]
```

`α = exp(−1 / (0.103 × sample_rate))`. `g_max` and `env_target` are exposed as `inline constexpr` constants at the top of `ChoirCompander.h` (default: `g_max = 4.0`, `env_target = 0.1`) for L4 calibration.

The 103 ms release constant comes directly from the service notes: R = 4.7 MΩ, C = 22 nF, τ = R×C = 103 ms.

**API:**

```cpp
class ChoirCompander {
public:
  explicit ChoirCompander(int sample_rate);
  // in and out may alias (in-place safe).
  void process(const float* in, float* out, std::size_t frames);
private:
  double envelope_ = 0.0;
  double alpha_release_;
};
```

---

## SynthesisEngine integration

**Modified files:** `domain/include/vp330/engine/SynthesisEngine.h`, `domain/src/engine/SynthesisEngine.cpp`

New members:

```cpp
ChoirCompander choir_compander_;
Ensemble ensemble_;
std::vector<float> choir_mono_, choir_scratch_;
```

Updated `render()`:

```cpp
choir_.process(lower_8_, lower_4_, upper_8_, upper_4_,
               choir_mono_.data(), choir_scratch_.data(), frames);
choir_compander_.process(choir_mono_.data(), choir_mono_.data(), frames);
ensemble_.process(choir_mono_.data(), left, right, frames);
```

New setters: `set_ensemble_enabled(bool)`, `set_ensemble_rate(Hertz)`, `set_ensemble_depth(float)`.

---

## ChoirConstants update

In `domain/include/vp330/section/ChoirConstants.h`, raise the post-cascade gain scalar:

```cpp
float gain = 8.72f;  // was 2.7f; compander now prevents clipping
```

---

## File structure

```text
domain/include/vp330/ensemble/
  BbdLine.h
  Ensemble.h
  EnsembleConstants.h     // kCentreDelayShort, kCentreDelayLong, kEnsembleMixWeights, etc.

domain/src/ensemble/
  BbdLine.cpp
  Ensemble.cpp

domain/include/vp330/section/
  ChoirCompander.h        // includes inline constexpr g_max, env_target

domain/src/section/
  ChoirCompander.cpp
```

---

## Testing

| Layer | Target | Criterion |
|---|---|---|
| L1 | `BbdLine` | Delay accurate to ±0.1 ms; bypass output == input |
| L1 | `BbdLine` | Lowpass corner decreases as delay increases (freq response check at 2 delays) |
| L2 | `BbdLine` | 1 kHz fundamental preserved ±1 dB across full delay range |
| L2 | `BbdLine` | Bandwidth at max delay ≤ 7 kHz (−3 dB from flat, both chip types) |
| L1 | `Ensemble` | With ensemble on: L ≠ R for a sustained tone |
| L1 | `Ensemble` | Bypass: L == R == dry (bit-exact) |
| L2 | `Ensemble` | Stereo cross-correlation < 0.8 (decorrelated) at centre rate/depth |
| L2 | `Ensemble` | LFO modulation depth matches `depth_short_` / `depth_long_` within ±5 % |
| L1 | `ChoirCompander` | Envelope attack ≤ 2 samples for a step input |
| L1 | `ChoirCompander` | Release τ within ±10 % of 103 ms |
| L1 | `ChoirCompander` | Steady-state gain at `env_target` ≈ g_max / 2 |
| L2 | `ChoirCompander` | No clipping at 8.72× filter gain for a full-chord input |
| Golden | full engine | Update `single-c4-1s` baseline with ensemble on, gain 8.72× |
| L4 | full engine | A/B against author's VP-330 MkII captures — author signs off |

---

## What is not in scope

- Strings section (Phase 5+): `Ensemble` accepts a single mono input for now. When Strings are added, their signal is summed into the choir mono bus before `Ensemble::process`.
- BBD noise floor modelling: the ETH16 pre/post compander is omitted — it is transparent by design.
- GUI controls: all parameters exposed as setters on `SynthesisEngine`; wired to JUCE in Phase 5.
- CLAP format: Phase 5.
