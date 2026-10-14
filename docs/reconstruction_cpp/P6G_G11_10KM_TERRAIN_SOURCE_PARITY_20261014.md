# P6G G11 — source-validated terrain stage on 10 km, 120 km multi-station annual case

Date: 2026-10-14. Draft PR #60 stacked on G10 PR #59; previous G5–G9 remain separate. **Do not merge to main** or enable G5 P6/all until actual full original CALMET numerical parity.

## Implemented physics in the separate P6G branch

```text
GEO.DAT original-format 2.1 [10 km, 12×12, source landuse, source terrain]
  └─ original selected dynamic READGE → original TERSET(HMAX,HMIN,DISTHMAX,DISTHMIN)
SURF.DAT [2 SS] + UP.DAT [2 US] + source station deck + UTC model clock
  └─ per-site SIMILT and per-US VERTAV / PREPDI
     → original source-selected 2+2 WIND1 first-guess U/V
     → **original TERSET→FRADJ→SLOPE** (with caller-provided per-hour thermal fields)
     → original L5748 corrected UG/VG boundary snapshot (WINDBC)
     → source-selected U/V INTER2
     → original SMOOTH → DIVCEL → W integration and boundary refresh
```

New standalone module `include/cteas/p6/p6g_terrain_hour.hpp` holds explicit `TerrainStaticG11` and `TerrainHourG11`. TERSET is calculated once from the actual native READGE GEO.DAT domain, **not recomputed within every hourly wind simulation**. The geometry supports 6×6×9 and 12×12×9 with `DGRIDKM=10`, and guarded generic `NX,NY,NZ` within the scoped subroutine constraints. FRADJ and SLOPE run as already source-ported C++ physical equations on the post-WIND1 field. The **corrected** U/V field is passed to INTER2 and used to save the pre-INTER2 boundary snapshot. No independent post-INTER2 boundary substitution occurs.

The per-hour `TerrainHourG11` requires full horizontal grids for lapse rate GAMMA, temperature TEMP2D, sensible heat QH, and the scalar ISURFT, TINF and CRITFN controls. It **does not** estimate any missing field from SURF.DAT or ERA5, silently invent constant values, or claim to reproduce the P6E meteorological system. All G11 tests currently label these fields `SyntheticReferenceFixture`. `Missing` and `ValidatedP6EHourlyPhysics` provenance are fail-closed until a real P6E conversion and acceptance route exists. The tested source path is `ISURFT=-1`. It is an **actual FRADJ/SLOPE calculation with synthetic inputs**, not a simulation with the terrain correction turned off.

The existing G10 `synthetic_identity_diagno_tail` mode remains separate from G11 `source_terrain_diagno_tail`; both simultaneously are rejected. G11 also rejects absent hourly thermodynamics, unmatched GEO shape, nonfinite fluxes, bad layer faces and malformed smoothing controls. Zero true W is accepted.

## Fast multi-site test grid

| Mode | Horizontal spacing | Domain | Station counts | Duration |
| --- | --- | --- | --- | --- |
| Cross-year synthetic meteorology | **10 km** | 60×60 km, 6×6×9 | 2 SS, 2 US | 48 h over 2023/24 |
| **Full leap-year synthetic meteorology** | **10 km** | **120×120 km, 12×12×9** | **2 SS, 2 US** | **8,784 UTC hourly calculations** |
| Direct original-Fortran source terrain test | **10 km** | **120×120 km, 12×12×9** | Prescribed gridded U/V; not a multi-SS input driver | one source-selected terrain operation |
| Frozen real Hefei reference | **100 m** | 5×5 km, 50×50×9 | 1 SS, 1 US | 143 hours, **unchanged** |

The 120 km fixture uses nonflat synthetic topography derived from original-format GEO.DAT, `HTOPO = 100 + 250*i + 150*j` m (zero-based coordinates), TERRAD=35 km and negative sensible heat. These are **synthetic stress inputs**, not measured topography nor a valid resolved 10 km representation of Hefei.

## Direct vendor Fortran source comparison: PASS

`reference/compare_g11_original_terrain_10km.py` extracts the **unmodified original Fortran** `TERSET`, `FRADJ` and `SLOPE` subroutines from `engine/vendor/src/CALMET_v6.5.0_L150223/calmet.for` (verified git blob `437f43bb7f0e51a1919e56664663e1a046986ed0`, SHA256 `430381b4fb87f15127e20b13cc3e99be8a0e0fccf1d5821110af7f980558d134`). A tiny standalone Fortran driver supplies identical controlled 10 km 12×12×9 input fields and executes the original TERSET→FRADJ→SLOPE ordering. C++ independently calls the rewritten source-selected kernels.

The test deliberately uses an upstream wind with **positive U/V** so the Froude branch is entered rather than merely called; the independent C++ run reports **843 altered FRADJ wind cells and 378 nonzero SLOPE contributions**. The original Fortran comparison records **1,686 changed U/V component values**. At all 1,296 3D cells, both wind components (**2,592 U/V comparisons**) match at 9-digit printed numerical precision: `max_abs_delta=0` in **both Debug and Release**. This proves matching numbers to that output precision on the explicitly selected original source case, **not binary bit identity across compiler memory** and **not full original CALMET 3D meteorological-field parity**.

GitHub-hosted Ubuntu (GCC11/GFortran) final source comparison and 8,784h computational integration CI:
- Both Debug and Release direct original Fortran source gates PASS.
- Both Debug and Release 5/5 P6G + scoped frozen CTests PASS.
- Independent tests verify missing/unsupported P6E provenance is rejected and that original SLOPE skips GEO water categories.
- Explicit correction call order and boundary snapshot are audited.

## Outstanding scientific acceptance

The tested complete WIND1→terrain→INTER2→DIAGNO tail chain now **runs**, but still lacks full-calmet numerical parity because:

1. **P6E heat and stability interface:** temperature, lapse and QH were *synthetic*, not calculated for each station and grid cell from actual surface radiative and meteorological inputs. The source-ported SIMILT roughness is also controlled at `Z0=0.1 m` in this fixture, not verified to be chosen from each station's original GEO grid cell.
2. Missing optional upstream branches: pre-DIAGNO kinematic TOPOF2, alternate FRADJ/SLOPE switches, lake/overwater breeze, O'Brien MINIM as applicable and original real-case switches, plus model-full scheduling.
3. **Independent full reference:** original Fortran CALMET 6.5.0 must be run for identical real 10 km P4/P5 multi-station source data, same `met.inp`, dynamic domain, switches and every hour. Compare actual WIND1, FRADJ/SLOPE, INTER2, full U/V/W, USTAR/EL/WSTAR, and P6F MET.DAT at 1h/6h/48h before treating 8,784h as production physics. Do not infer successful stateful full-physics coupling from synthetic consecutive hourly evaluations.
4. Restore high resolution as a separate physical accuracy/sensitivity study after model functional validation; a 10 km spacing cannot resolve 100 m/500 m near-field gradients.

G11 status: `G11_FRADJ_SLOPE_SOURCE_SELECTED_10KM=PASS`; `G11_MULTI_SITE_8784H_SYNTHETIC_CHAIN=PASS`; `P6G_REAL_THERMAL_P6E_INPUT=NOT_ACCEPTED`; `P6G_FULL_CALMET_FORTRAN_PARITY=NOT_ACCEPTED`; `P6F_DYNAMIC_FULL_OUTPUT=NOT_ACCEPTED`; `HEFEI_143H_FROZEN=UNCHANGED`.
