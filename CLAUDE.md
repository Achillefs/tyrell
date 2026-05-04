# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Status

**Pre-implementation.** The repo currently contains only `docs/SPEC.md` and the license. There is no source tree, build system, or test harness yet — Phase 0 of the roadmap (Section 9 of the spec) creates them.

`docs/SPEC.md` is the contract. It is the source of truth for architecture, terminology, scope, and process. Read it before doing non-trivial work. Sections marked **[CC]** in the spec are direct instructions to you and override anything in this file if they conflict. If implementation reveals the spec is wrong, update `docs/SPEC.md` in a dedicated commit before changing code.

## What this project is

A C++20 / JUCE 8 digital recreation of the **Roland VP-330 MkII** (1979/1980) Vocoder Plus, shipping as a cross-format plugin (VST3/AU; CLAP added in Phase 5) and as a standalone module on Raspberry Pi 4 / Elk Audio OS. The goal is architectural fidelity to the original (top-octave divider, paraphonic voicing, BBD ensemble) — not a generic polysynth approximation.

**MkII only.** MkI is explicitly out of scope; the two revisions are audibly different and the author's reference unit is the MkII. Do not use MkI service-manual values or schematics.

License is **GPL-3** (transitive from JUCE). New dependencies must be GPL-3-compatible and recorded in `THIRD_PARTY.md`.

## Architecture: load-bearing invariants

The codebase will follow hexagonal (ports & adapters) layout once Phase 0 lands:

- `domain/` — pure C++ core. **No JUCE, no libsndfile, no ALSA, no third-party non-stdlib headers.** Depends only on the C++ standard library and `<cmath>`. CI enforces this via grep on `domain/`.
- `infrastructure/` — adapters (JUCE plugin, CLI renderer, Elk/Sushi config). Depends on `domain/`.
- `tests/` — links `vp330_domain` directly; never links JUCE.
- `tools/` — A/B comparison and convenience renderers.

Dependency direction is one-way: `infrastructure → domain`. Never the reverse. If you find yourself wanting to include JUCE inside `domain/` "just for this," stop — there is always a port-shaped solution.

## Test discipline (graded by layer)

Spec §3.3 and §7 define four layers. The discipline is graded:

- **L1 (unit, `tests/unit/`)** — strict TDD. Every pure function in `domain/` ships with its L1 test in the same patch, and the test must have been failing before the implementation. If you cannot construct a failing test for a chunk of code, that chunk likely belongs in an adapter, not the domain. Whole-suite budget: < 5 s.
- **L2 (characterization, `tests/characterization/`)** — every DSP component has an L2 contract (frequency response, time-domain behavior, stability, property-based invariants via rapidcheck) **before integration into the engine**. A component without an L2 spec will not be merged. Budget: < 30 s.
- **L3 (golden render, `tests/golden/`)** — fixture MIDI rendered to WAV, compared to a baseline WAV (Git LFS) via log-spectral distance. **Baselines are committed.** Updating one requires an explicit commit message `golden: update <fixture> — <reason>`. Never run `--update-baseline` to silence a failing test without explaining the change and getting confirmation.
- **L4 (reference comparison, `tests/reference/`)** — A/B against the author's VP-330 captures. Not a CI gate; run at phase boundaries. Reviewed by ear by the author.

## Ubiquitous language

Spec §4 is the glossary. Code names match it exactly; renames in code require a glossary update in the same commit. Highlights worth memorizing:

- **Section** — Choir, Strings, or Vocoder. A complete signal path with its own bus.
- **Paraphonic** (not "polyphonic") — single voicing path shared by all sounding keys downstream of `KeyGate`s. Reserved term.
- **Ensemble** (not "Chorus") — the BBD effect. **Always engaged on Strings**, defeatable on Choir.
- **ChoirFilterBank** — pool of 7 fixed bandpass filters (MkII).
- **ChoirVariant** — one of 4 voicings; each selects 4 of the 7 filters via the Male/Female switches.
- **KeyGate** — per-key on/off with attack/release; 49 instances on the MkII.
- **Voice** — **banned**. Use `KeyGate`, `Section`, or `Note`.

Don't invent local jargon. Propose glossary additions in the same PR that introduces a new concept.

## Reference captures are read-only

`reference-captures/` (Git LFS, possibly a private companion repo — TBD per spec §11) holds recordings of the author's MkII. **Never modify, regenerate, or delete anything under `reference-captures/`.** If a capture looks wrong, surface it to the author; do not fix it.

## Working style (from spec §10)

- **Plan before code at phase starts.** When asked to start a phase, propose a sequenced task list and wait for confirmation before implementing.
- **One concept per commit.** A commit should be reviewable in five minutes. Do not bundle unrelated changes.
- **Ask before introducing dependencies** — they affect license and the embedded footprint on the Pi.
- **Ask, don't assume**, on: DSP design choices not pinned by the MkII service manual or an L2 contract; user-facing parameter ranges/curves; anything the spec marks "calibrated by ear" (the ear is the author's); whether to rename a term already in use.
- **Do without asking**: refactors that don't change an L2 contract, adding missing L1 tests, fixing bugs caught by L1/L2 with a regression test, CMake/CI hygiene, doc improvements.

## Build & tooling (once Phase 0 lands)

Per spec §6 — implement these as you reach them, don't preempt:

- CMake ≥ 3.22, top-level `project(VP330 CXX)`, C++20. JUCE/Catch2/rapidcheck via `FetchContent` pinned to tags.
- Targets: `vp330_domain` (static lib), `VP330` (JUCE plugin: VST3/AU/Standalone in Phase 0; CLAP added in Phase 5), `vp330_render` (CLI), `vp330_tests` (Catch2; links domain only).
- `.clang-format` LLVM base, `ColumnLimit: 100`, `PointerAlignment: Left`, `AccessModifierOffset: -2`.
- `.clang-tidy` enables `bugprone-*`, `cert-*`, `cppcoreguidelines-*`, `modernize-*`, `performance-*`, `readability-*`.
- CI: `ci.yml` (macOS-14 + ubuntu-24.04 build/test), `arm-cross.yml` (aarch64 cross-compile against Elk SDK), `lint.yml` (format check, clang-tidy on changed files, **domain-isolation grep check**).

Until Phase 0 produces these, there are no commands to run — `make`/`cmake`/`ctest` will not work.
