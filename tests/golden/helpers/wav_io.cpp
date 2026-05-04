#include "wav_io.h"

#include <sndfile.h>

#include <cmath>
#include <memory>
#include <stdexcept>

namespace vp330::test {

Wav load_wav(const std::string& path) {
    SF_INFO info{};
    auto sf = std::unique_ptr<SNDFILE, decltype(&sf_close)>(
        sf_open(path.c_str(), SFM_READ, &info), &sf_close);
    if (!sf) {
        throw std::runtime_error("load_wav: sf_open failed for " + path + ": "
                                 + sf_strerror(nullptr));
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

float max_abs_sample(const Wav& wav) {
    float peak = 0.0f;
    for (float s : wav.samples) {
        const float a = std::fabs(s);
        if (a > peak) peak = a;
    }
    return peak;
}

}  // namespace vp330::test
