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

1. Run the golden test and inspect the rendered WAV in `build/`.
2. Compare audibly / spectrally to the prior baseline. Convince yourself
   the change is intentional.
3. Replace the baseline:

   ```bash
   cp /path/to/new/render.wav tests/golden/baselines/<fixture>.wav
   ```

4. Commit with the explicit message form:

   ```bash
   git commit -m "golden: update <fixture> — <reason>"
   ```

Never re-generate baselines as a "fix" for a failing test without
explaining what changed.
