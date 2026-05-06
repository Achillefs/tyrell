# Elk Audio OS Deployment

VP330 runs on Raspberry Pi 4 under Elk Audio OS, with the plugin binary hosted
by the Sushi plugin engine. This directory contains the Sushi config
(`sushi-config.json`) and the deployment helper (`deploy.sh`).

---

## Hardware setup

| Component | Part |
|-----------|------|
| SBC | Raspberry Pi 4 (4 GB or 8 GB) |
| Audio I/O | HiFiBerry DAC+ ADC Pro |
| OS | Elk Audio OS (1.x, aarch64) |
| MIDI | USB MIDI adapter or hardware DIN via HiFiBerry MIDI+ hat |

### HiFiBerry DAC+ ADC Pro configuration

Add to `/boot/config.txt` on the Pi (already done if using Elk's standard image):
```
dtoverlay=hifiberry-dacplusadcpro
```

Elk OS manages ALSA automatically; no further OS-level audio configuration
is required. Sushi reads the hardware bus assignments from `sushi-config.json`.

---

## Audio I/O parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| Sample rate | 48 000 Hz | Set in `sushi-config.json → host_config.samplerate` |
| Buffer size | 64 samples | Configured in the Sushi systemd unit (`--blocksize`) |
| Output channels | Stereo (L/R) | Engine bus 0 → HiFiBerry stereo output |
| Input channels | None | VP330 is a synthesizer; no audio input needed |
| Measured round-trip latency | **TBD** | Fill in after hardware A/B session |

> **To measure round-trip latency:** connect the Pi audio output to an audio
> interface, send a MIDI note-on, and record the gap between note-on timestamp
> and first audio sample using a DAW or `jack_iodelay`. Document the result in
> the table above.

---

## Acquiring the Elk Audio OS SDK

The Elk SDK provides the `aarch64-linux-gnu` sysroot for cross-compilation.
Obtain it from your Elk license portal or build it from the Elk Pi repo:
<https://github.com/elk-audio/elk-pi>.

The SDK tarball should extract to a directory containing
`sysroots/aarch64-elk-linux/` (or `sysroots/cortexa72-elk-linux/`).

### GitHub Actions (CI cross-compile)

Set the repository secret `ELK_SDK_URL` to an HTTPS URL of the SDK tarball
(e.g. an S3 pre-signed URL). The `arm-cross.yml` workflow will:

1. Download and extract the SDK.
2. Auto-locate the aarch64 sysroot.
3. Cross-compile the VP330 VST3 plugin.
4. Upload the bundle as the `VP330-aarch64-linux-gnu` workflow artifact.

Until `ELK_SDK_URL` is set, the workflow runs as a documented no-op.

### Local cross-compile

```bash
# 1. Extract the SDK (one time).
mkdir -p ~/elk-sdk
tar -xzf <elk-sdk>.tar.gz -C ~/elk-sdk

# 2. Set the sysroot path (adjust to match the extracted layout).
export ELK_SYSROOT=~/elk-sdk/sysroots/aarch64-elk-linux

# 3. Configure with the committed toolchain.
cmake -B build-arm -S . \
    -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-elk.cmake \
    -DELK_SYSROOT="$ELK_SYSROOT" \
    -DVP330_ENABLE_TESTS=OFF \
    -DVP330_ENABLE_RENDER=OFF \
    -DCMAKE_BUILD_TYPE=Release

# 4. Build.
cmake --build build-arm --config Release

# The VST3 bundle lands at:
#   build-arm/infrastructure/juce/VP330_artefacts/Release/VST3/VP330.vst3/
```

---

## Deploying to the Pi

```bash
# Deploy with defaults (Pi reachable at elk-pi.local, user 'mind').
./infrastructure/elk/deploy.sh

# Override host and user.
PI_HOST=192.168.1.42 PI_USER=root ./infrastructure/elk/deploy.sh
```

The script will:
1. Stop the Sushi systemd service.
2. `rsync` the VST3 bundle to `/usr/lib/lxvst/VP330.vst3/` on the Pi.
3. Copy `sushi-config.json` to the remote home directory.
4. Restart Sushi.

### First-time setup on the Pi

```bash
# SSH into the Pi.
ssh mind@elk-pi.local

# Create the VST3 directory if it does not exist.
sudo mkdir -p /usr/lib/lxvst

# If Sushi is managed by systemd with a custom config path, edit the unit:
sudo systemctl edit --force sushi
# Add: ExecStart=/usr/bin/sushi --multicore-processing=2 -j \
#        -c /home/mind/sushi-config.json
```

---

## Acceptance test

Phase 6 is complete when:
- [ ] Cross-compile CI job produces a `VP330-aarch64-linux-gnu` artifact.
- [ ] `deploy.sh` copies the bundle to the Pi without error.
- [ ] Sushi starts with no errors in `journalctl -u sushi`.
- [ ] Author connects an external MIDI keyboard and plays the instrument.
  Acceptance: **feels playable live** (no audible glitches, latency below ~10 ms
  round-trip, MIDI response correct across the 49-key range).
- [ ] Measured latency recorded in the table above.
- [ ] Tag `v1.0-elk` applied.
