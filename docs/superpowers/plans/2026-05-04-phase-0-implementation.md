# Phase 0 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stand up the VP-330 MkII recreation repo: build system, empty cross-format plugin, walking-skeleton golden test, CI/lint, docs, and Python capture tooling — exactly the deliverables in spec §9 Phase 0.

**Architecture:** Hexagonal layout per spec §3.1 and §5. `domain/` is a pure-stdlib static library; `infrastructure/juce` and `infrastructure/cli` are adapters; `tests/` links the domain only. The walking-skeleton golden test renders an empty MIDI → 1 s of silence WAV → compares to a pre-committed silent baseline (LFS) using `max(abs(sample)) < 1e-6`. Capture tooling is Python under `tools/capture/`; the captures themselves live in a private companion repo at `$VP330_CAPTURES_DIR`.

**Tech Stack:**
- C++20, CMake ≥ 3.22.
- **JUCE 8.0.12** (FetchContent, tag `8.0.12`).
- **Catch2 v3.14.0** (FetchContent, tag `v3.14.0`).
- **rapidcheck** commit `b2d9ed2dddefc4b84318d664b4f221eb792d89c7` (no tags exist; pinned by SHA).
- **libsndfile** via system package (`brew install libsndfile` on macOS, `apt install libsndfile1-dev` on Ubuntu).
- Python 3.11+ for capture tooling: `mido`, `python-rtmidi`, `sounddevice`, `soundfile`, `jsonschema`, `numpy`.
- GitHub Actions: `actions/checkout@v4` (with `lfs: true`), `actions/cache@v4`.

**Source design doc:** `docs/superpowers/specs/2026-05-04-phase-0-design.md`. Read it before starting; this plan implements it.

**Repo state at start of execution:**
- HEAD has `LICENSE`, `.gitignore`, `docs/SPEC.md`, `docs/superpowers/specs/2026-05-04-phase-0-design.md`.
- `CLAUDE.md` and `.DS_Store` are untracked. `.DS_Store` is macOS noise; this plan adds it to `.gitignore`. `CLAUDE.md` gets committed as part of Task 1.

---

## File Structure (target end-state)

```
.
├── .clang-format                                   [Task 10]
├── .clang-tidy                                     [Task 11]
├── .gitattributes                                  [Task 1]
├── .github/
│   └── workflows/
│       ├── ci.yml                                  [Task 8]
│       ├── arm-cross.yml                           [Task 9]
│       └── lint.yml                                [Task 12]
├── .gitignore                                      [Task 1, edited]
├── CLAUDE.md                                       [Task 1, committed as-is]
├── CMakeLists.txt                                  [Task 2; edited Tasks 3–5,7,13]
├── CONTRIBUTING.md                                 [Task 16]
├── GLOSSARY.md                                     [Task 15]
├── LICENSE                                         [pre-existing]
├── README.md                                       [Task 16]
├── THIRD_PARTY.md                                  [Task 15]
├── docs/
│   ├── SPEC.md                                     [pre-existing]
│   └── superpowers/
│       ├── plans/2026-05-04-phase-0-implementation.md  [this plan, committed before Task 1]
│       └── specs/2026-05-04-phase-0-design.md      [pre-existing]
├── domain/
│   ├── CMakeLists.txt                              [Task 2]
│   ├── include/vp330/.gitkeep                      [Task 1]
│   └── src/stub.cpp                                [Task 2]
├── infrastructure/
│   ├── CMakeLists.txt                              [Task 3]
│   ├── juce/
│   │   ├── CMakeLists.txt                          [Task 3; edited Task 7]
│   │   ├── PluginProcessor.h                       [Task 3]
│   │   └── PluginProcessor.cpp                     [Task 3]
│   ├── cli/
│   │   ├── CMakeLists.txt                          [Task 4]
│   │   └── render_main.cpp                         [Task 4]
│   └── elk/
│       └── README.md                               [Task 9]
├── tests/
│   ├── CMakeLists.txt                              [Task 5]
│   ├── unit/.gitkeep                               [Task 5]
│   ├── characterization/.gitkeep                   [Task 5]
│   ├── golden/
│   │   ├── CMakeLists.txt                          [Task 5; edited Task 6]
│   │   ├── golden_test.cpp                         [Task 5; edited Task 6]
│   │   ├── helpers/render.h                        [Task 6]
│   │   ├── helpers/render.cpp                      [Task 6]
│   │   ├── helpers/wav_io.h                        [Task 6]
│   │   ├── helpers/wav_io.cpp                      [Task 6]
│   │   ├── fixtures/silence-1s.mid                 [Task 6, generated]
│   │   └── baselines/silence-1s.wav                [Task 6, generated, LFS]
│   └── reference/.gitkeep                          [Task 5]
└── tools/
    ├── install-hooks.sh                            [Task 14]
    ├── golden/
    │   ├── gen-silence-fixture.py                  [Task 6]
    │   └── gen-silence-baseline.py                 [Task 6]
    └── capture/
        ├── README.md                               [Task 17]
        ├── requirements.txt                        [Task 20]
        ├── schema.json                             [Task 17]
        ├── sample-manifest.json                    [Task 17]
        ├── scaffold.py                             [Task 18]
        ├── validate.py                             [Task 19]
        └── driver.py                               [Task 20]
```

**Bootstrap step (before Task 1):**

- [ ] **Commit this plan**

```bash
cd /Users/ac/dev/tyrell
git add docs/superpowers/plans/2026-05-04-phase-0-implementation.md
git commit -m "$(cat <<'EOF'
docs: add phase 0 implementation plan

Implements design doc 2026-05-04-phase-0-design.md as 21 ordered commits.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

---

## Task 1: Repo skeleton + LFS rules + CLAUDE.md

**Files:**
- Create: `.gitattributes`
- Create: `domain/include/vp330/.gitkeep`
- Modify: `.gitignore` (append `.DS_Store` and macOS noise)
- Track: `CLAUDE.md` (already on disk, untracked)

- [ ] **Step 1: Create `.gitattributes` with LFS rules**

```gitattributes
# Audio under tests/golden/baselines and reference-captures/ uses Git LFS.
tests/golden/baselines/**/*.wav  filter=lfs diff=lfs merge=lfs -text
reference-captures/**/*.wav      filter=lfs diff=lfs merge=lfs -text

# Line-ending hygiene
*.cpp        text eol=lf
*.h          text eol=lf
*.cmake      text eol=lf
CMakeLists.txt text eol=lf
*.md         text eol=lf
*.yml        text eol=lf
*.json       text eol=lf
*.py         text eol=lf
*.sh         text eol=lf
*.wav        binary
*.mid        binary
```

- [ ] **Step 2: Append macOS noise to `.gitignore`**

Append the following lines to existing `.gitignore`:

```gitignore

# macOS
.DS_Store
**/.DS_Store
```

- [ ] **Step 3: Create `domain/include/vp330/.gitkeep`**

Empty file. Reserves the directory; future tasks add real headers.

```bash
mkdir -p domain/include/vp330
touch domain/include/vp330/.gitkeep
```

- [ ] **Step 4: Verify Git LFS is installed locally**

Run:
```bash
git lfs version
```

Expected: prints LFS version (e.g., `git-lfs/3.x.x`). If "command not found", install via `brew install git-lfs && git lfs install` on macOS, then retry.

- [ ] **Step 5: Verify untracked `.DS_Store` is now ignored**

Run:
```bash
git status --short
```

Expected: `.DS_Store` no longer appears as `??`. `CLAUDE.md` still appears.

- [ ] **Step 6: Stage and commit**

```bash
git add .gitattributes .gitignore domain/include/vp330/.gitkeep CLAUDE.md
git commit -m "$(cat <<'EOF'
chore: repo skeleton, LFS rules, track CLAUDE.md

LFS rules cover tests/golden/baselines and reference-captures (private companion
repo, but rule lives here so a future merge-in stays correct). Adds .DS_Store
to .gitignore. Reserves domain/include/vp330/ via .gitkeep.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `git status` is clean (except `.DS_Store` if present, which should be ignored). `git lfs ls-files` returns nothing yet (no WAVs committed).

---

## Task 2: Top-level CMake + empty `vp330_domain`

**Files:**
- Create: `CMakeLists.txt`
- Create: `domain/CMakeLists.txt`
- Create: `domain/src/stub.cpp`

- [ ] **Step 1: Create top-level `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.22)
project(VP330 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Treat warnings as errors only in CI; locally just warn.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(domain)
```

- [ ] **Step 2: Create `domain/CMakeLists.txt`**

```cmake
add_library(vp330_domain STATIC
    src/stub.cpp
)

target_include_directories(vp330_domain
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(vp330_domain PUBLIC cxx_std_20)
```

- [ ] **Step 3: Create `domain/src/stub.cpp`**

```cpp
// Placeholder TU so vp330_domain has something to compile until Phase 1 lands
// real value types and entities. Delete when the first real .cpp arrives.
namespace vp330::detail {
inline void anchor() {}
}  // namespace vp330::detail
```

- [ ] **Step 4: Verify the build succeeds**

Run:
```bash
cmake -B build -S .
cmake --build build --target vp330_domain
```

Expected: configure succeeds, build emits `build/domain/libvp330_domain.a` (macOS) without errors.

- [ ] **Step 5: Verify the static library artifact**

```bash
ls -la build/domain/libvp330_domain.a
```

Expected: file exists, non-empty.

- [ ] **Step 6: Commit**

```bash
git add CMakeLists.txt domain/CMakeLists.txt domain/src/stub.cpp
git commit -m "$(cat <<'EOF'
build: top-level CMake + empty vp330_domain static library

C++20, warnings on. domain/ has a stub TU; real headers and entities arrive
in Phase 1.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `cmake --build build` produces `vp330_domain` static library. No JUCE, no libsndfile, no third-party deps yet.

---

## Task 3: JUCE FetchContent + empty plugin (VST3 + Standalone)

**Files:**
- Modify: `CMakeLists.txt` (add JUCE FetchContent, `add_subdirectory(infrastructure)`)
- Create: `infrastructure/CMakeLists.txt`
- Create: `infrastructure/juce/CMakeLists.txt`
- Create: `infrastructure/juce/PluginProcessor.h`
- Create: `infrastructure/juce/PluginProcessor.cpp`

- [ ] **Step 1: Update top-level `CMakeLists.txt` to fetch JUCE**

Replace the `add_subdirectory(domain)` line with:

```cmake
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

add_subdirectory(domain)
add_subdirectory(infrastructure)
```

- [ ] **Step 2: Create `infrastructure/CMakeLists.txt`**

```cmake
add_subdirectory(juce)
```

- [ ] **Step 3: Create `infrastructure/juce/CMakeLists.txt`**

```cmake
juce_add_plugin(VP330
    PRODUCT_NAME              "VP330"
    COMPANY_NAME              "VP330 Project"
    BUNDLE_ID                 "org.vp330.VP330"
    PLUGIN_MANUFACTURER_CODE  Vp33
    PLUGIN_CODE               Vp33
    FORMATS                   VST3 Standalone
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

target_sources(VP330 PRIVATE
    PluginProcessor.cpp
)

target_compile_definitions(VP330 PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VP330 PRIVATE
    vp330_domain
    juce::juce_audio_utils
    juce::juce_audio_processors
)
```

- [ ] **Step 4: Create `infrastructure/juce/PluginProcessor.h`**

```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class VP330Processor : public juce::AudioProcessor {
  public:
    VP330Processor();
    ~VP330Processor() override = default;

    void prepareToPlay(double sample_rate, int samples_per_block) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "VP330"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VP330Processor)
};
```

- [ ] **Step 5: Create `infrastructure/juce/PluginProcessor.cpp`**

```cpp
#include "PluginProcessor.h"

VP330Processor::VP330Processor()
    : juce::AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

void VP330Processor::prepareToPlay(double, int) {}
void VP330Processor::releaseResources() {}

void VP330Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;
    buffer.clear();
}

bool VP330Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VP330Processor();
}
```

- [ ] **Step 6: Configure and build (cold; JUCE will clone, ~5 min)**

```bash
cmake --build build --target VP330_VST3 VP330_Standalone
```

If CMake complains about needing reconfigure, run `cmake -B build -S .` first.

Expected: VST3 bundle and Standalone app built. Look for:
- `build/infrastructure/juce/VP330_artefacts/Release/VST3/VP330.vst3` (or `Debug/`)
- `build/infrastructure/juce/VP330_artefacts/Release/Standalone/VP330.app` (macOS)

If the configuration didn't pick a build type, the artefacts may live under `Debug/`.

- [ ] **Step 7: Sanity-launch the standalone**

```bash
open build/infrastructure/juce/VP330_artefacts/*/Standalone/VP330.app
```

Expected: app launches, opens an audio I/O dialog (since `hasEditor()` returns false the window will be minimal). Close it.

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt infrastructure/
git commit -m "$(cat <<'EOF'
build: empty VP330 plugin (VST3 + Standalone, silence)

JUCE 8.0.12 via FetchContent. AudioProcessor clears the output buffer in
processBlock — silence by construction. AU added in a follow-up commit.
(CLAP deferred to Phase 5; see SPEC 1.3 change log.)

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** Plugin builds; standalone launches without crashing; output is silence.

---

## Task 4: CLI render binary

**Files:**
- Modify: `CMakeLists.txt` (add libsndfile lookup before `add_subdirectory(infrastructure)`)
- Create: `infrastructure/cli/CMakeLists.txt`
- Create: `infrastructure/cli/render_main.cpp`
- Modify: `infrastructure/CMakeLists.txt` (add `add_subdirectory(cli)`)

- [ ] **Step 1: Verify libsndfile is installed**

```bash
brew list libsndfile >/dev/null 2>&1 || brew install libsndfile
pkg-config --modversion sndfile
```

Expected: prints a version (e.g., `1.2.x`).

- [ ] **Step 2: Add libsndfile lookup to top-level `CMakeLists.txt`**

Insert immediately after `FetchContent_MakeAvailable(JUCE)`:

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)
```

- [ ] **Step 3: Update `infrastructure/CMakeLists.txt`**

```cmake
add_subdirectory(juce)
add_subdirectory(cli)
```

- [ ] **Step 4: Create `infrastructure/cli/CMakeLists.txt`**

```cmake
add_executable(vp330_render
    render_main.cpp
)

target_link_libraries(vp330_render PRIVATE
    vp330_domain
    PkgConfig::SNDFILE
)

target_compile_features(vp330_render PRIVATE cxx_std_20)
```

- [ ] **Step 5: Create `infrastructure/cli/render_main.cpp`**

```cpp
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
```

- [ ] **Step 6: Build and run**

```bash
cmake -B build -S .
cmake --build build --target vp330_render
./build/infrastructure/cli/vp330_render --output /tmp/test.wav --duration 1.0
```

Expected: command exits 0; `/tmp/test.wav` is a valid 1-second silent WAV at 48 kHz / 24-bit stereo.

- [ ] **Step 7: Verify the output file**

```bash
file /tmp/test.wav
```

Expected output contains `RIFF` and `WAVE`. Optionally inspect with `sndfile-info /tmp/test.wav` (if installed): sample rate 48000, channels 2, frames 48000.

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt infrastructure/
git commit -m "$(cat <<'EOF'
build: vp330_render CLI writes silent WAV via libsndfile

48 kHz / 24-bit stereo. Args: --output (required), --input, --duration,
--sample-rate. Engine wiring deferred to Phase 1; for now emits zeros.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `vp330_render --output X.wav --duration 1.0` produces a valid 1 s silent WAV.

---

## Task 5: Catch2 v3 + tests target with one trivial test

**Files:**
- Modify: `CMakeLists.txt` (add Catch2 FetchContent, `enable_testing()`, `add_subdirectory(tests)`)
- Create: `tests/CMakeLists.txt`
- Create: `tests/unit/.gitkeep`
- Create: `tests/characterization/.gitkeep`
- Create: `tests/reference/.gitkeep`
- Create: `tests/golden/CMakeLists.txt`
- Create: `tests/golden/golden_test.cpp` (placeholder; real golden test arrives in Task 6)

- [ ] **Step 1: Update top-level `CMakeLists.txt`**

After the libsndfile block, before `add_subdirectory(domain)`, insert:

```cmake
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.14.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

enable_testing()
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
```

After `add_subdirectory(infrastructure)` add:

```cmake
add_subdirectory(tests)
```

- [ ] **Step 2: Create `tests/CMakeLists.txt`**

```cmake
add_executable(vp330_tests)

target_link_libraries(vp330_tests PRIVATE
    vp330_domain
    Catch2::Catch2WithMain
)

target_compile_features(vp330_tests PRIVATE cxx_std_20)

add_subdirectory(golden)

catch_discover_tests(vp330_tests)
```

- [ ] **Step 3: Create empty placeholder dirs**

```bash
mkdir -p tests/unit tests/characterization tests/reference tests/golden
touch tests/unit/.gitkeep tests/characterization/.gitkeep tests/reference/.gitkeep
```

- [ ] **Step 4: Create `tests/golden/CMakeLists.txt`**

```cmake
target_sources(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/golden_test.cpp
)
```

- [ ] **Step 5: Create `tests/golden/golden_test.cpp` (trivial placeholder)**

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("test harness boots", "[golden][smoke]") {
    REQUIRE(2 + 2 == 4);
}
```

- [ ] **Step 6: Configure, build, run**

```bash
cmake -B build -S .
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure
```

Expected: configure succeeds; `vp330_tests` builds and links only `vp330_domain` and Catch2 (no JUCE, no libsndfile); `ctest` reports 1 test passed.

- [ ] **Step 7: Verify the test binary does NOT link JUCE**

On macOS:
```bash
otool -L build/tests/vp330_tests | grep -i juce || echo "OK: no JUCE linkage"
```

Expected: prints `OK: no JUCE linkage`.

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt tests/
git commit -m "$(cat <<'EOF'
test: Catch2 v3.14.0 + vp330_tests skeleton

vp330_tests links vp330_domain only — no JUCE, no libsndfile in the test
binary, per spec §5 invariant. catch_discover_tests integrates with ctest.
Placeholder smoke test will be replaced by the walking-skeleton in Task 6.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `ctest --test-dir build` passes 1 test. `otool -L` shows no JUCE in `vp330_tests`.

---

## Task 6: Walking-skeleton golden test (TDD)

This is the only Task in Phase 0 with proper red→green→refactor: write the failing test, then build the renderer plumbing to make it pass.

**Files:**
- Create: `tests/golden/helpers/render.h`, `helpers/render.cpp`
- Create: `tests/golden/helpers/wav_io.h`, `helpers/wav_io.cpp`
- Modify: `tests/golden/CMakeLists.txt` (add helpers, define BASELINES_DIR macro, link libsndfile)
- Modify: `tests/golden/golden_test.cpp` (replace smoke test with walking-skeleton test)
- Create: `tools/golden/gen-silence-fixture.py`
- Create: `tools/golden/gen-silence-baseline.py`
- Create: `tests/golden/fixtures/silence-1s.mid` (generated, ~30 bytes, plain commit)
- Create: `tests/golden/baselines/silence-1s.wav` (generated, ~288 KB, **LFS**)

- [ ] **Step 1: Write `gen-silence-fixture.py`**

Create `tools/golden/gen-silence-fixture.py`:

```python
#!/usr/bin/env python3
"""Generate tests/golden/fixtures/silence-1s.mid: a 1-second MIDI file with no notes."""

import argparse
import struct
from pathlib import Path


def variable_length_quantity(value: int) -> bytes:
    if value == 0:
        return b"\x00"
    parts = []
    while value:
        parts.append(value & 0x7F)
        value >>= 7
    parts.reverse()
    return bytes((p | 0x80) for p in parts[:-1]) + bytes([parts[-1]])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    # Header: format 0, 1 track, 480 ticks per quarter note.
    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, 480)

    # Track: tempo = 500_000 microseconds/quarter (120 BPM); end-of-track at tick 1920 (4 quarters @ 120 BPM = 1.0s).
    tempo_event = b"\x00\xFF\x51\x03" + b"\x07\xA1\x20"
    end_of_track = variable_length_quantity(1920) + b"\xFF\x2F\x00"
    track_data = tempo_event + end_of_track
    track = b"MTrk" + struct.pack(">I", len(track_data)) + track_data

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(header + track)
    print(f"wrote {args.output} ({len(header) + len(track)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Generate the fixture**

```bash
python3 tools/golden/gen-silence-fixture.py --output tests/golden/fixtures/silence-1s.mid
```

Expected: prints `wrote tests/golden/fixtures/silence-1s.mid (XX bytes)` (around 30–40 bytes).

- [ ] **Step 3: Write `gen-silence-baseline.py`**

Create `tools/golden/gen-silence-baseline.py`:

```python
#!/usr/bin/env python3
"""Generate tests/golden/baselines/silence-1s.wav: 48 kHz / 24-bit / stereo / 1 s of zeros."""

import argparse
import struct
from pathlib import Path


def write_wav_24bit_silence(path: Path, sample_rate: int, duration_seconds: float) -> int:
    channels = 2
    bits_per_sample = 24
    bytes_per_sample = bits_per_sample // 8
    num_frames = int(round(sample_rate * duration_seconds))
    block_align = channels * bytes_per_sample
    byte_rate = sample_rate * block_align
    data_size = num_frames * block_align

    header = (
        b"RIFF"
        + struct.pack("<I", 36 + data_size)
        + b"WAVE"
        + b"fmt "
        + struct.pack("<IHHIIHH", 16, 1, channels, sample_rate, byte_rate, block_align, bits_per_sample)
        + b"data"
        + struct.pack("<I", data_size)
    )

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(header)
        # Stream zeros to avoid allocating ~288 KB up front (small here, but pattern matters for larger baselines later).
        chunk = bytes(block_align * 1024)
        remaining = data_size
        while remaining > 0:
            n = min(len(chunk), remaining)
            f.write(chunk[:n])
            remaining -= n

    return data_size + len(header)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--sample-rate", type=int, default=48000)
    parser.add_argument("--duration", type=float, default=1.0)
    args = parser.parse_args()

    total = write_wav_24bit_silence(args.output, args.sample_rate, args.duration)
    print(f"wrote {args.output} ({total} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: Generate the baseline**

```bash
python3 tools/golden/gen-silence-baseline.py --output tests/golden/baselines/silence-1s.wav
```

Expected: prints `wrote tests/golden/baselines/silence-1s.wav (288044 bytes)` (288 000 data + 44 header).

- [ ] **Step 5: Verify LFS will track the baseline WAV**

```bash
git check-attr filter -- tests/golden/baselines/silence-1s.wav
```

Expected: `tests/golden/baselines/silence-1s.wav: filter: lfs`

- [ ] **Step 6: Write `tests/golden/helpers/wav_io.h`**

```cpp
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace vp330::test {

struct Wav {
    int sample_rate = 0;
    int channels = 0;
    std::size_t frames = 0;
    // Interleaved float samples in [-1, 1].
    std::vector<float> samples;
};

Wav load_wav(const std::string& path);

float max_abs_sample(const Wav& wav);

}  // namespace vp330::test
```

- [ ] **Step 7: Write `tests/golden/helpers/wav_io.cpp`**

```cpp
#include "wav_io.h"

#include <sndfile.h>

#include <cmath>
#include <stdexcept>

namespace vp330::test {

Wav load_wav(const std::string& path) {
    SF_INFO info{};
    SNDFILE* sf = sf_open(path.c_str(), SFM_READ, &info);
    if (!sf) {
        throw std::runtime_error("load_wav: sf_open failed for " + path + ": " + sf_strerror(nullptr));
    }

    Wav out;
    out.sample_rate = info.samplerate;
    out.channels = info.channels;
    out.frames = static_cast<std::size_t>(info.frames);
    out.samples.resize(out.frames * static_cast<std::size_t>(out.channels));

    const auto read = sf_readf_float(sf, out.samples.data(), info.frames);
    sf_close(sf);
    if (read != info.frames) {
        throw std::runtime_error("load_wav: short read on " + path);
    }
    return out;
}

float max_abs_sample(const Wav& wav) {
    float peak = 0.0f;
    for (float s : wav.samples) {
        const float a = std::fabs(s);
        if (a > peak) peak = a;
    }
    return peak;
}

}  // namespace vp330::test
```

- [ ] **Step 8: Write `tests/golden/helpers/render.h`**

```cpp
#pragma once

#include "wav_io.h"

#include <string>

namespace vp330::test {

// Calls the vp330_render CLI to produce a WAV from a fixture MIDI.
// Returns the loaded WAV. Throws on failure.
Wav render_fixture(const std::string& fixture_filename,
                   int sample_rate,
                   double duration_seconds);

}  // namespace vp330::test
```

- [ ] **Step 9: Write `tests/golden/helpers/render.cpp`**

```cpp
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
```

- [ ] **Step 10: Update `tests/golden/CMakeLists.txt`**

```cmake
target_sources(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/golden_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/wav_io.cpp
)

target_include_directories(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers
)

target_link_libraries(vp330_tests PRIVATE
    PkgConfig::SNDFILE
)

target_compile_definitions(vp330_tests PRIVATE
    VP330_RENDER_BINARY="$<TARGET_FILE:vp330_render>"
    GOLDEN_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    GOLDEN_BASELINES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/baselines"
)

add_dependencies(vp330_tests vp330_render)
```

> Note: linking `PkgConfig::SNDFILE` into `vp330_tests` is **only** for test helpers (`wav_io.cpp` reads WAVs). It does **not** make `vp330_domain` depend on libsndfile — the spec §5 invariant only restricts `domain/`, not `tests/`.

- [ ] **Step 11: Replace `tests/golden/golden_test.cpp` with the walking-skeleton test**

```cpp
#include "render.h"
#include "wav_io.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace vp330::test;

TEST_CASE("walking skeleton: silence renders to silence", "[golden]") {
    const auto wav = render_fixture("silence-1s.mid", 48000, 1.0);
    const auto baseline = load_wav(std::string(GOLDEN_BASELINES_DIR) + "/silence-1s.wav");

    REQUIRE(wav.frames == baseline.frames);
    REQUIRE(wav.channels == baseline.channels);
    REQUIRE(wav.sample_rate == baseline.sample_rate);
    REQUIRE(max_abs_sample(wav) < 1e-6f);
    REQUIRE(max_abs_sample(baseline) < 1e-6f);
}
```

- [ ] **Step 12: Build the test (red-then-green: it should already be green, since the renderer writes silence and the baseline is silence)**

```bash
cmake -B build -S .
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure
```

Expected: 1 test passes (walking-skeleton). If the test fails, inspect:
- `wav.frames == baseline.frames` failure → mismatch between render duration and baseline duration; re-check Steps 2 and 4.
- `max_abs_sample(wav) < 1e-6f` failure → renderer is not actually writing zeros; re-check `render_main.cpp`.

- [ ] **Step 13: Stage and commit**

```bash
git add tools/golden/ \
        tests/golden/helpers/ \
        tests/golden/CMakeLists.txt \
        tests/golden/golden_test.cpp \
        tests/golden/fixtures/silence-1s.mid \
        tests/golden/baselines/silence-1s.wav

# Verify the WAV is staged as an LFS pointer:
git lfs ls-files
# Expected: prints exactly one entry for tests/golden/baselines/silence-1s.wav

git commit -m "$(cat <<'EOF'
test: walking-skeleton golden test (silence renders to silence)

End-to-end exercise of the L3 pipeline: vp330_render → WAV → load → compare.
Comparison is max(abs(sample)) < 1e-6 — silence-specific. Log-spectral metric
arrives in Phase 1+ when there's audio to compare. Generators for fixture and
baseline live under tools/golden/ and are committed for reproducibility.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `ctest --test-dir build` reports 1 test passed (`walking skeleton: silence renders to silence`). `git lfs ls-files` lists the baseline WAV.

---

## Task 7: Add AU plugin format (CLAP deferred to Phase 5)

**Files:**
- Modify: `infrastructure/juce/CMakeLists.txt`

> **Update during execution (2026-05-04):** JUCE 8.0.12 has no native CLAP support. The standard path (`free-audio/clap-juce-extensions`) introduces a new dependency that's not justified for an empty-plugin walking skeleton. CLAP is deferred to Phase 5, where it lands alongside the GUI. SPEC bumped to 1.3 in the same commit set.

- [ ] **Step 1: Update FORMATS line and fix PLUGIN_CODE collision**

In `infrastructure/juce/CMakeLists.txt`, replace:

```cmake
    PLUGIN_CODE               Vp33
    FORMATS                   VST3 Standalone
```

with:

```cmake
    PLUGIN_CODE               Vprc
    FORMATS                   VST3 AU Standalone
```

The PLUGIN_CODE change resolves a code-review finding from Task 3: hosts combine `(PLUGIN_MANUFACTURER_CODE, PLUGIN_CODE)` as a unique product identity, so the two should differ. Manufacturer stays `Vp33`.

- [ ] **Step 2: Reconfigure and build**

```bash
cmake -B build -S .
cmake --build build --target VP330_All
```

Expected (on macOS): three format targets build:
- `VP330_VST3` → `*.vst3` bundle
- `VP330_AU` → `*.component` bundle
- `VP330_Standalone` → `*.app`

- [ ] **Step 3: List the artefacts**

```bash
find build/infrastructure/juce/VP330_artefacts -maxdepth 3 -mindepth 1 -name 'VP330*' | sort
```

Expected: lines for VST3, AU, Standalone artefacts.

- [ ] **Step 4: Verify tests still pass**

```bash
ctest --test-dir build --output-on-failure
```

Expected: walking-skeleton test still passes.

- [ ] **Step 5: Commit**

```bash
git add infrastructure/juce/CMakeLists.txt
git commit -m "$(cat <<'EOF'
build: add AU plugin format; fix PLUGIN_CODE collision

VST3 + AU + Standalone build on macOS; ubuntu-24.04 will skip AU automatically.
PLUGIN_CODE was identical to PLUGIN_MANUFACTURER_CODE (both "Vp33"); hosts
combine the pair as a unique product identity, so they should differ.
PLUGIN_MANUFACTURER_CODE stays "Vp33"; PLUGIN_CODE becomes "Vprc".

CLAP deferred to Phase 5 — JUCE 8.0.12 has no native CLAP support, and the
clap-juce-extensions shim is unnecessary work for the walking skeleton.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** All four plugin format artefacts present after build.

---

## Task 8: `ci.yml` — macOS + Linux build/test matrix

**Files:**
- Create: `.github/workflows/ci.yml`

- [ ] **Step 1: Create `.github/workflows/ci.yml`**

```yaml
name: CI

on:
  push:
    branches: [main]
  pull_request:

concurrency:
  group: ci-${{ github.ref }}
  cancel-in-progress: true

jobs:
  build-test:
    name: ${{ matrix.os }}
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [macos-14, ubuntu-24.04]
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          lfs: true

      - name: Install libsndfile (macOS)
        if: runner.os == 'macOS'
        run: brew install libsndfile

      - name: Install build deps (Linux)
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            libsndfile1-dev \
            libasound2-dev \
            libjack-jackd2-dev \
            libcurl4-openssl-dev \
            libfreetype-dev \
            libfontconfig1-dev \
            libwebkit2gtk-4.1-dev \
            libgtk-3-dev \
            ladspa-sdk \
            mesa-common-dev \
            libgl1-mesa-dev

      - name: Cache FetchContent deps
        uses: actions/cache@v4
        with:
          path: build/_deps
          key: fc-${{ runner.os }}-juce8.0.12-catch3.14.0-${{ hashFiles('CMakeLists.txt') }}

      - name: Configure
        run: cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

      - name: Build all targets
        run: cmake --build build --config Release

      - name: Run tests
        run: ctest --test-dir build --output-on-failure -C Release
```

- [ ] **Step 2: Validate the YAML locally**

```bash
python3 -c "import yaml, sys; yaml.safe_load(open('.github/workflows/ci.yml'))" && echo "YAML OK"
```

Expected: prints `YAML OK`.

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/ci.yml
git commit -m "$(cat <<'EOF'
ci: build + test on macOS-14 and ubuntu-24.04

Caches FetchContent deps keyed on JUCE/Catch2 versions. Pulls LFS so the
walking-skeleton baseline WAV is present. Linux installs JUCE's full GUI
dep chain even though the plugin has no editor — JUCE links them
unconditionally.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

- [ ] **Step 4: Push and watch CI**

```bash
git push origin main
gh run watch
```

Expected: both `macos-14` and `ubuntu-24.04` jobs go green. Cold first run can take 15+ min while JUCE clones and compiles.

If it fails, inspect via `gh run view <id> --log-failed` and either fix locally or roll back. Common issues:
- Missing apt package on Ubuntu → add to the install step.
- LFS pointer not resolved → confirm `lfs: true` in checkout.

**Acceptance:** CI green on both platforms. Walking-skeleton test passes.

---

## Task 9: `arm-cross.yml` stub + `infrastructure/elk/README.md`

**Files:**
- Create: `.github/workflows/arm-cross.yml`
- Create: `infrastructure/elk/README.md`

- [ ] **Step 1: Create `infrastructure/elk/README.md`**

```markdown
# Elk Audio OS Deployment

This directory holds Elk-specific configuration: the Sushi plugin host config
and the deployment helper. Phase 0 ships only this README; Phase 6 fills in
the real artifacts.

## Cross-compile sysroot

`.github/workflows/arm-cross.yml` cross-compiles the plugin for
`aarch64-linux-gnu` against the Elk SDK sysroot. The sysroot is **not** committed
to this repo (license, size). To enable the workflow, set the `ELK_SDK_URL`
GitHub repository secret to a tarball URL. Until that secret is set, the
workflow runs as a documented no-op.

## Acquiring the Elk SDK

See <https://github.com/elk-audio/elk-pi> and
<https://github.com/elk-audio/sushi> for the current process. Targets at the
time of writing: `elk-pi 1.0.0` against `aarch64-linux-gnu` GCC 11.

## Deployment (Phase 6)

A `deploy.sh` script will scp the cross-compiled plugin to the Pi and restart
sushi. Out of scope until Phase 6.
```

- [ ] **Step 2: Create `.github/workflows/arm-cross.yml`**

```yaml
name: ARM cross-compile

on:
  push:
    branches: [main]
    tags: ['v*']
  workflow_dispatch:

jobs:
  cross-aarch64:
    name: aarch64-linux-gnu
    runs-on: ubuntu-24.04
    steps:
      - name: Checkout
        uses: actions/checkout@v4
        with:
          lfs: true

      - name: Check for Elk SDK secret
        id: check-sdk
        run: |
          if [ -z "${{ secrets.ELK_SDK_URL }}" ]; then
            echo "Elk SDK URL not configured; running as no-op stub."
            echo "available=false" >> "$GITHUB_OUTPUT"
          else
            echo "available=true" >> "$GITHUB_OUTPUT"
          fi

      - name: Stop early (no SDK)
        if: steps.check-sdk.outputs.available == 'false'
        run: |
          echo "::notice::Elk SDK secret not set. Configure ELK_SDK_URL repo secret to enable cross-compile."
          exit 0

      - name: Install cross toolchain
        if: steps.check-sdk.outputs.available == 'true'
        run: |
          sudo apt-get update
          sudo apt-get install -y g++-aarch64-linux-gnu

      - name: Fetch Elk SDK sysroot
        if: steps.check-sdk.outputs.available == 'true'
        run: |
          curl -fSL "${{ secrets.ELK_SDK_URL }}" -o sdk.tar.gz
          mkdir -p $HOME/elk-sdk
          tar -xzf sdk.tar.gz -C $HOME/elk-sdk

      - name: Configure (cross)
        if: steps.check-sdk.outputs.available == 'true'
        run: |
          cmake -B build-arm -S . \
            -DCMAKE_TOOLCHAIN_FILE=$HOME/elk-sdk/cmake/aarch64.cmake \
            -DCMAKE_BUILD_TYPE=Release

      - name: Build (cross)
        if: steps.check-sdk.outputs.available == 'true'
        run: cmake --build build-arm --config Release
```

- [ ] **Step 3: Validate YAML**

```bash
python3 -c "import yaml; yaml.safe_load(open('.github/workflows/arm-cross.yml'))" && echo "YAML OK"
```

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/arm-cross.yml infrastructure/elk/README.md
git commit -m "$(cat <<'EOF'
ci: arm-cross.yml stub + infrastructure/elk/README

Workflow is a documented no-op until the ELK_SDK_URL repo secret is set.
Phase 6 wires this up for real.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

- [ ] **Step 5: Push and verify**

```bash
git push origin main
gh run list --workflow=arm-cross.yml --limit 1
```

Expected: most recent run is green with the "no-op stub" notice.

**Acceptance:** `arm-cross.yml` runs green as a stub. `infrastructure/elk/README.md` documents the activation path.

---

## Task 10: `.clang-format`

**Files:**
- Create: `.clang-format`

- [ ] **Step 1: Create `.clang-format`**

```yaml
---
BasedOnStyle: LLVM
ColumnLimit: 100
PointerAlignment: Left
AccessModifierOffset: -2
AllowShortFunctionsOnASingleLine: InlineOnly
AllowShortIfStatementsOnASingleLine: WithoutElse
AllowShortLoopsOnASingleLine: false
BreakBeforeBraces: Attach
NamespaceIndentation: None
SpaceAfterTemplateKeyword: true
Standard: c++20
IncludeBlocks: Regroup
IncludeCategories:
  - Regex:           '^"[^/]+"'
    Priority:        1
  - Regex:           '^"[^"]+/'
    Priority:        2
  - Regex:           '^<juce_'
    Priority:        3
  - Regex:           '^<catch2/'
    Priority:        4
  - Regex:           '^<rapidcheck'
    Priority:        4
  - Regex:           '^<sndfile'
    Priority:        4
  - Regex:           '^<[a-z_]+>$'
    Priority:        5
  - Regex:           '.*'
    Priority:        6
```

- [ ] **Step 2: Run clang-format on existing C++ files**

```bash
clang-format --version  # ensure clang-format is installed (brew install clang-format on macOS)
clang-format -i \
  domain/src/stub.cpp \
  infrastructure/juce/PluginProcessor.h \
  infrastructure/juce/PluginProcessor.cpp \
  infrastructure/cli/render_main.cpp \
  tests/golden/golden_test.cpp \
  tests/golden/helpers/render.h \
  tests/golden/helpers/render.cpp \
  tests/golden/helpers/wav_io.h \
  tests/golden/helpers/wav_io.cpp
```

- [ ] **Step 3: Verify the build still works after reformat**

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build OK, test passes.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "$(cat <<'EOF'
style: add .clang-format and apply to existing sources

LLVM base, ColumnLimit 100, PointerAlignment Left, AccessModifierOffset -2
per spec §6.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `.clang-format` exists; `clang-format --dry-run -Werror <files>` exits 0.

---

## Task 11: `.clang-tidy`

**Files:**
- Create: `.clang-tidy`

- [ ] **Step 1: Create `.clang-tidy`**

```yaml
---
Checks: >
  bugprone-*,
  cert-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-identifier-length,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers,
  -cppcoreguidelines-pro-bounds-array-to-pointer-decay,
  -cppcoreguidelines-pro-bounds-pointer-arithmetic,
  -cppcoreguidelines-pro-type-vararg
WarningsAsErrors: ''
HeaderFilterRegex: '^(domain|infrastructure|tests)/.*'
FormatStyle: file
CheckOptions:
  - { key: readability-function-cognitive-complexity.Threshold, value: 25 }
```

The disables are the minimum we already know are too noisy: `use-trailing-return-type` is a stylistic-only rule we don't follow; `magic-numbers` and `identifier-length` fire constantly in DSP code; the `pro-bounds-*` and `pro-type-vararg` rules conflict with `printf`-style code in `render_main.cpp`. Per spec §6, every disable is documented inline (above) by the comment in this commit message.

- [ ] **Step 2: Run clang-tidy on existing files**

```bash
clang-tidy --version  # brew install llvm if missing; export PATH for brew's clang-tidy

# Generate compile_commands.json
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Lint all sources we've added so far
clang-tidy -p build \
  domain/src/stub.cpp \
  infrastructure/juce/PluginProcessor.cpp \
  infrastructure/cli/render_main.cpp \
  tests/golden/golden_test.cpp \
  tests/golden/helpers/render.cpp \
  tests/golden/helpers/wav_io.cpp
```

Expected: no errors. Warnings are acceptable for now; CI in Task 12 enforces no warnings on changed files only.

If clang-tidy reports issues you can't quickly fix, add the specific check to the `Checks:` disable list in `.clang-tidy` with an inline comment explaining the rationale.

- [ ] **Step 3: Commit**

```bash
git add .clang-tidy
git commit -m "$(cat <<'EOF'
lint: add .clang-tidy

Enables bugprone, cert, cppcoreguidelines, modernize, performance, readability.
Disables a small list of noisy rules with inline rationale per spec §6:
  - modernize-use-trailing-return-type: stylistic-only, not our convention
  - readability-identifier-length, *-magic-numbers: too noisy in DSP code
  - cppcoreguidelines-pro-bounds-*, -pro-type-vararg: conflict with printf
    family used in CLI rendering

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `.clang-tidy` exists; `clang-tidy -p build <files>` exits without errors.

---

## Task 12: `lint.yml` — format check, clang-tidy on changed files, domain isolation grep

**Files:**
- Create: `.github/workflows/lint.yml`

- [ ] **Step 1: Create `.github/workflows/lint.yml`**

```yaml
name: Lint

on:
  push:
    branches: [main]
  pull_request:

jobs:
  clang-format:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - name: Install clang-format
        run: |
          sudo apt-get update
          sudo apt-get install -y clang-format-18
      - name: Check formatting
        run: |
          mapfile -t files < <(find domain infrastructure tests -name '*.cpp' -o -name '*.h')
          if [ "${#files[@]}" -eq 0 ]; then
            echo "no C++ sources yet"; exit 0
          fi
          clang-format-18 --dry-run -Werror "${files[@]}"

  clang-tidy:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0
          lfs: true

      - name: Install build deps
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            clang-tidy-18 \
            libsndfile1-dev \
            libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev \
            libfreetype-dev libfontconfig1-dev libwebkit2gtk-4.1-dev \
            libgtk-3-dev ladspa-sdk mesa-common-dev libgl1-mesa-dev

      - name: Configure (compile_commands.json)
        run: cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

      - name: Determine changed files
        id: changed
        run: |
          base="${{ github.event.pull_request.base.sha || 'origin/main' }}"
          mapfile -t files < <(git diff --name-only --diff-filter=ACMR "$base...HEAD" -- '*.cpp' '*.h' || true)
          printf '%s\n' "${files[@]}" > changed.txt
          echo "count=${#files[@]}" >> "$GITHUB_OUTPUT"

      - name: Run clang-tidy on changed files
        if: steps.changed.outputs.count != '0'
        run: |
          mapfile -t files < changed.txt
          clang-tidy-18 -p build "${files[@]}"

  domain-isolation:
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4
      - name: Verify domain/ has no JUCE / sndfile / alsa includes
        run: |
          if grep -rE '#include[[:space:]]*<(juce_|sndfile|alsa)' domain/; then
            echo "::error::domain/ includes a forbidden third-party header (spec §3.1)"
            exit 1
          fi
          echo "OK: domain/ is isolated."
```

- [ ] **Step 2: Validate YAML**

```bash
python3 -c "import yaml; yaml.safe_load(open('.github/workflows/lint.yml'))" && echo "YAML OK"
```

- [ ] **Step 3: Commit and push**

```bash
git add .github/workflows/lint.yml
git commit -m "$(cat <<'EOF'
ci: lint workflow — clang-format, clang-tidy on changed, domain isolation

clang-format check is dry-run -Werror on all .cpp/.h under domain,
infrastructure, tests. clang-tidy runs on files changed vs base ref.
domain-isolation job greps for any forbidden third-party include in
domain/ — spec §3.1 invariant.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
git push origin main
gh run list --workflow=lint.yml --limit 1
```

Expected: lint job green.

**Acceptance:** `lint.yml` green on push. Adding a `#include <juce_core/juce_core.h>` to any file under `domain/` would fail the `domain-isolation` job.

---

## Task 13: rapidcheck FetchContent (no consumers yet)

**Files:**
- Modify: `CMakeLists.txt` (add rapidcheck FetchContent block)
- Modify: `tests/CMakeLists.txt` (link rapidcheck into vp330_tests)

- [ ] **Step 1: Add rapidcheck to top-level `CMakeLists.txt`**

After the `Catch2` `FetchContent_MakeAvailable` and before `enable_testing()`, insert:

```cmake
FetchContent_Declare(
    rapidcheck
    GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
    GIT_TAG b2d9ed2dddefc4b84318d664b4f221eb792d89c7
)
set(RC_ENABLE_CATCH ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidcheck)
```

`RC_ENABLE_CATCH=ON` builds the rapidcheck-Catch2 integration target (`rapidcheck_catch`) which we'll consume from L2 onwards.

- [ ] **Step 2: Link rapidcheck into `tests/CMakeLists.txt`**

Update the `target_link_libraries(vp330_tests ...)` block to:

```cmake
target_link_libraries(vp330_tests PRIVATE
    vp330_domain
    Catch2::Catch2WithMain
    rapidcheck
    rapidcheck_catch
)
```

- [ ] **Step 3: Build and run tests**

```bash
cmake -B build -S .
cmake --build build --target vp330_tests
ctest --test-dir build --output-on-failure
```

Expected: rapidcheck builds (~30 s cold), `vp330_tests` links it but doesn't reference any rapidcheck symbols yet, walking-skeleton test still passes.

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt
git commit -m "$(cat <<'EOF'
test: wire rapidcheck (commit b2d9ed2) into vp330_tests

No consumers yet — first property test arrives in Phase 2 with the
divider/octave math. Pinned by SHA because rapidcheck has no release tags.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `vp330_tests` links rapidcheck without error; tests still pass.

---

## Task 14: Pre-commit hook installer

**Files:**
- Create: `tools/install-hooks.sh`

- [ ] **Step 1: Create `tools/install-hooks.sh`**

```bash
#!/usr/bin/env bash
# Installs the project pre-commit hook into .git/hooks.
# Run once after cloning. Re-run to refresh after edits.
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
hook_path="$repo_root/.git/hooks/pre-commit"

cat > "$hook_path" <<'HOOK'
#!/usr/bin/env bash
# VP330 pre-commit hook:
#  1. clang-format on staged C++ files (auto-applied; re-stages).
#  2. domain-isolation grep on staged files under domain/.
set -euo pipefail

# 1. Format staged C++ files.
mapfile -t cpp_files < <(git diff --cached --name-only --diff-filter=ACMR \
                          | grep -E '\.(cpp|h)$' || true)
if [ "${#cpp_files[@]}" -gt 0 ]; then
    if ! command -v clang-format >/dev/null; then
        echo "pre-commit: clang-format not found; install it (brew install clang-format)." >&2
        exit 1
    fi
    clang-format -i "${cpp_files[@]}"
    git add "${cpp_files[@]}"
fi

# 2. Domain-isolation grep on staged files under domain/.
mapfile -t domain_files < <(git diff --cached --name-only --diff-filter=ACMR -- 'domain/' || true)
if [ "${#domain_files[@]}" -gt 0 ]; then
    if printf '%s\n' "${domain_files[@]}" \
       | xargs grep -l -E '#include[[:space:]]*<(juce_|sndfile|alsa)' 2>/dev/null; then
        echo "pre-commit: forbidden third-party include in domain/ (spec §3.1)." >&2
        exit 1
    fi
fi
HOOK

chmod +x "$hook_path"
echo "Installed pre-commit hook at $hook_path"
```

- [ ] **Step 2: Make the installer executable and run it**

```bash
chmod +x tools/install-hooks.sh
./tools/install-hooks.sh
```

Expected: prints `Installed pre-commit hook at .../pre-commit`.

- [ ] **Step 3: Smoke-test the hook**

Create a temporary mis-formatted file, stage it, and commit; the hook should auto-format. Then create a domain/ file with a forbidden include; the hook should reject.

```bash
# Format auto-fix smoke test:
echo 'int  foo (  )    {return 0;}' > /tmp/smoke.cpp
mv /tmp/smoke.cpp domain/src/smoke.cpp
git add domain/src/smoke.cpp
git commit -m "test smoke" --dry-run  # this won't actually invoke the hook; do a real commit
git commit -m "test smoke"
# After commit, the file should be reformatted by clang-format. Verify:
cat domain/src/smoke.cpp

# Domain isolation smoke test (this should FAIL):
cat >> domain/src/smoke.cpp <<'EOF'
#include <juce_core/juce_core.h>
EOF
git add domain/src/smoke.cpp
if git commit -m "should fail"; then
    echo "ERROR: hook did not reject forbidden include" >&2
    exit 1
else
    echo "OK: hook rejected forbidden include"
fi

# Clean up smoke test:
git reset HEAD~1 --hard
git rm -f domain/src/smoke.cpp 2>/dev/null || rm -f domain/src/smoke.cpp
```

- [ ] **Step 4: Commit**

```bash
git add tools/install-hooks.sh
git commit -m "$(cat <<'EOF'
build: pre-commit hook installer (clang-format + domain isolation)

tools/install-hooks.sh writes .git/hooks/pre-commit. Hook auto-formats
staged C++ files and re-stages them, and rejects any commit that adds a
JUCE/sndfile/alsa include under domain/ (spec §3.1 invariant).

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `./tools/install-hooks.sh` installs the hook; the smoke tests in Step 3 pass and fail as expected.

---

## Task 15: `GLOSSARY.md` and `THIRD_PARTY.md`

**Files:**
- Create: `GLOSSARY.md`
- Create: `THIRD_PARTY.md`

- [ ] **Step 1: Create `GLOSSARY.md`**

```markdown
# Glossary

This file mirrors `docs/SPEC.md` §4 "Ubiquitous Language" and is the source of
truth for terminology used in code. **Renames in code require a glossary update
in the same commit** (spec §10).

When this glossary and the spec disagree, the spec wins; reconcile by updating
this file in the same commit that fixes the discrepancy.

| Term | Meaning |
|---|---|
| **Section** | One of the three voicings: Choir, Strings, Vocoder. A complete signal path from divider output to its own audio bus. |
| **TopOctaveDivider** (TOD) | Master oscillator + 12 integer dividers producing the highest chromatic octave (C8…B8). Reference clock for all pitched sound. |
| **OctaveDivider** | Flip-flop chain (÷2, ÷4, ÷8…) producing lower octaves from a TOD output. |
| **KeyGate** | Per-key on/off switch with attack/release shaping. Sits between the divider matrix and the Section sum bus. One KeyGate per key (49 keys / 4 octaves on the MkII). |
| **Paraphonic** | Polyphony architecture in which all sounding keys share a single voicing path downstream of the KeyGates. *Not* "polyphonic." Reserved term. |
| **Ensemble** | The BBD-based three-line chorus effect. Always "Ensemble," never "Chorus." Defeatable on Choir; **always engaged on Strings**. |
| **BbdLine** | A single bucket-brigade-modeled variable delay line. The Ensemble contains three. |
| **ChoirFilterBank** | The pool of 7 fixed bandpass filters in the Choir section, from which 4 are selected to produce a given `ChoirVariant`. |
| **ChoirVariant** | One of 4 selectable choir voicings the MkII offers, expressed via the front-panel Male / Female switches. Catalog TBD pending service-manual reading. |
| **Vibrato** | Section-level pitch modulation, implemented as LFO into the TOD master clock frequency. |
| **Bender** | Pitch bend source from MIDI. Routes to TOD master clock alongside Vibrato. |
| **SynthesisEngine** | The top-level domain object. Owns the TOD, all KeyGates, all enabled Sections, and the Ensemble. |
| **ReferenceCapture** | An audio recording of the author's VP-330 MkII paired with the MIDI that produced it, used as ground truth. Lives in a private companion repo at `$VP330_CAPTURES_DIR`. |
| **ListeningReference** | A commercially released recording known to feature the VP-330, used for ear-training and aesthetic calibration. Not a test fixture. |
| **Voice** | **Banned.** Use KeyGate, Section, or Note depending on what you mean. |

Code naming: CamelCase for types, snake_case for variables and functions,
`kCamelCase` for constants.
```

- [ ] **Step 2: Create `THIRD_PARTY.md`**

```markdown
# Third-Party Dependencies

VP330 is licensed under GPL-3.0 (driven by JUCE). All dependencies must be
GPL-3-compatible. Adding a new dependency requires a row in this table and
explicit approval (spec §10).

| Dependency | License | Pin | Used in | GPL-3 compatible? |
|---|---|---|---|---|
| [JUCE](https://github.com/juce-framework/JUCE) | GPL-3.0 (or commercial) | tag `8.0.12` | `infrastructure/juce/` (plugin glue) | yes (same license) |
| [Catch2](https://github.com/catchorg/Catch2) | BSL-1.0 | tag `v3.14.0` | `tests/` | yes |
| [rapidcheck](https://github.com/emil-e/rapidcheck) | BSD-2-Clause | commit `b2d9ed2dddefc4b84318d664b4f221eb792d89c7` (no upstream tags) | `tests/` (property tests) | yes |
| [libsndfile](https://github.com/libsndfile/libsndfile) | LGPL-2.1 | system package (`brew install libsndfile`, `apt install libsndfile1-dev`) | `infrastructure/cli/`, `tests/golden/helpers/wav_io` | yes |

## How to add a dependency

1. Confirm the license is GPL-3-compatible. Common compatible licenses:
   permissive (MIT, BSD-2/3, Apache-2.0), public domain, GPL-3 (or GPL-2+),
   LGPL.
2. Pin to a specific tag or commit SHA in `CMakeLists.txt` — never a branch.
3. Add a row to the table above.
4. If the dependency is consumed under `domain/`, you have probably picked
   the wrong layer; the domain depends only on the C++ standard library.
5. Open the change as a PR and reference this file in the description.
```

- [ ] **Step 3: Commit**

```bash
git add GLOSSARY.md THIRD_PARTY.md
git commit -m "$(cat <<'EOF'
docs: GLOSSARY.md (mirrors spec §4) + THIRD_PARTY.md license inventory

GLOSSARY duplicates spec §4 to keep the source-of-truth glossary close to
where code lives. THIRD_PARTY documents JUCE 8.0.12, Catch2 v3.14.0,
rapidcheck (commit pin), libsndfile, and the process for adding deps.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** Both files exist with the content above.

---

## Task 16: `README.md` and `CONTRIBUTING.md`

**Files:**
- Create: `README.md`
- Create: `CONTRIBUTING.md`

- [ ] **Step 1: Create `README.md`**

```markdown
# VP-330 MkII Recreation

A C++20 / JUCE 8 digital recreation of the **Roland VP-330 Vocoder Plus
(MkII, 1980)**. Cross-format plugin (VST3 / AU / Standalone; CLAP added in
Phase 5) with a Raspberry Pi 4 / Elk Audio OS standalone target.

**Status:** Pre-implementation. Phase 0 (this milestone) stands up the build
system, walking-skeleton golden test, CI, and capture tooling. No DSP yet.

## Build

Prerequisites:
- CMake ≥ 3.22, a C++20 compiler.
- libsndfile (`brew install libsndfile` on macOS; `apt install libsndfile1-dev` on Ubuntu).
- For the capture tooling only: Python 3.11+.

Build everything:

```bash
cmake -B build -S .
cmake --build build
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

Plugin artefacts after build (paths may use `Debug/` instead of `Release/`):

- VST3: `build/infrastructure/juce/VP330_artefacts/Release/VST3/VP330.vst3`
- AU (macOS only): `build/infrastructure/juce/VP330_artefacts/Release/AU/VP330.component`
- Standalone: `build/infrastructure/juce/VP330_artefacts/Release/Standalone/VP330.app` (macOS) or `VP330` (Linux)
- CLAP: deferred to Phase 5 (JUCE 8.0.12 has no native CLAP support).

## Project orientation

- [`docs/SPEC.md`](docs/SPEC.md) is the contract: architecture, terminology, scope, and process.
- [`GLOSSARY.md`](GLOSSARY.md) is the project's vocabulary. Code names match it exactly.
- [`CONTRIBUTING.md`](CONTRIBUTING.md) covers commit conventions, PR process, and L1–L4 test discipline.
- [`THIRD_PARTY.md`](THIRD_PARTY.md) inventories dependencies and licenses.
- [`docs/superpowers/specs/`](docs/superpowers/specs/) holds per-phase design docs.

## License

GPL-3.0. JUCE's GPL-3 obligations apply transitively.
```

- [ ] **Step 2: Create `CONTRIBUTING.md`**

```markdown
# Contributing

Read [`docs/SPEC.md`](docs/SPEC.md) before non-trivial work. The spec wins
when it disagrees with anything else; if the spec is wrong, fix the spec
in a dedicated commit before changing code.

## Commit conventions

- **One concept per commit.** A commit should be reviewable in five minutes.
- **Glossary moves with code.** Renames in code require a glossary update in
  the same commit (spec §10).
- **Goldens are updated deliberately.** When updating a golden baseline,
  use the message form `golden: update <fixture> — <reason>`. Drift without
  justification is a red flag.
- Commits must pass:
  - the pre-commit hook (`tools/install-hooks.sh` to install once);
  - `clang-format` (CI runs it dry-run -Werror);
  - the domain-isolation grep (`domain/` may not include JUCE / sndfile / alsa);
  - all currently-passing tests.

## Pull requests

- CI must be green. Both `ci.yml` (build/test) and `lint.yml` (format,
  clang-tidy, domain-isolation).
- Reference the relevant spec section in the PR description.
- Keep PRs single-purpose. Stack instead of bundling.

## Test discipline (L1–L4)

See spec §3.3 and §7 for full detail.

- **L1 (`tests/unit/`)** — strict TDD. Every pure function in `domain/`
  ships with its L1 test in the same patch, and the test must have been
  failing before the implementation.
- **L2 (`tests/characterization/`)** — every DSP component has an L2
  contract (frequency response, time-domain behavior, stability,
  property-based invariants via rapidcheck) **before integration into
  the engine**.
- **L3 (`tests/golden/`)** — fixture MIDI rendered to WAV, compared to a
  baseline WAV (Git LFS) via log-spectral distance (silence uses
  max(abs(sample)) for now). Baselines are committed.
- **L4 (`tests/reference/`)** — A/B against the author's VP-330 captures.
  Not a CI gate; runs at phase boundaries against
  `$VP330_CAPTURES_DIR`.

## Ask, don't assume

- DSP design choices not pinned by the MkII service manual or an L2 contract.
- User-facing parameter ranges/curves.
- Anything the spec marks "calibrated by ear" — the ear is the author's.
- Whether to rename a term already in use.

## Do without asking

- Refactors that don't change an L2 contract.
- Adding missing L1 tests.
- Fixing bugs caught by L1/L2 with a regression test.
- CMake/CI hygiene.
- Doc improvements.

## Updating golden baselines

Use the deliberate flow:

```bash
# 1. Run the golden test and inspect the rendered WAV in build/.
# 2. Compare audibly / spectrally to the prior baseline. Convince yourself
#    the change is intentional.
# 3. Replace the baseline:
cp /path/to/new/render.wav tests/golden/baselines/<fixture>.wav
# 4. Commit with the explicit message form:
git commit -m "golden: update <fixture> — <reason>"
```

Never re-generate baselines as a "fix" for a failing test without
explaining what changed.
```

- [ ] **Step 3: Commit**

```bash
git add README.md CONTRIBUTING.md
git commit -m "$(cat <<'EOF'
docs: README + CONTRIBUTING

README orients new contributors (build, run, where to look). CONTRIBUTING
distills the commit conventions, PR process, L1-L4 discipline, and the
ask/don't-assume lists from spec §10.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** Both files exist; README's build instructions match what works locally.

---

## Task 17: Capture tooling scaffolding — README, schema, sample manifest

**Files:**
- Create: `tools/capture/README.md`
- Create: `tools/capture/schema.json`
- Create: `tools/capture/sample-manifest.json`

- [ ] **Step 1: Create `tools/capture/schema.json`**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://vp330.org/schemas/capture-manifest.json",
  "title": "VP-330 capture session manifest",
  "type": "object",
  "additionalProperties": false,
  "required": [
    "revision",
    "session_date",
    "session_name",
    "midi_input",
    "audio_output",
    "audio",
    "instrument_settings",
    "signal_chain"
  ],
  "properties": {
    "revision": {
      "type": "string",
      "enum": ["MkII"],
      "description": "VP-330 revision. MkI is out of scope."
    },
    "session_date": {
      "type": "string",
      "format": "date"
    },
    "session_name": {
      "type": "string",
      "minLength": 1
    },
    "midi_input": {
      "type": "string",
      "description": "Path relative to session dir, typically input.mid."
    },
    "audio_output": {
      "type": "string",
      "description": "Path relative to session dir, typically output.wav."
    },
    "audio": {
      "type": "object",
      "additionalProperties": false,
      "required": ["sample_rate", "bit_depth", "channels", "duration_seconds"],
      "properties": {
        "sample_rate": { "type": "integer", "minimum": 48000 },
        "bit_depth": {
          "oneOf": [
            { "type": "integer", "minimum": 24 },
            { "type": "string", "enum": ["float32", "float64"] }
          ]
        },
        "channels": { "type": "integer", "minimum": 1, "maximum": 2 },
        "duration_seconds": { "type": "number", "exclusiveMinimum": 0 }
      }
    },
    "instrument_settings": {
      "type": "object",
      "additionalProperties": false,
      "required": [
        "section",
        "choir_variant",
        "male_switch",
        "female_switch",
        "ensemble_on",
        "vibrato_rate",
        "vibrato_depth",
        "tone",
        "balance"
      ],
      "properties": {
        "section": {
          "type": "string",
          "enum": ["choir", "strings", "vocoder"]
        },
        "choir_variant": {
          "type": ["string", "null"],
          "description": "Free string until ChoirVariant catalog is locked (spec §11)."
        },
        "male_switch": { "type": "boolean" },
        "female_switch": { "type": "boolean" },
        "ensemble_on": { "type": "boolean" },
        "vibrato_rate": { "type": ["number", "null"] },
        "vibrato_depth": { "type": ["number", "null"] },
        "tone": { "type": ["number", "null"] },
        "balance": { "type": ["number", "null"], "minimum": -1, "maximum": 1 }
      }
    },
    "signal_chain": {
      "type": "array",
      "items": { "type": "string" },
      "minItems": 1
    },
    "room_conditions": {
      "type": "string"
    },
    "anomalies": {
      "type": "array",
      "items": { "type": "string" }
    }
  }
}
```

- [ ] **Step 2: Create `tools/capture/sample-manifest.json`**

```json
{
  "revision": "MkII",
  "session_date": "2026-05-04",
  "session_name": "choir-chromatic-variant-A-ensemble-off",
  "midi_input": "input.mid",
  "audio_output": "output.wav",
  "audio": {
    "sample_rate": 48000,
    "bit_depth": 24,
    "channels": 2,
    "duration_seconds": 12.5
  },
  "instrument_settings": {
    "section": "choir",
    "choir_variant": "variant-A-pending-name",
    "male_switch": true,
    "female_switch": false,
    "ensemble_on": false,
    "vibrato_rate": 0.0,
    "vibrato_depth": 0.0,
    "tone": 0.5,
    "balance": 0.0
  },
  "signal_chain": [
    "VP-330 MkII line out",
    "Apollo Twin DI",
    "Logic Pro X 24-bit capture"
  ],
  "room_conditions": "treated home studio, ambient noise -65 dBFS",
  "anomalies": []
}
```

- [ ] **Step 3: Create `tools/capture/README.md`**

```markdown
# Capture tooling

Tools for recording the author's VP-330 MkII into reproducible session
directories. The captures themselves are **not** in this repo — set
`VP330_CAPTURES_DIR` to point at the private companion repo / local clone.

## Layout produced

```
$VP330_CAPTURES_DIR/
├── README.md          # protocol summary (in the captures repo)
├── listening-references.md
└── sessions/
    └── YYYY-MM-DD-<short-name>/
        ├── manifest.json
        ├── input.mid
        ├── output.wav
        └── notes.md
```

`manifest.json` is validated against [`schema.json`](schema.json). See
[`sample-manifest.json`](sample-manifest.json) for a complete example.

## Tools

- [`scaffold.py`](scaffold.py) — create a new empty session directory.
- [`driver.py`](driver.py) — drive the MkII over MIDI and capture audio
  end-to-end into a session directory.
- [`validate.py`](validate.py) — walk every session, validate manifests
  and audio, report problems.

## Setup

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

export VP330_CAPTURES_DIR=/path/to/captures-repo
```

## First capture session

The Phase 0 capture deliverables (spec §8) are sessions 1–4: chromatic
scale across all four ChoirVariants, ensemble off and on. Suggested order:

1. Connect the VP-330 line outs to your audio interface (stereo, no insert).
2. Connect a USB-MIDI interface to the VP-330 MIDI IN.
3. Verify front-panel state matches the variant under capture.
4. Run:

   ```bash
   python3 driver.py \
     --name choir-chromatic-variant-A-ensemble-off \
     --midi fixtures/chromatic-49-keys.mid
   ```

5. Edit the resulting `manifest.json` to fill in the human-only settings
   (`choir_variant`, `male_switch`, `female_switch`, `ensemble_on`,
   `vibrato_*`, `tone`, `balance`, `signal_chain`, `room_conditions`).
6. Run `python3 validate.py` to confirm the session passes schema and
   audio checks.

The fixture MIDIs (chromatic scale, sustained C major, etc.) live in
`$VP330_CAPTURES_DIR/fixtures/`; create them with any MIDI tool you like.
```

- [ ] **Step 4: Validate the schema and sample together**

```bash
python3 -m pip install --user jsonschema
python3 -c "
import json, jsonschema
schema = json.load(open('tools/capture/schema.json'))
sample = json.load(open('tools/capture/sample-manifest.json'))
jsonschema.validate(sample, schema)
print('OK: sample manifest validates against schema')
"
```

Expected: prints `OK: sample manifest validates against schema`.

- [ ] **Step 5: Commit**

```bash
git add tools/capture/README.md tools/capture/schema.json tools/capture/sample-manifest.json
git commit -m "$(cat <<'EOF'
tools: capture scaffolding — README + manifest schema + sample

JSON Schema Draft 2020-12. revision is enum-locked to MkII (MkI is out of
scope, spec §1). choir_variant is a free string until the spec §11 catalog
is locked.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** Sample manifest validates against schema. README opens cleanly with the right links.

---

## Task 18: `scaffold.py`

**Files:**
- Create: `tools/capture/scaffold.py`

- [ ] **Step 1: Write `tools/capture/scaffold.py`**

```python
#!/usr/bin/env python3
"""Create a new VP-330 capture session directory under $VP330_CAPTURES_DIR/sessions."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import sys
from pathlib import Path


SLUG_RE = re.compile(r"^[a-z0-9][a-z0-9-]*$")


def manifest_template(session_date: str, session_name: str) -> dict:
    return {
        "revision": "MkII",
        "session_date": session_date,
        "session_name": session_name,
        "midi_input": "input.mid",
        "audio_output": "output.wav",
        "audio": {
            "sample_rate": 48000,
            "bit_depth": 24,
            "channels": 2,
            "duration_seconds": 0.0,
        },
        "instrument_settings": {
            "section": "choir",
            "choir_variant": None,
            "male_switch": False,
            "female_switch": False,
            "ensemble_on": False,
            "vibrato_rate": None,
            "vibrato_depth": None,
            "tone": None,
            "balance": None,
        },
        "signal_chain": ["TODO: describe the signal chain"],
        "room_conditions": "",
        "anomalies": [],
    }


def resolve_captures_dir() -> Path:
    raw = os.environ.get("VP330_CAPTURES_DIR")
    if not raw:
        sys.exit("error: $VP330_CAPTURES_DIR is not set. Point it at your captures repo.")
    p = Path(raw).expanduser()
    if not p.exists():
        sys.exit(f"error: $VP330_CAPTURES_DIR does not exist: {p}")
    return p


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True, help="Session slug, e.g. choir-chromatic-variant-A-ensemble-off")
    parser.add_argument("--date", default=None, help="Session date YYYY-MM-DD (defaults to today)")
    args = parser.parse_args()

    if not SLUG_RE.match(args.name):
        sys.exit(f"error: --name must match {SLUG_RE.pattern} (lowercase, digits, hyphens)")

    session_date = args.date or dt.date.today().isoformat()
    try:
        dt.date.fromisoformat(session_date)
    except ValueError:
        sys.exit(f"error: --date must be ISO 8601 (YYYY-MM-DD), got {session_date!r}")

    captures = resolve_captures_dir()
    session_dir = captures / "sessions" / f"{session_date}-{args.name}"
    if session_dir.exists():
        sys.exit(f"error: session already exists: {session_dir}")

    session_dir.mkdir(parents=True)
    (session_dir / "manifest.json").write_text(
        json.dumps(manifest_template(session_date, args.name), indent=2) + "\n"
    )
    (session_dir / "notes.md").write_text(f"# {args.name}\n\n_(free-form observations from the session)_\n")

    print(f"Created: {session_dir}")
    print("Next: drop input.mid, record output.wav (or use driver.py), edit manifest.json + notes.md.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Smoke-test scaffold.py**

```bash
export VP330_CAPTURES_DIR=/tmp/vp330-captures-test
mkdir -p $VP330_CAPTURES_DIR
python3 tools/capture/scaffold.py --name smoke-test
ls -la $VP330_CAPTURES_DIR/sessions/$(date +%Y-%m-%d)-smoke-test/
cat $VP330_CAPTURES_DIR/sessions/$(date +%Y-%m-%d)-smoke-test/manifest.json
```

Expected: directory created with `manifest.json` and `notes.md`. Manifest matches the template in step 1.

- [ ] **Step 3: Verify the scaffolded manifest validates against the schema**

```bash
python3 -c "
import json, jsonschema
schema = json.load(open('tools/capture/schema.json'))
manifest = json.load(open('$VP330_CAPTURES_DIR/sessions/$(date +%Y-%m-%d)-smoke-test/manifest.json'))
jsonschema.validate(manifest, schema)
print('OK')
"
```

Expected: prints `OK`.

- [ ] **Step 4: Clean up smoke test and commit**

```bash
rm -rf /tmp/vp330-captures-test
git add tools/capture/scaffold.py
git commit -m "$(cat <<'EOF'
tools: capture/scaffold.py — session-directory generator

Creates $VP330_CAPTURES_DIR/sessions/<date>-<slug>/ with a templated
manifest.json that already validates against schema.json, plus an empty
notes.md. Slug is enforced lowercase-and-hyphens.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** scaffold.py creates a valid session directory and the resulting manifest validates against the schema.

---

## Task 19: `validate.py`

**Files:**
- Create: `tools/capture/validate.py`

- [ ] **Step 1: Write `tools/capture/validate.py`**

```python
#!/usr/bin/env python3
"""Validate every session under $VP330_CAPTURES_DIR/sessions/."""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path

import jsonschema
import mido
import soundfile as sf


SCHEMA_PATH = Path(__file__).parent / "schema.json"


def resolve_captures_dir(arg: str | None) -> Path:
    raw = arg or os.environ.get("VP330_CAPTURES_DIR")
    if not raw:
        sys.exit("error: pass --root or set $VP330_CAPTURES_DIR.")
    p = Path(raw).expanduser()
    if not p.exists():
        sys.exit(f"error: captures root does not exist: {p}")
    return p


def midi_duration_seconds(path: Path) -> float:
    mid = mido.MidiFile(str(path))
    return float(mid.length)


def validate_session(session_dir: Path, schema: dict) -> list[str]:
    issues: list[str] = []
    manifest_path = session_dir / "manifest.json"

    if not manifest_path.exists():
        return [f"missing manifest.json"]

    try:
        manifest = json.loads(manifest_path.read_text())
    except json.JSONDecodeError as e:
        return [f"manifest.json is not valid JSON: {e}"]

    try:
        jsonschema.validate(manifest, schema)
    except jsonschema.ValidationError as e:
        issues.append(f"schema: {e.message} (at {'/'.join(str(p) for p in e.absolute_path)})")

    midi_name = manifest.get("midi_input", "input.mid")
    audio_name = manifest.get("audio_output", "output.wav")
    midi_path = session_dir / midi_name
    audio_path = session_dir / audio_name

    if not midi_path.exists():
        issues.append(f"missing midi_input file: {midi_name}")
    else:
        try:
            midi_dur = midi_duration_seconds(midi_path)
        except Exception as e:
            issues.append(f"midi parse failed: {e}")
            midi_dur = None
    if not audio_path.exists():
        issues.append(f"missing audio_output file: {audio_name}")
        return issues

    try:
        info = sf.info(str(audio_path))
    except Exception as e:
        issues.append(f"audio open failed: {e}")
        return issues

    audio_meta = manifest.get("audio", {})
    if info.samplerate < 48000:
        issues.append(f"audio sample_rate {info.samplerate} < 48000")
    if info.subtype not in {"PCM_24", "PCM_32", "FLOAT", "DOUBLE"}:
        issues.append(f"audio bit depth too low: {info.subtype} (need PCM_24+ or FLOAT)")
    if info.channels != audio_meta.get("channels", info.channels):
        issues.append(f"audio channels {info.channels} != manifest {audio_meta.get('channels')}")

    actual_duration = info.frames / info.samplerate
    declared_duration = audio_meta.get("duration_seconds")
    if declared_duration is not None and abs(actual_duration - declared_duration) > 0.5:
        issues.append(
            f"audio duration {actual_duration:.3f}s vs declared {declared_duration:.3f}s "
            f"(>0.5s tolerance)"
        )

    if midi_path.exists() and "midi_dur" in dir() and midi_dur is not None:
        # Allow up to 30s of tail (ensemble decay can be long).
        if actual_duration < midi_dur - 0.1:
            issues.append(
                f"audio duration {actual_duration:.3f}s is shorter than midi {midi_dur:.3f}s"
            )

    return issues


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=None, help="captures root (defaults to $VP330_CAPTURES_DIR)")
    args = parser.parse_args()

    root = resolve_captures_dir(args.root)
    sessions_root = root / "sessions"
    if not sessions_root.exists():
        print(f"no sessions/ under {root} — nothing to validate")
        return 0

    schema = json.loads(SCHEMA_PATH.read_text())

    session_dirs = sorted(p for p in sessions_root.iterdir() if p.is_dir())
    if not session_dirs:
        print(f"no sessions in {sessions_root}")
        return 0

    failures = 0
    for d in session_dirs:
        issues = validate_session(d, schema)
        if issues:
            failures += 1
            print(f"FAIL: {d.name}")
            for i in issues:
                print(f"  - {i}")
        else:
            print(f"OK:   {d.name}")

    print()
    print(f"{len(session_dirs) - failures}/{len(session_dirs)} sessions valid")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Smoke-test validate.py with a known-good session**

```bash
export VP330_CAPTURES_DIR=/tmp/vp330-captures-test
mkdir -p $VP330_CAPTURES_DIR
python3 tools/capture/scaffold.py --name validate-smoke

# The scaffolded session has no input.mid / output.wav, so validation should
# report "missing" for those — that's expected.
python3 tools/capture/validate.py --root $VP330_CAPTURES_DIR
```

Expected output ends with `0/1 sessions valid` and lists `missing midi_input file` and `missing audio_output file`. Exit code 1.

- [ ] **Step 3: Smoke-test with a fully-populated session**

Use the existing silence fixture/baseline as proxy MIDI/audio:

```bash
SESS=$VP330_CAPTURES_DIR/sessions/$(date +%Y-%m-%d)-validate-smoke
cp tests/golden/fixtures/silence-1s.mid $SESS/input.mid
cp tests/golden/baselines/silence-1s.wav $SESS/output.wav

# Update the manifest's audio.duration_seconds to ~1.0 so the tolerance check passes:
python3 -c "
import json, pathlib
p = pathlib.Path('$SESS/manifest.json')
m = json.loads(p.read_text())
m['audio']['duration_seconds'] = 1.0
p.write_text(json.dumps(m, indent=2) + '\n')
"

python3 tools/capture/validate.py --root $VP330_CAPTURES_DIR
```

Expected: `1/1 sessions valid`, exit code 0.

- [ ] **Step 4: Clean up and commit**

```bash
rm -rf /tmp/vp330-captures-test
git add tools/capture/validate.py
git commit -m "$(cat <<'EOF'
tools: capture/validate.py — manifest + audio checker

Validates each session against schema.json, parses input.mid with mido,
opens output.wav with soundfile, asserts ≥48 kHz / ≥24-bit / channel match
/ duration vs declared within 0.5s and ≥ midi length.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** validate.py reports correctly on both an incomplete session (FAIL) and a complete one (OK).

---

## Task 20: `driver.py` + `requirements.txt`

**Files:**
- Create: `tools/capture/requirements.txt`
- Create: `tools/capture/driver.py`

- [ ] **Step 1: Create `tools/capture/requirements.txt`**

```
mido==1.3.3
python-rtmidi==1.5.8
sounddevice==0.5.1
soundfile==0.12.1
jsonschema==4.23.0
numpy==2.1.3
```

These are recent stable versions at writing; they install on Python 3.11+.

- [ ] **Step 2: Install into a venv to verify**

```bash
python3 -m venv /tmp/vp330-venv
source /tmp/vp330-venv/bin/activate
pip install -r tools/capture/requirements.txt
python3 -c "import mido, sounddevice, soundfile, jsonschema, numpy; print('imports OK')"
deactivate
rm -rf /tmp/vp330-venv
```

Expected: prints `imports OK`. (`python-rtmidi` is a transitive dep of `mido` for MIDI I/O backends; the explicit pin keeps it from drifting.)

- [ ] **Step 3: Create `tools/capture/driver.py`**

```python
#!/usr/bin/env python3
"""End-to-end VP-330 capture: send MIDI to the unit, record audio, write a session dir."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

import mido
import numpy as np
import sounddevice as sd
import soundfile as sf

# Re-use scaffold helpers without duplication.
HERE = Path(__file__).parent
sys.path.insert(0, str(HERE))
from scaffold import manifest_template, resolve_captures_dir, SLUG_RE  # noqa: E402


def list_and_pick_midi_out(preselect: str | None) -> str:
    names = mido.get_output_names()
    if not names:
        sys.exit("error: no MIDI output ports available.")
    if preselect:
        for n in names:
            if preselect.lower() in n.lower():
                return n
        sys.exit(f"error: --midi-out {preselect!r} did not match any of: {names}")
    print("MIDI output ports:")
    for i, n in enumerate(names):
        print(f"  [{i}] {n}")
    idx = int(input("Pick MIDI output index: ").strip())
    return names[idx]


def list_and_pick_audio_in(preselect: str | None) -> int:
    devices = sd.query_devices()
    inputs = [(i, d) for i, d in enumerate(devices) if d["max_input_channels"] > 0]
    if not inputs:
        sys.exit("error: no audio input devices available.")
    if preselect:
        for i, d in inputs:
            if preselect.lower() in d["name"].lower():
                return i
        sys.exit(f"error: --audio-in {preselect!r} did not match any input device.")
    print("Audio input devices:")
    for i, d in inputs:
        print(f"  [{i}] {d['name']} (max in: {d['max_input_channels']})")
    return int(input("Pick audio input index: ").strip())


class RingRecorder:
    """Records to a list of np arrays via the sounddevice callback. Tracks live RMS."""

    def __init__(self, sample_rate: int, channels: int):
        self.sample_rate = sample_rate
        self.channels = channels
        self._chunks: list[np.ndarray] = []
        self._lock = threading.Lock()
        self._latest_rms_db = -120.0

    def _callback(self, indata, frames, time_info, status):  # noqa: ANN001
        if status:
            print(f"[audio status] {status}", file=sys.stderr)
        block = indata.copy()
        with self._lock:
            self._chunks.append(block)
        rms = float(np.sqrt(np.mean(block.astype(np.float32) ** 2) + 1e-30))
        self._latest_rms_db = 20.0 * np.log10(rms + 1e-30)

    def stream(self, device: int):
        return sd.InputStream(
            device=device,
            channels=self.channels,
            samplerate=self.sample_rate,
            dtype="float32",
            callback=self._callback,
        )

    @property
    def rms_db(self) -> float:
        return self._latest_rms_db

    def collect(self) -> np.ndarray:
        with self._lock:
            if not self._chunks:
                return np.zeros((0, self.channels), dtype=np.float32)
            return np.concatenate(self._chunks, axis=0)


def play_midi(port_name: str, midi_path: Path) -> None:
    midi = mido.MidiFile(str(midi_path))
    with mido.open_output(port_name) as port:
        for msg in midi.play():
            port.send(msg)


def wait_for_silence(rec: RingRecorder, *, tail_min: float, thresh_db: float,
                     window_s: float, max_tail: float) -> None:
    start = time.monotonic()
    deadline = start + max_tail
    # Mandatory minimum tail.
    time.sleep(tail_min)
    # Then poll RMS; break when we've been below threshold for window_s seconds.
    below_since: float | None = None
    while time.monotonic() < deadline:
        if rec.rms_db < thresh_db:
            if below_since is None:
                below_since = time.monotonic()
            elif time.monotonic() - below_since >= window_s:
                return
        else:
            below_since = None
        time.sleep(0.05)


def open_editor(path: Path) -> None:
    editor = os.environ.get("EDITOR")
    if not editor:
        return
    try:
        subprocess.run([editor, str(path)], check=False)
    except FileNotFoundError:
        pass


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True)
    parser.add_argument("--midi", required=True, type=Path, help="Fixture MIDI path")
    parser.add_argument("--midi-out", default=None)
    parser.add_argument("--audio-in", default=None)
    parser.add_argument("--sample-rate", type=int, default=48000)
    parser.add_argument("--channels", type=int, default=2)
    parser.add_argument("--bit-depth", type=int, default=24, choices=[24, 32])
    parser.add_argument("--tail-min", type=float, default=5.0)
    parser.add_argument("--silence-thresh-db", type=float, default=-50.0)
    parser.add_argument("--silence-window", type=float, default=1.0)
    parser.add_argument("--max-tail", type=float, default=30.0)
    parser.add_argument("--no-editor", action="store_true")
    args = parser.parse_args()

    if not SLUG_RE.match(args.name):
        sys.exit(f"error: --name must match {SLUG_RE.pattern}")
    if not args.midi.exists():
        sys.exit(f"error: --midi does not exist: {args.midi}")

    captures = resolve_captures_dir()
    session_date = dt.date.today().isoformat()
    session_dir = captures / "sessions" / f"{session_date}-{args.name}"
    if session_dir.exists():
        sys.exit(f"error: session already exists: {session_dir}")
    session_dir.mkdir(parents=True)

    shutil.copyfile(args.midi, session_dir / "input.mid")

    midi_port = list_and_pick_midi_out(args.midi_out)
    audio_dev = list_and_pick_audio_in(args.audio_in)

    rec = RingRecorder(args.sample_rate, args.channels)
    print(f"Opening audio input #{audio_dev} @ {args.sample_rate} Hz / {args.channels} ch ...")
    with rec.stream(audio_dev):
        time.sleep(0.25)  # warmup
        print(f"Sending MIDI from {args.midi.name} → {midi_port} ...")
        play_midi(midi_port, args.midi)
        print(f"Tail: min {args.tail_min}s, then silence < {args.silence_thresh_db} dB for "
              f"{args.silence_window}s, hard cap {args.max_tail}s")
        wait_for_silence(
            rec,
            tail_min=args.tail_min,
            thresh_db=args.silence_thresh_db,
            window_s=args.silence_window,
            max_tail=args.max_tail,
        )

    audio = rec.collect()
    duration = float(audio.shape[0] / args.sample_rate)
    out_wav = session_dir / "output.wav"
    subtype = "PCM_24" if args.bit_depth == 24 else "PCM_32"
    sf.write(str(out_wav), audio, args.sample_rate, subtype=subtype)
    print(f"Wrote {out_wav} ({duration:.3f}s)")

    manifest = manifest_template(session_date, args.name)
    manifest["audio"] = {
        "sample_rate": args.sample_rate,
        "bit_depth": args.bit_depth,
        "channels": args.channels,
        "duration_seconds": round(duration, 3),
    }
    (session_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    notes = session_dir / "notes.md"
    notes.write_text(f"# {args.name}\n\n_(free-form observations from the session)_\n")

    print(f"\nSession ready: {session_dir}")
    print("Edit manifest.json to fill in instrument_settings, signal_chain, room_conditions.")

    if not args.no_editor:
        open_editor(notes)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: Smoke-test driver.py argument parsing (without hardware)**

```bash
python3 tools/capture/driver.py --help
```

Expected: prints usage text without errors. (Full hardware test requires the MkII connected; that's done out-of-band by the author.)

- [ ] **Step 5: Commit**

```bash
git add tools/capture/requirements.txt tools/capture/driver.py
git commit -m "$(cat <<'EOF'
tools: capture/driver.py — full-automation MkII capture driver

Streams audio in via sounddevice, sends MIDI via mido, hybrid stop policy
(min tail + silence detect + hard cap), writes a session dir that already
satisfies schema.json. Reuses scaffold.manifest_template and slug rules.

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>
EOF
)"
```

**Acceptance:** `driver.py --help` prints usage. The author can run it against the MkII out-of-band to produce session 1.

---

## Task 21: Tag `phase-0`

**Files:** none.

- [ ] **Step 1: Verify CI is green at HEAD**

```bash
gh run list --limit 3
```

Expected: most recent runs of `CI`, `Lint`, and `ARM cross-compile` are all green.

- [ ] **Step 2: Verify the full local checklist from the design doc**

```bash
# Build + test locally one more time
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure

# Confirm artefacts (CLAP deferred to Phase 5):
test -f build/infrastructure/juce/VP330_artefacts/*/VST3/VP330.vst3/Contents/Info.plist
test -d build/infrastructure/juce/VP330_artefacts/*/AU/VP330.component
test -d build/infrastructure/juce/VP330_artefacts/*/Standalone/VP330.app
test -f build/infrastructure/cli/vp330_render
test -f build/tests/vp330_tests
echo "all artefacts present"

# Confirm domain isolation:
! grep -rE '#include[[:space:]]*<(juce_|sndfile|alsa)' domain/ && echo "domain isolated"
```

Expected: all targets exist; "all artefacts present" and "domain isolated" both print.

- [ ] **Step 3: Tag and push**

```bash
git tag -a phase-0 -m "$(cat <<'EOF'
phase-0: setup & reference-capture tooling complete

Software scaffolding (CMake / JUCE 8.0.12 plugin / vp330_render CLI /
vp330_tests with Catch2 + rapidcheck), CI on macOS-14 + ubuntu-24.04,
arm-cross stub, lint + pre-commit hooks, full docs (GLOSSARY, README,
CONTRIBUTING, THIRD_PARTY), walking-skeleton golden test passing,
Python capture tooling (scaffold + validate + driver) ready for the
author's first reference-capture session.
EOF
)"
git push origin phase-0
```

**Acceptance:** Tag `phase-0` exists locally and on the remote. Phase 0's done-list is fully checked.

---

## Self-Review

I ran the spec-coverage / placeholder / type-consistency pass after writing the plan. Issues found and fixed inline:

1. **Linker linkage** — Task 6 step 10 originally implied `target_link_libraries` directly on `vp330_tests` from a subdirectory CMake, which is fine because CMake propagates the target globally; no fix needed but the comment about not violating spec §5 is added explicitly.
2. **Double-check on render binary path under multi-config generators** — `$<TARGET_FILE:vp330_render>` works under both single- and multi-config generators; no fix needed.
3. **Tasks 6 step 12 wording** — TDD sequence: technically the test is "green from the start" because the renderer already produces silence in Task 4. The plan calls this out plainly so the reader doesn't expect a red→green→refactor cycle.
4. **Tasks 19/20 slug regex import** — `validate.py` does not import slug rules (it doesn't need them); `driver.py` does. Verified consistency.
5. **Manifest schema vs scaffold output** — `scaffold.py`'s template matches `schema.json`'s required fields. Verified by step 3 of Task 18.

Spec coverage check (against `docs/SPEC.md` §9 Phase 0 done-list):

- Repo created with §5 skeleton → Tasks 1, 2, 3, 4, 5.
- CMake building empty `vp330_domain` / `vp330_render` / `VP330` plugin (silence) → Tasks 2, 3, 4.
- CI workflows green on no-op commit → Tasks 8, 9.
- `.clang-format`, `.clang-tidy`, `.gitattributes` for LFS → Tasks 1, 10, 11.
- `GLOSSARY.md`, `README.md`, `CONTRIBUTING.md` → Tasks 15, 16. (`THIRD_PARTY.md` also added, Task 15.)
- First reference capture session committed → **out of plan scope by design** (see design doc §2 decision; tooling for it is in Tasks 17–20).
- Walking-skeleton golden test → Task 6.

All Phase 0 done-list items are covered or explicitly out-of-scope by design.

---

## Execution Handoff

**Plan complete and saved to `docs/superpowers/plans/2026-05-04-phase-0-implementation.md`. Two execution options:**

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration.

**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints.

**Which approach?**
