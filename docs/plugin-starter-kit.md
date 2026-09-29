# Wave Factory plugin starter kit

Create a new Essentials effect without asking a model to copy and rename an existing product.
The generator uses Python 3.9+ and the standard library; generated DSP uses C++17 and the
repository's pinned iPlug2. No additional production dependency is introduced.

## Create an effect

1. Copy `starter/manifests/StarterGain.json` to a manifest for your product. Set all five
   fields: internal name, display name, four-character unique ID, bundle identity base,
   and semantic version. The identity base is `com.wavefactoryessentials.<InternalName>`;
   AU/VST3/CLAP insert their format segment before the internal name.
2. Run `python3 scripts/new-plugin.py --manifest <manifest> --dry-run` to inspect output.
3. Run the same command without `--dry-run`. Output goes to `plugins/generated/<InternalName>`.
4. Reconfigure CMake. A manifest-bearing generated directory automatically registers its
   format targets and DSP tests. Stage the new product explicitly when it is ready for review.

Names and IDs are checked against both existing and generated plugins. A repeat invocation
fails without overwriting owned code. Rendering is staged on the same filesystem and renamed
into place under an exclusive generation lock. If another process is generating, retry after
it finishes. A lock left by an interrupted process is an explicit error: inspect the process
and `.starter-*` staging directories before removing the stale lock. Never delete another
agent's active lock or product. Dry-run creates no directories or lock files.

Starter version `1.0.0` is recorded in `starter-manifest.json`. Generated source belongs to the
product; template updates do not overwrite it. Review and port future starter fixes deliberately.

## Build and verify

For dependency setup and macOS builds, reuse `scripts/configure-macos.sh`. It initializes
the pinned submodule and uses a temporary dependency path without spaces to work around
an upstream Xcode prefix-header issue. It also disables deployment into host plugin folders.
After running it:

```sh
cmake --build build/plugins --config Release --target wfe-generated-plugins wfe-starter-checks
ctest --test-dir build/plugins -C Release --output-on-failure
python3 scripts/package-generated.py --build-dir build/plugins --platform macOS
```

For a universal build, explicitly reconfigure with `-DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'`
before building. Verify both slices with `lipo -archs`; do not infer them from the ZIP name.

On Windows, initialize the submodule, run its VST3 and CLAP SDK download scripts, then:

```powershell
cmake -S . -B build/starter -G "Visual Studio 17 2022" -A x64 -DIPLUG_DEPLOY_PLUGINS=OFF -DWFE_BUILD_SPIRIT_MIRROR=OFF -DBUILD_TESTING=ON
cmake --build build/starter --config Release --target wfe-generated-plugins wfe-starter-checks
ctest --test-dir build/starter -C Release --output-on-failure
python scripts/package-generated.py --build-dir build/starter --platform Windows
```

Run fast contracts independently with `python3 tests/test_plugin_starter.py`. To test DSP
without iPlug2, configure with `-DWFE_BUILD_PLUGINS=OFF -DWFE_BUILD_SPIRIT_MIRROR=OFF`,
build, and run CTest. CI generates both StarterGain and StarterTrim in its disposable
checkout and checks platform builds, existing regressions, DSP and packaging. It also runs
[pluginval 1.0.4](https://github.com/Tracktion/pluginval) at strictness 5 for generated VST3
state/editor validation. This GPL-3.0 tool runs as a separate development executable, is
pinned to a release, and is neither linked into nor packaged with the plugins. Do not commit
these two reference fixtures as released products: their manifests are test input.

The packager enumerates registered manifests, requires every platform format, checks macOS
bundle identity/version and asset presence, preserves executable permission bits in ZIPs,
and writes SHA-256 sidecars. It rejects ambiguous build configurations and empty binaries.
Use a dedicated build directory rather than mixing Debug and Release artifacts. It does not
install, sign, notarize, publish, or imply real-host acceptance.

## Customize the product

The initial audible contract is smoothed mono/stereo gain (-60 to +12 dB), bypass, and Unity /
Headroom presets. Stable parameter indices are Gain=0 and Bypass=1; append future parameters.
State uses iPlug2's normal parameter serialization. The adapter reads host parameters at block
boundaries, and DSP smooths both gain and bypass transitions with a 10 ms time constant.
It performs no allocation, locking, I/O, logging, or exceptions on the audio callback.

`GainProcessor.h` owns audio behavior; the plugin adapter owns parameter and buffer translation.
Reuse `plugins/shared/WaveFactoryUI.h` and iPlug2 controls. The reference layout is functional
scaffolding, not a finished illustrated product. Help includes a visible Close area and handles
Escape when the host forwards it. Confirm focus behavior in the real host.

System Arial is loaded before controls. Add artwork as PNG 1x/2x pairs under `resources/img`
and fonts as TTF under `resources/fonts`; CMake derives macOS and Windows resource lists from
the same asset set. Resource basenames must be unique. Explicitly load any new fonts before
controls and apply the existing asset-specific frame and real-renderer acceptance gates.

## Host acceptance and delivery

Close DAWs before installing a complete tester bundle. For local macOS testing, ad-hoc sign
the complete generated bundle with `codesign --force --sign - --timestamp=none <bundle>`
and verify it with `codesign --verify --deep --strict <bundle>`. Ad-hoc signing is not
production signing or notarization. Validate the generated AU with
`auval -v aufx <unique_id> WvFy`, then verify in Logic: audio, every control, presets,
automation, save/reopen, repeated editor open/close, help open/close/reopen/Escape and multiple
instances. Exercise VST3 and CLAP in compatible macOS and Windows hosts. Cover mono/stereo,
44.1/48/96 kHz, buffer changes, offline render, bypass and sustained playback. Capture exact
host/version/architecture and screenshots from the real renderer. A passing build or auval
is not this acceptance evidence.

Keep one active Asana parent per workstream. Record verification evidence on its scoped
subtasks and move blocked work back to Backlog. Integrate reviewed changes through a release
branch cut from main, run its checks, and deploy only from that release branch. Merge it back
only after deployment verification. Starter readiness does not complete commercial signing,
notarization, store delivery, or release activation tickets.

## Agent context and measurements

Read this file, the product manifest and relevant DSP/adapter symbols first. Use Serena for
bounded live code lookup where available, and the private vault for task handoff. Never ship
private notes with plugin resources. Generation and test-result extraction are deterministic;
no model inference is necessary. Use the local-AI router dry-run before any offload, and only
qualified categories may use local inference. Consequential edits and verification stay with
Codex. Do not claim token savings from cached-token percentages or successful process exits.

Record generation wall time, time to first passing build, failed attempts and corrections,
context size when available, and comparable baseline conditions. Two generated fixtures prove
name independence; they do not by themselves prove productivity or token savings.

Current local results and open gates: [verification record](plugin-starter-verification.md).
