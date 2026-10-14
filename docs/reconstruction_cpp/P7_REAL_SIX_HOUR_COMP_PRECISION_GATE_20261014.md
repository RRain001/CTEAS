# P7 — original CALPUFF real six-hour COMP × REAL*4 joint gate

**Date:** 2026-10-14. **PR:** [#41](https://github.com/RRain001/CTEAS/pull/41), Draft.
**Original:** CALPUFF v7.2.1 L150618, vendor SHA256
`da40372cab1c181158d0ee5faac16f678e876fc14415073c38c8e3bdabfbc568`.
**Scope:** Original frozen Hefei 2022 hourly CALMET `MET.DAT`, unchanged `puff.inp`,
and original `puff.con` as a read-only external oracle; real `MDRY=1`,
`MCTADJ=2`, `MXSAM=99`, two transported species, **H-3 concentration DATA
only** (`143` hourly species DATA fields, **not** `286` fields; `286`
are the period/grid metadata records). **MWET=0.**

## One source-ordered native scheduling authority

`OriginalCompRiseMultiPuffHours::advance_hour_point_mxsam99_adaptive_bounded_capacity_sampled`
owns the **single** stateful sequence of six real MET hours, complete
historical puff objects, original POINTS1/PRFIN/RLSMET hourly release,
old-puff-before-new-puff order, variable `KSAM/FSTEP` and residual/reentry
time, `WDIV→VMASS→SETPUF→QOLD→DRY→CALCPF→late-history`, capacity slots,
and original post-step puff retirement. The six-hour test uses the same
production physical modules as the first-hour bitwise acceptance test; no
separate approximate equations were written.

**Case-scoped fix for real hour two:** old puff **22**, `KSAM=1`,
starts at `(X,Y)=(18.251936,49.648922)` cells, inside MET domain.
The C++ `PTLAPS` midpoint look-up incorrectly used the **strict**
`original_met_cell` even when the puff crossed a boundary during its
current substep. The original `GETCELL/GRIDIJ` maps such a midpoint/end
position to its nearest valid MET cell and only afterwards does
`COMP` evaluate final puff retirement. New
`original_met_cell_clamped` implements that already-source-ported
`grid_cell` rule in the **post-ADVECT midpoint** call sites (calm and
PTLAPS); strict pre-step look-up remains unchanged. A regression checks
both modes separately. Follow-up exception diagnostics preserve the old
fail-closed `std::invalid_argument` and `std::out_of_range` behavior.

## Actual six-hour native versus original H-3 hourly concentrations

Original concentration `puff.con` never supplies native velocity, mass,
sampling or puff state. Every value below is from independent native
`COMP` calculation followed by a REAL*4 word-for-word comparison:

| Original 2022 hour | Old puffs | New puffs | Active after | Original and native physical substeps | H-3 bitwise words | Max abs deviation (Bq/m³) | Verdict |
|---|---:|---:|---:|---:|---:|---:|---|
| 1 | 0 | 54 | 33 | 1,275 | **2,500/2,500** | **0** | ACCEPTED selected first-hour H-3 |
| 2 | 33 | 83 | 25 | 2,242 | 2,426/2,500 | 3.099441528e-6 | OPEN |
| 3 | 25 | 71 | 26 | 1,906 | 2,481/2,500 | 3.576278687e-6 | OPEN |
| 4 | 26 | 64 | 26 | 2,132 | 2,189/2,500 | 1.549720764e-6 | OPEN |
| 5 | 26 | 65 | 24 | 1,742 | 2,418/2,500 | 2.384185791e-6 | OPEN |
| 6 | 24 | 80 | 25 | 2,161 | 2,464/2,500 | 2.384185791e-7 | OPEN |

- All six MET hours execute under **one historical driver**; original
  hour-table FSTEP row counts match exactly, **11,458/11,458**.
- Across all 15,000 H-3 receptor words, **14,478/15,000 bitwise**;
  first differing concentration field is hour 2, 0-based receptor **1374**.
- A successful *diagnostic* workflow is **not** 6-hour scientific parity.
  The hourly H-3 equality state is printed individually and labelled OPEN
  until every real-hour native value matches its frozen original counterpart.
- Capacity pressure is still **nonbinding** at `MXPUFF=4096` for this six-hour
  scope. Full original MXPUFF-pressure-triggered `SWAP/ROLLDN` is a separate gate.

## Authenticated same-puff/same-KSAM original DGM comparison

Independent read-only Fortran instrumentation using
`reference/dgm_p7_advection_2022/original_puff_step_dump.py` replays the
full 143-hour vendor source, proves unchanged `puff.con`, hashes the full
`DGM` against the pinned committed original `dgm_puffstep.json`, then
`reference/extract_original_six_hours.py` extracts the six-hour slice.
The six-hour C++ test optionally reads that slice *after its independent
physical step has already completed*: original state never drives physics.
[Verified hosted Ubuntu run #38513544264](https://github.com/RRain001/CTEAS/actions/runs/38513544264).

Original 6-hour DGM slice: **11,458 records ×352 bytes**,
SHA256 `8b848e9b064ee0c687c166d297978196c39cb253ccdc9d01dc2044976fe71b40`.

| Indexed source state across 11,458 original COMP substeps | REAL*4 bitwise equality |
|---|---:|
| Puff episode identity and `KSAM` | **11,458 / 11,458** |
| Post-ADVECT X/Y position | **11,458 / 11,458** |
| Stored horizontal dispersion `SIGY` | **11,458 / 11,458** |
| Stored vertical dispersion `SIGZ` | **11,220 / 11,458** |

First stored `SIGZ` disagreement is source DGM record **3874**,
hour **3**, puff **138**, `KSAM=23`:
original `SIGZ=134.639679` m versus native `134.639694` m
(about one binary32 ULP). Post-step `X/Y` and stored `SIGY` at
that **same** substep are bitwise equal. The original/natively derived
first/last episode IDs per hour and their exact substep counts are now
mandatory fail-closed regression expectations from the pinned original
DGM metadata.

**Interpretation:** The physical/temporal `COMP` step ordering,
stable episode ID mapping, source release counts, horizontal advection,
and stored `SIGY` have **direct original six-hour bitwise parity**.
Do **not** infer that untraced cross-hour internal states, `VMASS`,
`DRY`, or receptor-specific `CALCPF` stages are equivalent solely from
this. Concentration differences already occur in **hour 2**, when all
four DGM fields above remain equal; therefore troubleshoot the
original-vs-native old-puff `CALCPF` receptor intermediates and REAL4
accumulation independently from the first stored `SIGZ` discrepancy in
hour 3. Do not fit a concentration tolerance to conceal either.

## Next true acceptance work, retaining original control flow

1. Trace original and native *same old puff, same hour 2 KSAM, same
   receptor* `QOLD/QNEW,SYREC,SZREC,FRAC,VCOUP,PFSAMP XI1/XI2,T1/T2`
   and REAL4 running `CHISAM`, to distinguish historical starting
   masses/virtual sigmas from source-specific sampling or accumulation.
2. Trace original and native DGM row 3874 vertical stages:
   original stored input `SIGZ` and `WDIV` correction, `VMASS` and
   `SETPUF` begin/mid/end `SIGZ`, rounding order, and accumulated
   history. Repair the first differing source expression, then rerun
   all six-hour data and DGM comparison gates.
3. Require **six hours × 2,500 H-3 REAL4 exact** and original-stage
   vertical history equality before the real 143-hour acceptance run.
   At 143 hours enforce original puff lifetime/conditional
   `SWAP/ROLLDN` capacity and full native `COMP→OUTPUT` data records.
4. Preserve **first-hour 2,500/2,500** and the source-tethered
   `PFSAMP/CALCPF` 196/196 stage parity as nonnegotiable regression gates.
   Wet removal, other source modes, compiler/math-library changes
   and complete file-byte equivalence are **out of scope**.

**Decision:** Real six-hour compute/scheduling diagnostic PASS;
source-validated COMP episode/cadence/XY/SIGY PASS;
strict real six-hour concentration and SIGZ parity **OPEN**.
PR #41 remains Draft and unmerged; P6 and immutable Fortran untouched.
