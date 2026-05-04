#pragma once

namespace vp330 {

class Clock {
public:
  virtual ~Clock() = default;
  Clock(const Clock&) = delete;
  Clock& operator=(const Clock&) = delete;
  Clock(Clock&&) = delete;
  Clock& operator=(Clock&&) = delete;

  virtual int sample_rate() const = 0;
};

} // namespace vp330
