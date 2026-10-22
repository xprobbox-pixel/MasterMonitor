#ifndef AppVersion
 #error AppVersion must be supplied from cmake/Identity.cmake
#endif
#ifndef BuildRoot
 #define BuildRoot "..\build\windows-x64\MasterMonitor_artefacts\Release"
#endif
#ifndef OutputRoot
 #define OutputRoot "..\release"
#endif
[Setup]
AppId={#AppId}
AppName=MASTER MONITOR
AppVersion={#AppVersion}
AppPublisher={#Publisher}
DefaultDirName={autopf}\MASTER MONITOR
DefaultGroupName=MASTER MONITOR
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
MinVersion=10.0.17763
OutputDir={#OutputRoot}
OutputBaseFilename=MASTER-MONITOR-{#AppVersion}-Windows-x64
Compression=lzma2
SolidCompression=yes
UninstallDisplayName=MASTER MONITOR
CloseApplications=yes
[Types]
Name: "full"; Description: "Complete installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom
[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full custom
Name: "standalone"; Description: "Standalone application"; Types: full custom
[Files]
Source: "{#BuildRoot}\VST3\MASTER MONITOR.vst3\*"; DestDir: "{commoncf64}\VST3\MASTER MONITOR.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#BuildRoot}\Standalone\MASTER MONITOR.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
Source: "..\docs\THIRD-PARTY-NOTICES.md"; DestDir: "{app}\Licenses"; Flags: ignoreversion
Source: "..\docs\licenses\*"; DestDir: "{app}\Licenses"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\MASTER MONITOR"; Filename: "{app}\MASTER MONITOR.exe"; Components: standalone
Name: "{group}\Uninstall MASTER MONITOR"; Filename: "{uninstallexe}"
