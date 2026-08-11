# Validation and Release Gates

## Current checkpoint — August 11, 2026

- Passed: focused DSP and contract tests.
- Passed: local Universal macOS Audio Unit, VST3, and CLAP compilation for both plugins.
- Passed: strict code-signature verification on both installed Audio Units.
- Passed: installation of the **0.2.0** candidate Audio Units.
- Passed: Apple Audio Unit validation, including custom editors, parameters, factory presets, channel layouts, and render tests.
- Verified: Apple reports both component versions as **2.0.0 (`0x00020000`)**, matching the candidate metadata.
- Passed: real Logic Pro editor loading for both Audio Units with no crash or recovery warning.
- Passed: Logic host-control smoke testing for Valley Spirit Sync, `1/8` note division, Freeze, and Spiral feedback path.
- Passed: Logic factory-preset recall for Threefold Palm. `Iron Center` restored Warm, Amount `48%`, Mix `82%`, Output `-0.8 dB`, and Auto Match on; the illustrated editor mirrored those values after reopening.
- Passed: both Audio Units loaded as separate instances in the same Logic channel strip.
- Pending: listening tests with representative program material, Windows CI, packaging, final screenshots, and publication.

The development build has replaced the locally installed tester plugins. The previous installed bundles were retained at `/private/tmp/cinematic-plugins-backup.PpS3sz` for this validation session.

Candidate version: **0.2.0** for both products. The version is reserved for this expansion branch and is not a published release until every gate below passes.

## DSP and state

- Run deterministic unit tests at multiple sample rates.
- Verify finite output, silence behavior, identity/default behavior, mono/stereo handling, and long feedback/freeze runs.
- Verify appended parameter indices and default values preserve existing sessions.
- Check that every new parameter restores correctly and updates the visible editor.

## Builds

- Build the framework-independent DSP tests.
- Build macOS Audio Unit, VST3, and CLAP targets.
- Run Windows CI for x64 VST3 and CLAP.
- Confirm all fonts and bitmaps are present in every format-specific resource bundle.

## Host gates

1. Run Apple Audio Unit validation.
2. Open each Audio Unit editor in Logic Pro.
3. Exercise every control, Form, Path, Sync state, Freeze transition, help panel, animation, automation lane, save/reopen flow, and multiple instances.
4. Listen for clicks, runaway feedback, unexpected loudness jumps, pitch artifacts, and mismatched restored state.

Passing one gate does not imply that another gate passed.

## Packaging and publication

- Close hosts before installation or replacement.
- Rebuild and sign complete macOS bundles after any resource change.
- Package only binaries that exist; fail when an expected artifact is missing.
- Capture screenshots from the actual plugin editor.
- Update README, help text, compatibility claims, changelog/release notes, and tester instructions.
- Push intentionally, wait for checks, publish both platform ZIPs, and verify direct asset URLs.
