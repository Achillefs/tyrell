#include "vp330/modulation/Vibrato.h"
#include "vp330/section/ChoirConstants.h"
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
  for (int i = 0; i < 100; ++i) {
    REQUIRE(v.tick(256).value() ==
            Catch::Approx(vp330::mkii::kMasterClockHz.value()).margin(1.0));
  }
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
