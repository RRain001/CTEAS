# P7 remaining original CALPUFF functions and strict acceptance gates — 2026-10-12

Scope: private `RRain001/CTEAS`, PR #41, frozen `hefei5km_2022_reference_case_20261011`, 2022 143 hourly intervals, 50×50×9 METFM=1, 100 m, one continuous point source, original `MWET=0`. P6 and original CALMET/CALPUFF source must remain unchanged. Wet removal/deposition is intentionally handled AFTER dry-case P6/P7 completion.

**Fixed acceptance checklist:** P7-I through P7-VI plus P7-FINAL. These are audit categories, not seven newly-created implementation milestones. A closed header gate must not reappear as an open task.

| ID | Functional group | Completed evidence | Remaining acceptance |
|---|---|---|---|
| P7-I | Original-ordered POINTS1 → COMP release/case scheduler | POINTS1 admission verified, native X/Y/distance/age persisted over 143h; 3,036 genuine cross-hour state links | Run all 143 hours from native deck+MET (no DGM driving input), manage injection timing, puff array SWAP/ROLLDN, retirement, repeatability |
| P7-II | Full native per-puff vertical state | `PUFFDZ` formula matches 254,687/254,687 original substeps, including 1,170 history-dependent branches in dedicated original-ZI runner; no mismatches | Independently maintain `ZIOLD/ZIMAX` and SIGZ across steps and hours, including release and state transitions; reference ZI/SIGZ must only be verification output, not a solver input |
| P7-III | Native coupled RISEWIND, plume rise and METFM1 | Full native REAL4 GETCELL + ADVECT from actual original MET: 254,687/254,687 exact, PG classification exact | Native source meteorology → GRISE → RISEWIND/calm each step in original order, no final adjusted wind/vertical height fed by Fortran |
| P7-IV | Full SETPUF, sigma, terrain and dry removal | Source-ported sigma/update, CTADJ2, WALLTRAP, dry deposition modules with independent Fortran kernel/shard tests | Hook active branches and persist all begin/mid/end sigma, mass and altitude states; 1h/6h/143h state parity, no per-hour oracle reseeding |
| P7-V | Hourly 2500 receptor concentration integration | **Source-order corrected:** 600 true native COMP substeps, 2500 receptors/hour, `PUFRECS` receptor virtual time/distance + `SIGTY/SIGTZ` + no-building `GRISE` and **receptor-specific `VCOUP`** (original `CALCPF` L4234–4275), rather than a global `VCBAR`. 6h has **2088 nonzero receptor values**; 20,937 increments differ from the old substep VCBAR path, with REAL4 concentration write/read bitwise parity. [Hosted Actions #38388401882](https://github.com/RRain001/CTEAS/actions/runs/38388401882) **PASS**. The 143h 429-record DATA writer separately passed structure/readback. | Frozen Hefei deck explicitly uses **`MCTADJ=2`** (terrain `PUFFCT2` receptor height branch), **`MDRY=1`** (dry removal), and `MTRANS=1`. Existing live receptor branch supports **flat `MCTADJ=0`, no dry removal**, synthetic MET; neither real terrain nor dry deposition is integrated yet. Complete native case-specific first-hour, six-hour, 143-hour original receptor/field parity still **NOT ACCEPTED**. |
| P7-VI | Full WROUT1 / `puff.con` writer | **HEADER ACCEPTED**: native C++ independently reconstructs all **3,226** source/version/control/MET/meta records (2+3218+6) from `puff.inp`, pinned `MET.DAT` and original WROUT1 semantics; every record is byte-exact versus frozen original (GitHub Actions [#38383774781](https://github.com/RRain001/CTEAS/actions/runs/38383774781)). The prior 3,224 missing-header estimate is withdrawn. Existing 143h DATA writer has REAL4 temporal/record regression. | Attach accepted complete header to **real native 143h** concentration output (429 period/TOTAL/H-3 records). Compare all nonzero receptor values and source timing against original; until then full `puff.con` parity remains OPEN. |
| P7-FINAL | Whole-case equivalence and release gate | Untouched original 143h reference with pinned `puff.con` SHA256; 1h/6h/143h partial motion and current wind/PUFFDZ formula gates | Independent native 1h → 6h → 143h end-to-end, per-hour and per-receptor numerical comparison, first divergence, exact float/ULP/error metrics, identical chronology, controlled mass conservation, Debug/Release/cross-platform evidence |

## Next deterministic first-divergence target (2026-10-13)

The previous description of a *single global VCBAR as the correct CALCPF field* has been superseded. Original CALCPF L4234–4275 calls `PUFRECS` for every receptor and then `VCOUP(ICODE,ZRPOLE,ZPR,SZREC,HLID,HLIDMAX)`; the corrected native sampler now uses that per-receptor coupling, not VCBAR. The independent earlier VCBAR kernel remains useful but does not substitute for the gridded receptor path. Source inspection and [Actions #38388401882](https://github.com/RRain001/CTEAS/actions/runs/38388401882) establish a scoped synthetic six-hour PASS, **not** original-case equivalence.

The remaining concentration physics gap is now concrete, not a new WROUT1 task. Original CALPUFF `PUFRECS` source (inspected via GitHub-hosted source-port oracle [#38386533532](https://github.com/RRain001/CTEAS/actions/runs/38386533532)) performs, for no-building noncalm receptors:

1. `SLGFRAC/FCLIP` gives receptor `FRAC`, `XTT1`, `TREC` (native grid-to-metre adapter already source-verified).
2. `GRISE(XTT1)` and gradual/final-rise `RFACSQ` set buoyancy-induced sigma variance `ADDBID/SUBBID`.
3. At exact `FRAC==0/1`, take begin/end sigma with BID removal. Otherwise `VTY/VDY` and `VTZ/VDZ` are interpolated **in REAL*4** and forwarded through `SIGTY/SIGTZ`, then restored with `ADDBID`, `SY0SQ` and source-sigma floors (vendor L5200–5272).
4. Plume/receptor height applies `MTRANS/MTILT/MCTADJ` after sigma evaluation (vendor L5276 onward).

**Integration barrier**: the current live `make_original_flat_calcpf_sampler` uses a single step-midpoint sigma-y and one vertical factor for the entire field, even though `VCBAR` now runs for every physical substep. Connecting the above per-receptor `FRAC`→`SIGTY/SIGTZ`→`GRISE` path is the next actual solver change. Preserve the old restricted sampler as a control. Frozen 2022 DGM cannot be used to drive the native solver; compare independently derived receptor fields with the frozen original **after** producing them.

## Immutable scientific references

- Original `MET.DAT`: 64,762,556 bytes; SHA256 `b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30`.
- Original `puff.con`: 1,896,056 bytes, 3,655 records; SHA256 `0d51bdb68ff1a5798048bc41067107d3ffe10dc6222f25336df856bcbf48ba10`.
- Original 143h DGM: 254,687 × 352-byte records; SHA256 `cbbdaa29c6222e6ca6aefe9f605ad13e7dd6d258f6ad243afcac07cb578108c6`.
- NEW separate original per-substep historical `ZIOLD/ZIMAX`: 254,687 × 8-byte REAL4 pairs; SHA256 `3a7e519c91f26c17a649a7292435d37b09beef55f3d5d9765aace777781a14c4`. The reader requires the vendor-source SHA; unmodified/instrumented `puff.con` and original DGM SHA must stay unchanged. [Real CI #38266326640](https://github.com/RRain001/CTEAS/actions/runs/38266326640) **1/1 success**: original `PUFFDZ` 254,687 exact pairs, 0 different, 0 skipped, with **historical ZI and previous SIGZ oracle-fed**. This does NOT prove native history reconstruction.
- 143h native original-input GETCELL/ADVECT and historical-DGM motion state [CI #38262191553](https://github.com/RRain001/CTEAS/actions/runs/38262191553): 17/17 PASS, 254,687 REAL4 wind/PG cell comparisons equal. New prior-head 17/17 physical gates and 2/2 scoped gates also passed in #38263727326 and #38263727392.

## Execution order and parallelism

One original **calculation-authority** driver with deterministic puff ordering. Use **GitHub-hosted ubuntu-22.04 Actions only** (no removed self-hosted runner labels). Keep independently pinned comparisons, isolate outputs, and never launch duplicate full vendor 143h runs merely to populate CI capacity.

Next, focus on **original-receptor physics and source-ordered 1h→6h→143h native execution**, then feed its independent 2,500-cell hourly concentration fields to the now-accepted WROUT1 header/record writer. Do not create more isolated WROUT1 sub-gates. Native E2E may use the frozen DGM only for post-run comparison, never as an input or hourly reset. P7-FINAL and PR #41 remain DRAFT until real native concentrations and complete 3,655-record `puff.con` numeric parity pass.
