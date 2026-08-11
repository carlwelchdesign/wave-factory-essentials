$ErrorActionPreference = "Stop"

$packageRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$vst3Source = Join-Path $packageRoot "Goodband.vst3"
$clapSource = Join-Path $packageRoot "Goodband.clap"
$commonPlugins = Join-Path $env:LOCALAPPDATA "Programs\Common"
$vst3Destination = Join-Path $commonPlugins "VST3\Goodband.vst3"
$clapDestination = Join-Path $commonPlugins "CLAP\Goodband.clap"
$backupRoot = Join-Path $env:LOCALAPPDATA "Carl Welch Audio\Backups"
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"

if (-not (Test-Path $vst3Source)) {
  throw "Goodband.vst3 is missing beside this installer."
}

if (-not (Test-Path $clapSource)) {
  throw "Goodband.clap is missing beside this installer."
}

New-Item -ItemType Directory -Force -Path (Split-Path $vst3Destination), (Split-Path $clapDestination), $backupRoot | Out-Null

foreach ($plugin in @(
  @{ Source = $vst3Source; Destination = $vst3Destination; BackupName = "Goodband-$timestamp.vst3" },
  @{ Source = $clapSource; Destination = $clapDestination; BackupName = "Goodband-$timestamp.clap" }
)) {
  if (Test-Path $plugin.Destination) {
    Move-Item -Path $plugin.Destination -Destination (Join-Path $backupRoot $plugin.BackupName)
  }
  Copy-Item -Path $plugin.Source -Destination $plugin.Destination -Recurse
}

Write-Host "Installed Threefold Palm 0.1.16 for the current Windows user."
Write-Host "Reopen your DAW and rescan plugins if necessary."
