# Starter-kit verification — 2026-09-13 (America/Los_Angeles)

## Verified locally

- Clean review branch: `agent/plugin-starter-kit-review`, based on main `f4631ae`.
  At this checkpoint, implementation was staged and signing awaited GPG unlock.
- 14 Python generator/packaging contracts pass, including a regression for main's
  notice-only licensing layout. Packaging preserves whichever license/notice files exist;
  it does not import or invent licensing terms from another branch.
- 10 CTest checks pass on the main-based review tree after generating two fixtures.
- Native integration baseline `2328f67`: 12 CTest checks pass. Both generated adapters,
  DSP processors, configurations and tests match the main-based review fixtures byte for byte.
  The shared UI header and pinned framework are identical across these baselines.
- StarterGain and StarterTrim each build AU, VST3 and CLAP with `x86_64 arm64` slices.
  All six bundles pass strict ad-hoc signature verification.
- Both AUs pass Apple auval. The temporary installed copies were removed afterward.
- Both VST3s pass pluginval 1.0.4 strictness 5, seed 12345, including editor and state tests.
- Both tester ZIPs pass CRC, SHA-256 and extracted-bundle signature checks.
- Workflow YAML parses locally. Remote CI has not run.
- Original source checkout status matches its pre-task snapshot; unrelated Spirit Mirror
  changes were preserved. No existing installed plugin was replaced.

## Measured setup

Each fixture generated 12 files. StarterGain took 0.752 seconds and StarterTrim 1.082 seconds
in this local run, with zero inference calls. These timings cover generation only, not SDK
setup, compilation, testing, or agent reasoning. No comparative token savings are claimed.

## Corrections and limitations

- The first Xcode build used a dependency path containing spaces and failed. Reusing the
  existing temporary dependency-path workaround resolved it.
- CLAP exposed an ambiguous base constructor name; the template now uses `iplug::Plugin`.
  CI format builds retain the reproducer.
- A build invocation referenced a newly added aggregate target before reconfiguration;
  explicitly rerunning CMake resolved it.
- The Windows VST3 packaging fixture exposed a nested-binary discovery error. The permanent
  package test now exercises the Windows bundle layout.
- The main-based package run exposed absence of `LICENSE`; main has `LICENSE-NOTICE.md`.
  The permanent notice-only regression covers this baseline.
- StarterGain's first auval attempt could not discover the newly installed component.
  An explicit retry after a five-second discovery interval passed. No caches were deleted.
- Serena activated the integration worktree, but its bounded shared-header query timed out
  at 30 seconds. Narrow source reads were used; no successful semantic lookup is claimed.
- The deterministic local-AI test parser (`wfe-sk-tests`) returned correct total/status but
  zero passed count for an eight-test all-pass log. Raw CTest output remained authoritative;
  the content-free ledger outcome was marked partial. No local synthesis was attempted.
- GPG was initially absent from the shell PATH. After using Homebrew's PATH, configured
  signing timed out waiting for pinentry/key unlock. Signing was not disabled.

## Outstanding gates at the September 13 checkpoint

Unlock the configured GPG signing key, commit the staged starter-only change, push the review
branch and open the prepared draft PR. Run macOS/Windows CI on that exact commit. Hands-on
Logic control/help/focus/listening acceptance, CLAP-host acceptance, and Windows host evidence
remain open. Pluginval does not replace those checks. Integrate through a reviewed release
branch only after its required gates; no public release or commercial activation occurred.

Raw local evidence and tester ZIPs are under `build/evidence/` in the review worktree.
The private project checkpoint records absolute paths and Asana task links.

## September 29 delivery preparation

The scoped review branch was refreshed onto current origin/main `bdf62f8`, preserving its
license and screenshot updates. The 14 Python contracts pass again. The implementation
remains inside this repository; generated products record starter version 1.0.0 and own
their source after generation. PR checks and Asana track current delivery status; the
native build and host-validator results above remain explicitly dated September 13.

The first Windows CI run exposed a test assumption that POSIX execute bits exist on
Windows. The permanent archive test now checks preservation of the recorded source mode:
0755 on POSIX, Windows' actual mode there. This preserves the executable-bit contract on
macOS without imposing Unix filesystem semantics on Windows.
