# Actual validation — 22 October 2026

MASTER MONITOR 0.1.0, development identity. This report records executed checks, not promises of cross-platform or standards compliance. This is a functional development release; it is not a signed, notarised or fully production-qualified public release.

## Delivery status

| Platform | Source / build setup | Built binaries | Installer | Signing | Executed validation |
|---|---|---|---|---|---|
| macOS arm64 + x86_64 universal | Implemented | VST3, AU, standalone; both slices verified | Native selectable PKG generated | Bundles ad hoc signed and verified; PKG unsigned; not notarised | 261 deterministic checks per architecture, pluginval level 5 per architecture, strict auval per architecture; 9 FFmpeg reference fixtures; default/minimum UI inspection; standalone startup; package contents/choices verified |
| Windows x64 | CMake target/preset, PowerShell scripts, Inno definition and native CI supplied | Not built here | EXE not generated here | Not performed | Not executed; no Windows environment or connected CI runner available |

## Host and dependencies actually used

- Apple Silicon Mac, macOS 15.7.9. Apple clang 17.0.0, Command Line Tools, macOS SDK 15.5. Universal Release compilation used `CMAKE_OSX_ARCHITECTURES=arm64;x86_64`, deployment target 11.0. Intel execution used Rosetta on this Mac, not separate Intel hardware.
- Local CMake 4.4.4 and Ninja 1.13.2, installed into the scratch workspace. No system package manager changes were required.
- JUCE 8.0.9, `f72bad64d29715216226685810c5196bd0d79d77`; libebur128 1.2.6, `67b33abe1558160ed76ada1322329b0e9e058b02`.
- pluginval 1.0.3 macOS universal binary. Apple auval 1.10.0. Independent comparison used installed FFmpeg 5.0.1's `ebur128=peak=true` filter.
- `security find-identity -v -p codesigning` returned **0 valid identities**. No Developer ID or notarisation credentials were available.

## Build execution

Actual build directory was `work/build` outside the delivered source directory, with explicitly supplied local dependency checkouts. Equivalent invocation:

```
cmake -S outputs/MasterMonitor -B work/build -G Ninja \
  -DJUCE_SOURCE_DIR=/absolute/work/JUCE \
  -DEBUR128_SOURCE_DIR=/absolute/work/libebur128 \
  -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build work/build --parallel 6
```

Configure, initial compilation and subsequent source revision build logs are in `release/validation/`. The compiler emitted benign warnings for a redundant standalone macro, the ignored JUCE splash flag, empty optional ARA/LV2 object files and libebur128's ARM denormal fallback. These did not prevent compilation or validation. No Windows compiler, Inno Setup execution or native CI run was performed in this session.

## Deterministic suite: 261 checks, zero failures, on each architecture

The final `MMTests` executable was invoked directly with `MM_NO_USER_SETTINGS=1`; CTest is configured to invoke the same suite, but the CTest frontend was not separately run. Logs contain each expected value, actual value and tolerance.

- Routing at 44.1/48/96/192 kHz, blocks 1/17/256/1024/8192: unity pass-through, host bypass and Active off, trim gain (1e−5 amplitude tolerance), Dim (1e−5), anti-phase Mono cancellation (1e−7), nonfinite input sanitisation. Trim ramp max adjacent amplitude change <0.001 for the tested step.
- Metering at 32/44.1/48/96/192 kHz: startup unavailable, 400 ms integrated startup, 3 second short-term startup, valid silent −∞, reset and elapsed time, −23 LUFS stereo 1 kHz reference (±0.12 LU), quiet-tail gating (±0.2 LU), sample peaks (±0.04 dB), VU steady-state (±0.05 dB) and approximately 300 ms rise (±0.12 dB).
- True peak: original sample clipping/exceedance latches/reset; 48 kHz, Fs/4 sine, pi/4 phase, 0.98 amplitude. Reconstructed maximum within 0.6 dB of continuous sine amplitude, and more than 2 dB above original sample peaks.
- Mono/inverted/uncorrelated correlation and width, 4 kHz spectrum band selection, noise response, inverted stereo spectrum without cancellation, Nyquist handling, correct single-channel loudness weighting.
- State round-trip including UI/analysis/audio settings; invalid input state rejection; zero reported latency.
- Fixed ring capacity, overflow refusal, ordering and empty state. Editor-closed analysis and an integration test proving measurement includes trim but excludes Mono/Dim. Idle resets publish without another audio callback.

This does not establish absence of every real-time issue. No allocation-interposition, dtrace real-time safety audit, extended soak test or broad CPU/hardware performance matrix was run.

## Independent FFmpeg comparison: 9/9 passed

Deterministically generated stereo fixtures at 44.1/48/96 kHz: steady sine, gated quiet tail and inter-sample peak sine. No user audio was used. The tolerances in `tests/reference.py` are **0.2 LU integrated** and **0.6 dBTP**. Observed largest integrated difference was approximately **0.083 LU**, and largest true-peak difference approximately **0.472 dB**. FFmpeg values are reported at its summary precision; startup/end reconstruction transients contribute to maximum differences. All actual/reference pairs are in `release/validation/reference.json`.

This is a limited comparison against a trusted implementation. A full published EBU/ITU conformance corpus, all true-peak frequencies/phases, reconstruction boundary cases and high-rate true-peak accuracy sweeps remain unexecuted. No BS.1770, true-peak or formal VU compliance label is displayed in the UI.

## Native host validation

- Final packaged VST3: pluginval strictness **5**, native arm64 **exit 0**, x86_64 under Rosetta **exit 0**. Includes editor creation/processing, automation and mono/stereo bus checks. Logs are `pluginval-arm64.txt` and `pluginval-x86_64.txt`. Its optional external Steinberg validator integration was not configured; an empty “auval” section in VST3 pluginval output is not an AU test.
- Final AU payload: `auval -strict -v aufx Mmon MmDv` and the same invocation under `arch -x86_64`, both **AU VALIDATION SUCCEEDED**, exit 0. It was temporarily placed in the per-user standard Components directory, then removed. The first discovery attempts failed before a force-refresh of AudioComponentRegistrar; that initial failure log is retained. No system-wide plugin installation was performed.
- Standalone: native arm64 process remained alive for a four-second startup smoke test, then was explicitly terminated. No controls were enabled or audio listening tests performed. The default input mute is confirmed in the pinned JUCE wrapper source. First-launch device configuration/microphone permissions and live routing have not been manually exercised.
- No manual DAW session, live audio listening, hardware audio-interface matrix or separate physical Intel Mac test was performed. Intel deterministic/validator tests ran through Rosetta.

## UI inspection

Actual JUCE editor snapshots were rendered at **1280×960** and **1024×768**. Signal snapshots use a real 3.2-second generated stereo multitone fed through the processor and worker; this is not simulated meter activity. Startup was also captured.

Visual inspection found and corrected Latin-1 interpretation of UTF-8 separator labels, repeated units and a snapshot utility that appended to existing PNGs. Final images were overwritten correctly and re-inspected at both sizes. No control overlaps or clipped essential readouts were found. Spectrum secondary labels are compact at minimum size; default size is clearer. All numerical controls retain native editable text boxes and accessible names. Screen-reader behaviour, every host scaling factor, Windows rendering and interactive device-settings UI remain untested.

## Installer/package verification

Native Apple `pkgbuild` generated component packages; `productbuild` generated the final selectable PKG. `installer -showChoicesXML` confirmed its choices. `pkgutil --expand-full` succeeded. Each extracted executable SHA-256 matched the separately delivered signed payload, and PackageInfo locations matched `/Library/Audio/Plug-Ins/VST3`, `/Library/Audio/Plug-Ins/Components` and `/Applications`.

All three bundle executables were verified to contain arm64 and x86_64. Packaging applied ad hoc signatures and `codesign --verify --deep --strict` passed for each bundle. This is not Developer ID signing. `pkgutil --check-signature` reports **no signature**. Native `spctl --assess --type install` returned **3 / rejected / source=no usable signature**, as expected for this development package. Signing and notarisation branches were not executed. No privileged installation, upgrade, uninstall or actual Inno Setup execution was performed.

The build/package scripts were corrected during execution for Apple's `lipo` input argument order and macOS Bash 3.2 empty-array behaviour. Final PKG generation and payload checks passed after those fixes. Native signing/CI scripts are supplied but their credential-dependent branches remain unverified.

## Release work still required

Run the Windows native CI job and examine its VST3/standalone/Inno artifacts and pluginval results. Exercise real DAWs and standalone device settings/listening, installer installation/upgrade/uninstall on clean machines, real HiDPI host scaling, extended performance/overflow and metering conformance material. Set a real manufacturer/distribution identity, resolve source/asset/JUCE licensing, provide signing credentials, sign and notarise, then repeat final release validation. Until those checks pass, treat these artifacts as development builds.
