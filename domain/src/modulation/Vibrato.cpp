#include "vp330/modulation/Vibrato.h"

#include "vp330/section/ChoirConstants.h"
#include "vp330/tod/MkIIConstants.h"

namespace vp330 {

Vibrato::Vibrato(int sample_rate) : lfo_{sample_rate} {
}

void Vibrato::set_rate(Hertz rate) {
  lfo_.set_rate(rate);
}
void Vibrato::set_depth(float d) {
  lfo_.set_depth(d);
}

Hertz Vibrato::tick(std::size_t frames) {
  lfo_.advance(static_cast<int>(frames));
  return Hertz{mkii::kMasterClockHz.value() +
               (static_cast<double>(lfo_.value()) * mkii::kVibratoMaxClockOffsetHz)};
}

} // namespace vp330
