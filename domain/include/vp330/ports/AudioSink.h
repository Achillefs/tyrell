#pragma once

#include <cstddef>

namespace vp330 {

class AudioSink {
public:
  virtual ~AudioSink() = default;
  AudioSink(const AudioSink&) = delete;
  AudioSink& operator=(const AudioSink&) = delete;
  AudioSink(AudioSink&&) = delete;
  AudioSink& operator=(AudioSink&&) = delete;

  virtual void write(const float* left, const float* right, std::size_t frames) = 0;
};

} // namespace vp330
