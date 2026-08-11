# Architecture

## Dependency direction

```text
DAW / standalone host
        |
        v
iPlug2 adapter targets
        |
        v
wave_factory_dsp
  |              |
  v              v
Threefold Palm  Pitch Trails
```

The DSP library has no dependency on iPlug2, a UI framework, networking, file I/O, or host state. Plugin adapters translate host parameters and buffers into the DSP API.

## Real-time constraints

- Allocate buffers during `Prepare`, never inside the per-sample processing path.
- Keep parameters bounded before they enter DSP calculations.
- Avoid locks, network access, file access, logging, and exceptions on the audio thread.
- Keep stereo dynamics linked where independent gain changes would disturb the image.
- Reset state explicitly when sample rate or channel configuration changes.

## Threefold Palm signal path

```text
input
  -> complementary low / mid / high split
  -> linked-stereo envelope and gain calculation per band
  -> character-dependent soft saturation
  -> recombine
  -> dry/wet mix
  -> output trim
```

The alpha uses subtractive complementary splits so the unprocessed bands reconstruct the input. The crossover implementation is intentionally isolated so it can later be replaced by a higher-order design without changing the plugin adapter.

## Pitch Trails signal path

```text
input + bounded feedback
  -> circular delay
  -> two crossfaded variable-delay read heads
  -> diffusion all-pass stages
  -> feedback memory
  -> dry/wet mix
```

The pitch algorithm is a lightweight time-domain prototype. Formal listening tests will determine whether the production version retains it or moves to a higher-quality granular or phase-vocoder implementation.
