# CTEAS P7-C1 / P7-C2 — true P6→P7 weather-variant scientific integration

**Created 2026-10-15. Current verdict: IN PROGRESS; neither C1 nor C2 can be asserted PASS until their own integrated native P7 execution and comparisons complete.**

## Frozen authorities (no hidden branch-tip drift)

| Component | Authority | Scientific scope |
|---|---|---|
| P6-C1 (PR [#53](https://github.com/RRain001/CTEAS/pull/53)) | `8d6642272d0abb77153880591e150f93e14d4479` | Hefei 2022, 1SS/1US, 50×50×9, 100m, 143h |
| P6-C1 accepted artifact | hosted run [#38417406969](https://github.com/RRain001/CTEAS/actions/runs/38417406969), `p6-causal-original-case-143h` | `native_causal_independent_header_calmet.dat`, generated without original `MET.DAT` in model inputs |
| P6-C2 (PR [#63](https://github.com/RRain001/CTEAS/pull/63)) | `7a13cc6a2f2d7b4c99e18b8866043a0319733454` | G35/G36 2024 synthetic genuine-format 2SS/2US, 12×12×9, 10km, 143h |
| P6-C2 accepted CI | hosted [#38731010343](https://github.com/RRain001/CTEAS/actions/runs/38731010343) | original+native P6 meteorology, original CALPUFF read, atomic publication, replay-based restart |
| P7 original-CALMET-science baseline | `6d2ad9fc00297d0ed382c59d7654bbd79c3c50ab` and [#38699935854](https://github.com/RRain001/CTEAS/actions/runs/38699935854) | vendor MET → native P7, 143h / 357,500 REAL4 / complete frozen `puff.con` bitwise equal |

**No 8784-hour run is required.** No P6D/P6E/P7 plume/sigma/deposition formula is changed by this integration project.

## Four arms in EACH frozen case

| Arm | MET.DAT producer | Puff solver | Required evidence |
|---|---|---|---|
| A | unchanged original CALMET | unchanged original CALPUFF | independent reference |
| B | unchanged original CALMET | native P7 | same-MET vendor/native concentration equality |
| C | PR53/PR63 genuine reconstructed CALMET | unchanged original CALPUFF | validated third-party interoperability and 143h concentration |
| D | PR53/PR63 genuine reconstructed CALMET | native P7 | independently generated 143h concentration and full original-format `puff.con` |

Two *different* scientific questions cannot be collapsed into a single file hash:

1. **Same-input solver correctness:** A ↔ B and C ↔ D. Report exact REAL4 word equality first; if a difference exists, document its first original/native calculation stage and do not relax scientific acceptance thresholds.
2. **Different-input propagation:** B ↔ D and A ↔ C. Measure per-hour H-3, source footprint, first changed cell, max/mean/RMSE concentration error and temporal/integrated totals; these may legitimately differ when original/native CALMET numerical fields differ within their own strict source tolerances.

A source-independent P6F OUTHD can change many echoed metadata records, so **whole-file `puff.con` SHA inequality is NOT evidence of concentration error**. Conversely, a green result without authentic 143×H-3 REAL4 fields is not accepted.

## C1 implementation and preliminary gate

The isolated workflow [`p7-c1-pr53-original-vs-native-met.yml`](../../.github/workflows/p7-c1-pr53-original-vs-native-met.yml) downloads only the pinned real PR53 full-143h weather artifact, builds original CALPUFF without modifying vendor sources and runs both A and C. It insists that newly rerun A produces the same original H-3 357,500 float32 words as the pinned historical A artifact. Its test-only native P7 C1 mode runs the real, stateful source-faithful COMP on the **PR53-generated** MET.DAT, with the corresponding vendor-C file used only for an **after-the-fact** scientific comparator, never for state initialization.

The opt-in `CTEAS_P7_INTEGRATION_C1=1` removes **only frozen MET-specific oracle assertions and metadata comment-count assumptions**, not core plume physics or the original-MET strict 143h regression. It requires all 143 hours, the same locked source deck, and a full native P7 `puff.con` artifact. The ordinary P7 143h science gate has no C1 flag and remains strict.

The dedicated [`compare_p7_hourly_concentrations.py`](../../native_cpp/modules/p7_calpuff/reference/compare_p7_hourly_concentrations.py) parses actual Fortran-sequential `H-3` records (not text dumps), rejects malformed markers, missing/nonconsecutive record counts, nonfinite/negative or all-zero fields, and reports H-3 float32 word equality and all concentration deltas.

**Do not mark C1 accepted on a queued/running Actions job.** Inspect actual A/C, B/D, C/D scientific JSON and full generated D output. A failed C1 diagnostic must be debugged from the earliest source stage without injecting original puff states.

## C2 planned sequence (not yet accepted)

G35 original and native weather must be rebuilt or staged **within the same accepted 2024 12×12×9/10km scientific scope**, preserving their exact source manifest, first/last UTC timestamps, all 143×45 records, 2SS/2US upper-air bracket and G36 READY seal.

G36A already shows that **original CALPUFF** can read 143h *native* P6G weather. It does **not** prove original-CALMET weather ↔ native-P7 physics or native P7 can run 12×12 weather. The G36A original `puff.inp` fixture adjusts run clock, meteorological domain, receptor and sampling indices for 2024; preserve the original source/diffusion options and only make physically admissible geometry changes.

The current P7 strict physics test still contains explicit 2022/50×50/100m assumptions. C2 requires an independent source-driven **configurable grid, clock, source-admission, receptor-count and WROUT1** path. Do not bypass bounds checks, inject reference concentration, or call an all-zero plume a pass. First run vendor/P7 on the *same* original multi-station MET.DAT, then the *same* native multi-station MET.DAT, and compare all 143 hourly H-3 grids and puff chronology where applicable.

**Stage verdict at creation:** C1 A/C/C/D runner written and CI launched; C1 cannot pass until actual job logs and artifact comparisons are independently verified. C2 input-adapter scientific execution has not started; no P7 12×12 equivalence is claimed.
