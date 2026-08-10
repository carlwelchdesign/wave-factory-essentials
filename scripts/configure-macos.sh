#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dependency_link="${TMPDIR:-/tmp}/wave-factory-essentials-iplug2"

git -C "${repo_root}" submodule update --init --recursive
ln -sfn "${repo_root}/third_party/iPlug2" "${dependency_link}"

cmake \
  -S "${repo_root}" \
  -B "${repo_root}/build/plugins" \
  -G Xcode \
  -DIPLUG2_DIR="${dependency_link}" \
  -DIPLUG_DEPLOY_PLUGINS=OFF \
  -DBUILD_TESTING=ON
