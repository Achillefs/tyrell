#include <sndfile.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Args {
    std::string input_midi;
    std::string output_wav;
    double duration_seconds = 1.0;
    int sample_rate = 48000;
};

bool parse_args(int argc, char** argv, Args& out) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing value for %s\n", name);
                return nullptr;
            }
            return argv[++i];
        };
        if (a == "--input") {
            if (auto v = next("--input")) out.input_midi = v;
            else return false;
        } else if (a == "--output") {
            if (auto v = next("--output")) out.output_wav = v;
            else return false;
        } else if (a == "--duration") {
            if (auto v = next("--duration")) out.duration_seconds = std::atof(v);
            else return false;
        } else if (a == "--sample-rate") {
            if (auto v = next("--sample-rate")) out.sample_rate = std::atoi(v);
            else return false;
        } else {
            std::fprintf(stderr, "unknown arg: %s\n", a.c_str());
            return false;
        }
    }
    if (out.output_wav.empty()) {
        std::fprintf(stderr, "--output is required\n");
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    Args args;
    if (!parse_args(argc, argv, args)) {
        std::fprintf(stderr,
            "usage: vp330_render --output <wav> [--input <mid>] "
            "[--duration <sec>] [--sample-rate <hz>]\n");
        return 2;
    }

    const int channels = 2;
    const auto frames = static_cast<sf_count_t>(args.duration_seconds * args.sample_rate);

    SF_INFO info{};
    info.samplerate = args.sample_rate;
    info.channels = channels;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    SNDFILE* sf = sf_open(args.output_wav.c_str(), SFM_WRITE, &info);
    if (!sf) {
        std::fprintf(stderr, "sf_open failed: %s\n", sf_strerror(nullptr));
        return 1;
    }

    // Engine isn't built yet — render silence. Phase 1 wires this to SynthesisEngine.
    std::vector<float> block(static_cast<std::size_t>(channels) * 1024, 0.0f);
    sf_count_t remaining = frames;
    while (remaining > 0) {
        const auto chunk = std::min<sf_count_t>(1024, remaining);
        const auto wrote = sf_writef_float(sf, block.data(), chunk);
        if (wrote != chunk) {
            std::fprintf(stderr, "sf_writef_float short write\n");
            sf_close(sf);
            return 1;
        }
        remaining -= chunk;
    }

    sf_close(sf);
    return 0;
}
