# MASTER MONITOR

Native JUCE/C++ stereo bus metering and monitoring suite, version 0.1.0. Dark hardware-style interface built around the supplied reference and original logo. This is a tested development implementation; it is not a certified measurement instrument or a signed public release. Read [actual validation](docs/VALIDATION.md) before relying on the artifacts.

## Audio behaviour

Input → smoothed Bus Trim → measurement tap → smoothed Mono/Dim → Output. Mono and Dim affect listening AND exported audio. They are not analysis-only switches. Active off or host bypass passes dry unity audio and pauses measurements; resuming starts a new programme. No limiter, normalisation, saturation or added processing latency. The true-peak limit is an indicator only. See [measurement conventions, algorithms and limitations](docs/MEASUREMENT.md).

Defaults: Active on, trim 0 dB, Mono/Dim off; target −14 LUFS, TP limit −1 dBTP, 0 VU=−18 dBFS sine peak. Targets/display preferences are non-automatable; monitoring and trim are automatable. Settings and editor size persist; a DAW project also saves versioned plugin state. Reset Measurements starts a new programme; Peak Hold Reset clears peak/clipping/spectrum holds without clearing loudness.

## Requirements and dependencies

- CMake ≥3.22 (tested local 4.4.4), C++17 compiler, Git, Python 3.9+, Ninja for the macOS preset.
- macOS 11+ deployment target, Command Line Tools or Xcode with a universal-capable SDK; native Apple packaging tools. Builds arm64 + x86_64.
- Windows x64: Visual Studio 2022 Desktop C++ workload and Windows SDK, Inno Setup 6. Static MSVC runtime (`/MT`) avoids a separate VC redist installation; OS libraries still require supported Windows.
- Pinned JUCE 8.0.9 and libebur128 1.2.6 commit hashes are centralised in `cmake/Identity.cmake`. FetchContent retrieves exact commits. Network access is needed once. Review [licences](docs/THIRD-PARTY-NOTICES.md); JUCE licensing must suit the intended distribution.

To supply existing checkouts, use `JUCE_SOURCE_DIR` and `EBUR128_SOURCE_DIR` as environment variables with the scripts, or `-DJUCE_SOURCE_DIR=/absolute/JUCE -DEBUR128_SOURCE_DIR=/absolute/libebur128` with CMake. They must match the recorded commits for reproducible builds. Check with `git -C /path rev-parse HEAD`.

```sh
python3 -m pip install cmake==3.31.6 ninja==1.11.1.4
# macOS, from repository root:
bash scripts/build-macos.sh
# Or configure/build/test individually:
cmake --preset mac-universal
cmake --build --preset mac-universal --parallel 4
ctest --preset mac-universal
MM_NO_USER_SETTINGS=1 build/mac-universal/MMRender_artefacts/Release/MMRender release/validation
bash scripts/package-macos.sh build/mac-universal release
```

```powershell
# Windows, from repository root, Developer PowerShell:
python -m pip install cmake==3.31.6 ninja==1.11.1.4
./scripts/build-windows.ps1
# Equivalent individual steps:
cmake --preset windows-x64
cmake --build --preset windows-x64 --config Release --parallel 4
ctest --preset windows-x64
./scripts/package-windows.ps1
```

CMake outputs `MasterMonitor_artefacts/Release/{VST3,AU,Standalone}` under the build directory. Packaging outputs `release/`, including a SHA-256 release manifest. The copy supplied in this delivery is under `release/`; actual build commands and logs are recorded in the validation report. Development identifiers/manufacturer/version/plugin codes are in `cmake/Identity.cmake`. Choose a real manufacturer identity before public distribution; changing plugin IDs later can break session compatibility.

## Installation and removal

macOS: open the PKG, use Customise to select VST3, Audio Unit and/or standalone. It installs to `/Library/Audio/Plug-Ins/VST3/MASTER MONITOR.vst3`, `/Library/Audio/Plug-Ins/Components/MASTER MONITOR.component` and `/Applications/MASTER MONITOR.app`. Administrator credentials are required for these system locations. An unsigned/unnotarised package can be blocked by macOS; it is a development artifact. Use only your organisation's approved local development installation procedure. Packaging is tested separately from privileged installation.

To uninstall, close audio hosts and remove exactly those three bundles as desired in Finder. Optional per-user preferences are under `~/Library/Application Support/MasterMonitor/` and the standalone wrapper's JUCE settings location. Receipts can be listed with `pkgutil --pkgs | grep com.mastermonitor.dev`; forgetting receipts does not remove bundles. No uninstaller deletes unrelated user audio or host settings.

Windows: run the EXE and select VST3 and/or standalone. VST3 installs to `%CommonProgramFiles%\VST3\MASTER MONITOR.vst3`; standalone installs under `%ProgramFiles%\MASTER MONITOR`. Use Windows Apps / Installed apps, or the Start-menu uninstall entry, to remove installed components. Per-user settings can be removed separately from the MasterMonitor application data directory.

Standalone: use the Options/audio settings panel for devices and I/O. JUCE's feedback protection mutes input at first launch. Explicitly disable mute only after confirming input/output routing; saved routing and monitoring choices can affect subsequent launches. No microphone-to-speaker path is intentionally enabled on a fresh launch.

## Validation commands

```sh
ctest --preset mac-universal
MM_NO_USER_SETTINGS=1 python3 tests/reference.py build/mac-universal/MMTests_artefacts/Release/MMTests > release/validation/reference.json
pluginval --strictness-level 5 --validate 'release/macos/MASTER MONITOR.vst3'
# Once AU is installed in a standard discovery location:
auval -strict -v aufx Mmon MmDv
lipo 'release/macos/MASTER MONITOR.vst3/Contents/MacOS/MASTER MONITOR' -verify_arch arm64 x86_64
```

FFmpeg is optional for the independent reference comparison. Automated deterministic tests need no input files; generated signals test rates 32/44.1/48/96/192 kHz and routing blocks 1/17/256/1024/8192. Tolerances live next to assertions and actual values are printed. UI rendering generates actual JUCE component snapshots, not a separate mockup. Automated host validation and manual DAW listening tests are distinct; see the report for performed and unexecuted checks.

## Signing and release

Local macOS: securely import Developer ID Application and Developer ID Installer certificates to your keychain. Set `MM_APP_SIGN` and `MM_INSTALLER_SIGN` to their identities. Store notarisation credentials using `xcrun notarytool store-credentials`, then set `MM_NOTARY_PROFILE`. The packaging script signs each bundle with hardened runtime and timestamp, verifies both architectures and signatures, signs the PKG, waits for notarisation acceptance, staples and validates the PKG and separately delivered app, and assesses them with `spctl`. Missing credentials produce an ad-hoc-signed binary set inside an unsigned, unnotarised development PKG. Ad hoc signing is not Developer ID signing.

Local Windows: import the Authenticode certificate securely into the user certificate store, make Windows SDK `signtool` available on PATH, and set `MM_WIN_CERT_THUMBPRINT`. The script signs/verifies EXE and VST3 executable payloads and the final Inno installer. Set `ISCC` if Inno Setup is in a nonstandard location. Do not put passwords or private keys into this repository.

The native CI matrix runs on macOS and Windows, builds/tests/packages each platform and uploads separate artifacts. It does not imply Windows success until it runs. Optional GitHub secrets: `MAC_CERTIFICATES_P12_BASE64`, `MAC_CERTIFICATES_PASSWORD`, `MAC_KEYCHAIN_PASSWORD`, `MM_APP_SIGN`, `MM_INSTALLER_SIGN`; notarisation `NOTARY_KEY_P8_BASE64`, `NOTARY_KEY_ID`, `NOTARY_ISSUER`; Windows `WINDOWS_CERT_PFX_BASE64`, `WINDOWS_CERT_PASSWORD`. Hardware-backed signing may need provider-specific configuration beyond the certificate-store method. CI should be run and reviewed before release. Third-party Actions major tags are provided; organisations requiring fully immutable CI should pin their reviewed action commits.

## Structure

`src/Processor.*`: APVTS, routing, smoothing and validated state. `src/Analysis.*`: lock-free audio transport, worker, libebur128, FFT, diagnostics and scope/history. `src/Editor.*`: custom vector graphics, cached static chassis, native accessibility names, editable slider values, tooltips and original logo. `tests/`: generated-signal checks, actual JUCE UI snapshots and FFmpeg reference comparison. `scripts/`, `packaging/`, `.github/workflows/`: native build, validation, installer and optional signing paths.
