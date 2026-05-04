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

constexpr Hertz operator+(Hertz a, Hertz b) {
  return Hertz{a.value() + b.value()};
}
constexpr Hertz operator-(Hertz a, Hertz b) {
  return Hertz{a.value() - b.value()};
}
constexpr Hertz operator*(Hertz a, double s) {
  return Hertz{a.value() * s};
}
constexpr Hertz operator*(double s, Hertz a) {
  return Hertz{a.value() * s};
}
constexpr Hertz operator/(Hertz a, double s) {
  return Hertz{a.value() / s};
}

} // namespace vp330
