param([string]$BuildRoot='build/windows-x64/MasterMonitor_artefacts/Release', [string]$OutputRoot='release')
$ErrorActionPreference='Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$id = (python scripts/identity.py | ConvertFrom-Json)
if ($LASTEXITCODE -ne 0) { throw 'Identity read failed' }
New-Item -Force -ItemType Directory $OutputRoot | Out-Null
$build=(Resolve-Path $BuildRoot).Path; $out=(Resolve-Path $OutputRoot).Path
function Sign([string]$Path) {
 if ($env:MM_WIN_CERT_THUMBPRINT) {
  & signtool sign /sha1 $env:MM_WIN_CERT_THUMBPRINT /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 $Path
  if ($LASTEXITCODE -ne 0) { throw 'Authenticode signing failed' }
  & signtool verify /pa /v $Path
  if ($LASTEXITCODE -ne 0) { throw 'Authenticode verification failed' }
 }
}
Sign "$build/Standalone/MASTER MONITOR.exe"
Get-ChildItem "$build/VST3" -Recurse -Filter *.vst3 | Where-Object {-not $_.PSIsContainer} | ForEach-Object { Sign $_.FullName }
# Preserve the signed executable payloads beside the installer for delivery and QA.
$payload=Join-Path $out 'windows'
New-Item -Force -ItemType Directory $payload | Out-Null
Copy-Item -Recurse -Force "$build/Standalone" $payload
Copy-Item -Recurse -Force "$build/VST3" $payload
$iscc = if ($env:ISCC) {$env:ISCC} else {'C:/Program Files (x86)/Inno Setup 6/ISCC.exe'}
& $iscc "/DAppVersion=$($id.MM_VERSION)" "/DAppId=$($id.MM_WINDOWS_APP_ID)" "/DPublisher=$($id.MM_MANUFACTURER)" "/DBuildRoot=$build" "/DOutputRoot=$out" packaging/windows.iss
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup failed' }
Sign "$out/MASTER-MONITOR-$($id.MM_VERSION)-Windows-x64.exe"
python scripts/manifest.py $out
if ($LASTEXITCODE -ne 0) { throw 'Manifest generation failed' }
