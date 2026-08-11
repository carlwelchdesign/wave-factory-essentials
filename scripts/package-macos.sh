#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${1:-${repo_root}/build/plugins/out/Release}"
dist_root="${2:-${repo_root}/dist}"
threefold_version="$(sed -n 's/^#define PLUG_VERSION_STR "\([^"]*\)"/\1/p' "${repo_root}/plugins/Goodband/config.h")"
valley_version="$(sed -n 's/^#define PLUG_VERSION_STR "\([^"]*\)"/\1/p' "${repo_root}/plugins/PitchTrails/config.h")"
threefold_name="Threefold-Palm-${threefold_version}-macOS-Universal"
valley_name="Valley-Spirit-${valley_version}-macOS-Apple-Silicon"

for bundle in Goodband.component Goodband.vst3 Goodband.clap PitchTrails.component PitchTrails.vst3 PitchTrails.clap; do
  [[ -d "${build_root}/${bundle}" ]] || { echo "Missing release bundle: ${bundle}" >&2; exit 1; }
  codesign --verify --deep --strict "${build_root}/${bundle}"
done

for binary in "${build_root}/Goodband.component/Contents/MacOS/Goodband" "${build_root}/Goodband.vst3/Contents/MacOS/Goodband" "${build_root}/Goodband.clap/Contents/MacOS/Goodband"; do
  architectures="$(lipo -archs "${binary}")"
  [[ "${architectures}" == *arm64* && "${architectures}" == *x86_64* ]] || { echo "Threefold Palm is not universal: ${binary}" >&2; exit 1; }
done

for binary in "${build_root}/PitchTrails.component/Contents/MacOS/PitchTrails" "${build_root}/PitchTrails.vst3/Contents/MacOS/PitchTrails" "${build_root}/PitchTrails.clap/Contents/MacOS/PitchTrails"; do
  [[ "$(lipo -archs "${binary}")" == *arm64* ]] || { echo "Valley Spirit is missing its Apple Silicon slice: ${binary}" >&2; exit 1; }
done

rm -rf "${dist_root:?}/${threefold_name}" "${dist_root:?}/${valley_name}"
rm -f "${dist_root}/${threefold_name}.zip" "${dist_root}/${valley_name}.zip"
for product in "${threefold_name}" "${valley_name}"; do
  mkdir -p "${dist_root}/${product}/Plug-Ins/AU" "${dist_root}/${product}/Plug-Ins/VST3" "${dist_root}/${product}/Plug-Ins/CLAP"
done

ditto "${build_root}/Goodband.component" "${dist_root}/${threefold_name}/Plug-Ins/AU/Goodband.component"
ditto "${build_root}/Goodband.vst3" "${dist_root}/${threefold_name}/Plug-Ins/VST3/Goodband.vst3"
ditto "${build_root}/Goodband.clap" "${dist_root}/${threefold_name}/Plug-Ins/CLAP/Goodband.clap"
cp "${repo_root}/packaging/README-Threefold-Palm-macOS.txt" "${dist_root}/${threefold_name}/README.txt"
cp "${repo_root}/packaging/Install-Threefold-Palm.command" "${dist_root}/${threefold_name}/Install-Threefold-Palm.command"

ditto "${build_root}/PitchTrails.component" "${dist_root}/${valley_name}/Plug-Ins/AU/PitchTrails.component"
ditto "${build_root}/PitchTrails.vst3" "${dist_root}/${valley_name}/Plug-Ins/VST3/PitchTrails.vst3"
ditto "${build_root}/PitchTrails.clap" "${dist_root}/${valley_name}/Plug-Ins/CLAP/PitchTrails.clap"
cp "${repo_root}/packaging/README-Valley-Spirit-macOS.txt" "${dist_root}/${valley_name}/README.txt"
cp "${repo_root}/packaging/Install-Valley-Spirit.command" "${dist_root}/${valley_name}/Install-Valley-Spirit.command"

chmod +x "${dist_root}/${threefold_name}/Install-Threefold-Palm.command" "${dist_root}/${valley_name}/Install-Valley-Spirit.command"
(
  cd "${dist_root}"
  COPYFILE_DISABLE=1 zip -qry -X "${threefold_name}.zip" "${threefold_name}"
  COPYFILE_DISABLE=1 zip -qry -X "${valley_name}.zip" "${valley_name}"
)

echo "${dist_root}/${threefold_name}.zip"
echo "${dist_root}/${valley_name}.zip"
