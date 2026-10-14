# P6G G10 — 10 km multisite WIND1→INTER2→DIAGNO selected tail integration

Date: 2026-10-14. Stack PR #54 → #55 → #56 → #57 → #58 → **PR #59** (Draft, unmerged). All CI uses GitHub-hosted Ubuntu 22.04 GCC/GFortran Debug and Release.

## Change and physical scope

Previous G9 established original-Fortran numerical equivalence of the *selected* multi-station WIND1 1/r² interpolation (two land-surface and two upper-air sites; calm, missing, vertical bias); connected its nonzero U/V first guess to the source-scoped INTER2 objective analysis.

G10 exercises the next **selected source subchain**:

```text
original-format GEO.DAT + 2 SURF station streams + 2 UP station streams
    → strict UTC model clock / station IDs / upper sounding brackets
    → SIMILT + VERTAV + PREPDI
    → source-proven selected multistation WIND1
    → source-selected INTER2 (U then V)
    → original post-upstream WINDBC snapshot (audit uses identity here)
    → source-selected SMOOTH (sequential in-place)
    → DIVCEL (all levels, zero initial W)
    → vertical W face integration and WINDBC refresh
```

Importantly, original CALMET performs additional upstream processes **between WIND1 and INTER2**, including terrain/thermal FRADJ and SLOPE. The G10 synthetic test **does not perform those physical stages**. Therefore this is gated by `SelectedBridgeGrid.synthetic_identity_diagno_tail=true`. It means *explicitly* that those upstream corrections are held as the identity solely to test the following arithmetic and indexing, **not** that they are proven negligible. With the flag false (default), the DIAGNO tail remains off. Missing/invalid smoothing settings are rejected; a genuine zero-W solution is allowed.

The boundary snapshot uses the WIND1 step-one U/V, not the post-INTER2 fields; this models the source timing of WINDBC after the omitted upstream correction phase. The frozen 50×50×9 D9A program and production P6D remain unchanged.

## Horizontal resolution and time cases

| Case | Resolution | Grid | Surface/upper stations | Window |
|---|---|---|---|---|
| Cross-year software check | 10 km | 6 × 6 × 9 (60×60 km) | 2/2 | 48 UTC hours across 2023-12-31 → 2024-01-02 |
| Expanded annual software check | **10 km** | **12 × 12 × 9 (120×120 km)** | **2/2** | **8,784 hours, leap-year 2024** |
| Standalone original source DIAGNO tail | **10 km** | **12 × 12 × 9** | Controlled synthetic gridded winds, no meteorological station reader | Four input classes |
| Frozen original Hefei case | 100 m | 50 × 50 × 9 | 1/1 | 143 h, **untouched** |

Original-format G10 stress file generation uses native dynamic READGE, SURF and UP readers, source-ported COORDLIB and compiled selected kernels; all station values are synthetic and are not historical meteorological observations.

## Direct original Fortran DIAGNO source comparison

`reference/compare_g10_original_diagno_10km.py` imports the previously verified original source extraction (vendor CALMET `calmet.for`, git blob `437f43bb7f0e51a1919e56664663e1a046986ed0`). For the 12×12×9, 10,000 m grid it compiles:

- **Unmodified literal Fortran DIAGNO L6119–6148**.
- Unmodified original `SMOOTH`, `DIVCEL`, `WINDBC` subroutines.
- A 10 km test-specific Fortran driver with identical gridded U/V, 9-layer DZ, smoothing controls and boundary snapshots to the standalone C++ `p6g_g10_10km_diagno_reference_main.cpp`.

Four tests: calm zero, nonzero-divergent flow, layer-selective smoothing, rough winds with distinct external boundary fields. Each run compares 1,296 grid cells × 5 fields (U, V, DIV, W below/above) and 432 final boundary values, total **25,920 field comparisons + 1,728 boundaries** over four cases **per build mode**. The original source C++ comparison uses strict numerical tolerances: wind/W/boundaries absolute ≤2e−4 m/s, divergence absolute ≤1e−6 s−1, as configured by the frozen standalone DIAGNO source comparison. The comparison reports *actual* maxima in CI; passing does not justify treating arbitrary tolerance as zero.

## Acceptance status and next work

| Gate | Status |
|---|---|
| P6G dynamic GEO, multi-SURF, multi-UP and strict clock | Tested |
| 10 km, 2SS+2US selected WIND1 vs literal original Fortran | G9 selected route PASS |
| **10 km DIAGNO tail SMOOTH/DIVCEL/W vs literal original Fortran** | **G10 selected route PASS** |
| **120×120 km, 8,784-hour integrated selected kernels** | **G10 synthetic route PASS** |
| Original FRADJ/SLOPE/thermal flow and original WIND1 + INTER2 true case input | **NOT ACCEPTED** |
| Real multi-station original full CALMET 1h/6h/48h/year U/V/W | **NOT ACCEPTED** |
| Real multi-station P6E USTAR/EL/WSTAR, P6F MET.DAT | **NOT ACCEPTED** |
| GUI G5 P6 unlocked | **NO** |

**Next priority:** build identical real 10 km multi-site P4/P5 observations and original CALMET `met.inp`; link genuine terrain/thermal/flux-dependent DIAGNO and P6E hourly state; then compare actual original CALMET U/V/W and outputs at 1h → 6h → 48h → full year, without altering the frozen Hefei 143h reference or weakening tolerances.

**Do not** describe the four source-tail verification cases as a complete side-by-side original CALMET simulation; they are test drivers for exactly one selected segment. The 8,784-hour test calls selected kernels on consecutive source-record hours, not the full original stateful CALMET P6D↔P6E time integration.
