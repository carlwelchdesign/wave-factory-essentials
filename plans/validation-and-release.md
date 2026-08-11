# Validation and Release Gates

## Current checkpoint — August 11, 2026

- Passed: focused DSP and contract tests.
- Passed: local Universal macOS Audio Unit, VST3, and CLAP compilation for both plugins.
- Passed: strict code-signature verification on both built Audio Units.
- Pending: installation, Apple Audio Unit validation, Logic editor and sound testing, Windows CI, packaging, screenshots, versioning, and publication.

The development build has not replaced the currently installed tester plugins.

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
