#include "render.h"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <spawn.h>
#include <sys/wait.h>

extern char** environ;

namespace vp330::test {

namespace {

#ifndef VP330_RENDER_BINARY
#error "VP330_RENDER_BINARY must be defined by CMake"
#endif

#ifndef GOLDEN_FIXTURES_DIR
#error "GOLDEN_FIXTURES_DIR must be defined by CMake"
#endif

// Deterministic path is fine while there's one L3 test; once Phase 1+ adds
// more, append a unique suffix (test name hash, PID) to support `ctest -j`.
std::string make_temp_path(const std::string& fixture_filename) {
  auto dir = std::filesystem::temp_directory_path() / "vp330_golden";
  std::filesystem::create_directories(dir);
  return (dir / (fixture_filename + ".out.wav")).string();
}

// Run vp330_render via posix_spawn (no shell, no quoting/injection surface).
// Throws on spawn failure or non-zero exit.
void exec_render(const std::vector<std::string>& argv) {
  std::vector<char*> raw;
  raw.reserve(argv.size() + 1);
  for (const auto& s : argv)
    raw.push_back(const_cast<char*>(s.c_str()));
  raw.push_back(nullptr);

  pid_t pid = 0;
  const int spawn_rc = posix_spawnp(&pid, raw[0], nullptr, nullptr, raw.data(), environ);
  if (spawn_rc != 0) {
    throw std::runtime_error(std::string("render_fixture: posix_spawnp failed: ") +
                             std::strerror(spawn_rc));
  }

  int status = 0;
  while (true) {
    const pid_t got = waitpid(pid, &status, 0);
    if (got == pid) break;
    if (got == -1 && errno == EINTR) continue;
    throw std::runtime_error(std::string("render_fixture: waitpid failed: ") +
                             std::strerror(errno));
  }

  if (WIFSIGNALED(status)) {
    throw std::runtime_error("render_fixture: vp330_render killed by signal " +
                             std::to_string(WTERMSIG(status)));
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    throw std::runtime_error("render_fixture: vp330_render exited " +
                             std::to_string(WIFEXITED(status) ? WEXITSTATUS(status) : -1));
  }
}

} // namespace

Wav render_fixture(const std::string& fixture_filename, int sample_rate, double duration_seconds) {
  const std::string fixture_path = std::string(GOLDEN_FIXTURES_DIR) + "/" + fixture_filename;
  const std::string out_path = make_temp_path(fixture_filename);

  const std::vector<std::string> argv = {
      VP330_RENDER_BINARY,
      "--input",
      fixture_path,
      "--output",
      out_path,
      "--duration",
      std::to_string(duration_seconds),
      "--sample-rate",
      std::to_string(sample_rate),
  };
  exec_render(argv);

  return load_wav(out_path);
}

} // namespace vp330::test
