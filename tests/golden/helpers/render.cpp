#include "render.h"

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

/**
 * @brief Construct a deterministic temporary output path for a golden fixture.
 *
 * Ensures a directory named "vp330_golden" exists under the system temporary
 * directory and returns the path formed by appending ".out.wav" to
 * fixture_filename inside that directory.
 *
 * @param fixture_filename Filename of the golden fixture (base name or relative
 *        filename) used to derive the output WAV name.
 * @return std::string Full path to the temporary output WAV
 * (system-temp/vp330_golden/<fixture_filename>.out.wav).
 */
std::string make_temp_path(const std::string& fixture_filename) {
  auto dir = std::filesystem::temp_directory_path() / "vp330_golden";
  std::filesystem::create_directories(dir);
  // Note: deterministic path is fine while there's one L3 test; when Phase 1+
  // adds more, append a unique suffix (test name hash, PID) to support `ctest -j`.
  return (dir / (fixture_filename + ".out.wav")).string();
}

} // namespace

Wav render_fixture(const std::string& fixture_filename, int sample_rate, double duration_seconds) {
  const std::string fixture_path = std::string(GOLDEN_FIXTURES_DIR) + "/" + fixture_filename;
  const std::string out_path = make_temp_path(fixture_filename);

  const std::string command = std::string(VP330_RENDER_BINARY) + " --input " + fixture_path +
                              " --output " + out_path + " --duration " +
                              std::to_string(duration_seconds) + " --sample-rate " +
                              std::to_string(sample_rate);

  const int rc = std::system(command.c_str());
  if (rc != 0) {
    throw std::runtime_error("render_fixture: vp330_render exited " + std::to_string(rc));
  }

  return load_wav(out_path);
}

} // namespace vp330::test
