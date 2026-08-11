# Project History and Decisions

## Origin

The line began with member requests for focused music-production tools. The two selected non-AI ideas were:

- a multiband “sound-good” processor that was not Maximus
- a delay/reverb effect with pitch shifting that did not require automation

The product boundary is deterministic local DSP. No AI processing, accounts, telemetry, uploads, or hosted services are required.

## Products

### Threefold Palm

- Public identity: **Threefold Palm**
- Stable internal identity: `Goodband`
- Meaning: a controlled three-band finishing technique
- Current controls: Character, Amount, Mix, Output
- Character modes: Clean, Warm, Punch, Wide
- Positioning: three-band dynamics, saturation, and mastering energy

### Valley Spirit

- Public identity: **Valley Spirit**
- Stable internal identity: `PitchTrails`
- Meaning: the valley receives energy and returns it transformed
- Current controls: Echo Time, Pitch, Feedback, Diffusion, Mix
- Positioning: pitch-shifting delay and diffusion

## Product and visual decisions

- Function creates the mythology; cultural references are transformed into an original product world rather than copied.
- Highly illustrated raster assets are the visual foundation. Generic vector approximations are not the target aesthetic.
- Background art must preserve aspect ratio and ship in prefiltered densities to prevent distortion and jagged scaling.
- Decorative frames extend to the actual host-visible edge.
- Animated Sensei gestures use complete alternate scene frames and crossfades.
- Magical energy uses textured sprites, layered additive ribbons, sparks, runes, and wisps rather than primitive circles or lines.
- Motion responds to settings and briefly intensifies after interaction, but it remains bounded and deterministic.
- Illustrated controls still require readable labels, live values, and a legible help panel.

## Engineering decisions

- iPlug2 is the host adapter and packaging layer.
- Framework-independent C++ processors own the DSP.
- Public product names may change, but bundle filenames, manufacturer IDs, plugin IDs, and existing parameter indices remain stable.
- Cross-platform resources must be registered for both macOS and Windows.
- A signed installed bundle is immutable; resource changes require rebuilding and signing the complete bundle.

## Current product strategy

The next release should deepen the two validated plugins before starting a third one. The first expansion phase adds controls that improve trust and playability without destabilizing the core sound:

- Threefold Palm gains metering, level-matched judgment, and useful starting Forms.
- Valley Spirit gains rhythmic integration, performable sustain, and explicit control over how transformations accumulate.

