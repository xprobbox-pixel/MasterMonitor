param([switch]$SkipPackage)
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
function Run([string]$Tool, [string[]]$ArgList) { & $Tool @ArgList; if ($LASTEXITCODE -ne 0) { throw "$Tool failed: $LASTEXITCODE" } }
$configure = @('--preset','windows-x64')
if ($env:JUCE_SOURCE_DIR) { $configure += "-DJUCE_SOURCE_DIR=$env:JUCE_SOURCE_DIR" }
if ($env:EBUR128_SOURCE_DIR) { $configure += "-DEBUR128_SOURCE_DIR=$env:EBUR128_SOURCE_DIR" }
Run -Tool cmake -ArgList $configure
Run -Tool cmake -ArgList @('--build','--preset','windows-x64','--parallel','4')
Run -Tool ctest -ArgList @('--preset','windows-x64','--verbose')
if (-not $SkipPackage) { Run -Tool powershell -ArgList @('-NoProfile','-ExecutionPolicy','Bypass','-File','scripts/package-windows.ps1') }
