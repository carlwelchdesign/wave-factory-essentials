param(
  [string]$BuildDir = "build/windows",
  [string]$DistDir = "dist"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$goodbandConfigText = Get-Content (Join-Path $repoRoot "plugins/Goodband/config.h") -Raw
$valleySpiritConfigText = Get-Content (Join-Path $repoRoot "plugins/PitchTrails/config.h") -Raw

if ($goodbandConfigText -notmatch '#define PLUG_VERSION_STR "([^"]+)"') {
  throw "Could not read the Threefold Palm version from config.h."
}

$goodbandVersion = $Matches[1]
if ($valleySpiritConfigText -notmatch '#define PLUG_VERSION_STR "([^"]+)"') {
  throw "Could not read the Valley Spirit version from config.h."
}

$valleySpiritVersion = $Matches[1]
$buildRoot = Join-Path $repoRoot $BuildDir
$distRoot = Join-Path $repoRoot $DistDir
$goodbandPackageName = "Threefold-Palm-$goodbandVersion-Windows-x64"
$goodbandPackageRoot = Join-Path $distRoot $goodbandPackageName
$goodbandZipPath = Join-Path $distRoot "$goodbandPackageName.zip"
$goodbandVst3Source = Join-Path $buildRoot "out/Goodband.vst3"
$goodbandClapSource = Join-Path $buildRoot "out/Goodband.clap"
$valleySpiritPackageName = "Valley-Spirit-$valleySpiritVersion-Windows-x64"
$valleySpiritPackageRoot = Join-Path $distRoot $valleySpiritPackageName
$valleySpiritZipPath = Join-Path $distRoot "$valleySpiritPackageName.zip"
$valleySpiritVst3Source = Join-Path $buildRoot "out/PitchTrails.vst3"
$valleySpiritClapSource = Join-Path $buildRoot "out/PitchTrails.clap"

if (-not (Test-Path $goodbandVst3Source)) {
  throw "Windows VST3 build is missing: $goodbandVst3Source"
}

if (-not (Test-Path $goodbandClapSource)) {
  throw "Windows CLAP build is missing: $goodbandClapSource"
}

if (-not (Test-Path $valleySpiritVst3Source)) {
  throw "Windows VST3 build is missing: $valleySpiritVst3Source"
}

if (-not (Test-Path $valleySpiritClapSource)) {
  throw "Windows CLAP build is missing: $valleySpiritClapSource"
}

foreach ($path in @($goodbandPackageRoot, $goodbandZipPath, $valleySpiritPackageRoot, $valleySpiritZipPath)) {
  if (Test-Path $path) {
    Remove-Item -Recurse -Force $path
  }
}

New-Item -ItemType Directory -Force -Path $goodbandPackageRoot | Out-Null
Copy-Item -Recurse $goodbandVst3Source (Join-Path $goodbandPackageRoot "Goodband.vst3")
Copy-Item $goodbandClapSource (Join-Path $goodbandPackageRoot "Goodband.clap")
Copy-Item (Join-Path $repoRoot "packaging/README-Windows.txt") (Join-Path $goodbandPackageRoot "README.txt")
Copy-Item (Join-Path $repoRoot "packaging/Install-Threefold-Palm.ps1") $goodbandPackageRoot

New-Item -ItemType Directory -Force -Path $valleySpiritPackageRoot | Out-Null
Copy-Item -Recurse $valleySpiritVst3Source (Join-Path $valleySpiritPackageRoot "PitchTrails.vst3")
Copy-Item $valleySpiritClapSource (Join-Path $valleySpiritPackageRoot "PitchTrails.clap")
Copy-Item (Join-Path $repoRoot "packaging/README-Valley-Spirit-Windows.txt") (Join-Path $valleySpiritPackageRoot "README.txt")
Copy-Item (Join-Path $repoRoot "packaging/Install-Valley-Spirit.ps1") $valleySpiritPackageRoot

Compress-Archive -Path $goodbandPackageRoot -DestinationPath $goodbandZipPath -CompressionLevel Optimal
Compress-Archive -Path $valleySpiritPackageRoot -DestinationPath $valleySpiritZipPath -CompressionLevel Optimal
Write-Host $goodbandZipPath
Write-Host $valleySpiritZipPath
