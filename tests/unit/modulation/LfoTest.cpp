#include "vp330/modulation/Lfo.h"

#include "vp330/values/Hertz.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using vp330::Hertz;
using vp330::Lfo;

TEST_CASE("Lfo: zero depth produces all-zero output", "[lfo][L1]") {
  Lfo lfo{48000};
  lfo.set_rate(Hertz{5.0});
  lfo.set_depth(0.0f);
  for (int i = 0; i < 200; ++i) {
    REQUIRE(lfo.tick() == 0.0f);
  }
}

TEST_CASE("Lfo: output bounded to [-1, 1] at unity depth", "[lfo][L1]") {
  Lfo lfo{48000};
  lfo.set_rate(Hertz{440.0});
  lfo.set_depth(1.0f);
  for (int i = 0; i < 48000; ++i) {
    const float s = lfo.tick();
    REQUIRE(s >= -1.0f);
    REQUIRE(s <= 1.0f);
  }
}

TEST_CASE("Lfo: full cycle in sr/rate ticks", "[lfo][L1]") {
  // 8 Hz at 48 kHz → 6000 samples per cycle.
  Lfo lfo{48000};
  lfo.set_rate(Hertz{8.0});
  lfo.set_depth(1.0f);
  const float first = lfo.tick();
  for (int i = 1; i < 6000; ++i) {
    lfo.tick();
  }
  const float after_cycle = lfo.tick(); // sample 6000 ≈ sample 0
  REQUIRE(after_cycle == Catch::Approx(first).margin(0.01f));
}

TEST_CASE("Lfo: advance(N) produces the same phase as N tick()s", "[lfo][L1]") {
  Lfo a{48000};
  Lfo b{48000};
  a.set_rate(Hertz{5.0});
  b.set_rate(Hertz{5.0});
  a.set_depth(1.0f);
  b.set_depth(1.0f);
  for (int i = 0; i < 256; ++i) {
    a.tick();
  }
  b.advance(256);
  REQUIRE(b.value() == Catch::Approx(a.value()).margin(1e-5f));
}
