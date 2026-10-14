# P6G G13 — original SURFVAR 2SS, 10 km, 12×12 numerical gate

Date: 2026-10-14. Stacked Draft on #61 (G12); independent source-locking gate, **not** complete P6 acceptance.

## Objective

G12 connects observed surface temperatures and cloud observations to SURFVAR→HEATFX→DIAGNO on a 10 km grid. Its existing 2SS test was structural and scientific consistency only. Prior literal original SURFVAR source parity used a different 4SS 5×4 grid. G13 supplies the missing direct original-source multi-station grid-size/spacing gate without editing G12 production kernels, original GUI or frozen Hefei physics.

## Gate

`reference/compare_g13_original_surfvar_10km.py` extracts **literal unchanged CALMET 6.5.0 SURFVAR** from the pinned vendor source (Git blob SHA `437f43bb7f0e51a1919e56664663e1a046986ed0`, enforced at runtime), compiles the Fortran and compares its TEMP2D, RH2D, IPCODE2D to the actual C++ `p6e_surfvar_multistation_reference` implementation. The original selected control path is all-land NOOBS=0, ITPROG=0, IRHPROG=0, NPSTA=0, IALL=1, IAVET=0, no water or overwater stations; this does not imply global CALMET option coverage.

- 10 km spacing, **12×12** grid, **2 different surface stations**, 72 deterministic scenes.
- Two station inputs at approximately 10 and 100 km, with perturbed locations, temperature, and humidity, observed missing-value cases, distance-weight/radius and IRAD=1/2/NUMTS control variation.
- **10,368 grid cells × 3 fields = 31,104 numerical checks per build.**
- TEMP2D absolute tolerance **0.0003 K** (the existing original Fortran SURFVAR source gate's unchanged threshold); RH2D and IPCODE2D require exact integer match.
- GitHub-hosted **Ubuntu 22.04**, GCC/GFortran Debug and Release, G12 bridge regression required in same workflow. Report includes original vendor SHA256, extracted source line range and first mismatches.

The check is hard-fail: source drift, missing compiler, field ordering differences, numeric exceedances, and incomplete output fail CI. Neither the original source nor the numerical tolerance was changed.

## Acceptance boundary

A PASS establishes only literal original **SURFVAR selected multi-station interpolation** for the stated inputs and, via retained G12 software test, its C++ hourly connection to explicit synthetic solar/heat input. It does **not** prove original 10 km full CALMET equivalence, authentic P6E SUN/CGAMMA/HEATFX thermal forcing, stateful DIAGNO↔COMP, original MET.DAT parity, wet deposition or any G5 production unlock. Frozen Hefei 143h PR #53 is unchanged. **Accepted selected original source gate:** [hosted Actions #38503047383](https://github.com/RRain001/CTEAS/actions/runs/38503047383) has both Debug and Release **PASS**, each 72 scenes / 10,368 cells / 31,104 scalar checks; TEMP2D maximum printed absolute difference 0 K; RH2D/IPCODE2D exact, mismatches 0. G12 thermal bridge CTest also PASS in both. **Keep PR Draft/unmerged**: scoped source gate does not validate full original CALMET 10km multi-station production.

## Test-harness diagnostic (closed)

The first G13 run [#38502825919](https://github.com/RRain001/CTEAS/actions/runs/38502825919) correctly failed (5,673 mismatched grid points) because it reused the older `p6e_surfvar_multistation_reference` driver with its **implicit 100 m spacing**, while original Fortran was set to **10,000 m**. This was an incomparable fixture, not evidence of SURFVAR algorithm failure. Fixed by introducing `src/p6g_g13_surfvar_10km_reference_main.cpp`, which strictly requires `12×12`, two stations and **explicit 10,000 m grid spacing**, supplied as the fifth case-header value. Fortran consumes the unchanged first four header values and independently sets `DGRID=10000.` in the literal-source fixture. No change to the P6E `surfvar_multistation` scientific implementation or old 100 m reference driver. The subsequent original-source comparison is 0-mismatch under the same tolerance.
