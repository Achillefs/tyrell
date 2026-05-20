#!/usr/bin/env bash
# deploy.sh — copy the cross-compiled VP330 VST3 to the Elk Pi and restart Sushi.
#
# Required:
#   PI_HOST  — hostname or IP of the Pi (default: elk-pi.local)
#   PI_USER  — SSH user on the Pi (default: mind)
#
# Override defaults via environment:
#   PI_HOST=192.168.1.42 PI_USER=root ./infrastructure/elk/deploy.sh
#
# Requires: bash, ssh, rsync, scp (all standard on macOS and Linux)
set -euo pipefail

PI_HOST="${PI_HOST:-elk-pi.local}"
PI_USER="${PI_USER:-mind}"
VST3_BUNDLE="${VST3_BUNDLE:-build-arm/infrastructure/juce/VP330_artefacts/Release/VST3/VP330.vst3}"
REMOTE_VST3_DIR="/usr/lib/lxvst"
REMOTE_CONFIG_DIR="/home/${PI_USER}"
SUSHI_CONFIG="infrastructure/elk/sushi-config.json"

if [[ ! -d "$VST3_BUNDLE" ]]; then
    echo "ERROR: VST3 bundle not found at '$VST3_BUNDLE'."
    echo "Cross-compile first:"
    echo "  cmake -B build-arm -S . \\"
    echo "    -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64-elk.cmake \\"
    echo "    -DELK_SYSROOT=\$ELK_SYSROOT \\"
    echo "    -DVP330_ENABLE_TESTS=OFF \\"
    echo "    -DVP330_ENABLE_RENDER=OFF \\"
    echo "    -DCMAKE_BUILD_TYPE=Release"
    echo "  cmake --build build-arm --config Release"
    exit 1
fi

echo "Deploying VP330 to ${PI_USER}@${PI_HOST} ..."

# Stop Sushi so it releases the plugin file lock.
# 'true' prevents failure if Sushi is not currently running.
ssh "${PI_USER}@${PI_HOST}" "sudo systemctl stop sushi 2>/dev/null || true"

# Push the VST3 bundle (rsync preserves timestamps and only sends changed files).
rsync -av --delete \
    "$VST3_BUNDLE/" \
    "${PI_USER}@${PI_HOST}:${REMOTE_VST3_DIR}/VP330.vst3/"

# Push the Sushi config.
scp "$SUSHI_CONFIG" \
    "${PI_USER}@${PI_HOST}:${REMOTE_CONFIG_DIR}/sushi-config.json"

# Restart Sushi with the new config.
# On Elk OS the sushi systemd unit reads /etc/sushi/config.json by default;
# if yours is configured differently, adjust the ExecStart in the unit file
# or pass --config explicitly.
ssh "${PI_USER}@${PI_HOST}" "sudo systemctl start sushi 2>/dev/null || \
    sushi --multicore-processing=2 -j -c '${REMOTE_CONFIG_DIR}/sushi-config.json' &"

echo ""
echo "Done. VP330 is now active on ${PI_HOST}."
echo "Connect an external MIDI keyboard to the Pi and play."
