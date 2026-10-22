# Destructive installation tests are restricted to disposable GitHub Actions runners.
$ErrorActionPreference='Stop'
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'Installer QA requires a disposable GitHub Actions runner' }
Set-Location (Split-Path $PSScriptRoot -Parent)
$id=python scripts/identity.py | ConvertFrom-Json
if ($LASTEXITCODE -ne 0) { throw 'Identity read failed' }
$release=(Resolve-Path release).Path
$installDir=Join-Path $env:RUNNER_TEMP 'MasterMonitor-install-test'
$plugin=Join-Path ${env:CommonProgramFiles} 'VST3/MASTER MONITOR.vst3'
$installer=Join-Path $release "MASTER-MONITOR-$($id.MM_VERSION)-Windows-x64.exe"
$installLog=Join-Path $release 'validation/installer-install.txt'
$uninstallLog=Join-Path $release 'validation/installer-uninstall.txt'
if ((Test-Path $installDir) -or (Test-Path $plugin)) { throw 'QA destination exists; refusing to overwrite' }
$setup=Start-Process -FilePath $installer -Wait -PassThru -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-','/COMPONENTS=vst3,standalone',"/DIR=`"$installDir`"","/LOG=`"$installLog`"")
if ($setup.ExitCode -ne 0) { throw "Install failed: $($setup.ExitCode)" }
$uninstaller=Join-Path $installDir 'unins000.exe'
$matches=$false
try {
 $actualApp=Join-Path $installDir 'MASTER MONITOR.exe'
 $sourceApp=Join-Path $release 'windows/Standalone/MASTER MONITOR.exe'
 if ((Get-FileHash $actualApp).Hash -ne (Get-FileHash $sourceApp).Hash) { throw 'Installed standalone differs from delivered payload' }
 $sourcePlugin=Join-Path $release 'windows/VST3/MASTER MONITOR.vst3'
 foreach ($file in Get-ChildItem $sourcePlugin -Recurse -File) {
  $relative=[IO.Path]::GetRelativePath($sourcePlugin,$file.FullName)
  if ((Get-FileHash $file.FullName).Hash -ne (Get-FileHash (Join-Path $plugin $relative)).Hash) { throw "Installed VST3 mismatch: $relative" }
 }
 $matches=$true
} finally {
 if (Test-Path $uninstaller) {
  $uninstall=Start-Process -FilePath $uninstaller -Wait -PassThru -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/LOG=`"$uninstallLog`"")
  if ($uninstall.ExitCode -ne 0) { throw "Uninstall failed: $($uninstall.ExitCode)" }
 }
}
for ($i=0; $i -lt 30 -and ((Test-Path $plugin) -or (Test-Path (Join-Path $installDir 'MASTER MONITOR.exe'))); $i++) { Start-Sleep -Milliseconds 500 }
if ((Test-Path $plugin) -or (Test-Path (Join-Path $installDir 'MASTER MONITOR.exe'))) { throw 'Uninstall left installed binary payloads' }
@{platform='windows-x64';installer_generated=$true;install_exit=0;payloads_match=$matches;uninstall_exit=0;uninstalled_payloads_absent=$true;manual_audio_listening='not performed'} | ConvertTo-Json | Set-Content (Join-Path $release 'validation/installer-result.json')
