# Valley Spirit Roadmap

## Product promise

Valley Spirit is a pitch-shifting echo instrument: a sound enters an open space, travels, and returns transformed. Its design should reward live gestures as much as static settings.

## Current signal path

Bounded circular delay → dual crossfaded variable-delay pitch read → two-stage diffusion → feedback memory → dry/wet mix.

## Phase 1: rhythmic and performable energy

### Tempo sync

- Add an appended Sync boolean and note-division enum.
- Read host tempo once per processing block with a safe 120 BPM fallback.
- Convert musical divisions to milliseconds in the adapter; the DSP remains host-independent.
- Initial divisions: 1/16, 1/8 triplet, 1/8, dotted 1/8, 1/4 triplet, 1/4, dotted 1/4, 1/2, and 1 bar.
- When Sync is enabled, the interface should show the chosen division instead of implying that the millisecond control is active.

### Freeze

- Add an appended boolean parameter.
- Fade input injection out and the recirculation level up instead of switching instantly.
- Use bounded feedback, slight damping, and soft limiting so frozen material can sustain without uncontrolled growth.
- Continue passing the dry signal according to Mix so Freeze remains playable as an effect gesture.

### Feedback Path

Path controls which transformations become part of the next return:

| Path | Feedback source | Audible behavior |
| --- | --- | --- |
| Reflection | unpitched delayed signal | stable repeated interval; pitch is heard without compounding |
| Spiral | pitched signal before diffusion | pitch accumulates on each return |
| Cloud | pitched and diffused signal | pitch and mist accumulate together |

The names belong to the Valley Spirit philosophy and the help panel must explain the actual routing plainly.

## Phase 2: composed motion

- Pitch step sequencing
- Key and scale constraints
- Envelope-driven movement and controlled random modulation
- Optional feedback placement refinements based on listening tests

These are deferred until the Phase 1 routing is stable, because sequencing and scale logic depend on trustworthy pitch and feedback behavior.

## Acceptance criteria

- Tempo-synced repeats follow host tempo and use the fallback safely when tempo is unavailable.
- Freeze enters and exits without a hard click and remains finite during long tests.
- Reflection, Spiral, and Cloud produce measurably distinct feedback behavior.
- Existing sessions retain the prior Cloud-like behavior because new parameters default to Sync off, Freeze off, and Cloud.
- The new controls fit the illustrated design system and remain legible at the shipped editor size.
- Help content explains the philosophy and every control at a comfortable reading size.

