# Threefold Palm Roadmap

## Product promise

Threefold Palm is a fast three-band finishing processor for adding cohesion, weight, punch, warmth, air, or width without exposing a conventional mastering-console workflow.

## Current signal path

Complementary low/mid/high split → linked-stereo envelope control → character-dependent saturation and makeup → recombination → optional stereo widening → dry/wet mix → output trim.

## Phase 1: visible mastery

### Three-band gain-reduction meters

- Show low, mid, and high reduction independently.
- Use positive dB-of-reduction readings even though the internal gain calculation is negative.
- Publish readings from the audio adapter through lock-free atomics; the editor only reads them.
- Use restrained jade/gold arcs integrated around the three illustrated controls rather than generic rectangular meters.
- Meter decay should be readable without pretending to be a laboratory analyzer.

### Automatic gain matching

- Add an appended boolean parameter; default off for backward-compatible sound.
- Compare smoothed dry energy with the post-character, post-mix signal before Output trim.
- Ignore silence, clamp compensation to a safe range, and smooth changes slowly enough to avoid pumping.
- Keep Output as an intentional final trim after automatic matching.
- Show the current compensation in the interface and explain that matching supports honest comparison rather than loudness maximization.

### Factory Forms

Forms are complete, host-visible starting points. Character remains the tone/behavior choice; a Form sets Character, Amount, Mix, Output, and Auto Match together.

Initial Forms:

| Form | Intent |
| --- | --- |
| First Stance | Subtle, transparent mix-bus control |
| Iron Center | Dense low-mid authority |
| Striking Drum | Punch and transient focus |
| Parallel Flame | Strong processing blended in parallel |
| Open Hand | Air and width with restrained level |

Selecting a Form must update the host parameters and every visible linked control. Manual edits do not need to preserve a misleading “selected preset” state.

## Phase 2: deeper control

- Adjustable low/mid and mid/high crossover controls with safe ranges and click-free smoothing
- Oversampling for nonlinear stages, with measured quality/CPU choices and latency reporting
- Listening tests on full mixes, drums, vocals, bass, and parallel buses

These features are deferred until Phase 1 is stable in real hosts.

## Acceptance criteria

- Existing sessions retain their original sound when new features are left at defaults.
- Meter activity corresponds to actual compression and falls back cleanly.
- Auto Match stays finite, bounded, smooth, and inactive during silence.
- Every Form produces the documented host parameter values.
- Help content describes meters, Auto Match, Forms, Character, Amount, Mix, and Output at a comfortable reading size.

