#pragma once

namespace vp330 {

// The four front-panel switches of the MkII Choir section.
// Array index in kVoiceWeights MUST match static_cast<int>(ChoirSwitch).
enum class ChoirSwitch { LowerMale8 = 0, LowerMale4 = 1, UpperMale8 = 2, UpperFemale4 = 3 };

} // namespace vp330
