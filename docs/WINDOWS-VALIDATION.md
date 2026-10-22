# Windows x64 native CI validation — 22 October 2026

**Result: build, automated validation, EXE packaging, installation and uninstall passed.**

Repository: [xprobbox-pixel/MasterMonitor](https://github.com/xprobbox-pixel/MasterMonitor).
Executed [Windows job](https://github.com/xprobbox-pixel/MasterMonitor/actions/runs/39809361641/job/119931089052), run #1, conclusion **success**. The macOS matrix job is separate; Windows success does not assert the overall matrix conclusion.

Built commit: **f737f9b892fdd811137d86d70aace27118ffca0d**. Subsequent documentation and manifest-reporting corrections do not change these executable payloads.

| Check | Actual result |
|---|---|
| Native environment | Windows Server 2022 Datacenter 10.0.20348; windows-2022 hosted runner |
| Compiler/configuration | MSVC 19.44.35229.0, Visual Studio 2022, Release x64 |
| Dependencies | Pinned JUCE 8.0.9 and libebur128 1.2.6; CMake 3.31.6 |
| Built audio binaries | VST3 bundle and standalone EXE; both PE machine **0x8664 / x86_64** |
| Deterministic suite | **261 checks, zero failures**; CTest reports 1/1 suite passed |
| pluginval | Version 1.0.3, strictness **5**, exit **0**, including editor, processing, automation and buses |
| UI | Actual Windows JUCE snapshots rendered and visually inspected at 1280×960 and 1024×768; no essential control/readout clipping found |
| Standalone | Native process remained running for four seconds, then was closed by test; no audio hardware/listening claim |
| Installer generation | Inno Setup **6.7.1**, successful compile; EXE generated |
| Install QA | Silent installation on clean disposable Windows runner, exit **0** |
| Payload QA | Installed VST3 files and standalone hashes matched delivered executable payloads |
| Uninstall QA | Exit **0**; installed audio binary payloads confirmed absent afterwards |
| Signing | **Unsigned** installer and both audio binaries; PE certificate-table offset and size are zero |
| Artifact transfer | Downloaded ZIP SHA-256 matched GitHub artifact digest; all **17 original CI-manifest file hashes** verified locally |

## Deliverables

- `release/windows-ci/MASTER-MONITOR-0.1.0-Windows-x64.exe`: selectable-component EXE installer, approximately 4.55 MB. Its Inno bootstrap is PE x86, normal for this installer; its installed audio payloads are x64 and installation is constrained to x64-compatible Windows.
- `release/windows-ci/windows/VST3/MASTER MONITOR.vst3/`: complete native VST3 bundle.
- `release/windows-ci/windows/Standalone/MASTER MONITOR.exe`: native x64 standalone.
- `release/windows-ci/MASTER-MONITOR-Windows-x64.zip`: exact original GitHub Actions artifact, including executables, manifest, UI captures and test/installer logs.
- `release/windows-ci/manifest.json`: portable delivery manifest with per-file hashes, actual PE architectures and unsigned status. `manifest-ci-original.json` preserves the original native CI manifest unmodified.
- `release/windows-ci/validation/github-windows-job.txt`: full executed native job log, including compiler and 261-check test output. The PowerShell transcript alone omits some native command output; use the full GitHub log for those results.

The exact GitHub artifact ID is **12440261400**, ZIP SHA-256:

```
2388be9b2673c6e468fba67d2e04aee792b62e366431bb40d029893c610a8714
```

## Pipeline fixes and reporting issue

Before this run, the test/render utilities received a 16 MiB Windows main-stack reserve because their test-only local processor objects contain a large preallocated queue. Production plugin objects are heap-created. Windows CI now preserves the standalone and VST3 binaries with the installer, captures validation logs, performs the standalone startup/UI checks, and exercises installation/uninstallation. Installer creation follows successful binary validation.

The original CI manifest's Windows PowerShell Authenticode status query failed to load `Microsoft.PowerShell.Security`, despite the overall packaging step returning success. This was a **reporting failure**, not a signing pass. It remains visible in the preserved original manifest. Independent PE-header inspection confirmed no certificate table on any of the three executable files, establishing that all are unsigned. The manifest generator was corrected to inspect PE architecture/certificate tables portably and normalise paths to forward slashes; that corrected path was tested against all three actual Windows executable files and every delivered file checksum. A certificate table, if present, is explicitly not treated as proof of a valid signature. Credential-dependent signing remains unexecuted.

## Limits

No manual Windows DAW session, live audio-device configuration/listening, physical hardware matrix, or additional metering conformance corpus was performed. There are no supplied Authenticode credentials, and no Windows SmartScreen reputation or signed-distribution claim. The installed Inno compiler identifies its licence as **“Non-commercial use only”** in the native log; distribution licensing must be resolved separately, along with the JUCE and other dependency obligations.

These are functioning, automatically validated **unsigned development artifacts**, not a fully production-qualified commercial release.
