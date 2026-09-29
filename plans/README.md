# Threefold Palm + Valley Spirit Plans

This folder is the durable product and engineering memory for the plugin line. It records what has been decided, what is being built, what remains exploratory, and the validation required before release.

## Current implementation phase

The first expansion phase is intentionally focused:

- **Threefold Palm:** three-band gain-reduction meters, automatic gain matching, and factory presets presented as Forms.
- **Valley Spirit:** host-tempo sync, Freeze, and selectable feedback Path modes.

This phase does **not** include Threefold Palm crossover editing or oversampling, nor Valley Spirit pitch sequencing, key/scale constraints, or envelope-driven movement. Those remain planned follow-ups after the foundational controls have been tested in real sessions.

Implementation status on August 11, 2026: the DSP, host parameters, illustrated controls, help updates, and focused tests are implemented on `agent/vnext-foundations`. Local macOS Audio Unit, VST3, and CLAP builds pass. Installation, `auval`, Logic interaction testing, Windows CI, packaging, and publication remain intentionally pending.

## Plan index

- [Project history and decisions](project-history-and-decisions.md)
- [Threefold Palm roadmap](threefold-palm-roadmap.md)
- [Valley Spirit roadmap](valley-spirit-roadmap.md)
- [Validation and release gates](validation-and-release.md)
- [Product backlog](product-backlog.md)

## Working rules

1. Preserve the stable internal plugin names and IDs (`Goodband` and `PitchTrails`) even when public branding changes.
2. Append parameter IDs; never reorder existing IDs.
3. Keep DSP independent of iPlug2 and the audio thread free of allocation, locks, file/network access, logging, and exceptions.
4. Make important behavior visible in the interface and explain it in the help panel.
5. Treat unit tests, a successful build, Audio Unit validation, and a successful Logic editor load as separate gates.
6. Do not bump versions or publish packages until the implementation has passed the release gates.
