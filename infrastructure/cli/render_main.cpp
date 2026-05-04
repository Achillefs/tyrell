#include <sndfile.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>

namespace {

struct Args {
  std::string input_midi;
  std::string output_wav;
  double duration_seconds = 1.0;
  int sample_rate = 48000;
};

/**
 * @brief Parse a NUL-terminated string as a double and ensure the whole string is consumed.
 *
 * Converts `str` to a `double` and stores the value in `out` when the entire
 * input represents a valid floating-point number. On failure, an error
 * message referencing `name` is printed to stderr.
 *
 * @param str Input NUL-terminated string to parse.
 * @param out Output location for the parsed double (assigned only on success).
 * @param name Name used in the error message when parsing fails.
 * @return true if parsing succeeded and the entire string was consumed, false otherwise.
 */
bool parse_double(const char* str, double& out, const char* name) {
  char* end = nullptr;
  out = std::strtod(str, &end);
  if (end == str || *end != '\0' || !std::isfinite(out) || out <= 0.0) {
    std::fprintf(stderr, "invalid numeric value for %s: %s (must be a finite positive number)\n",
                 name, str);
    return false;
  }
  return true;
}

/**
 * @brief Parse a base-10 integer from a C string and validate its range.
 *
 * Parses the entire NUL-terminated string `str` as a base-10 integer, verifies
 * the string was fully consumed and that the value is between 0 and 1,000,000,000
 * inclusive, and stores the result in `out` on success.
 *
 * @param str NUL-terminated input string containing the integer to parse.
 * @param out Reference to the destination integer written on success.
 * @param name Human-readable parameter name used in the error message on failure.
 * @return true if parsing succeeded and `out` was assigned, false otherwise.
 *
 * On failure a diagnostic is printed to stderr.
 */
bool parse_int(const char* str, int& out, const char* name) {
  char* end = nullptr;
  const long v = std::strtol(str, &end, 10);
  if (end == str || *end != '\0' || v <= 0 || v > 1'000'000'000) {
    std::fprintf(stderr, "invalid integer value for %s: %s (must be a positive integer ≤ 1e9)\n",
                 name, str);
    return false;
  }
  out = static_cast<int>(v);
  return true;
}

/**
 * @brief Parses command-line arguments and populates an Args structure.
 *
 * Recognized options:
 * - --input <path>        : optional, sets Args::input_midi
 * - --output <path>       : required, sets Args::output_wav
 * - --duration <seconds>  : optional, parsed into Args::duration_seconds
 * - --sample-rate <hz>    : optional, parsed into Args::sample_rate
 *
 * On parse errors, missing values, unknown options, or invalid numeric
 * conversions this function prints an error message to stderr and returns
 * `false`.
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 * @param out Destination Args object to populate with parsed values.
 * @return `true` if parsing succeeded and required options are present, `false` otherwise.
 */
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
      if (auto v = next("--input"))
        out.input_midi = v;
      else
        return false;
    } else if (a == "--output") {
      if (auto v = next("--output"))
        out.output_wav = v;
      else
        return false;
    } else if (a == "--duration") {
      if (auto v = next("--duration")) {
        if (!parse_double(v, out.duration_seconds, "--duration")) return false;
      } else
        return false;
    } else if (a == "--sample-rate") {
      if (auto v = next("--sample-rate")) {
        if (!parse_int(v, out.sample_rate, "--sample-rate")) return false;
      } else
        return false;
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

} // namespace

int main(int argc, char** argv) {
  Args args;
  if (!parse_args(argc, argv, args)) {
    std::fprintf(stderr, "usage: vp330_render --output <wav> [--input <mid>] "
                         "[--duration <sec>] [--sample-rate <hz>]\n");
    return 2;
  }

  const int channels = 2;
  const double frame_count = args.duration_seconds * static_cast<double>(args.sample_rate);
  if (!std::isfinite(frame_count) || frame_count <= 0.0 ||
      frame_count > static_cast<double>(std::numeric_limits<sf_count_t>::max())) {
    std::fprintf(stderr, "render too long: duration*sample_rate=%g exceeds sf_count_t range\n",
                 frame_count);
    return 2;
  }
  const auto frames = static_cast<sf_count_t>(frame_count);

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
