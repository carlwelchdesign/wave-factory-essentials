#!/bin/zsh
set -euo pipefail

package_dir="${0:A:h}"
source_root="${package_dir}/Plug-Ins"
backup_root="${HOME}/Library/Application Support/Threefold Palm and Valley Spirit/Backups"
timestamp="$(date +%Y%m%d-%H%M%S)"

if pgrep -x "Logic Pro" >/dev/null 2>&1; then
  echo "Logic Pro is open. Quit Logic Pro, then run this installer again."
  read -r "?Press Return to close this window."
  exit 1
fi

typeset -a sources destinations backup_names
sources=("${source_root}/AU/PitchTrails.component" "${source_root}/VST3/PitchTrails.vst3" "${source_root}/CLAP/PitchTrails.clap")
destinations=("${HOME}/Library/Audio/Plug-Ins/Components/PitchTrails.component" "${HOME}/Library/Audio/Plug-Ins/VST3/PitchTrails.vst3" "${HOME}/Library/Audio/Plug-Ins/CLAP/PitchTrails.clap")
backup_names=("PitchTrails-${timestamp}.component" "PitchTrails-${timestamp}.vst3" "PitchTrails-${timestamp}.clap")

mkdir -p "${backup_root}"
for index in {1..3}; do
  source_plugin="${sources[$index]}"
  destination_plugin="${destinations[$index]}"
  [[ -d "${source_plugin}" ]] || { echo "Missing plug-in bundle: ${source_plugin}"; exit 1; }
  mkdir -p "${destination_plugin:h}"
  [[ ! -d "${destination_plugin}" ]] || mv "${destination_plugin}" "${backup_root}/${backup_names[$index]}"
  ditto "${source_plugin}" "${destination_plugin}"
  xattr -cr "${destination_plugin}"
  codesign --verify --deep --strict "${destination_plugin}"
done

killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true
set +e
/usr/bin/auval -v aufx WfPt WvFy
validation_status=$?
set -e
echo "Installed Valley Spirit 0.1.5. Apple validation status: ${validation_status}"
read -r "?Press Return to close this window."
exit ${validation_status}
