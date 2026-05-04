#include "SmfReader.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace vp330::cli {

namespace {

class Cursor {
public:
  explicit Cursor(const std::vector<std::uint8_t>& bytes) : bytes_{bytes} {}

  std::uint8_t u8() {
    require(1);
    return bytes_[pos_++];
  }
  std::uint16_t u16_be() {
    const auto hi = u8();
    const auto lo = u8();
    return static_cast<std::uint16_t>((hi << 8) | lo);
  }
  std::uint32_t u32_be() {
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i)
      v = (v << 8) | u8();
    return v;
  }
  // Variable-length quantity: up to 4 bytes, high bit = more bytes follow.
  std::uint32_t vlq() {
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
      const auto b = u8();
      v = (v << 7) | (b & 0x7Fu);
      if ((b & 0x80u) == 0) return v;
    }
    throw std::runtime_error("SMF: VLQ exceeds 4 bytes");
  }
  void skip(std::size_t n) {
    require(n);
    pos_ += n;
  }
  std::uint8_t peek() {
    require(1);
    return bytes_[pos_];
  }
  std::size_t pos() const { return pos_; }

private:
  void require(std::size_t n) const {
    if (pos_ + n > bytes_.size()) throw std::runtime_error("SMF: unexpected EOF");
  }

  const std::vector<std::uint8_t>& bytes_;
  std::size_t pos_ = 0;
};

std::vector<std::uint8_t> read_all(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("SMF: cannot open " + path);
  return {std::istreambuf_iterator<char>(f), {}};
}

bool match_tag(Cursor& c, const char (&tag)[5]) {
  for (int i = 0; i < 4; ++i) {
    if (c.u8() != static_cast<std::uint8_t>(tag[i])) return false;
  }
  return true;
}

} // namespace

ParsedMidi read_smf(const std::string& path) {
  const auto bytes = read_all(path);
  Cursor c{bytes};

  if (!match_tag(c, "MThd")) throw std::runtime_error("SMF: missing MThd");
  if (c.u32_be() != 6) throw std::runtime_error("SMF: bad MThd size");
  const auto format = c.u16_be();
  const auto ntracks = c.u16_be();
  const auto division = c.u16_be();
  if (format > 1) throw std::runtime_error("SMF: format >1 unsupported");
  if (division & 0x8000u) throw std::runtime_error("SMF: SMPTE timing unsupported");
  if (ntracks == 0) throw std::runtime_error("SMF: zero tracks");

  ParsedMidi out;
  out.ppqn = division;

  std::uint32_t us_per_quarter = 500'000; // 120 BPM default

  for (std::uint16_t t = 0; t < ntracks; ++t) {
    if (!match_tag(c, "MTrk")) throw std::runtime_error("SMF: missing MTrk");
    const auto track_bytes = c.u32_be();
    const auto track_end = c.pos() + track_bytes;

    // Per-track timeline restart at 0. Format-1 multi-track inputs are accepted but
    // produce a merged-events vector that is NOT globally time-sorted — fix when
    // Phase 4 needs real-DAW format-1 imports.
    double seconds = 0.0;
    // Running status: reuse the last channel-message status byte when the
    // next event's first byte has bit 7 clear (i.e., it's a data byte).
    std::uint8_t running_status = 0;

    while (c.pos() < track_end) {
      const auto delta = c.vlq();
      const double seconds_per_tick =
          static_cast<double>(us_per_quarter) / 1'000'000.0 / static_cast<double>(out.ppqn);
      seconds += static_cast<double>(delta) * seconds_per_tick;

      std::uint8_t status = c.peek();
      if (status >= 0x80u) {
        running_status = status;
        (void)c.u8();
      } else {
        if (running_status == 0) throw std::runtime_error("SMF: running status with no prior");
        status = running_status;
      }

      if (status == 0xFFu) {
        // Meta event — read type + VLQ length, handle tempo and end-of-track.
        const auto meta_type = c.u8();
        const auto len = c.vlq();
        if (meta_type == 0x51u && len == 3) {
          const std::uint32_t a = c.u8();
          const std::uint32_t b = c.u8();
          const std::uint32_t cc = c.u8();
          us_per_quarter = (a << 16) | (b << 8) | cc;
        } else if (meta_type == 0x2Fu && len == 0) {
          break; // End of Track
        } else {
          c.skip(len);
        }
      } else if (status == 0xF0u || status == 0xF7u) {
        // SysEx — skip payload.
        const auto len = c.vlq();
        c.skip(len);
      } else {
        const auto kind = static_cast<std::uint8_t>(status & 0xF0u);
        if (kind == 0x90u || kind == 0x80u) {
          const auto note_byte = c.u8();
          const auto velocity = c.u8();
          const auto note_opt = vp330::MidiNote::try_from_int(note_byte);
          if (!note_opt) throw std::runtime_error("SMF: note out of range");
          // NoteOn with velocity 0 is the common shorthand for NoteOff.
          const bool is_off = (kind == 0x80u) || (kind == 0x90u && velocity == 0);
          out.events.push_back({seconds, vp330::MidiEvent{is_off ? vp330::MidiEvent::Kind::NoteOff
                                                                 : vp330::MidiEvent::Kind::NoteOn,
                                                          *note_opt, velocity, 0}});
        } else if (kind == 0xA0u || kind == 0xB0u || kind == 0xE0u) {
          // Aftertouch, CC, pitch-bend: two data bytes, discard.
          (void)c.u8();
          (void)c.u8();
        } else if (kind == 0xC0u || kind == 0xD0u) {
          // Program change, channel pressure: one data byte, discard.
          (void)c.u8();
        } else {
          throw std::runtime_error("SMF: unknown channel event");
        }
      }
    }
  }
  return out;
}

} // namespace vp330::cli
