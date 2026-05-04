#include "wav_io.h"

#include <sndfile.h>

#include <cmath>
#include <stdexcept>

namespace vp330::test {

Wav load_wav(const std::string& path) {
    SF_INFO info{};
    SNDFILE* sf = sf_open(path.c_str(), SFM_READ, &info);
    if (!sf) {
        throw std::runtime_error("load_wav: sf_open failed for " + path + ": " + sf_strerror(nullptr));
    }

    Wav out;
    out.sample_rate = info.samplerate;
    out.channels = info.channels;
    out.frames = static_cast<std::size_t>(info.frames);
    out.samples.resize(out.frames * static_cast<std::size_t>(out.channels));

    const auto read = sf_readf_float(sf, out.samples.data(), info.frames);
    sf_close(sf);
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
