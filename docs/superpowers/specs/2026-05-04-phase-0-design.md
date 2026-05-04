# Phase 0 Design — Setup & Reference Captures

**Status:** Approved (brainstorm complete, awaiting implementation plan).
**Spec reference:** `docs/SPEC.md` §9 Phase 0.
**Date:** 2026-05-04.

This document is the design output of the brainstorming session for Phase 0 of the VP-330 MkII recreation. It describes the target shape of the repo at the end of Phase 0, the commit sequence, and the per-area design for each deliverable. The implementation plan that follows turns this into ordered tasks.

---

## 1. Phase 0 Goals (recap from spec §9)

Phase 0 makes the rest of the project possible. Its definition-of-done from the spec:

- Repo with the §5 directory skeleton.
- CMake building empty `vp330_domain`, `vp330_render`, and `VP330` plugin (silence).
- CI workflows green on a no-op commit.
- `.clang-format`, `.clang-tidy`, `.gitattributes` LFS rules.
- `GLOSSARY.md`, `README.md`, `CONTRIBUTING.md` written.
- One walking-skeleton golden test passing (1 s silence rendered → compared to 1 s silence baseline).
- *(Spec also lists a first reference-capture session; see §2 below for how that's split.)*

## 2. Decisions Made During Brainstorm

The brainstorm resolved the following choices. Each is treated as binding for the implementation plan.

| Decision | Choice | Notes |
|---|---|---|
| Phase 0 scope | Software scaffolding **+** capture tooling. **No** capture session. | Recording happens out-of-band on author's hardware after Phase 0 ships. |
| Reference-captures repo placement | Private companion repo, linked via env var `VP330_CAPTURES_DIR`. | Closes spec §11 open question. Spec §11 to be updated in same commit as this doc lands. |
| Capture-tooling depth | Full automation (driver script). | Schema + scaffolder + validator + driver. |
| Capture-tooling runtime | Python 3.11+. | `mido`, `python-rtmidi`, `sounddevice`, `soundfile`, `jsonschema`, `numpy`. |
| Capture-tooling stop policy | Hybrid: fixed minimum tail + silence detection up to a hard cap. | Implemented in-process via `numpy` RMS over the live ring buffer. |
| Plugin formats from Phase 0 | All four (VST3, AU, CLAP, Standalone) where the platform supports them. | AU only on macOS. |
| `rapidcheck` and pre-commit hooks | Both wired up in Phase 0. | rapidcheck has no consumers yet (first L2 test arrives in Phase 2/3). Hooks live-validated in 0c. |
| Build sequence | Walking-skeleton-first (Approach B). | Get render→compare→CI green before adding lint/docs/tooling. |
| Repo visibility | Public (resolved live during brainstorm). | Unmetered GH Actions minutes; full matrix stands. |

## 3. Target Shape at End of Phase 0

When Phase 0 closes, the repo contains:

**Working software:**
- `vp330_domain` static library — public headers under `domain/include/vp330/` per spec §5; no implementation yet (one stub TU to satisfy the linker).
- `VP330` JUCE plugin — VST3/AU/CLAP/Standalone on macOS, VST3/CLAP/Standalone on Linux. Loads in any host. `processBlock` writes silence.
- `vp330_render` CLI — `--input <mid> --output <wav> --duration <sec>`, writes a 48 kHz / 24-bit WAV. Output is silence.
- `vp330_tests` — Catch2 binary, links `vp330_domain` only (per spec §5 invariant), contains the walking-skeleton golden test.

**CI green:**
- `ci.yml` — macOS-14 + ubuntu-24.04 matrix; build + test on each. Walking-skeleton test passes on both.
- `arm-cross.yml` — committed; active if the Elk SDK sysroot is available, otherwise a documented no-op stub for Phase 6 to enable.
- `lint.yml` — `clang-format --dry-run`, `clang-tidy` on changed files, **domain-isolation grep check**.

**Docs:**
- `README.md`, `GLOSSARY.md` (mirrors spec §4), `CONTRIBUTING.md`, `THIRD_PARTY.md`.

**Capture tooling (no captures committed):**
- `tools/capture/` — Python driver, manifest JSON Schema, validator, session scaffolder, README, `requirements.txt`.
- `reference-captures/` does **not** exist in the public repo. Its layout is documented at the path `$VP330_CAPTURES_DIR` points to (in the private companion repo).

**Lint, hooks, LFS:**
- `.clang-format`, `.clang-tidy`, `.gitattributes` (LFS rules for `*.wav` under `tests/golden/` and `reference-captures/`), `tools/install-hooks.sh`.

**Explicitly NOT in Phase 0:**
- Any DSP code, `Hertz` / `MidiNote` value types (Phase 1).
- `KeyGate`, dividers, sections, ensemble (Phases 2–4).
- Reference captures themselves (recorded out-of-band).
- GUI work (Phase 5).

## 4. Build Order — Commit Sequence

Walking-skeleton-first, organized into six sub-phases. Each numbered item is one commit (one concept per commit, per spec §10). ~21 commits total.

### Phase 0a — Walking-skeleton green locally

1. Repo skeleton: §5 directory layout, `.gitignore`, `.gitattributes` (LFS for `*.wav` under `tests/golden/` and `reference-captures/`).
2. Top-level `CMakeLists.txt` (`project(VP330 CXX)`, C++20) + empty `vp330_domain` static library (one stub TU).
3. JUCE FetchContent pinned tag + empty `VP330` plugin, **VST3+Standalone only** at this step, `processBlock` writes silence.
4. Empty `vp330_render` CLI: argument parsing, libsndfile-backed WAV writer of silence.
5. Catch2 v3 FetchContent + `vp330_tests` target with one trivial passing test.
6. **Walking-skeleton golden test passing locally**: `silence-1s.mid` fixture (empty MIDI, 1 s long), `silence-1s.wav` baseline in LFS, comparison = `max(abs(sample)) < 1e-6`.

### Phase 0b — CI green across the matrix

7. Add AU + CLAP plugin formats.
8. `ci.yml`: macOS-14 + ubuntu-24.04 matrix; walking-skeleton passes on both.
9. `arm-cross.yml` committed (active if Elk SDK sysroot available; otherwise a documented no-op stub).

### Phase 0c — Lint + hooks

10. `.clang-format` (LLVM base + project overrides per spec §6).
11. `.clang-tidy` (enabled categories per spec §6; no disables initially).
12. `lint.yml`: format check, clang-tidy on changed files, domain-isolation grep check.
13. rapidcheck FetchContent (linked into `vp330_tests`, no consumers yet).
14. `tools/install-hooks.sh` + pre-commit hook (clang-format on staged C++ files + domain-isolation grep).

### Phase 0d — Docs

15. `GLOSSARY.md` (mirrors spec §4) + `THIRD_PARTY.md`.
16. `README.md` + `CONTRIBUTING.md`.

### Phase 0e — Capture tooling (Python)

17. `tools/capture/README.md` + `schema.json` + a sample manifest.
18. `tools/capture/scaffold.py` — generates a new session directory.
19. `tools/capture/validate.py` — manifest + file validator.
20. `tools/capture/driver.py` + `requirements.txt` — capture driver.

### Phase 0f — Phase close

21. Tag `phase-0`.

### Notes on ordering

- LFS rules ship in commit 1 so the baseline WAV in commit 6 is correctly LFS-tracked.
- AU/CLAP land *after* the walking-skeleton is locally green (commit 6) but *before* CI (commit 8), so all four formats are validated together on macOS via CI.
- rapidcheck (13) lands as pure plumbing — no consumer until Phase 2/3.
- Capture tooling is last because it has zero dependencies on the rest of Phase 0.

## 5. Walking-Skeleton Golden Test

The smallest meaningful end-to-end test that exercises the full L3 pipeline: render → write WAV → load baseline WAV → compare.

**Fixture (`tests/golden/fixtures/silence-1s.mid`):**
A type-0 SMF with one track, one tempo event (120 BPM), one end-of-track at tick 1920, no note events. Effective duration: 1.0 s. Generated once by a small script and committed (~30 bytes; not LFS).

**Baseline (`tests/golden/baselines/silence-1s.wav`):**
48 000 stereo 24-bit-PCM frames of zeros (~288 KB), matching the renderer's output format (spec §7 specifies L3 renders at 48 kHz / 24-bit). LFS-tracked. Generated by a one-shot script committed alongside it for reproducibility.

**Test body (`tests/golden/golden_test.cpp`):**

```cpp
TEST_CASE("walking skeleton: silence renders to silence", "[golden]") {
    auto wav = render_fixture("silence-1s.mid", /*sr=*/48000, /*duration=*/1.0);
    auto baseline = load_wav(BASELINES_DIR "/silence-1s.wav");
    REQUIRE(wav.frames == baseline.frames);
    REQUIRE(wav.channels == baseline.channels);
    REQUIRE(wav.sample_rate == baseline.sample_rate);
    REQUIRE(max_abs_sample(wav) < 1e-6f);
    REQUIRE(max_abs_sample(baseline) < 1e-6f);
}
```

**Comparison metric — why `max_abs_sample` not log-spectral distance:**
Log-spectral distance (the eventual L3 metric per spec §3.3) has log-of-zero issues on pure silence. Phase 0 uses `max_abs_sample < ε` as a silence-specific predicate. The full log-spectral comparator is a Phase 1+ deliverable, plugged into the same fixture/baseline plumbing.

**What this proves:**
- `vp330_render` builds and links libsndfile.
- The render path writes a valid WAV at the requested format.
- LFS is configured correctly (baseline downloads on clone).
- Catch2 + the test harness work.
- CI can resolve LFS pointers on both macOS and Linux runners.

**What this deliberately does NOT prove:**
- That the engine produces correct audio for non-silent input (Phase 1+).
- That log-spectral comparison works (Phase 1+).
- That MIDI is parsed (no notes in the fixture; renderer just emits N samples of silence).

**Generation scripts:**
`tools/golden/gen-silence-fixture.py` writes the MIDI; `tools/golden/gen-silence-baseline.py` writes the WAV. Committed with the fixtures for reproducibility.

## 6. Capture Tooling

All under `tools/capture/`. Python 3.11+. Dependencies pinned in `requirements.txt`: `mido`, `python-rtmidi`, `sounddevice`, `soundfile`, `jsonschema`, `numpy`.

### 6.1 Manifest JSON Schema (`schema.json`)

JSON Schema Draft 2020-12. Required fields:

- `revision` — enum `["MkII"]`. Enum prevents silent drift to MkI (which is out of scope per spec §1).
- `session_date` — ISO 8601 date.
- `session_name` — short slug.
- `midi_input` — filename relative to session dir (typically `input.mid`).
- `audio_output` — filename (typically `output.wav`).
- `audio` — object: `sample_rate` (≥48000), `bit_depth` (≥24 or `"float32"`), `channels`, `duration_seconds`.
- `instrument_settings` — object: `section` (`"choir" | "strings" | "vocoder"`), `choir_variant` (free string until spec §11 open question is closed; then locked to enum), `male_switch` (bool), `female_switch` (bool), `ensemble_on` (bool), `vibrato_rate`, `vibrato_depth`, `tone`, `balance`.
- `signal_chain` — array of stages (free strings: e.g. `["MkII line out", "Apollo Twin DI", "..."]`).
- `room_conditions` — free string.
- `anomalies` — array of strings; may be empty.

`notes.md` is a sibling free-form file in the session directory; not part of the schema.

### 6.2 `scaffold.py`

`python scaffold.py --name <slug> [--date YYYY-MM-DD]`

- Resolves `$VP330_CAPTURES_DIR` (errors clearly if unset).
- Creates `sessions/<date>-<slug>/` with a templated `manifest.json` (settings fields filled with defaults / `null`), an empty `notes.md`.
- Prints the path.

### 6.3 `validate.py`

`python validate.py [--root $VP330_CAPTURES_DIR]`

For each session under `sessions/`:
- Validates `manifest.json` against `schema.json`.
- Checks `input.mid` exists and parses with `mido`.
- Checks `output.wav` exists, opens with `soundfile`, asserts sample rate ≥48 kHz, bit depth ≥24 (or float32), channels match the manifest.
- Asserts `wav_duration ≈ midi_duration + expected_tail` within ±0.5 s.

Exit code 0 if all sessions pass, 1 otherwise. Prints a per-session report.

### 6.4 `driver.py`

```
python driver.py --name <slug> --midi <fixture.mid> \
  [--midi-out <port>] [--audio-in <device>] \
  [--tail-min 5.0] [--silence-thresh-db -50] [--silence-window 1.0] [--max-tail 30.0] \
  [--sample-rate 48000] [--bit-depth 24]
```

Workflow:

1. If `--midi-out` / `--audio-in` are omitted, list devices and prompt interactively.
2. Resolve `$VP330_CAPTURES_DIR`, scaffold the session directory.
3. Copy the fixture to `input.mid`.
4. Open `sounddevice` input stream into a ring buffer; warm up ~250 ms.
5. Send MIDI events (`mido.open_output(...).send(...)`) on schedule.
6. After last MIDI event: wait `tail_min` seconds unconditionally, then poll the ring buffer's RMS over a `silence_window` window — stop when RMS drops below `silence_thresh_db`, or hard-stop at `max_tail`.
7. Trim the recording to the stop point; write `output.wav` via `soundfile`.
8. Write `manifest.json` with auto-filled audio fields. Instrument-settings fields remain pre-templated for the operator to edit.
9. Open `notes.md` in `$EDITOR` (skippable with `--no-editor`).

Hybrid stop policy is implemented in-process via `numpy.sqrt((buf**2).mean())` against a dB threshold over the live ring buffer.

### 6.5 README

`tools/capture/README.md` covers: protocol summary referencing spec §8, schema field documentation, capture-rig setup notes (audio interface, MIDI interface, signal chain hygiene), and a "first session" walkthrough using captures #1–#4 from spec §8.

## 7. Docs Deliverables

None of these duplicate `docs/SPEC.md`; they reference it.

**`GLOSSARY.md`** — verbatim copy of spec §4's glossary table, with a header noting it must stay in sync with the spec and that renames in code require a glossary update in the same commit (spec §10).

**`README.md`:**
- One-paragraph pitch (what it is, MkII-only, GPL-3, status: pre-implementation / Phase 0).
- Build instructions for macOS and Linux (`cmake -B build && cmake --build build`).
- How to run the test suite (`ctest --test-dir build`).
- How to load the plugin (paths to VST3/AU/CLAP after build).
- Where to learn more: `docs/SPEC.md` is the contract; `GLOSSARY.md` is the vocabulary; `CONTRIBUTING.md` is the process.
- License (GPL-3, with the JUCE-transitivity note).

**`CONTRIBUTING.md`:**
- Commit conventions (one concept per commit, five-minute review rule, glossary-with-code rule).
- PR process (CI must be green; lint must be green; goldens must not drift without justification).
- L1–L4 test discipline summary (with pointers to spec §3.3 and §7).
- The "ask, don't assume" / "do without asking" lists from spec §10.
- How to update a golden baseline (the deliberate `golden: update <fixture> — <reason>` flow).

**`THIRD_PARTY.md`** — table of dependencies with license, version pin, purpose, and GPL-3 compatibility note:

| Dependency | License | Pin | Used in | GPL-3 compatible? |
|---|---|---|---|---|
| JUCE | GPL-3 (or commercial) | tag `8.x.y` | `infrastructure/juce`, plugin glue | yes (same license) |
| Catch2 | BSL-1.0 | tag `v3.x.y` | `tests/` | yes |
| rapidcheck | BSD-2 | tag (or commit pin) | `tests/` | yes |
| libsndfile | LGPL-2.1 | system or vendored tag | `infrastructure/cli`, test WAV I/O | yes |

Plus a "how to add a dependency" footer matching spec §10 (ask before introducing dependencies).

## 8. Dependency Pins, CI, Lint Specifics

### Dependency pins (in CMake via `FetchContent_Declare(... GIT_TAG ...)`)

- **JUCE** — latest 8.x release tag at implementation time. Tag, not branch.
- **Catch2** — latest v3.x tag.
- **rapidcheck** — latest tagged release; if no recent tag, a specific commit SHA.
- **libsndfile** — system package on Linux (`libsndfile1-dev`), Homebrew on macOS. Vendoring deferred unless cross-compile complications surface in Phase 6.

Exact tags get pinned in the implementation plan after a quick check of latest releases.

### `ci.yml`

- Triggers: `push` and `pull_request`.
- Jobs: `build-macos` (macOS-14), `build-linux` (ubuntu-24.04). Each runs: configure → build all targets → `ctest`.
- Caches: CMake build dir keyed on `CMakeLists.txt` + lock files; `FetchContent` cache keyed on tags.
- LFS: `actions/checkout@v4` with `lfs: true` so baselines are present.

### `arm-cross.yml`

- Triggers: push to `main`, tags.
- Pulls Elk SDK sysroot (acquisition documented in `infrastructure/elk/README.md`). If the sysroot is unavailable at Phase 0, the workflow logs that and exits 0 with a notice. Phase 6 makes it real.

### `lint.yml`

- `clang-format --dry-run -Werror` on all `.h`/`.cpp` under `domain/` and `infrastructure/`.
- `clang-tidy` on changed files only (computed from `git diff --name-only origin/main...HEAD`).
- Domain-isolation grep: `! grep -rE '#include[[:space:]]+<(juce_|sndfile|alsa)' domain/` — any match fails the job.

### `.clang-format`

`BasedOnStyle: LLVM`, `ColumnLimit: 100`, `PointerAlignment: Left`, `AccessModifierOffset: -2`. Per spec §6.

### `.clang-tidy`

Enabled categories per spec §6: `bugprone-*`, `cert-*`, `cppcoreguidelines-*`, `modernize-*`, `performance-*`, `readability-*`. No disables initially; we add them with inline rationale only when a check fires inappropriately on Phase 1+ DSP code.

### Pre-commit hook

`tools/install-hooks.sh` installs `.git/hooks/pre-commit`:
- Runs `clang-format -i` on staged C++ files and re-stages them.
- Runs the domain-isolation grep against staged files.
- Fails if either step finds problems.

## 9. Deferred / Open

**Pinned to "current at impl time" (not this doc):**
- Exact JUCE 8.x tag, Catch2 v3.x tag, rapidcheck tag/SHA.
- Exact GitHub Actions versions.

**Resolved during execution:**
- The list of `.clang-tidy` checks to disable for DSP code (start empty; add only when needed).
- Elk SDK sysroot acquisition mechanics (Phase 6 makes `arm-cross.yml` real).

**Open in spec §11, untouched here:**
- ChoirVariant naming → schema field stays a free string until Phase 3.
- Service-manual-derived numbers → all Phase 2+.
- Captures repo placement → **resolved as env-var-linked private repo by this brainstorm**. Spec §11 to be updated in the same commit as this design doc lands.

**Not done in Phase 0:**
- Any actual capture session — recorded out-of-band on author's hardware after Phase 0 closes.
- Any DSP code, value types, or domain components — Phase 1+.
- GUI work — Phase 5.
