# PadSampler 0.1.0

A six-slot macOS sample instrument for playing expressive one-shots from an Alesis
SamplePad 4 or another MIDI controller. Generated from starter 1.0.0, then graduated
to a product-owned instrument; `starter-manifest.json` records its origin.

## Play

1. Connect the SamplePad by USB. Sound comes from the **computer's audio output**.
   Avoid simultaneously monitoring the SamplePad's built-in sounds.
2. Launch PadSampler.app and open Preferences to choose MIDI input, audio output,
   sample rate and buffer. Start at 128 frames. In Logic, load the AU on a software
   instrument track and route the SamplePad's MIDI to it instead.
3. Drop a mono/stereo uncompressed WAV or AIFF onto a pad, or select it and use Load.
   Click to audition at the chosen audition velocity. Click the selected name to rename.
4. Select each slot and press MIDI Learn, then strike its hardware pad. Repeat for
   External Tip and External Ring with the splitter. Channel 0 means all channels.
   Duplicate note/channel mappings are flagged and intentionally trigger both slots.
5. Soft/Hard Brightness control the low-pass cutoff at low/high playing velocities.
   Hard below Soft holds the effective cutoff at Soft; it never reverses the response.
   Tone Curve shapes the transition. Velocity Volume independently controls loudness.
   Tone Bypass is an A/B comparison. These controls shape new hits; existing voices
   retain the settings captured at their start. Pan is stereo balance.
6. Save Kit creates a **new** `.padkit` folder containing `kit.json` and collected
   float WAV samples at their original levels/sample rates. Open Kit selects that
   `kit.json`. Move/copy the whole folder. Existing kit folders are never overwritten.
7. DAW state saves parameters and sample references, not embedded audio. Keep those
   files or a collected kit available. Missing files silence the affected slots and
   show an error; select Load / Relink to replace them. Failed normal replacement
   leaves the old sample playable. Clear removes a slot for future hits; Stop All
   fades ringing notes. MIDI CC120/123 also stops playback.

Sample limits: 60 seconds, 1–2 channels, 8–384 kHz, PCM/float WAV/AIFF; 256 MiB decoded
memory including queued/retired samples still playing. To release memory, Clear a
slot, Stop All, and let the audio device run before retrying. No automatic normalization,
velocity layers, round-robin, SD-card export or hardware firmware changes.

## Build and verify

Requires CMake, Xcode, pinned iPlug2 submodule and its VST3/CLAP SDK downloads.
This product targets macOS 12+ and universal arm64/x86_64 binaries. Set `IPLUG2_DIR`
to a path without spaces if the dependency checkout path contains spaces.

```
cmake -S . -B build/padsampler -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DIPLUG_DEPLOY_PLUGINS=OFF
cmake --build build/padsampler --config Release --target padsampler-formats padsampler-tests
ctest --test-dir build/padsampler -C Release -R padsampler --output-on-failure
python3 scripts/package-padsampler.py --build-dir build/padsampler
```

The app uses the existing iPlug2 audio/MIDI Preferences and native file dialogs.
No audio callback performs filesystem access, allocation, decoding or locking.
The loader owns sample allocations; a bounded single-producer/audio-consumer queue
transfers references. Each ringing voice retains its original sample. Only the
background worker reclaims assets after their atomic reference count reaches zero.
The 64 voices and 512 MIDI-event slots are preallocated; overflow is visible in the UI.
A 1 ms envelope handles start/end/stop transitions; stolen voices leave a 1 ms
crossfade tail. MIDI note-offs do not gate one-shots.

Parameter IDs: six contiguous blocks of nine (Level, Pan, Velocity Volume,
Soft Brightness, Hard Brightness, Tone Curve, Tone Bypass, MIDI Note, MIDI Channel),
then Audition Velocity=54 and Master=55. Kit/state schema version=1. Instruments:
AU `aumu/WfP6/WvFy`; bundle IDs `com.wavefactoryessentials.{app,audiounit,vst3}.PadSampler`.

Reused dependencies: pinned iPlug2/WDL controls, MIDI/APP host, SVF filter and bundled
nlohmann JSON; Apple AudioToolbox for decoding/encoding. Framework standalone menu
and preferences resources are adapted from its IPlugInstrument example, under the
framework's included license. The portable engine accepts a filter adapter and the
loader has a platform interface. No additional third-party runtime dependency.

## Acceptance boundaries

Tester builds are ad-hoc signed, not Developer ID notarized. Automated DSP, file/kit,
AU and VST3 validation do not establish physical-trigger independence, listening
quality or round-trip latency. Verify all six hardware sources and their velocity
ranges with the real splitter. If Tip/Ring produce the same MIDI events, software
cannot distinguish them. Logic session recall, actual file-drop interactions, live
rolls and device/buffer latency need the recorded host checklist in docs/padsampler.md.

## Precision interface and editable curves

The Precision editor uses satin aluminum, graphite pads and cobalt selection accents.
Selection is separate from the short hit flash and each slot's load/error state. Motion
can be disabled in the footer. Tab moves focus; Enter activates buttons or edits values;
Space auditions a focused pad. Help includes its own Close action and Escape dismissal.

Each pad has an independent tone curve with fixed endpoints and up to six interior
points. Double-click the graph to add a point, drag it, or select it and use arrow keys
(Shift for fine adjustment). Delete removes an interior point. The two numeric fields
edit velocity and normalized brightness. Points cannot cross or turn downward. The
cutoff readout translates brightness into Hz between Soft and Hard using a logarithmic
frequency range, safely clamped below Nyquist. MIDI velocity zero does not trigger a hit.

Linear, Early Open, Late Open and S-Curve presets affect only the selected pad. Edited
patterns read Custom. Undo/Redo retains 32 curve edits per pad; an entire drag is one edit.
Histories reset when loading a kit/session. Edits affect the next strike, never a ringing
voice. Tone bypass preserves the curve for comparison and does not disable editing.

Curves are kit/session state, not individual host automation parameters. Version-1
kits and sessions load in Legacy mode, preserving the original exponent calculation and
its automation. Editing a point or choosing a preset converts the selected pad to point
mode. Undo can restore Legacy mode. The original exponent parameter retains its ID but
has no effect in point mode. New saves use version 2; older PadSampler binaries cannot
read version-2 kits. Existing 56 parameter IDs and plugin identity remain unchanged.
