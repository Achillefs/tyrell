#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace vp330::test {

struct Wav {
    int sample_rate = 0;
    int channels = 0;
    std::size_t frames = 0;
    // Interleaved float samples in [-1, 1].
    std::vector<float> samples;
};

Wav load_wav(const std::string& path);

float max_abs_sample(const Wav& wav);

}  // namespace vp330::test
