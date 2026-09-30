# PadSampler implementation and acceptance

## Product contract

A macOS 12+ standalone/AU/VST3 instrument with four physical-pad positions and two
external-trigger slots. MIDI comes from the SamplePad 4; audio plays on the computer.
One sample per slot, independent velocity-to-volume and velocity-to-low-pass tone.
Source lives in `plugins/PadSampler`, generated from starter 1.0.0 and graduated to
an explicit product target because its formats and instrument behavior differ from
the reference effects. The starter templates and packaging contracts remain intact.

See [product guide](../plugins/PadSampler/README.md) for sample limits, stable parameter
IDs, kit/state format, setup, dependency choices and build commands.

## Verified locally — September 29, 2026 (America/Los_Angeles)

- Universal arm64/x86_64 standalone, AU and VST3 targets built with Xcode 27.
- All nine registered CTest checks passed, including the new sampler suite.
- Sampler tests cover high-frequency energy increasing with velocity, cutoff/Nyquist
  bounds, independent amplitude response and tone bypass, sample-offset MIDI,
  simultaneous six-source playback, channel/zero-velocity handling, MIDI Learn,
  bounded event overflow, 64-voice stealing, Stop All fades, sample-rate reset,
  no audio-scope C++ allocations, and retained asset lifetime through replacement.
- File tests cover float WAV round trips, stereo preservation, 16-bit AIFF conversion,
  corrupt/missing files, 60-second and decoded-memory limits, failed replacement
  preserving old audio, portable-kit collection and reopening after moving the kit
  and removing the original source file.
- pluginval 1.0.4 strictness 5/seed 12345 passed, including editor, editor automation,
  processing and plugin-state tests. These validator runs start with empty slots;
  loaded sample behavior is covered by the sampler suite and still needs host QA.
- Apple `auval -v aumu WfP6 WvFy` passed after AudioComponentRegistrar discovered the
  temporarily installed AU. Initial immediate discovery failed; the validation copy
  was removed. No existing installed plugin was overwritten or cache cleared.
- The standalone window was opened and captured in the real renderer. Six-slot
  geometry, controls, and readable layout were observed. A synthetic WAV was loaded
  through the native file picker, with filename, waveform and Ready status verified.
  An actual macOS file drag also loaded a second slot; Help opened and Escape
  dismissed it with the pointer outside the panel. This does not establish
  physical performance or listening quality.
- System USB inspection did not find the SamplePad, and CoreMIDI listed zero input
  sources at the local checkpoint. Physical independence cannot yet be claimed.

Raw local build/validator logs are retained in `build/evidence/padsampler` when packaged.
CI publishes fresh validation logs and universal tester ZIPs on the review PR.

## Required host checklist before release

Record tester commit, macOS/CPU, audio interface/driver, sample rate, buffer size,
SamplePad firmware/settings, USB connection, splitter and connected trigger models.

1. At a 128-frame buffer, strike all four pads and both external triggers separately.
   Record note/channel and minimum/maximum usable velocity for each. Verify no
   indistinguishable Tip/Ring MIDI events; flag hardware limitations explicitly.
2. Load WAV and AIFF by actual file drop and native file picker. Check waveform and
   name, audition each slot, rename it, replace while ringing, clear, then relink.
3. Repeatedly open Help, Close, reopen and dismiss with Escape while the mouse is
   outside the panel. Repeatedly use Load, MIDI Learn and Save/Open Kit; buttons must
   return to their resting appearance after activation and remain usable.
4. Listen to soft/medium/hard strikes with matched level and tone bypass. Confirm
   the brighter attack follows velocity, then adjust the volume response separately.
   Check rolls, simultaneous notes, ringing tails, voice stealing and Stop All.
5. Save a kit, move its entire folder, reopen it, then test a deliberately missing
   sample and relink only that slot. Existing kits must not be overwritten.
6. In Logic, save/close/reopen a session with loaded samples and automation. Wait for
   sample decoding to report Ready before playback/offline render; host state stores
   references and restores samples asynchronously. Repeat editor open/close and
   multiple-instance playback. Verify independent instances and no stuck notes.
7. Measure end-to-end trigger-to-audio latency using a recorded physical reference
   and output, and report measured samples/ms separately from nominal buffer latency.

## Delivery boundaries

The first tester is ad-hoc signed. Developer ID signing/notarization, physical-device
and Logic/listening acceptance remain release gates. No Windows product, CLAP product,
velocity layers, round-robin, hardware firmware changes or SD-card export are included.
Production integration follows working branch → reviewed release branch → verified
release → main. A review PR or green CI is not a production release.

## Integration correction

Windows starter run 36671995680 exposed a pre-existing parallel-link race: VST3
and CLAP emitted the same product `.lib`/`.exp` paths and MSVC reported LNK1104.
The starter now gives each format its own archive-output directory. Parallel builds
remain enabled in CI as the regression reproducer; deployed bundle names are unchanged.

## Precision revision

The approved silver/graphite/cobalt interface replaces the initial engineering layout.
See the product README for curve gestures, preset behavior and version-1 compatibility.
The custom graph reuses iPlug2 input/drawing and existing parameter controls; no new UI
framework. Code-authored satin material is generated reproducibly by
`scripts/generate-padsampler-materials.py` at native and Retina resolution, with all text,
knobs, values, waveforms and interaction states rendered independently.

Version-2 kit/state documents retain the original 56 numeric parameters and add six
`curves` objects, each containing `legacy` and 2–8 ordered `[x,y]` points. Endpoints are
fixed at [0,0] and [1,1]. Invalid documents are rejected before adapter mutation. Version-1
state retains exact power-law evaluation until an intentional conversion. Point curves
use linear segments in normalized brightness followed by logarithmic Hz interpolation.
Curve snapshots use atomic scalar storage and a generation check: the audio reader
attempts once per block, retaining the prior complete snapshot on contention. Writers
serialize off the rendering callback; no mutex, allocation or retry loop enters ProcessBlock.

See [Precision verification](padsampler-precision-verification.md) for actual-renderer
evidence, interaction regression instructions and remaining acceptance.

## Clip trimming revision

Version-3 kit/state documents add a `clip` object to each slot with `start` seconds
and an optional `end` seconds (`null` means the full source end). Version-1 and
version-2 documents play full-length. Validation checks all six ranges before any
document is applied. The original 56 parameter IDs and plugin identifiers remain
unchanged. Trim is session state and commits notify the host.

The editor's Tone / Clip switch uses the graph area for a larger waveform and
start/end handles. Excluded audio is shaded in this view and on the small pad
waveform. Both time fields accept seconds. Arrow keys move a focused handle by
10 ms; Shift moves one source frame. Start and end cannot cross, and the playable
range must contain at least two frames. Clip Reset Length and 32-entry Undo/Redo
are independent of the tone curve's history.

Decoded buffers are never shortened. A single atomic word publishes complete
frame bounds to new voices; each voice captures its own exclusive end. Ringing
voices continue with their captured bounds and source reference. Portable kits
collect the full audio so a moved kit can still reset or change its trim. Failed
replacement preserves the old audio and trim. Missing samples retain their saved
times for relink; a too-short relink resets to full playback with a visible warning.

For host acceptance, drag both handles, use keyboard and numeric editing, Reset,
Undo/Redo, switch pads, save/reopen a kit and a Logic session, and verify that
hits already ringing keep their original length. This remains separate from
physical SamplePad and listening acceptance.
