#include "vp330/tod/TopOctaveDivider.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <vector>

using vp330::Hertz;
using vp330::TopOctaveDivider;

TEST_CASE("TopOctaveDivider: pitch-class frequency matches divider math for every index", "[tod]") {
  std::array<int, 12> ratios{250, 200, 150, 125, 100, 80, 60, 50, 40, 30, 25, 20};
  TopOctaveDivider tod{Hertz{1.0e6}, ratios, /*sample_rate=*/48000};
  for (int p = 0; p < 12; ++p) {
    const double expected = 1.0e6 / (2.0 * static_cast<double>(ratios[p]));
    REQUIRE(tod.pitch_class_frequency(p).value() == Catch::Approx(expected).margin(1e-9));
  }
}

TEST_CASE("TopOctaveDivider: produces 50%-duty squares at the divided frequencies", "[tod]") {
  std::array<int, 12> ratios{};
  ratios.fill(50);
  TopOctaveDivider tod{Hertz{12000.0}, ratios, /*sample_rate=*/48000};

  std::vector<float> buf(48000);
  tod.render_pitch_class(0, buf.data(), buf.size());

  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i) {
    const bool prev_pos = buf[i - 1] >= 0.0f;
    const bool curr_pos = buf[i] >= 0.0f;
    if (prev_pos != curr_pos) ++crossings;
  }
  // 12000 / (2*50) = 120 Hz; 120 cycles × 2 sign-changes/cycle × 1 s = 240.
  REQUIRE(crossings == 240);
}

TEST_CASE("TopOctaveDivider: amplitude is ±1.0 (50% duty)", "[tod]") {
  std::array<int, 12> ratios{};
  ratios.fill(100);
  TopOctaveDivider tod{Hertz{48000.0}, ratios, /*sample_rate=*/48000};

  std::vector<float> buf(1024);
  tod.render_pitch_class(3, buf.data(), buf.size());

  bool saw_pos = false, saw_neg = false;
  for (auto s : buf) {
    REQUIRE((s == 1.0f || s == -1.0f));
    if (s > 0) saw_pos = true;
    if (s < 0) saw_neg = true;
  }
  REQUIRE(saw_pos);
  REQUIRE(saw_neg);
}

TEST_CASE("TopOctaveDivider: continuous phase across render calls", "[tod]") {
  std::array<int, 12> ratios{};
  ratios.fill(100);
  TopOctaveDivider tod_a{Hertz{48000.0}, ratios, 48000};
  TopOctaveDivider tod_b{Hertz{48000.0}, ratios, 48000};

  std::vector<float> one_shot(2048);
  tod_a.render_pitch_class(0, one_shot.data(), one_shot.size());

  std::vector<float> two_shot(2048);
  tod_b.render_pitch_class(0, two_shot.data(), 1024);
  tod_b.render_pitch_class(0, two_shot.data() + 1024, 1024);

  for (std::size_t i = 0; i < one_shot.size(); ++i)
    REQUIRE(one_shot[i] == two_shot[i]);
}
