#include "SmfReader.h"

#include "vp330/engine/MidiEvent.h"
#include "vp330/engine/SynthesisEngine.h"

#include <sndfile.h>

#include <algorithm>
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

bool parse_double(const char* str, double& out, const char* name) {
  char* end = nullptr;
  out = std::strtod(str, &end);
  if (end == str || *end != '\0' || !std::isfinite(out) || out <= 0.0) {
    std::fprintf(stderr, "invalid numeric value for %s: %s\n", name, str);
    return false;
  }
  return true;
}

bool parse_int(const char* str, int& out, const char* name) {
  char* end = nullptr;
  const long v = std::strtol(str, &end, 10);
  if (end == str || *end != '\0' || v <= 0 || v > 1'000'000'000) {
    std::fprintf(stderr, "invalid integer for %s: %s\n", name, str);
    return false;
  }
  out = static_cast<int>(v);
  return true;
}

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

struct ScheduledEvent {
  std::size_t sample_index;
  vp330::MidiEvent event;
};

std::vector<ScheduledEvent> schedule(const vp330::cli::ParsedMidi& parsed, int sample_rate,
                                     std::size_t max_frames) {
  std::vector<ScheduledEvent> out;
  out.reserve(parsed.events.size());
  for (const auto& te : parsed.events) {
    const auto idx =
        static_cast<std::size_t>(std::llround(te.time_seconds * static_cast<double>(sample_rate)));
    // Half-open frame range [0, max_frames): an event at exactly t==duration
    // (e.g., a NoteOff aligned with the render end) lands at idx==max_frames
    // and is intentionally dropped — the engine has no slot to apply it after
    // the last rendered sample.
    if (idx >= max_frames) continue;
    out.push_back({idx, te.event});
  }
  std::stable_sort(out.begin(), out.end(), [](const ScheduledEvent& a, const ScheduledEvent& b) {
    return a.sample_index < b.sample_index;
  });
  return out;
}

void interleave(const std::vector<float>& left, const std::vector<float>& right,
                std::vector<float>& out) {
  out.resize(left.size() * 2);
  for (std::size_t i = 0; i < left.size(); ++i) {
    out[2 * i] = left[i];
    out[2 * i + 1] = right[i];
  }
}

} // namespace

int main(int argc, char** argv) {
  Args args;
  if (!parse_args(argc, argv, args)) {
    std::fprintf(stderr, "usage: vp330_render --output <wav> [--input <mid>] "
                         "[--duration <sec>] [--sample-rate <hz>]\n");
    return 2;
  }

  const double frame_count_d = args.duration_seconds * static_cast<double>(args.sample_rate);
  if (!std::isfinite(frame_count_d) || frame_count_d <= 0.0 ||
      frame_count_d > static_cast<double>(std::numeric_limits<sf_count_t>::max())) {
    std::fprintf(stderr, "render too long: duration*sample_rate=%g\n", frame_count_d);
    return 2;
  }
  const auto frames = static_cast<std::size_t>(frame_count_d);

  vp330::cli::ParsedMidi parsed;
  if (!args.input_midi.empty()) {
    try {
      parsed = vp330::cli::read_smf(args.input_midi);
    } catch (const std::exception& e) {
      std::fprintf(stderr, "read_smf failed: %s\n", e.what());
      return 1;
    }
  }
  const auto events = schedule(parsed, args.sample_rate, frames);

  vp330::SynthesisEngine engine{args.sample_rate};

  SF_INFO info{};
  info.samplerate = args.sample_rate;
  info.channels = 2;
  info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

  SNDFILE* sf = sf_open(args.output_wav.c_str(), SFM_WRITE, &info);
  if (!sf) {
    std::fprintf(stderr, "sf_open failed: %s\n", sf_strerror(nullptr));
    return 1;
  }

  constexpr std::size_t kBlock = 1024;
  std::vector<float> left(kBlock), right(kBlock), interleaved;
  std::size_t produced = 0;
  std::size_t event_idx = 0;

  while (produced < frames) {
    const auto block = std::min<std::size_t>(kBlock, frames - produced);

    while (event_idx < events.size() && events[event_idx].sample_index < produced + block) {
      const auto& ev = events[event_idx].event;
      switch (ev.kind) {
      case vp330::MidiEvent::Kind::NoteOn:
        engine.note_on(ev.note);
        break;
      case vp330::MidiEvent::Kind::NoteOff:
        engine.note_off(ev.note);
        break;
      }
      ++event_idx;
    }

    left.assign(block, 0.0f);
    right.assign(block, 0.0f);
    engine.render(left.data(), right.data(), block);
    interleave(left, right, interleaved);

    const auto wrote = sf_writef_float(sf, interleaved.data(), static_cast<sf_count_t>(block));
    if (wrote != static_cast<sf_count_t>(block)) {
      std::fprintf(stderr, "sf_writef_float short write\n");
      sf_close(sf);
      return 1;
    }
    produced += block;
  }

  sf_close(sf);
  return 0;
}
