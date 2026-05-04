#include "render.h"
#include "wav_io.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

#ifndef GOLDEN_BASELINES_DIR
#error "GOLDEN_BASELINES_DIR must be defined by CMake"
#endif

using namespace vp330::test;

TEST_CASE("walking skeleton: silence renders to silence", "[golden]") {
    const auto wav = render_fixture("silence-1s.mid", 48000, 1.0);
    const auto baseline = load_wav(std::string(GOLDEN_BASELINES_DIR) + "/silence-1s.wav");

    REQUIRE(wav.frames == baseline.frames);
    REQUIRE(wav.channels == baseline.channels);
    REQUIRE(wav.sample_rate == baseline.sample_rate);
    REQUIRE(max_abs_sample(wav) < 1e-6f);
    REQUIRE(max_abs_sample(baseline) < 1e-6f);
}
