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
