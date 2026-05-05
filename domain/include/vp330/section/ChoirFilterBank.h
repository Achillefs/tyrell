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
  [[nodiscard]] const float* output(int filter_idx) const;
  void reset();

private:
  std::array<BpCascade, 7> filters_;
  std::array<std::vector<float>, 7> scratch_;
};

} // namespace vp330
