#pragma once

#include <cassert>
#include <compare>
#include <cstdint>
#include <optional>

namespace vp330 {

class MidiNote {
public:
  static constexpr std::optional<MidiNote> try_from_int(int v) {
    if (v < 0 || v > 127) return std::nullopt;
    return MidiNote{v};
  }

  constexpr explicit MidiNote(int v) : value_{static_cast<std::int8_t>(v)} {
    assert(v >= 0 && v <= 127);
  }

  constexpr int value() const { return value_; }

  constexpr bool operator==(const MidiNote&) const = default;
  constexpr auto operator<=>(const MidiNote&) const = default;

private:
  std::int8_t value_;
};

inline constexpr MidiNote kMidiC4{60};
inline constexpr MidiNote kMidiA4{69};

} // namespace vp330
