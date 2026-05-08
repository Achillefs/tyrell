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

### Helper scripts (`bin/`)

Three convenience scripts live in `bin/`. They are the recommended way to build, test, and run the plugin locally.

| Script | What it does |
|--------|-------------|
| `bin/build` | Configure + build via CMake (parallel, auto-detects core count). Passes extra args to `cmake -B`. |
| `bin/test` | Run the full test suite via `ctest --output-on-failure`. Passes extra args to `ctest`. |
| `bin/run` | Launch the Standalone app — `VP330.app` on macOS, `VP330` binary on Linux. Prefers Release, falls back to Debug. |

Set `BUILD_DIR` to override the default `./build` location for all three scripts.

```bash
bin/build          # configure + build
bin/test           # run all tests
bin/run            # launch standalone app
```

### Raw CMake commands

```bash
cmake -B build -S .
cmake --build build
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
- [`tools/capture/README.md`](tools/capture/README.md) explains the reference-capture protocol and tooling for the L4 reference repo at `$VP330_CAPTURES_DIR`.

## License

GPL-3.0. JUCE's GPL-3 obligations apply transitively.
