#include "vp330/ensemble/BbdLine.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using vp330::BbdLine;

TEST_CASE("BbdLine L1: delay accurate to ±0.1ms", "[bbd_line][L1]") {
  // At 2ms, clock_hz = 256/0.002 = 128 kHz, fc = 38.4 kHz > Nyquist
  // → lp_coeff_ = 0 (bypass), output is a clean 96-sample delayed copy.
  BbdLine bbd{48000, 256, 0.0128f};
  bbd.set_delay(0.002f); // 96 samples exactly

  const int n = 256;
  std::vector<float> in(n, 0.f), out(n);
  in[0] = 1.f;
  bbd.process(in.data(), out.data(), n);

  int peak = 0;
  for (int i = 1; i < n; ++i)
    if (std::fabs(out[i]) > std::fabs(out[peak])) peak = i;

  REQUIRE(std::abs(peak - 96) <= 5);                     // ±0.1ms = ±4.8 samples
  REQUIRE(out[96] == Catch::Approx(1.0f).margin(1e-4f)); // exact with no lowpass
}

TEST_CASE("BbdLine L1: lowpass corner decreases as delay increases", "[bbd_line][L1]") {
  // At 2ms: fc >> Nyquist → no HF attenuation.
  // At max 12.8ms: fc = 6kHz → significant 10kHz attenuation.
  const int fs = 48000;
  const int n = fs;
  std::vector<float> in(n), out_short(n), out_long(n);
  const float k = 2.f * std::numbers::pi_v<float> * 10000.f / static_cast<float>(fs);
  for (int i = 0; i < n; ++i)
    in[i] = std::sin(k * static_cast<float>(i));

  BbdLine bbd_short{fs, 256, 0.0128f};
  bbd_short.set_delay(0.002f);
  bbd_short.process(in.data(), out_short.data(), n);

  BbdLine bbd_long{fs, 256, 0.0128f};
  bbd_long.set_delay(0.0128f);
  bbd_long.process(in.data(), out_long.data(), n);

  auto rms2 = [&](const std::vector<float>& v) {
    float s = 0;
    for (int i = n / 2; i < n; ++i)
      s += v[i] * v[i];
    return std::sqrt(2.f * s / static_cast<float>(n));
  };

  // At least 6 dB more HF energy at short (unfiltered) delay
  REQUIRE(rms2(out_short) > rms2(out_long) * 2.f);
}
