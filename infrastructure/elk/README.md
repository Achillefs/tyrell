# Elk Audio OS Deployment

This directory holds Elk-specific configuration: the Sushi plugin host config
and the deployment helper. Phase 0 ships only this README; Phase 6 fills in
the real artifacts.

## Cross-compile sysroot

`.github/workflows/arm-cross.yml` cross-compiles the plugin for
`aarch64-linux-gnu` against the Elk SDK sysroot. The sysroot is **not** committed
to this repo (license, size). To enable the workflow, set the `ELK_SDK_URL`
GitHub repository secret to a tarball URL. Until that secret is set, the
workflow runs as a documented no-op.

## Acquiring the Elk SDK

See <https://github.com/elk-audio/elk-pi> and
<https://github.com/elk-audio/sushi> for the current process. Targets at the
time of writing: `elk-pi 1.0.0` against `aarch64-linux-gnu` GCC 11.

## Deployment (Phase 6)

A `deploy.sh` script will scp the cross-compiled plugin to the Pi and restart
sushi. Out of scope until Phase 6.
