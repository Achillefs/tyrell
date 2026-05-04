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
