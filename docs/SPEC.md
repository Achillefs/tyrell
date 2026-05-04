# VP-330 Recreation — Project Specification

**Version:** 1.4
**Status:** Phase 0 implemented; Phase 1 not yet begun
**License:** GPL-3.0 (driven by JUCE GPL usage)
**Hardware Reference:** Roland VP-330 **MkII**

This document is the contract between project intent and implementation. It is the source of truth for architecture, terminology, and process. When in doubt, this document wins. When this document is wrong, update it before changing code.

This spec is written to be consumed by both human contributors and Claude Code as an implementation agent. Sections marked **[CC]** contain direct instructions to Claude Code.

---

## 1. Project Goals

### What we are building

A digital recreation of the **Roland VP-330 Vocoder Plus** (1979), faithful enough to the original architecture that it can replace the hardware in a recording or live context. The project ships as:

1. A cross-format audio plugin (VST3, AU; CLAP added in Phase 5) for desktop DAWs.
2. A standalone module for stage use, running on a Raspberry Pi 4 under Elk Audio OS, the same plugin binary hosted by Sushi.

### What "faithful" means here

The VP-330 has a specific sound that cannot be reduced to "string ensemble + chorus." Its character comes from architectural choices: **top-octave-divider polyphony**, **paraphonic filtering**, **fixed per-section voicing networks**, and the **BBD-based ensemble effect** with its specific bandwidth and modulation behavior. A "modern polysynth approximating a VP-330" is explicitly not the goal. We model the architecture, not just the spectrum.

The author owns a working **VP-330 MkII** and will use it as the reference instrument. Recordings of the real unit are the ground truth. Subjective listening by the author is the final acceptance test.

**MkI vs. MkII — this project models the MkII.** The two revisions are audibly different: chorus, human voice, filter, and vibrato circuits all changed between MkI (1979, large toggle switches, beige Jupiter-4-style keyboard) and MkII (1980, touch buttons, slimmer Jupiter-8-style keyboard). The MkII is what the author owns and what the captures will document. **Modeling the MkI is out of scope.** If a community contributor later wants a MkI variant, that is a separate `MkISection` family alongside the MkII implementations, not a reinterpretation of existing code.

### Scope (versioned)

- **v1.0** — Choir / Human Voice section, with ensemble, on desktop and Elk.
- **v2.0** — Strings section added.
- **v3.0** — Vocoder section added.
- **vNext** — Hardware enclosure, on-device screen, encoder controls. Out of scope until v3 ships.

### Non-goals

- Multitimbral / multi-section playing simultaneously is not a v1 goal. (The real unit allows it; we will add it once one section sounds right.)
- Modern features that the original lacks (per-voice envelopes, MPE, modwheel routing matrix, etc.) — explicitly off-table unless we find we need them to compensate for some aspect of the model.
- Sample-based "shortcuts." This is a synthesis project.

---

## 2. Tech Stack (Pinned)

| Layer | Choice | Notes |
|---|---|---|
| Language | C++20 | `std::span`, concepts, designated initializers used freely. No C++23 features. |
| Plugin framework | JUCE 8.x (GPL-3) | VST3 / AU / Standalone built from Phase 0; CLAP added in Phase 5 via `clap-juce-extensions` (JUCE 8.0.12 has no native CLAP support). |
| Build | CMake ≥ 3.22 | JUCE's CMake support; no Projucer. |
| Test framework | Catch2 v3 | `catch2/catch_test_macros.hpp` style. |
| Property testing | rapidcheck | For DSP invariants (filter stability, envelope monotonicity, etc.). |
| Audio I/O in tests | libsndfile | WAV read/write only. |
| Hardware target | Raspberry Pi 4 (8GB) + Elk Audio OS | Sushi plugin host. HiFiBerry DAC+ ADC Pro audio I/O. |
| Format / lint | clang-format, clang-tidy | LLVM defaults with project overrides documented in `.clang-format`. |
| CI | GitHub Actions | macOS + Linux runners. ARM cross-compile in a separate workflow. |
| LFS | Git LFS | For `reference-captures/**/*.wav` and `tests/golden/**/*.wav`. |

### License note **[CC]**

JUCE GPL-3 obligations apply transitively. The plugin must be GPL-3. Do not introduce any dependency with an incompatible license without explicit approval. When adding a new dependency, record license in `THIRD_PARTY.md`.

---

## 3. Architectural Principles

### 3.1 Hexagonal architecture (Ports & Adapters)

The codebase is split into a domain core and adapters. **The domain core does not depend on JUCE, libsndfile, ALSA, or any other I/O library.** It depends on the C++ standard library and `<cmath>`. That's it.

```text
   ┌─────────────────────────────────────────────────────┐
   │                    Adapters                         │
   │  ┌─────────┐   ┌─────────────┐   ┌──────────────┐   │
   │  │  JUCE   │   │   CLI       │   │   Elk /      │   │
   │  │ Plugin  │   │  Renderer   │   │   Sushi      │   │
   │  └────┬────┘   └──────┬──────┘   └──────┬───────┘   │
   └───────┼───────────────┼─────────────────┼───────────┘
           │               │                 │
           ▼               ▼                 ▼
   ┌─────────────────────────────────────────────────────┐
   │                       Ports                         │
   │  MidiInput · AudioOutput · ParameterStore · Clock   │
   └─────────────────────────────────────────────────────┘
                            │
                            ▼
   ┌─────────────────────────────────────────────────────┐
   │                      Domain                         │
   │   TopOctaveDivider · OctaveDivider · KeyGate        │
   │   Section (Choir/Strings/Vocoder) · Ensemble        │
   │   Modulation · SynthesisEngine                      │
   └─────────────────────────────────────────────────────┘
```

**Invariant:** any file under `domain/` whose includes contain `<juce_*.h>`, `<sndfile.h>`, `<alsa/*>`, or any third-party non-stdlib header is a bug. CI enforces this.

### 3.2 Domain-Driven Design — what we use

DDD is a toolkit; we take what fits. We use:

- **Ubiquitous language** (Section 4) — every name in code matches the language documented there. Renames in code require a glossary update in the same commit.
- **Bounded contexts** — three contexts: *Synthesis Engine* (the domain), *Host Integration* (adapters), *Reference Testing* (capture management and A/B tooling). Each lives in its own top-level directory and has its own internal vocabulary if needed.
- **Value objects** — `Pitch`, `MidiNote`, `Hertz`, `Seconds`, `Decibels`, `NormalizedParam`. Strongly typed, immutable, no implicit conversions to primitive numerics. Prevents the entire class of "is this Hz or radians? is this normalized 0-1 or dB?" bugs.
- **Pure entities and services** — most domain types are stateful processors (`KeyGate`, `BbdLine`). They have identity through their position in the engine graph, not through repository keys.

We deliberately **do not use** aggregates, repositories, or domain events. There is no persistence model, and inventing one would be cargo-cult DDD.

### 3.3 TDD — what we actually mean

Strict red-green-refactor on every line is unrealistic for DSP work. We use a four-layer test strategy where the discipline is graded:

| Layer | What it tests | Style | Example |
|---|---|---|---|
| **L1 — Unit** | Pure functions, math, state machines | Strict TDD: red → green → refactor | `divider_ratio_for_note(MidiNote::A4) == 239` |
| **L2 — Characterization** | Component behavior in audio domain | Test-first when target is mathematical, test-after when calibrated | `BbdLine` at depth=0 preserves input fundamental within ±1 Hz |
| **L3 — Golden render** | Engine end-to-end regression | Test-after; locks current behavior | Render fixture MIDI → compare WAV with spectral distance metric |
| **L4 — Reference comparison** | Plugin vs. real VP-330 | Calibration loop, reviewed by ear | Render same MIDI through plugin and through hardware; compute similarity |

Rules:

- **L1 is non-negotiable**: every commit that adds a pure function adds a test for it first. If you cannot write the test first, the code is not pure enough — extract the pure part.
- **L2 must exist for every DSP component before it is integrated** into the engine. A component without an L2 spec is a black box and will not be merged.
- **L3 baselines are committed.** Updating a golden WAV requires an explicit commit message: `golden: update <fixture> — <reason>`. Drift without justification is a red flag.
- **L4 is run at phase boundaries** (Section 9). Not on every commit. Reviewed by author against captures.

---

## 4. Ubiquitous Language

This glossary defines the project's vocabulary. Code names match this glossary exactly. CamelCase for types, snake_case for variables and functions, `kCamelCase` for constants.

| Term | Meaning |
|---|---|
| **Section** | One of the three voicings: Choir, Strings, Vocoder. A Section is a complete signal path from divider output to its own audio bus. |
| **TopOctaveDivider** (TOD) | Master oscillator + 12 integer dividers producing the highest chromatic octave (C8…B8). The reference clock for all pitched sound. |
| **OctaveDivider** | Flip-flop chain (÷2, ÷4, ÷8…) producing lower octaves from a TOD output. |
| **KeyGate** | Per-key on/off switch with attack/release shaping. Sits between the divider matrix and the Section sum bus. One KeyGate exists for every key on the keyboard (49 keys / 4 octaves on the MkII). |
| **Paraphonic** | Polyphony architecture in which all sounding keys share a single voicing path (filter, envelope shape) downstream of the KeyGates. *Not* "polyphonic." This term is reserved and load-bearing. |
| **Ensemble** | The BBD-based three-line chorus effect. Always "Ensemble," never "Chorus." Defeatable on Choir; **always engaged on Strings** (architectural fact of the original — see §9 Phase 7). |
| **BbdLine** | A single bucket-brigade-modeled variable delay line. The Ensemble contains three. |
| **ChoirFilterBank** | The pool of **7 bandpass filters** in the Choir section, from which **4 are selected** to produce a given `ChoirVariant`. Filter coefficients are fixed (per the MkII schematic); only the selection mapping varies between variants. |
| **ChoirVariant** | One of the **4 selectable choir voicings** the MkII offers, expressed via the front-panel Male / Female switches. Exact variant catalog (e.g., Male only, Female only, Mixed, etc.) and their filter selections are TBD pending service-manual reading; recorded in `domain/section/Choir.md` once confirmed. |
| **Vibrato** | Section-level pitch modulation, implemented as LFO into the TOD master clock frequency. |
| **Bender** | Pitch bend source from MIDI. Routes to TOD master clock alongside Vibrato. |
| **SynthesisEngine** | The top-level domain object. Owns the TOD, all KeyGates, all enabled Sections, and the Ensemble. Driven by MIDI events and a sample-rate clock; produces stereo audio. |
| **ReferenceCapture** | An audio recording of the author's VP-330 MkII paired with the MIDI that produced it, used as ground truth. |
| **ListeningReference** | A commercially released recording known to feature the VP-330, used for ear-training and aesthetic calibration. *Not* a test fixture — there is no MIDI pairing and no controlled signal chain. |
| **Voice** | **Banned.** Too overloaded. Use KeyGate, Section, or Note depending on what you mean. |

When new concepts emerge during implementation, propose additions to this table in the same PR that introduces them. Do not invent local jargon.

---

## 5. Repository Structure

```text
.
├── docs/SPEC.md                    # This document.
├── README.md                       # Project pitch + quickstart.
├── GLOSSARY.md                     # Source of truth: copy of Section 4, kept in sync.
├── CONTRIBUTING.md                 # Commit conventions, PR process, test discipline.
├── THIRD_PARTY.md                  # License inventory.
├── CMakeLists.txt                  # Top-level. Adds subdirs, configures targets.
├── .clang-format                   # LLVM base + project overrides.
├── .clang-tidy                     # Enforced checks.
├── .gitattributes                  # LFS rules for *.wav under reference-captures and golden.
├── .gitignore
├── .github/
│   └── workflows/
│       ├── ci.yml                  # Build + test on macOS and Linux x86_64.
│       ├── arm-cross.yml           # Cross-compile for Pi 4 / Elk.
│       └── lint.yml                # clang-format, clang-tidy, domain-isolation check.
│
├── domain/                         # CORE. No JUCE. No I/O libraries.
│   ├── CMakeLists.txt              # Builds vp330_domain (static lib).
│   ├── include/vp330/
│   │   ├── values/                 # Pitch.h, Hertz.h, Seconds.h, Decibels.h, NormalizedParam.h
│   │   ├── note/                   # MidiNote.h, KeyState.h
│   │   ├── tod/                    # TopOctaveDivider.h, OctaveDivider.h
│   │   ├── modulation/             # Lfo.h, Vibrato.h, Bender.h
│   │   ├── gate/                   # KeyGate.h
│   │   ├── section/                # Section.h (interface), ChoirSection.h, StringsSection.h, VocoderSection.h
│   │   ├── ensemble/               # BbdLine.h, Ensemble.h
│   │   ├── engine/                 # SynthesisEngine.h
│   │   └── ports/                  # MidiSource.h, AudioSink.h, ParameterStore.h, Clock.h
│   └── src/                        # Mirrors include layout.
│
├── infrastructure/                 # ADAPTERS.
│   ├── juce/                       # Plugin wrapper.
│   │   ├── CMakeLists.txt
│   │   ├── PluginProcessor.{h,cpp}
│   │   ├── PluginEditor.{h,cpp}
│   │   └── parameters/             # APVTS bindings → domain ParameterStore impl.
│   ├── cli/                        # Headless WAV renderer (used by L3 tests + ad-hoc).
│   │   ├── CMakeLists.txt
│   │   └── render_main.cpp
│   └── elk/                        # Sushi config, deployment scripts. No code yet.
│       ├── sushi-config.json
│       └── deploy.sh
│
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/                       # L1.
│   ├── characterization/           # L2.
│   ├── golden/                     # L3.
│   │   ├── fixtures/               # *.mid input.
│   │   ├── baselines/              # *.wav locked-in output (LFS).
│   │   └── golden_test.cpp         # Render fixture, compare to baseline.
│   └── reference/                  # L4 — runs against reference-captures/.
│
└── tools/
    ├── install-hooks.sh            # Installs the project pre-commit hook (clang-format + domain isolation).
    ├── golden/                     # Generators for L3 fixtures and baselines (silence-1s.{mid,wav}).
    ├── capture/                    # Python tooling that drives the MkII and writes session dirs.
    ├── ab-compare/                 # Phase 4+: render plugin + load capture, compute spectral distance.
    └── render-cli/                 # Phase 4+: convenience wrapper around infrastructure/cli.
```

**Reference captures (out-of-tree).** Captures live in a private companion repo at `$VP330_CAPTURES_DIR` (resolved §11). The L4 reference tooling reads from there; the public repo never contains a `reference-captures/` directory. Each session directory has the layout documented in §8.

### Key invariants **[CC]**

- **Domain isolation:** `grep -rE '#include.*<juce_|<sndfile|<alsa' domain/` returns zero results. CI fails the build otherwise.
- **No circular dependencies:** `infrastructure/*` depends on `domain/`. `domain/` depends on nothing in this repo.
- **Tests can link domain directly.** They do not link JUCE. Test helpers under `tests/` may link libsndfile for WAV I/O — that is permitted because the spec §5 isolation invariant restricts `domain/`, not `tests/`.
- **One public header per type.** Implementation in `.cpp` next to the corresponding `.h`'s mirror location under `src/`.

---

## 6. Build & Tooling

### CMake structure

Top-level `CMakeLists.txt` declares `project(VP330 VERSION 0.1.0 LANGUAGES C CXX)`, sets C++20, and includes `domain/`, `infrastructure/`, `tests/` as subdirectories. (`tools/` is not a CMake subdirectory — it holds shell/Python helpers.) The `C` language is required because JUCE's `juce_add_plugin` emits some C compile rules. JUCE is fetched via `FetchContent` pinned to a tag. Catch2 and rapidcheck likewise.

Targets:

- `vp330_domain` — static library, the core.
- `VP330` — JUCE plugin (VST3, AU, Standalone; CLAP added in Phase 5).
- `vp330_render` — CLI renderer executable.
- `vp330_tests` — Catch2 test runner. Links `vp330_domain` directly and never links JUCE. Test helpers may link libsndfile for WAV I/O (`tests/golden/helpers/wav_io`).

### Formatting & linting

- `.clang-format`: `BasedOnStyle: LLVM`, `ColumnLimit: 100`, `PointerAlignment: Left`, `AccessModifierOffset: -2`. Pre-commit hook runs `clang-format -i` on staged files.
- `.clang-tidy`: `bugprone-*`, `cert-*`, `cppcoreguidelines-*` (with allowances for DSP code), `modernize-*`, `performance-*`, `readability-*`. Disabled checks documented inline.

### CI workflows

- **`ci.yml`** — On every push and PR. Build matrix: macOS-14 (Apple Silicon) + ubuntu-24.04. Steps: configure → build → run unit + characterization + golden tests. Caches CMake artifacts.
- **`arm-cross.yml`** — On push to `main` and tagged releases. Cross-compiles to `aarch64-linux-gnu` against the Elk SDK sysroot. Output uploaded as artifact.
- **`lint.yml`** — clang-format check (no auto-format in CI; just verify), clang-tidy on changed files, domain-isolation grep check.

---

## 7. Testing Strategy in Detail

### L1 — Unit tests (strict TDD)

Location: `tests/unit/`. One test file per domain header.

Examples of what L1 covers:

- `Hertz` value type arithmetic and conversions.
- `MidiNote` ↔ `Hertz` conversion.
- `divider_ratio_for_note(midi_note)` integer math.
- `KeyGate` state machine: Idle → AttackRamp → Sustain → ReleaseRamp → Idle, with sample-by-sample envelope output verified.
- `Lfo` phase increment, wraparound, output range.

Every L1 test runs in < 10 ms. Total L1 suite runtime budget: < 5 seconds. Run on every save during dev (via watcher) and in pre-commit hook.

### L2 — Characterization tests

Location: `tests/characterization/`. One file per DSP component.

Each component must specify its *contract* in test form before integration:

- Frequency response (magnitude at key frequencies, within tolerance).
- Time-domain behavior (impulse response shape, attack time, settling time).
- Stability under parameter sweeps (no NaN, no clipping at unity input, output bounded).
- Property-based: for any valid parameter, certain invariants hold. Use rapidcheck.

Example contract for `BbdLine`:

```text
Given:  BbdLine(sample_rate=48000)
When:   set modulation_depth = 0
And:    feed a 440 Hz sine at unity for 1 s
Then:   output peak fundamental is 440 Hz ± 1 Hz
And:    output rms is within 1 dB of input rms
And:    output bandwidth is within the spec'd cutoff (e.g., -3 dB at 6 kHz ± 500 Hz)
```

L2 runtime budget: < 30 seconds total. Runs in CI on every push.

### L3 — Golden render tests

Location: `tests/golden/`.

Procedure:

1. Fixed MIDI files in `tests/golden/fixtures/` (e.g., `single-c4-1s.mid`, `c-major-chord-2s.mid`, `chromatic-scale.mid`).
2. Test renders the engine to a WAV at 48 kHz / 24-bit.
3. Compares to a baseline WAV in `tests/golden/baselines/` (LFS) using a perceptual-ish metric: log-spectral distance per frame, averaged. Threshold per fixture, configurable.
4. On failure, the test writes the diff WAV to a build artifact for inspection.

Baselines are updated only with explicit human review. The test CLI supports `--update-baseline <fixture>` to regenerate, but doing so is a deliberate act, committed with a message explaining what changed and why.

### L4 — Reference comparison

Location: `tests/reference/`. Not run in CI; run on demand and at phase boundaries.

Procedure:

1. Pick a session from `reference-captures/sessions/`.
2. Render the same MIDI through the plugin with documented settings.
3. Compute spectral and envelope similarity metrics.
4. Output a report (HTML or markdown) with metric values and waveform/spectrogram overlays.
5. Author listens, A/Bs, decides what's acceptable for the current phase.

This is a calibration tool, not a pass/fail gate. Its output informs which DSP component needs more work next.

---

## 8. Reference Capture Protocol

The author's VP-330 is the source of truth. Captures are precious and must be reproducible. **[CC]** — never modify or regenerate files under `reference-captures/`.

### Capture session structure

Each session is a directory `reference-captures/sessions/YYYY-MM-DD-<short-name>/` containing:

- `manifest.json` — instrument **revision (MkII assumed; record explicitly)**, settings (section selected, ChoirVariant / Male+Female switch state, ensemble on/off, vibrato rate/depth, tone, balance), signal chain (DI box? mic? preamp? converter?), MIDI input filename, capture sample rate and bit depth, room conditions, any anomalies noted.
- `input.mid` — exact MIDI input.
- `output.wav` — the recording. 48 kHz / 24-bit minimum, stereo if the unit was operated in stereo.
- `notes.md` — free-form author observations.

### Required initial captures (Phase 0 deliverable)

All captures are of the author's **MkII**. Before serious DSP work begins, capture at minimum:

1. **Chromatic scale, choir section, each ChoirVariant, ensemble off, no vibrato** — one pass per Male/Female switch combination (4 passes). Single section, dry.
2. **Chromatic scale, choir section, each ChoirVariant, ensemble on, no vibrato** — same input, ensemble engaged. 4 passes.
3. **Sustained C major triad, choir section, each ChoirVariant, ensemble off** — for spectral analysis of the static voicing across variants.
4. **Single C4, choir section (default variant), ensemble off, vibrato sweep** — vibrato rate min → max while held.
5. **Strings section: chromatic scale and sustained C major triad.** Note that ensemble cannot be defeated on strings; capture the section as it actually exists. Capture both with and without vibrato.
6. **Single sustained note bursts at every C across the keyboard, choir section (one variant)** — for per-note voicing analysis. The fixed filter network may behave differently across registers, and the ChoirFilterBank selection may interact with key position.
7. **Front-panel attack/release controls swept** — single sustained note, attack control min → max, release control min → max. Documents the envelope shape range.

Total required for Phase 0: ~25–35 minutes of audio. Larger than the original v1.0 estimate because the ChoirVariant axis multiplies items 1–3 by 4.

### External listening references

These are **commercially released recordings** known to feature the VP-330. They are not test fixtures (no MIDI pairing, unknown signal chain, post-production processing applied) but they are valuable for ear-training and for setting aesthetic targets. Maintained as a markdown list in `reference-captures/listening-references.md`:

- **Vangelis — "Tears in Rain"** (*Blade Runner*, 1982). Choir + Strings + Ensemble. *The* reference for the v1 target sound.
- **Ozzy Osbourne — "Mr. Crowley"** (1980, Don Airey). Choir, Male + Female switches, ensemble on, doubled with CS-80.
- **Laurie Anderson — "O Superman"** (1981). Vocoder + Strings. Reference for v3.
- **Goldfrapp — "Number One"** (2005). Choir.
- **Beach House — *Once Twice Melody*** (2022). Choir textures.
- **Dua Lipa — "Levitating"** (2020). Strings/choir chord stab; producer Koz documented the part on Song Exploder.

When something in the model sounds wrong but the metrics look fine, listen to these. When the metrics look wrong but it sounds right, also listen to these.

### Privacy / publishing

Reference captures of the author's instrument are **personal recordings**. Decide before pushing to GitHub whether the public repo will include them, or whether they live in a private companion repo. Default assumption: **private companion repo**, referenced by a `.gitmodules` or environment variable. The public repo's `tests/reference/` is a no-op without the captures present.

---

## 9. Phased Roadmap

Each phase has a definition-of-done. A phase is not complete until every item is checked. Phase boundaries are commit-tag-worthy moments.

### Phase 0 — Setup & Reference Captures

**Goal:** make the rest of the project possible.

Deliverables:

- [x] Repo created with the Section 5 skeleton.
- [x] CMake building an empty `vp330_domain`, an empty `vp330_render`, an empty `VP330` plugin (just generates silence).
- [x] CI workflows green on a no-op commit.
- [x] `.clang-format`, `.clang-tidy`, `.gitattributes` for LFS in place.
- [x] `GLOSSARY.md`, `README.md`, `CONTRIBUTING.md` written.
- [ ] First reference capture session committed to the captures repo (Section 8 list, items 1–4 minimum). _Pending: tooling shipped at `tools/capture/` (scaffold + driver + validate) but the recording itself is a hardware task on the author's MkII; the session lives in the private companion repo at `$VP330_CAPTURES_DIR`._
- [x] One walking-skeleton golden test passing: render a 1-second silence, compare to a 1-second silence baseline. Proves the test plumbing works.

### Phase 1 — Skeleton Engine

**Goal:** MIDI in → naive square-wave polyphonic out, end-to-end through both adapters.

- [ ] `MidiNote`, `Hertz`, `Pitch` value types with full L1 coverage.
- [ ] `MidiSource`, `AudioSink`, `Clock` ports defined.
- [ ] `SynthesisEngine` skeleton: takes MIDI events, produces stereo audio. Internally just one-square-per-key, summed.
- [ ] JUCE adapter: receives MIDI, calls engine, fills audio buffer. Plugin loads in a host.
- [ ] CLI adapter: reads `.mid`, writes `.wav`. Used by golden tests.
- [ ] Golden test: render `single-c4-1s.mid` → check fundamental at ~261.6 Hz, RMS in expected range.
- [ ] Tag: `phase-1`.

### Phase 2 — TopOctaveDivider Architecture

**Goal:** the actual VP-330 keyboard engine, minus voicing and ensemble.

- [ ] `TopOctaveDivider` with master clock + 12 integer dividers. L1 covers divider math.
- [ ] `OctaveDivider` chains. L1 covers octave math.
- [ ] `KeyGate` with attack/release. L1 covers state machine; L2 covers envelope shape and click suppression.
- [ ] `SynthesisEngine` rewired to use TOD + KeyGates instead of the Phase 1 naïve oscillators.
- [ ] L2: tuning across the full keyboard within ±1 cent of equal temperament (and document the deviations from equal temperament that are *intrinsic* to integer top-octave division — these are part of the VP-330's character and we keep them).
- [ ] **L4 checkpoint:** sounds like a cheap organ. That's correct. A/B vs. capture #1 should show same fundamentals, very different timbre.
- [ ] Tag: `phase-2`.

### Phase 3 — Choir Section

**Goal:** the v1 instrument minus the ensemble. **MkII-specific.**

- [ ] Reverse-engineer the **MkII** choir voicing filter network from the service manual. The Choir section uses a `ChoirFilterBank` of 7 fixed bandpass filters; each `ChoirVariant` selects 4 of them. Document findings in `domain/include/vp330/section/Choir.md`: filter topologies, center frequencies, Q values, and the variant→selection mapping table.
- [ ] `ChoirFilterBank` value/component holding the 7 fixed filters. L1 covers the coefficient math; L2 covers per-filter magnitude response.
- [ ] `ChoirVariant` enum + selection mapping. L1 covers the mapping (each variant returns exactly 4 filter indices, and the catalog matches the manual).
- [ ] `ChoirSection` implementing the `Section` interface — sums the 4 selected filter outputs for the active variant. L2 covers the composite frequency response per variant against the corresponding capture.
- [ ] Section-level vibrato (LFO into TOD master clock). L1 covers LFO; L2 covers the resulting pitch deviation.
- [ ] Front-panel attack/release controls modeled (range and curve from capture #7).
- [ ] `SynthesisEngine` exposes the section + variant selection in its parameter store.
- [ ] **L4 checkpoint:** A/B against capture #1 for **each** ChoirVariant (chromatic, ensemble off). Spectral envelope should match within tolerance (define tolerance based on first measurements). Author signs off perceptually on all variants before the phase closes.
- [ ] Tag: `phase-3`.

### Phase 4 — Ensemble

**Goal:** v1.0 — the instrument sounds like a VP-330.

- [ ] `BbdLine`: variable-rate delay line with bandwidth limiting. L1 covers interpolation math; L2 covers fundamental preservation, bandwidth, and delay accuracy.
- [ ] `Ensemble`: three `BbdLine`s with quadrature LFO modulation. L2 covers stereo decorrelation, modulation depth, and rate.
- [ ] Optional: companding model. Decide based on L4 listening — only add if A/B reveals it's needed.
- [ ] `SynthesisEngine` integrates `Ensemble` post-Section.
- [ ] **L4 checkpoint:** A/B against capture #2 (chromatic, ensemble on). Author signs off perceptually. This is the v1.0 acceptance gate.
- [ ] Tag: `v1.0-rc1`.

### Phase 5 — Polish (v1.0)

- [ ] GUI in JUCE: modern flat layout, VP-330 color scheme. Section selection, ensemble bypass, vibrato controls, level controls.
- [ ] Preset save/load via JUCE's APVTS state.
- [ ] MIDI CC mapping for live-relevant params (level, vibrato depth, ensemble bypass).
- [ ] Plugin validates with `pluginval` at strictness level 8+ on macOS and Linux.
- [ ] **CLAP format added** via the `clap-juce-extensions` shim (MIT, GPL-3-compatible). Deferred from Phase 0 because JUCE 8.0.12 has no native CLAP support and the walking-skeleton goal does not benefit from CLAP. Add the dependency, record in `THIRD_PARTY.md`, ship a `VP330_CLAP` target, and verify load in a CLAP host.
- [ ] Tag: `v1.0`.

### Phase 6 — Elk Deployment

- [ ] Cross-compile toolchain working in `arm-cross.yml`.
- [ ] Sushi JSON config in `infrastructure/elk/`.
- [ ] Deployment script: builds plugin, scps to Pi, restarts Sushi.
- [ ] Documented audio I/O setup: HiFiBerry hat, sample rate, buffer size, measured round-trip latency.
- [ ] Author plays the Pi standalone from an external MIDI keyboard. Acceptance: feels playable live.
- [ ] Tag: `v1.0-elk`.

### Phase 7 — Strings Section (v2.0)

Compress repeats of Phases 3–4 with `StringsSection`. Reuse `Ensemble` (parameters tuned for strings).

**Architectural note — Ensemble is non-defeatable on Strings.** This is a real characteristic of the original instrument: the strings preset always passes through the ensemble effect, with no front-panel bypass. Preserve this in the model. The `StringsSection`'s output port routes unconditionally through `Ensemble`, regardless of any user-facing "ensemble bypass" parameter (which only affects Choir). This keeps the architecture honest and the strings sound right; users who want dry strings can use a different plugin.

### Phase 8 — Vocoder Section (v3.0)

Its own design phase. Out of scope for this version of the spec; will get a dedicated update before starting.

---

## 10. Working with Claude Code **[CC]**

This section is direct guidance to Claude Code as the implementing agent.

### Operating principles

1. **Plan before code, especially at phase starts.** When asked to start a phase, propose a sequenced task list and wait for confirmation before writing implementation code. Do not rebuild the whole phase in one pass.
2. **Test first for L1.** When writing any function in `domain/`, the corresponding L1 test must be in the same patch and must have been runnable-and-failing before the implementation. If you cannot construct a failing test for a chunk of code, that chunk likely belongs in an adapter, not the domain.
3. **One concept per commit.** A commit should be reviewable in five minutes. Do not bundle "added KeyGate + fixed CMake + renamed Hertz."
4. **Update the glossary in the same commit as the rename.** Diverging glossary and code is a project-wide failure mode; prevent it at commit time.
5. **Domain purity is a hard constraint.** If you find yourself wanting to include JUCE in the domain "just for this," stop and ask. There is always a port-shaped solution.
6. **Ask before introducing dependencies.** New third-party libraries require a `THIRD_PARTY.md` entry and explicit approval — they affect license and embedded footprint.
7. **Reference captures are read-only.** Never modify, regenerate, or delete files under `reference-captures/`. If a capture seems wrong, surface it to the author; do not fix it.
8. **Goldens are updated deliberately.** Never run `--update-baseline` as a "fix" for a failing test without first explaining what changed and getting confirmation. A drifting golden is the canary; do not silence it.
9. **When the spec is wrong, fix the spec first.** If implementation reveals that the spec is mistaken or unclear, update `SPEC.md` in a dedicated commit before changing code.

### Things to ask, not assume

- DSP design choices that aren't pinned by the service manual or by an L2 contract (e.g., specific filter topologies, ensemble modulation depth defaults).
- Parameter ranges and curves exposed to the user.
- Anything where the spec says "calibrated by ear" — the ear is the author's, not yours.
- Whether to keep or change a name once it is in use.

### Things you can do without asking

- Refactors within a domain component that don't change its L2 contract.
- Adding L1 tests for code that lacks them.
- Fixing bugs caught by L1/L2 tests, with a regression test added.
- CMake / CI hygiene.
- Documentation improvements.

---

## 11. Open Questions / Decisions Deferred

Recorded here so they don't get lost:

- **Service manual specifics (MkII).** Phase 0 includes a manual-reading session to nail: TOD master frequency, exact divider ratios, the 7 ChoirFilterBank filter coefficients, the 4 ChoirVariant → filter selection mapping, ensemble LFO rates and depths, vibrato LFO range, and front-panel attack/release control curves. These will be added as concrete numbers to the relevant phase docs. Use the **MkII** service manual specifically — MkI numbers are not interchangeable.
- **ChoirVariant naming.** Wikipedia documents 4 selectable choir sounds; the author's MkII front panel has Male and Female switches. The exact catalog (Male only / Female only / both / off-the-front-panel hidden combination?) and their official names are TBD pending manual reading and front-panel inspection. Once confirmed, the enum values in `domain/section/Choir.h` are locked.
- **Ensemble companding.** Whether to model it depends on what the Phase 4 L4 listening reveals. Default: skip until proven necessary.
- **Reference captures repo placement.** ~~Public + LFS, or private companion repo. Author decides during Phase 0.~~ **Resolved (2026-05-04, Phase 0 design):** private companion repo, linked via the environment variable `VP330_CAPTURES_DIR`. The public repo does not contain `reference-captures/`. L4 reference tests are no-ops when `VP330_CAPTURES_DIR` is unset. Capture tooling lives in the public repo under `tools/capture/`.
- **Pitch-bend range and behavior.** The original may not implement bender at all in the way modern players expect. To be confirmed against the MkII manual.
- **Vocoder section: separate carrier input on MkII.** Phase 8 (vocoder) will need to decide how MIDI/audio carrier routing maps onto the original's external-carrier conventions. Out of scope until v3.
- **Hardware enclosure for v3+.** Out of scope; revisit after vocoder ships.

---

## 12. Change Log

| Date       | Version | Change |
|------------|---------|--------|
| 2026-05-04 | 1.0     | Initial specification. |
| 2026-05-04 | 1.1     | Pinned hardware reference to **MkII** (author's unit). Added `ChoirFilterBank` and `ChoirVariant` to ubiquitous language; rewrote Phase 3 to reflect 7-filter pool / 4-selection-per-variant architecture. Documented Ensemble as non-defeatable on Strings (Phase 7). Corrected keyboard size (49 keys / 4 octaves on MkII). Expanded Phase 0 capture protocol to cover all ChoirVariants. Added external listening references list. MkI explicitly out of scope. |
| 2026-05-04 | 1.2     | Resolved §11 captures-repo-placement question: private companion repo, linked via `VP330_CAPTURES_DIR` env var. Recorded alongside Phase 0 design doc (`docs/superpowers/specs/2026-05-04-phase-0-design.md`). |
| 2026-05-04 | 1.3     | Defer CLAP format to Phase 5 — JUCE 8.0.12 lacks native CLAP support; using the `clap-juce-extensions` shim now is unnecessary work for Phase 0's walking-skeleton goal. Phase 0 ships VST3 / AU / Standalone; Phase 5 adds CLAP alongside the GUI. §1, §2, §6, Phase 5 done-list updated. |
| 2026-05-04 | 1.4     | Phase 0 final-review corrections, all observed during `phase-0-impl` execution: §5 tools/ tree updated to reflect what was actually delivered (`install-hooks.sh`, `golden/`, `capture/`); ab-compare and render-cli marked Phase 4+. §5 invariant on tests clarified — test helpers may link libsndfile (the isolation rule only binds `domain/`). §6 CMake project signature corrected to `project(VP330 VERSION 0.1.0 LANGUAGES C CXX)` — `C` is required by JUCE's `juce_add_plugin`. §6 vp330_tests description updated to match the build graph. |
