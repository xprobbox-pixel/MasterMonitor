# MASTER MONITOR 0.1.1 — release validation

Build source: `8c260138f27d9c217b63e6c378800f54ef47c828`.
Native CI: https://github.com/xprobbox-pixel/MasterMonitor/actions/runs/39856896492

## Change

Window scaling spans 50%–120% (640×480–1536×1152). UI SCALE offers 10% steps; the bottom-right grip supports intermediate sizes at fixed 4:3 proportions. Whole-interface controls/text scale together. The actual scale is displayed and persists through editor reopening and DAW state. Scope Zoom remains separate. The host's HiDPI transform is preserved.

## Actual validation

- Local Apple Silicon and Intel under Rosetta: 291 checks each, zero failures on the corrected 0.1.1 test build. This is not a physical Intel Mac test.
- macOS 14 native CI: CTest 1/1 passed; pluginval strictness 5 and strict auval succeeded. Universal arm64/x86_64 architectures were verified for VST3, AU and standalone.
- Downloaded macOS artifact: all 53 manifest file hashes and the GitHub ZIP digest verified. Local `lipo -verify_arch arm64 x86_64` and `codesign --verify --deep --strict` passed on all three bundles; bundle versions are 0.1.1. PKG expansion confirmed all three selectable component payloads at 0.1.1. Signature checks identify ad-hoc bundles and an unsigned PKG; Gatekeeper package assessment rejected it with “no usable signature”.
- macOS JUCE renders inspected at 50%, 80%, 100% and 120%: no panel/control clipping or overlap found.
- Windows Server 2022 native MSVC x64: 291 checks, zero failures; pluginval strictness 5 passed; standalone startup survived four seconds. This is not an audio-device/listening test.
- Windows installer: generated EXE, silent install succeeded, installed VST3/standalone payload hashes matched, uninstall exited 0 and both payloads were absent afterwards. `release/0.1.1/Windows/validation/installer-result.json` records the results.
- Windows artifact: 21 file hashes verified against the original CI manifest. ZIP SHA-256 matched the GitHub artifact digest. Header inspection confirmed both plugin and standalone payloads are x86_64; the Inno bootstrap itself is x86 and installs the x64 payloads.
- Windows actual JUCE renders inspected at 50%, 80%, 100% and 120%: no panel/control clipping or overlap found. At 50%, secondary text is small; a larger scale is preferable for reading.

## Failed first attempt

Run https://github.com/xprobbox-pixel/MasterMonitor/actions/runs/39855241667 compiled Windows successfully but the new UI test crashed during teardown. The restored processor could die before the editor and its parameter attachments. The corrected test declares both processors before the editor so they outlive it. The first failure is preserved in `release/0.1.1/validation/windows-first-attempt.txt`. The corrected Windows run passes. The first macOS job passed, but final delivered artifacts are from the corrected run.

## Unexecuted and release limitations

Manual DAW window dragging, keyboard interaction, multi-monitor DPI transitions, physical Intel Mac testing, and end-to-end audio-device/listening tests remain unexecuted. Component mouse-drag tests and pluginval do not replace those host checks. Privileged macOS system installation/uninstallation is not performed on this workstation. Windows artifacts are unsigned; macOS bundles use ad-hoc signing and the PKG is unsigned/unnotarised. Developer ID / Authenticode credentials were not supplied. These are development installers. Existing dependency/JUCE licence obligations and Inno Setup licence suitability remain to be resolved for distribution.

Original per-platform CI manifests retain checksums, build commit, architecture and signing inspection details. This report describes the resizing release; the historical metering-validation methodology and numerical tolerances remain in `docs/VALIDATION.md` and the test assertions.

## Installer SHA-256

- `MASTER-MONITOR-0.1.1-Windows-x64.exe`: `303f67e83cb772f30a7ee39dc0e2b0425425d71a9b9d94410730761bc9779ea6`
- `MASTER-MONITOR-0.1.1-macOS.pkg`: `edef7120e0962b984c265d7fda766345365fdce056287271791ac425a8ffda06`

## Completion

| Platform | Source | Binaries | Installer | Release signing | Validation |
|---|---|---|---|---|---|
| Windows x64 | Complete | VST3 + standalone built | EXE generated | Unsigned | 291 checks, pluginval 5, startup, install/hash/uninstall passed |
| macOS Universal | Complete | VST3 + AU + standalone built | PKG generated | Ad-hoc bundles; unsigned/unnotarised PKG | CTest, pluginval 5, auval, architectures and package contents passed |
