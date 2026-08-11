param(
  [string]$BuildDir = "build/windows",
  [string]$DistDir = "dist"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$configText = Get-Content (Join-Path $repoRoot "plugins/Goodband/config.h") -Raw

if ($configText -notmatch '#define PLUG_VERSION_STR "([^"]+)"') {
  throw "Could not read the Threefold Palm version from config.h."
}

$version = $Matches[1]
$buildRoot = Join-Path $repoRoot $BuildDir
$distRoot = Join-Path $repoRoot $DistDir
$packageName = "Threefold-Palm-$version-Windows-x64"
$packageRoot = Join-Path $distRoot $packageName
$zipPath = Join-Path $distRoot "$packageName.zip"
$vst3Source = Join-Path $buildRoot "out/Goodband.vst3"
$clapSource = Join-Path $buildRoot "out/Goodband.clap"

if (-not (Test-Path $vst3Source)) {
  throw "Windows VST3 build is missing: $vst3Source"
}

if (-not (Test-Path $clapSource)) {
  throw "Windows CLAP build is missing: $clapSource"
}

if (Test-Path $packageRoot) {
  Remove-Item -Recurse -Force $packageRoot
}
if (Test-Path $zipPath) {
  Remove-Item -Force $zipPath
}

New-Item -ItemType Directory -Force -Path $packageRoot | Out-Null
Copy-Item -Recurse $vst3Source (Join-Path $packageRoot "Goodband.vst3")
Copy-Item $clapSource (Join-Path $packageRoot "Goodband.clap")
Copy-Item (Join-Path $repoRoot "packaging/README-Windows.txt") (Join-Path $packageRoot "README.txt")
Copy-Item (Join-Path $repoRoot "packaging/Install-Threefold-Palm.ps1") $packageRoot

Compress-Archive -Path $packageRoot -DestinationPath $zipPath -CompressionLevel Optimal
Write-Host $zipPath
