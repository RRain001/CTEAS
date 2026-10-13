# P7 COMP source-order correction and architecture gates — 2026-10-13

Repository: `RRain001/CTEAS`. Development authority: [draft PR #41](https://github.com/RRain001/CTEAS/pull/41). Do not introduce competing P7 branches or merge the draft without complete frozen-case parity.

## Original CALPUFF 7.2.1 authority

Original bundled source: `engine/vendor/src/CALPUFF_v7.2.1_L150618/calpuff.for`. The selected `COMP` path requires this partial order:

1. `SETUP` initializes output header via `WROUT1` (L49981).
2. `COMP` loads hourly meteorology, prepares output accumulators, and calls `INITPUF/POINTS1` (L9463+, L9796).
3. Old and newly admitted puffs traverse source/slot and `KSAM` loops (L9916+, L10417+); each actual substep resolves `GETCELL/PUFFDZ/ADVECT/RISEWIND/FSTEP`, then updates position.
4. `VMASS` operates **before** `SETPUF` (L10859 vs L10940). It consumes **old** `SIGZB` and can alter `ICODE`, `QM/QU`, `ZIOLD/ZIMAX` internally. The next `SETPUF` consumes the updated `ICODE`.
5. `SETPUF` and optional `CHEM/WET/DRY` precede `CALCPF` receptor concentration sampling (L10940+, L11022+, L11053+, L11084+, L11174+).
6. Only **after** concentration sampling does `COMP` finalize `IPUFCD`, `ZIMAX=max(ZIMAX,HLID)`, and `ZIOLD=HLID` (L11391–11398), then evaluate domain exit and FSTEP re-entry (L11400+).
7. Hourly `OUTPUT` data records (L11553, L57584+) are **not** the same call as `WROUT1` header initialization.

The 2026-10-13 native patch separates `commit_vmass`, `commit_sigma`, and `commit_lid` in `OriginalVerticalLifecycle`. Both restricted neutral and rising drivers now use this ordering and the `VendorSubstepOrder` guard. Unit regression forces a real `ICODE 1 -> 5` transition with reduced PBL, rejects stale SETPUF input, and asserts that historical lid state remains pending until the final phase.

## Architecture decision

Preserve separately testable physical kernels (RISEWIND, FSTEP, VMASS, SETPUF, receptors, deposition), but delegate actual source/vendor ordering to **one** native COMP orchestrator. Own the **whole-hour transaction** in that orchestrator, not separately in coupled physics components. Use one canonical puff inventory preserving mutable source slots and stable internal IDs.

Required stage boundaries for full-case mode:

- **Admission and inventory**: old puff slots, original POINTS1 release count, any `SWAP`, inactive `ICODE=99` holes, capacity `MXPUFF`, conditional `ROLLDN`. Existing post-hour eager compaction is a prototype and not original-equivalent capacity timing. A future change must model this pressure explicitly, rather than compacting unconditionally.
- **Per-puff substep**: previous complete state -> wind/cell/rise/FSTEP -> moved position -> pre-SETPUF `VMASS` -> `SETPUF` -> optional species conversion/removal -> `CALCPF` receptor accumulation -> late next-step history -> bounds/retirement -> FSTEP re-entry.
- **Hour boundary**: source-specific concentration sums, hourly `OUTPUT` records, mass budget, restart/checkpoint. A failure anywhere must roll back puffs, receptor accumulators, source scheduler and output writes.
- **Shared state**: physical puff XY/Z, sigma and age, ICODE, mixed/upper species masses, `ZIOLD/ZIMAX`, and source identity survive across meteorological hours. The 143h case must **not** be modeled as 143 independent one-hour runs.

## Explicit remaining gates

**Not validated:** full `CHEM/WET/DRY -> CALCPF -> late lid` integration, original condition-based `ROLLDN/SWAP` timing, complete `WROUT1` and `OUTPUT` binary records, autonomous 1h/6h/143h frozen Hefei 2022 puff trajectories and 2500-receptor concentrations. Existing 143h synthetic continuity is not frozen-case numerical parity.

Suggested order: (1) native VMASS source-order and ICODE tests plus CI; (2) source-slot inventory and capacity-pressure ROLLDN admission; (3) CALCPF per-substep callback integrated with mass/deposition and hour-atomic accumulator; (4) complete output header/data writer; (5) frozen-case first mismatch at 1h, then 6h, then 143h. Preserve the user's postponed precipitation/wet deposition extension gate; do not label `MWET=0` as wet-deposition support.

Acceptance must always record original source line anchors, actual PR commit SHA, full CI result (including failures), and the first independently reproduced numerical mismatch. No PASS without tested equivalence.
