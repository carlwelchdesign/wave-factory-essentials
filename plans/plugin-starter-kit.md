# Priority: WFE-SK-001 plugin starter kit

Owner authorization: dedicated Wave Factory Essentials Asana project; v1 creates effects
inside the existing repository. Accountable owner: Carl Welch. Workstream: Codex starter-kit.
Asana: https://app.asana.com/1/9789386902387/project/1218439786655039

Implementation and verification commands: [starter guide](../docs/plugin-starter-kit.md).

## Reuse decision

Reuse pinned iPlug2 `b64192fe18afd9bc9a1fe324db5aceb48f4a0eee`, C++17, CMake, the existing
shared UI style, parameter serialization and platform packaging conventions. Upstream
`iplug_add_plugin` already handles format adapters and targets. A small standard-library
Python generator is justified by project-specific identity validation, manifests, resource
registration and context documentation. No framework upgrade, new UI system, gesture helper,
commerce integration or migration of existing plugins is included.

## Ordered delivery

- WFE-SK-002: establish the committed baseline and preserve unrelated work.
- WFE-SK-003: deterministic generation, collision protection, dry-run and rollback tests.
- WFE-SK-004: smoothed gain reference, bypass, presets, state, host notifications and help.
- WFE-SK-005: manifest-discovered builds, shared asset registration, CI and packaging.
- WFE-SK-006: automated regressions plus actual supported-host acceptance.
- WFE-SK-007: compact guide, repeatable two-product generation and measured evidence.

Public release and commercial activation remain separate. Existing store EPIC 06 and its
signing/host-evidence ticket are linked from the starter parent, not marked complete by it.

## Source baseline and delivery boundary

Initial isolated development baseline: `2328f67`, from `agent/formless-palm-foundation`.
Nine existing CTest checks passed before edits. The original checkout had unrelated Spirit
Mirror changes and was not modified. The starter uses shared code also available on main;
its review branch contains only the starter changes rather than the unpublished product history.
See the PR and Asana evidence for the exact final commit and outstanding host gates.

Current evidence: [verification record](../docs/plugin-starter-verification.md).
