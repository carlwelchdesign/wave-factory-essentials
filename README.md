# Wave Factory Essentials

Wave Factory Essentials is a small line of free, offline audio plugins shaped by requests from Wave Factory members.

The first two products are:

- **Threefold Palm** — an approachable three-band dynamics and saturation processor that makes its processing understandable. The internal `Goodband` target and Audio Unit identity remain stable for compatibility with existing test sessions.
- **Pitch Trails** — a pitch-shifting delay and diffusion effect that creates musical movement without drawn automation.

## Product boundaries

- Local DSP only: no AI, accounts, telemetry, uploads, or hosted services.
- Separate focused plugins rather than one overloaded interface.
- Framework-independent DSP with thin iPlug2 adapters.
- Honest auditioning through dry/wet controls and bounded parameters.
- VST3, Audio Unit, and CLAP development targets on macOS.

This repository is an early engineering foundation, not a distributable release. Signing, notarization, installer work, DAW compatibility testing, sound-design review, accessibility review, and final licensing remain release gates.

## Build

Requirements on macOS:

- CMake 3.25+
- Xcode command-line tools
- Git submodules initialized

```sh
git submodule update --init --recursive
./scripts/configure-macos.sh
cmake --build build/plugins --config Debug
ctest --test-dir build/plugins -C Debug --output-on-failure
```

The configure script supplies iPlug2 through a temporary path without spaces. This works around an upstream Xcode prefix-header flag issue when the repository lives under `Github Projects`; it does not move or duplicate the dependency.

For fast DSP-only development:

```sh
cmake -S . -B build/dsp -DWFE_BUILD_PLUGINS=OFF
cmake --build build/dsp
ctest --test-dir build/dsp --output-on-failure
```

See [docs/product-brief.md](docs/product-brief.md), [docs/architecture.md](docs/architecture.md), and [docs/roadmap.md](docs/roadmap.md).
