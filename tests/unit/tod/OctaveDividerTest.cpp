#include "vp330/tod/OctaveDivider.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

using vp330::Hertz;
using vp330::OctaveDivider;

TEST_CASE("OctaveDivider: octave 0 passes input frequency through", "[octave-divider]") {
  OctaveDivider div{Hertz{2000.0}, /*sample_rate=*/48000};
  std::vector<float> buf(48000);
  div.render(/*octave_down=*/0, buf.data(), buf.size());

  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i)
    if ((buf[i - 1] >= 0.0f) != (buf[i] >= 0.0f)) ++crossings;
  REQUIRE(crossings == 4000); // 2000 Hz square → 4000 sign changes/sec
}

TEST_CASE("OctaveDivider: each octave-down halves the frequency", "[octave-divider]") {
  OctaveDivider div{Hertz{2000.0}, 48000};
  std::vector<float> buf(48000);

  for (int k : {1, 2, 3, 4}) {
    std::ranges::fill(buf, 0.0f);
    div.render(k, buf.data(), buf.size());
    std::size_t crossings = 0;
    for (std::size_t i = 1; i < buf.size(); ++i)
      if ((buf[i - 1] >= 0.0f) != (buf[i] >= 0.0f)) ++crossings;
    const std::size_t expected = static_cast<std::size_t>(2000.0 / (1 << k) * 2.0);
    REQUIRE(crossings == expected);
  }
}

TEST_CASE("OctaveDivider: per-octave phase counters are independent", "[octave-divider]") {
  // Aliasing all octaves to one phase counter would corrupt continuity when
  // calls interleave: rendering k=1, then k=2, then k=1 again would leave
  // k=1 with the wrong counter value. Verify the second k=1 chunk continues
  // bit-identically to a one-shot k=1 render of the combined length.
  OctaveDivider interleaved{Hertz{2000.0}, 48000};
  std::vector<float> first(1024), middle(1024), last(1024);
  interleaved.render(/*octave_down=*/1, first.data(), first.size());
  interleaved.render(/*octave_down=*/2, middle.data(), middle.size()); // perturbs k=2 only
  interleaved.render(/*octave_down=*/1, last.data(), last.size());

  OctaveDivider one_shot{Hertz{2000.0}, 48000};
  std::vector<float> reference(2048);
  one_shot.render(1, reference.data(), reference.size());

  for (std::size_t i = 0; i < 1024; ++i) {
    REQUIRE(first[i] == reference[i]);
    REQUIRE(last[i] == reference[i + 1024]);
  }
}

TEST_CASE("OctaveDivider: set_input_frequency changes output zero-crossing rate",
          "[octave-divider][L1]") {
  OctaveDivider div{Hertz{2000.0}, 48000};
  div.set_input_frequency(Hertz{4000.0});
  std::vector<float> buf(48000);
  div.render(/*octave_down=*/0, buf.data(), buf.size());

  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i) {
    if ((buf[i - 1] >= 0.f) != (buf[i] >= 0.f)) {
      ++crossings;
    }
  }
  REQUIRE(crossings == 8000); // 4000 Hz × 2 crossings/cycle × 1 s
}

TEST_CASE("OctaveDivider: each octave maintains continuous phase across calls",
          "[octave-divider]") {
  OctaveDivider div{Hertz{2000.0}, 48000};
  std::vector<float> a(1024), b(1024);
  div.render(0, a.data(), a.size());
  div.render(0, b.data(), b.size());

  OctaveDivider one_shot{Hertz{2000.0}, 48000};
  std::vector<float> full(2048);
  one_shot.render(0, full.data(), full.size());
  for (std::size_t i = 0; i < 1024; ++i) {
    REQUIRE(full[i] == a[i]);
    REQUIRE(full[i + 1024] == b[i]);
  }
}
