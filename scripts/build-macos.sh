#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
set --
[[ -z "${JUCE_SOURCE_DIR:-}" ]] || set -- "$@" "-DJUCE_SOURCE_DIR=$JUCE_SOURCE_DIR"
[[ -z "${EBUR128_SOURCE_DIR:-}" ]] || set -- "$@" "-DEBUR128_SOURCE_DIR=$EBUR128_SOURCE_DIR"
cmake --preset mac-universal "$@"
cmake --build --preset mac-universal --parallel "${MM_JOBS:-4}"
ctest --preset mac-universal
build/mac-universal/MMRender_artefacts/Release/MMRender "$PWD/release/validation"
for f in 'Standalone/MASTER MONITOR.app' 'VST3/MASTER MONITOR.vst3' 'AU/MASTER MONITOR.component'; do
 lipo "build/mac-universal/MasterMonitor_artefacts/Release/$f/Contents/MacOS/MASTER MONITOR" -verify_arch arm64 x86_64
done
bash scripts/package-macos.sh build/mac-universal release
