# P7 remaining original CALPUFF functions and strict acceptance gates — 2026-10-12

Scope: private `RRain001/CTEAS`, PR #41, frozen `hefei5km_2022_reference_case_20261011`, 2022 143 hourly intervals, 50×50×9 METFM=1, 100 m, one continuous point source, original `MWET=0`. P6 and original CALMET/CALPUFF source must remain unchanged. Wet removal/deposition is intentionally handled AFTER dry-case P6/P7 completion.

**Counting rule:** six unfinished functional workstreams plus one FINAL full-case scientific acceptance gate = **7 remaining groups**. A ported isolated formula is not a completed integrated solver.

| ID | Functional group | Completed evidence | Remaining acceptance |
|---|---|---|---|
| P7-I | Original-ordered POINTS1 → COMP release/case scheduler | POINTS1 admission verified, native X/Y/distance/age persisted over 143h; 3,036 genuine cross-hour state links | Run all 143 hours from native deck+MET (no DGM driving input), manage injection timing, puff array SWAP/ROLLDN, retirement, repeatability |
| P7-II | Full native per-puff vertical state | `PUFFDZ` formula matches 254,687/254,687 original substeps, including 1,170 history-dependent branches in dedicated original-ZI runner; no mismatches | Independently maintain `ZIOLD/ZIMAX` and SIGZ across steps and hours, including release and state transitions; reference ZI/SIGZ must only be verification output, not a solver input |
| P7-III | Native coupled RISEWIND, plume rise and METFM1 | Full native REAL4 GETCELL + ADVECT from actual original MET: 254,687/254,687 exact, PG classification exact | Native source meteorology → GRISE → RISEWIND/calm each step in original order, no final adjusted wind/vertical height fed by Fortran |
| P7-IV | Full SETPUF, sigma, terrain and dry removal | Source-ported sigma/update, CTADJ2, WALLTRAP, dry deposition modules with independent Fortran kernel/shard tests | Hook active branches and persist all begin/mid/end sigma, mass and altitude states; 1h/6h/143h state parity, no per-hour oracle reseeding |
| P7-V | Hourly 2500 receptor concentration integration | Native receptor sampling and 143h 429-record DATA writer; strict writer bits/temporal continuity verified | Hourly independent native H-3 receptor fields, original time weighting, mass bookkeeping and dispersion/no-wet deposition behavior; `AVET=PGTIME=60` makes PG scaling factor 1, NOT hourly output accumulation 0 |
| P7-VI | Full WROUT1 / `puff.con` writer | Native writer emits REAL4 hourly records and supports 132-byte embedded comments; original 3,655 records **decoded** as 2 version/count + **3,218 132-byte embedded lines** + 6 unclassified metadata + 429 hourly data records (CI #38379037656). Prior claim of 3,224 independently missing header records **withdrawn**. | Reproduce the original **3,218** embedded lines from source semantics (not copied from frozen output); classify/implement remaining **6** records and independently construct the complete original 3,655-record stream. No binary or concentration-value parity yet. |
| P7-FINAL | Whole-case equivalence and release gate | Untouched original 143h reference with pinned `puff.con` SHA256; 1h/6h/143h partial motion and current wind/PUFFDZ formula gates | Independent native 1h → 6h → 143h end-to-end, per-hour and per-receptor numerical comparison, first divergence, exact float/ULP/error metrics, identical chronology, controlled mass conservation, Debug/Release/cross-platform evidence |

## Immutable scientific references

- Original `MET.DAT`: 64,762,556 bytes; SHA256 `b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30`.
- Original `puff.con`: 1,896,056 bytes, 3,655 records; SHA256 `0d51bdb68ff1a5798048bc41067107d3ffe10dc6222f25336df856bcbf48ba10`.
- Original 143h DGM: 254,687 × 352-byte records; SHA256 `cbbdaa29c6222e6ca6aefe9f605ad13e7dd6d258f6ad243afcac07cb578108c6`.
- NEW separate original per-substep historical `ZIOLD/ZIMAX`: 254,687 × 8-byte REAL4 pairs; SHA256 `3a7e519c91f26c17a649a7292435d37b09beef55f3d5d9765aace777781a14c4`. The reader requires the vendor-source SHA; unmodified/instrumented `puff.con` and original DGM SHA must stay unchanged. [Real CI #38266326640](https://github.com/RRain001/CTEAS/actions/runs/38266326640) **1/1 success**: original `PUFFDZ` 254,687 exact pairs, 0 different, 0 skipped, with **historical ZI and previous SIGZ oracle-fed**. This does NOT prove native history reconstruction.
- 143h native original-input GETCELL/ADVECT and historical-DGM motion state [CI #38262191553](https://github.com/RRain001/CTEAS/actions/runs/38262191553): 17/17 PASS, 254,687 REAL4 wind/PG cell comparisons equal. New prior-head 17/17 physical gates and 2/2 scoped gates also passed in #38263727326 and #38263727392.

## Execution order and parallelism

One original **calculation-authority** driver with deterministic puff ordering. Use **GitHub-hosted ubuntu-22.04 Actions only** (no removed self-hosted runner labels). Keep independently pinned comparisons, isolate outputs, and never launch duplicate full vendor 143h runs merely to populate CI capacity.

The next production milestone is **P7-I/P7-II joint native persisted state**, not another oracle-fed acceptance test: start with 1h new puff and old puff, then 6h uninterrupted transitions, then 143h. Native E2E may use the frozen DGM only for post-run side-by-side verification, never as input. P7-FINAL and PR #41 remain DRAFT until native concentrations and full WROUT1 achieve numeric/record parity.
