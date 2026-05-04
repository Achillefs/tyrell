#include "render.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace vp330::test {

namespace {

#ifndef VP330_RENDER_BINARY
#error "VP330_RENDER_BINARY must be defined by CMake"
#endif

#ifndef GOLDEN_FIXTURES_DIR
#error "GOLDEN_FIXTURES_DIR must be defined by CMake"
#endif

std::string make_temp_path(const std::string& fixture_filename) {
    auto dir = std::filesystem::temp_directory_path() / "vp330_golden";
    std::filesystem::create_directories(dir);
    return (dir / (fixture_filename + ".out.wav")).string();
}

}  // namespace

Wav render_fixture(const std::string& fixture_filename, int sample_rate, double duration_seconds) {
    const std::string fixture_path = std::string(GOLDEN_FIXTURES_DIR) + "/" + fixture_filename;
    const std::string out_path = make_temp_path(fixture_filename);

    char command[1024];
    std::snprintf(command, sizeof(command),
                  "%s --input %s --output %s --duration %f --sample-rate %d",
                  VP330_RENDER_BINARY, fixture_path.c_str(), out_path.c_str(),
                  duration_seconds, sample_rate);

    const int rc = std::system(command);
    if (rc != 0) {
        throw std::runtime_error("render_fixture: vp330_render exited " + std::to_string(rc));
    }

    return load_wav(out_path);
}

}  // namespace vp330::test
