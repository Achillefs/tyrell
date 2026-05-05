# Capture tooling

Tools for recording the author's VP-330 MkII into reproducible session
directories. The captures themselves are **not** in this repo — set
`VP330_CAPTURES_DIR` to point at the private companion repo / local clone.

## Layout produced

```text
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

## L4: Rendering through the plugin

After capturing hardware sessions, render the same MIDI through the plugin and A/B compare.
The binary is at `build/infrastructure/cli/vp330_render` (built alongside the rest of the project).

Flags:

- `--input <mid>` — MIDI fixture (same file sent to the hardware)
- `--output <wav>` — output path
- `--duration <sec>` — render length in seconds; **must cover the full MIDI duration or later
  notes are silently dropped**. For the 49-note chromatic fixture: 65 s.
- `--sample-rate <hz>` — default 48000

Example (chromatic fixture, from the repo root):

```bash
./build/infrastructure/cli/vp330_render \
  --input  $VP330_CAPTURES_DIR/fixtures/chromatic-49-keys.mid \
  --output /tmp/plugin-chromatic.wav \
  --sample-rate 48000 \
  --duration 65
```

Load both WAVs in a DAW or audition tool and compare pitches / timbre.
