$ErrorActionPreference = "Stop"

$packageRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$vst3Source = Join-Path $packageRoot "PitchTrails.vst3"
$clapSource = Join-Path $packageRoot "PitchTrails.clap"
$commonPlugins = Join-Path $env:LOCALAPPDATA "Programs\Common"
$vst3Directory = Join-Path $commonPlugins "VST3"
$clapDirectory = Join-Path $commonPlugins "CLAP"
$vst3Target = Join-Path $vst3Directory "PitchTrails.vst3"
$clapTarget = Join-Path $clapDirectory "PitchTrails.clap"

if (-not (Test-Path $vst3Source)) {
  throw "PitchTrails.vst3 is missing beside this installer."
}

if (-not (Test-Path $clapSource)) {
  throw "PitchTrails.clap is missing beside this installer."
}

New-Item -ItemType Directory -Force -Path $vst3Directory | Out-Null
New-Item -ItemType Directory -Force -Path $clapDirectory | Out-Null

if (Test-Path $vst3Target) {
  Remove-Item -Recurse -Force $vst3Target
}
if (Test-Path $clapTarget) {
  Remove-Item -Force $clapTarget
}

Copy-Item -Recurse $vst3Source $vst3Target
Copy-Item $clapSource $clapTarget

Write-Host "Installed Valley Spirit VST3 and CLAP test builds."
Write-Host "Reopen your DAW and rescan plug-ins if Valley Spirit does not appear."
