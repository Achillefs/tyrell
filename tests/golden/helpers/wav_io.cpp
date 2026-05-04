#include "wav_io.h"

#include <sndfile.h>

#include <cmath>
#include <memory>
#include <stdexcept>

namespace vp330::test {

/**
 * @brief Load a WAV file into a Wav structure.
 *
 * Reads the WAV file at the given path and returns a Wav populated with
 * sample_rate, channels, frames, and interleaved float samples.
 *
 * @param path Path to the WAV file to load.
 * @return Wav Populated Wav object containing sample metadata and samples.
 * @throws std::runtime_error If the file cannot be opened or if the number of frames
 *                            read is less than expected.
 */
Wav load_wav(const std::string& path) {
  SF_INFO info{};
  auto sf = std::unique_ptr<SNDFILE, decltype(&sf_close)>(sf_open(path.c_str(), SFM_READ, &info),
                                                          &sf_close);
  if (!sf) {
    throw std::runtime_error("load_wav: sf_open failed for " + path + ": " + sf_strerror(nullptr));
  }

  Wav out;
  out.sample_rate = info.samplerate;
  out.channels = info.channels;
  out.frames = static_cast<std::size_t>(info.frames);
  out.samples.resize(out.frames * static_cast<std::size_t>(out.channels));

  const auto read = sf_readf_float(sf.get(), out.samples.data(), info.frames);
  if (read != info.frames) {
    throw std::runtime_error("load_wav: short read on " + path);
  }
  return out;
}

/**
 * @brief Compute the maximum absolute sample value in a WAV buffer.
 *
 * Scans all float samples in the provided Wav and returns the largest absolute
 * magnitude found. If the sample buffer is empty, returns 0.0f.
 *
 * @param wav Wav object whose interleaved float samples will be examined.
 * @return float Maximum absolute sample value, or 0.0f if there are no samples.
 */
float max_abs_sample(const Wav& wav) {
  float peak = 0.0f;
  for (float s : wav.samples) {
    const float a = std::fabs(s);
    if (a > peak) peak = a;
  }
  return peak;
}

} // namespace vp330::test
