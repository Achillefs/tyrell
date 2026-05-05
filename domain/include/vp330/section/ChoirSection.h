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
  [[nodiscard]] bool get_switch(ChoirSwitch sw) const;

  void process(const float* lower, const float* upper,
               float* left, float* right, std::size_t frames);

private:
  ChoirFilterBank lower_bank_, upper_bank_;
  std::array<bool, 4> switches_{}; // all off by default
};

} // namespace vp330
