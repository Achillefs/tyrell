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
