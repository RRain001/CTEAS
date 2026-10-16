# CTEAS P1–P7 independent integration rehearsal — 2026-10-16

## Scope and non-destructive policy

This document records the **integration branch**, not a main merge.
`integration/p1-p7` originated at main SHA
`3b94cdbfbb5f44146ba51a4ef112114f199d28b9`.
All changes and provisional science checks are confined to that branch.
Original PRs #53/#63/#68/#69 remain open and their heads are unchanged.
Do not merge any of those to `main` merely because this preflight passes.

P1–P5 were already in `main`, with scoped scientific reference
acceptance. Their numeric/reference scope is **not** broad legacy-option
equivalence, arbitrary-grid GUI support, or universal geographic coverage.

## Git ancestry / integration history

| Authority | Original head | Rehearsal operation | Result |
|---|---|---|---|
| P1–P5 | `main` | Ancestor of integration | already inherited |
| P6 single, PR #53 | `8d664227` | temporary [PR #70](https://github.com/RRain001/CTEAS/pull/70), merge commit | `eab5dc6` |
| P6 multi, PR #63 | `7a13cc6` | temporary [PR #71](https://github.com/RRain001/CTEAS/pull/71), merge commit | `9677178` |
| P7 core (PR #41/#67 already merged to its foundation) | `6d2ad9f` | guarded manual two-parent Git-tree merge; temporary [PR #72](https://github.com/RRain001/CTEAS/pull/72) automatically recognized ancestry | `334d0a5` |
| P7 C1, PR #68 | `c8dc2f6` | temporary [PR #73](https://github.com/RRain001/CTEAS/pull/73), merge commit | `03a84c5` |
| P7 C2, PR #69 | `6ff9387` | temporary [PR #74](https://github.com/RRain001/CTEAS/pull/74), merge commit | `ff20340` |

**Actual P6/P7 core conflict:** independent [no-commit dry-run](https://github.com/RRain001/CTEAS/actions/runs/38845342842)
found **nine** content-conflicted files, all in
`.github/workflows/p6d*.yml`. Two `p4-read62-*.yml`
workflow files were also touched on both branch lines but Git would
auto-merge them. No C++ physics source file was conflicted.

Resolution: retain P6 multistation accepted versions of all **eleven**
overlapping P4/P6 workflows; adopt the precise original Git blobs for the
**227 nonoverlapping P7 paths**, including 149 P7-module paths.
New two-parent merge `334d0a5` has original P7-core commit and the
integrated P6 branch as real Git parents. No squash, no source-code
conflict marker, no force push. Because two P4 workflow files were
resolved to the newer P6 copies rather than Git's auto-merged candidate,
their review history should be retained in case any later P4 CI trigger
behavior needs to be revisited.

The first automatic resolution attempt was fail-closed by a broad
whitespace audit of preexisting P7 source whitespace. The next
attempt validated all nine conflicts and unchanged P6 code but was
**blocked by the Actions GitHub App's workflow-writing permissions**.
Neither attempt pushed an integration merge. The final Git-object
two-parent merge is the only successful P7-core integration operation.
One-time diagnostic and merge-attempt workflows were then **removed
from the integration branch**; no unattended future write workflow remains.

## Source-tree SHA integrity audit (read from actual GitHub trees)

| Scoped path | Authority | Audit result |
|---|---|---|
| `native_cpp/src/p1_terrain_cli.cpp` and `src/geo/` | main | byte-identical Git blobs/trees |
| `native_cpp/src/p2_gen_fraction_cli.cpp` | main | identical |
| `native_cpp/src/p3_makegeo_cli.cpp` | main | identical |
| `native_cpp/modules/p4_read62/` | PR #63 P6 head | identical subtree |
| `native_cpp/modules/p5_smerge/` | PR #63 P6 head | identical subtree |
| `native_cpp/modules/p6_calmet/` | PR #63 P6 head | **identical subtree**, SHA `0325287b3441e4a728c470e95f7bcd97754b34e3` |
| `native_cpp/modules/p7_calpuff/` | PR #69 P7 head | **identical subtree**, SHA `4c563d8d3e58224ff4aaceee64c297386d43628b` |
| `engine/gui_g5/` | PR #63 P6 head | identical subtree |
| `engine/vendor/src/` | main | **identical untouched vendor subtree** |

The GUI's legacy `engine/gui/` subtree differs from `main`, due to
P6 development changes; it is NOT in the frozen numerical P6/P7 core
assertion. The G5 arbitrary-case P1–P7 executor remains explicitly
restricted as previously audited. Scientific source identity does not
establish GUI production completeness.

## Unified codebase science

### C2: 2024 multi-station 2SS/2US, 12×12×9 @ 10 km, 143h — PASS

[Live integration science run #38846032814](https://github.com/RRain001/CTEAS/actions/runs/38846032814):
**SUCCESS**, true original and reconstructed CALMET plus original and
reconstructed CALPUFF, all on the selected four-arm scenario.
Both C++ **P6 and P7 were checked out from one identical integration
SHA** `68d329dae01ca509f75f0b44a1e839145a9faa48`, verified by a dedicated
fail-closed same-head source step. This **does not** use pinned PR63
code for the native P6 build.

- 143h original and reconstructed CALMET 6,435-science-field gate PASS.
- A/B same original MET, H-3 REAL4: **20,592/20,592 bit exact**.
- C/D same reconstructed MET, H-3 REAL4:
  **20,592/20,592 bit exact**.
- AC cross-MET: **20,576/20,592** words matching; MAX_ABS =
  `7.450580596923828e-09 Bq/m³`.
- Scientific final line `P7_C2_143H_FOUR_ARM_SCIENTIFIC_GATE=PASS`,
  verdict `C2_ACCEPTED=true`. No original DGM injected into P7.
- This was 143 continuous model hours, **NOT the 2024 leap-year 8,784h**.

### C1: 2022 single-station original MET, 143h strict — UNDERWAY AT INITIAL REPORT

Dedicated integration C1 [Actions #38846349292](https://github.com/RRain001/CTEAS/actions/runs/38846349292)
reruns original Fortran DGM and real native P7 on the integrated branch
in a full stateful 143h replay with fail-closed H-3 / DGM / binary
`puff.con` checks. Its final verdict MUST be refreshed from actual
live workflow logs, not inferred from historical PR69 success.

## Acceptance boundaries and next action

Status at the initial report: **P1–P7 GIT_INTEGRATED_ON_ISOLATED_BRANCH;
C2_UNIFIED_SCIENCE_PASS; C1_INTEGRATION_FULL143_PENDING. MAIN_UNTOUCHED.**

- Verify C1 integration run and publish its actual result.
- Keep source SHA pinning evidence and review the eleven accepted P6
  workflow resolutions before final `main` merge.
- For final main integration, preserve original PR ancestry
  (merge commits; no squash/rebase history rewrite), retarget stacked PRs
  carefully, and require science CI on the **actual** new main candidate.
- This rehearsal proves scoped numerical compatibility, not arbitrary
  user-defined stack, general site/geography, full-year model runs,
  all-legacy-options parity, or end-to-end GUI/native P1–P7 orchestration.

No original PR has been merged to `main` by this rehearsal.
