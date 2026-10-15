# P7-C2 2024 2SS/2US source-first 143h original/native CALPUFF divergence

**Scope and verdict:** Diagnostic evidence ONLY; **C2 = NOT ACCEPTED**. This is an independent same-MET vendor/native physical mismatch and must not be hidden by rounding, tolerance changes, whole-file SHA comparisons, or by relabeling A/C as B/D. Original PR53 C1 **remains accepted**, separate PR [#68](https://github.com/RRain001/CTEAS/pull/68).

**Frozen authorities:** 2024 G35 original + PR63 G14 2SS/2US (12×12×9, 10km, 143h) generated from the SAME original-format observations. PR63 SHA `7a13cc6a2f2d7b4c99e18b8866043a0319733454`. SOURCE XY=(565.885,3586.269) km, controlled inside domain for both real vendor/native calculations, unchanged H-3 emission rate, exit velocity/diameter, stack and all diffusion/chemistry/dry controls. All 6,435 original/native meteorological field groups within frozen source tolerances. First nontrivial A↔C vendor comparison [#38743223360](https://github.com/RRain001/CTEAS/actions/runs/38743223360) remains accepted only for meteorology-to-dispersion sensitivity.

## Real completed four-arm 143h scientific data

[GitHub-hosted #38750014364](https://github.com/RRain001/CTEAS/actions/runs/38750014364) has actual, independently produced 143h original and reconstructed P7 B/D `puff.con` results, and original CALPUFF A/C. Its [evidence artifact](https://github.com/RRain001/CTEAS/actions/runs/38750014364/artifacts/11981440975) includes the three independent REAL4 science JSONs, original/native P7 hourly logs and explicit failed strict verdict.

| Comparison | Matches | Non-identical REAL4 words | MAX absolute difference [Bq/m³] |
|---|---:|---:|---:|
| A original MET + original CALPUFF vs B same original MET + native P7 | 20,545/20,592 | **47** | `2.5265035219490528e-05` |
| C native PR63 MET + original CALPUFF vs D same native MET + native P7 | 20,545/20,592 | **47** | `2.5265035219490528e-05` |
| A original vendor MET vs C native vendor MET, fixed original CALPUFF | 20,576/20,592 | 16 | `7.450580596923828e-09` |
| B original MET vs D PR63 MET, fixed native P7 | 20,577/20,592 | 15 | `7.450580596923828e-09` |

All 47 same-MET mismatch records lie at the **same receptor**, `i=4, j=6` (0-based cell index `63`). First discrepancy is H1. The 47 affected hours are:

`1–7, 24–31, 48–55, 72–79, 96–103, 120–127`

The other **96/143 hourly gridded H-3 fields agree in every REAL4 word** for both original- and native-MET comparisons. The 47 mismatching hourly fields have **143/144** equal words; each has ONE different concentration cell, not a diffuse gridwide discrepancy.

First original-MET H1 vendor concentration at receptor (4,6): `0.0016952104633674026 Bq/m³`. Native P7 H1: `0.0017160858260467649 Bq/m³`. This is a locally measurable ≈1.23% overprediction at that receptor, **not** merely a rounding ULP. The largest abs difference occurs H24 at the same receptor: original `0.0018680267967283726` vs native `0.001893291831947863 Bq/m³` (≈1.35% local).

For the original-MET arm, the full 143h sum of all 12×12 H-3 gridded hourly concentrations is vendor `3.22340187637` and native `3.22430167415` (same field units summed over cells/hours), a +`0.00089979778` net difference (≈0.0279% aggregate). This aggregate statistic is diagnostic, **not** a substitute for required REAL4 exactness.

The difference repeats with the 2024 synthetic daypart pattern and is *nearly the same with both independently generated meteorological sources*. This points toward a receptor-specific original-vs-native CALPUFF physical branch, not a new PR63 CALMET fault. **It does not yet prove which original physics stage is wrong**; in particular distinguish GETELEV/PUFFCT2 terrain sampling and PUFRECS receptor elevation/FRAC, PG SIGTY/SIGTZ, VCOUP, and mass/DRY.

## Source-backed fixes and their strict boundary

Real unmodified CALPUFF Fortran line audit [#38748386141](https://github.com/RRain001/CTEAS/actions/runs/38748386141), [#38749085636](https://github.com/RRain001/CTEAS/actions/runs/38749085636):
- `SETPUF` **L48362–48365** resets `strain=1, fracz1=0, lup1=.FALSE.` at the start of every step; it does **NOT** carry prior puff flags. Earlier candidate carry-over code was removed.
- `SETPUF` **L48613–48632** calls CTADJ2 + SIGTZ only for odd, Gaussian vertical `ICODE`; even codes skip CTADJ2.
- `PUFRECS` **L5321–5323** invokes PUFFCT2 when MCTADJ=2; native even puff uses authentic per-step zero/FALSE and actual native before/end point coordinates instead of a fabricated height or injected DGM state.
- `POINTS1` **L15603** computes `HEFF=HTSTAK-TIPDW+RISE_B`, which can be below actual stack height; duplicate above-stack-only validation was corrected without changing formula.
- The actual P7 C2 native program now executes all 143h in both arms and writes complete WROUT1-form `puff.con` for science comparison, but fails the exact same-MET gate **by design** until the 47 receptor differences are understood.

## Required next step

1. Trace the FIRST *nonzero* contributor to H1 receptor (4,6) in original vendor PUFRECS (after PFSCRN, PUFRECS SIGTY/SIGTZ, PUFFCT2, VCOUP, CALCPF integration/DRY) and the corresponding C++ `OriginalCalcpfReceiverIntermediate`. Instrument only read-only stage evidence, not physics, and use identical source/period/meteorology.
2. Find the **earliest diverging physical variable**, not just final concentration. Ensure original/native puff IDs, KSAM, release/transport trajectory, source mass and sampling clock correspond.
3. Make the smallest source-backed correction that preserves frozen single-station 2022 P7 strict 143h gate. Rerun both 2024 same-MET vendor/native 143h comparisons and reject unless **20,592/20,592** REAL4 bits per arm.
4. Keep PR [#69](https://github.com/RRain001/CTEAS/pull/69) **Draft, no merge**. Never equate complete execution with scientific equivalence.

**Real science status:** C1 accepted; C2 P6 weather accepted; C2 original A/C weather sensitivity measured; C2 native B/D runs fully executed but **47/20,592 source-stage concentration differences remain OPEN**.
