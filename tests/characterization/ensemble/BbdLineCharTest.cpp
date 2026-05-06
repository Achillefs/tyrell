#include "vp330/ensemble/BbdLine.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using vp330::BbdLine;

namespace {

std::vector<float> make_sine(float freq, int n, int fs) {
  std::vector<float> v(n);
  const float k = 2.f * std::numbers::pi_v<float> * freq / static_cast<float>(fs);
  for (int i = 0; i < n; ++i)
    v[i] = std::sin(k * static_cast<float>(i));
  return v;
}

float rms_second_half(BbdLine& bbd, const std::vector<float>& in) {
  const int n = static_cast<int>(in.size());
  std::vector<float> out(n);
  bbd.process(in.data(), out.data(), n);
  float s = 0;
  for (int i = n / 2; i < n; ++i)
    s += out[i] * out[i];
  return std::sqrt(2.f * s / static_cast<float>(n));
}

} // namespace

TEST_CASE("BbdLine L2: 1kHz fundamental preserved ±1dB across full delay range (MN3009)",
          "[bbd_line][L2]") {
  const int fs = 48000, n = fs;
  auto sine_1k = make_sine(1000.f, n, fs);

  BbdLine bbd_min{fs, 256, 0.0128f};
  bbd_min.set_delay(0.00064f); // MN3009 min: 0.64ms, fc >> Nyquist → no filter
  const float rms_min = rms_second_half(bbd_min, sine_1k);

  BbdLine bbd_max{fs, 256, 0.0128f};
  bbd_max.set_delay(0.0128f); // MN3009 max: 12.8ms, fc = 6kHz
  const float rms_max = rms_second_half(bbd_max, sine_1k);

  // ±1 dB = ratio in [0.891, 1.122]
  REQUIRE(rms_max > rms_min * 0.891f);
  REQUIRE(rms_max < rms_min * 1.122f);
}

TEST_CASE("BbdLine L2: 1kHz fundamental preserved ±1dB across full delay range (MN3004)",
          "[bbd_line][L2]") {
  const int fs = 48000, n = fs;
  auto sine_1k = make_sine(1000.f, n, fs);

  BbdLine bbd_min{fs, 512, 0.0256f};
  bbd_min.set_delay(0.00256f); // MN3004 min: 2.56ms, fc >> Nyquist
  const float rms_min = rms_second_half(bbd_min, sine_1k);

  BbdLine bbd_max{fs, 512, 0.0256f};
  bbd_max.set_delay(0.0256f); // MN3004 max: 25.6ms, fc = 6kHz
  const float rms_max = rms_second_half(bbd_max, sine_1k);

  REQUIRE(rms_max > rms_min * 0.891f);
  REQUIRE(rms_max < rms_min * 1.122f);
}

TEST_CASE("BbdLine L2: bandwidth at max delay ≤ 7kHz, −3dB (MN3009)", "[bbd_line][L2]") {
  const int fs = 48000, n = fs;

  BbdLine bbd_7k{fs, 256, 0.0128f};
  bbd_7k.set_delay(0.0128f);
  const float rms_7k = rms_second_half(bbd_7k, make_sine(7000.f, n, fs));

  BbdLine bbd_ref{fs, 256, 0.0128f};
  bbd_ref.set_delay(0.0128f);
  const float rms_ref = rms_second_half(bbd_ref, make_sine(100.f, n, fs));

  // 7kHz should be below −3dB (0.707×) relative to passband at max delay
  REQUIRE(rms_7k < rms_ref * 0.707f);
}

TEST_CASE("BbdLine L2: bandwidth at max delay ≤ 7kHz, −3dB (MN3004)", "[bbd_line][L2]") {
  const int fs = 48000, n = fs;

  BbdLine bbd_7k{fs, 512, 0.0256f};
  bbd_7k.set_delay(0.0256f);
  const float rms_7k = rms_second_half(bbd_7k, make_sine(7000.f, n, fs));

  BbdLine bbd_ref{fs, 512, 0.0256f};
  bbd_ref.set_delay(0.0256f);
  const float rms_ref = rms_second_half(bbd_ref, make_sine(100.f, n, fs));

  REQUIRE(rms_7k < rms_ref * 0.707f);
}
