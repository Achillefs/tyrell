# Third-Party Dependencies

VP330 is licensed under GPL-3.0 (driven by JUCE). All dependencies must be
GPL-3-compatible. Adding a new dependency requires a row in this table and
explicit approval (spec §10).

| Dependency | License | Pin | Used in | GPL-3 compatible? |
|---|---|---|---|---|
| [JUCE](https://github.com/juce-framework/JUCE) | GPL-3.0 (or commercial) | tag `8.0.12` | `infrastructure/juce/` (plugin glue) | yes (same license) |
| [Catch2](https://github.com/catchorg/Catch2) | BSL-1.0 | tag `v3.14.0` | `tests/` | yes |
| [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) | MIT | commit `e8de9e8` (post-0.26.0; JUCE 8 fix, no later tag) | `infrastructure/juce/` (CLAP plugin format shim) | yes |
| [rapidcheck](https://github.com/emil-e/rapidcheck) | BSD-2-Clause | commit `b2d9ed2dddefc4b84318d664b4f221eb792d89c7` (no upstream tags) | `tests/` (property tests) | yes |
| [libsndfile](https://github.com/libsndfile/libsndfile) | LGPL-2.1 | system package (`brew install libsndfile`, `apt install libsndfile1-dev`) | `infrastructure/cli/`, `tests/golden/helpers/wav_io` | yes |

## Python tooling (`tools/capture/requirements.txt`)

These dependencies ship the reference-capture driver and validator. They are
not linked into the C++ binaries — only invoked at recording time on the
author's machine.

| Dependency | License | Pin | Used in | GPL-3 compatible? |
|---|---|---|---|---|
| [mido](https://github.com/mido/mido) | MIT | `==1.3.3` | `tools/capture/{driver,validate}.py` | yes |
| [python-rtmidi](https://github.com/SpotlightKid/python-rtmidi) | MIT | `==1.5.8` | `tools/capture/driver.py` (mido backend) | yes |
| [sounddevice](https://github.com/spatialaudio/python-sounddevice) | MIT | `==0.5.1` | `tools/capture/driver.py` | yes |
| [soundfile](https://github.com/bastibe/python-soundfile) | BSD-3-Clause | `==0.12.1` | `tools/capture/{driver,validate}.py` | yes |
| [jsonschema](https://github.com/python-jsonschema/jsonschema) | MIT | `==4.23.0` | `tools/capture/validate.py` | yes |
| [numpy](https://github.com/numpy/numpy) | BSD-3-Clause | `==2.1.3` | `tools/capture/driver.py` (RMS / ring buffer) | yes |

## How to add a dependency

1. Confirm the license is GPL-3-compatible. Common compatible licenses:
   permissive (MIT, BSD-2/3, Apache-2.0), public domain, GPL-3 (or GPL-2+),
   LGPL.
2. Pin to a specific tag or commit SHA in `CMakeLists.txt` — never a branch.
3. Add a row to the table above.
4. If the dependency is consumed under `domain/`, you have probably picked
   the wrong layer; the domain depends only on the C++ standard library.
5. Open the change as a PR and reference this file in the description.
