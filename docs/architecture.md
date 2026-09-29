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
Threefold Palm  Valley Spirit
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
  -> optional smoothed automatic gain compensation
  -> output trim
```

The alpha uses subtractive complementary splits so the unprocessed bands reconstruct the input. The crossover implementation is intentionally isolated so it can later be replaced by a higher-order design without changing the plugin adapter.

Per-band gain-reduction values stay inside the DSP processor until the adapter publishes one reading per band through relaxed atomics. The animated editor reads those values without locking the audio thread. Auto Match compares long-term dry and post-mix energy, ignores silence, clamps compensation, and leaves Output as the final intentional trim.

## Valley Spirit signal path

```text
manual time or host-tempo division
  -> smoothed input injection / Freeze state
  -> input + bounded feedback selected from Reflection, Spiral, or Cloud
  -> circular delay
  -> two crossfaded variable-delay read heads
  -> diffusion all-pass stages
  -> feedback memory
  -> dry/wet mix
```

The adapter translates host tempo and time signature into milliseconds, keeping the DSP independent of host APIs. Reflection feeds back the unpitched delay, Spiral feeds back the pitched signal before diffusion, and Cloud feeds back the pitched/diffused signal. Freeze fades new input out while raising a damped, soft-limited feedback path. The pitch algorithm remains a lightweight time-domain prototype; formal listening tests will determine whether the production version retains it or moves to a higher-quality granular or phase-vocoder implementation.
