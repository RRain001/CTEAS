# P6G G9 — multi-station WIND1 candidate, 10 km expanded annual CALMET kernel integration

Date: 2026-10-14. Draft PR #58 stacked on G8 PR #57 (G7 #56 → G6 #55 → G5 copied GUI #54). No merge to main.

## Major change after G8

The G8 yearly test previously passed with `INTER2.step1_mps=0`, i.e. a synthetic/empty WIND1 first guess. That does **not** validate the WIND1→INTER2 integration.

G9 adds `p6g_multi_wind1_firstguess.hpp` and feeds its nonzero, hour-specific, full 3-D U/V field into the already source-scoped `original_inter2_selected` implementation. Each hour:

1. Parse original-format synthetic `GEO.DAT 2.1`, `SURF.DAT 2.1`, and individual upper-station `UP.DAT 2.1` source files. Strictly validate UTM grid, station identity/order and each hour's bracketing soundings.
2. Convert UTM station latitude with the existing source-ported COORDLIB inverse. Evaluate reconstructed **SIMILT** for each surface station and **VERTAV/PREPDI** for each upper station.
3. Compute cell-centre inverse-square WIND1 weights for each station on the dynamic grid; apply vertical BIAS and calm/missing station policy. For the one-surface/one-upper case, retain the frozen REAL*4 branch order for numerical regression, rather than adding a second renormalization.
4. Feed computed `WIND1.UG/VG` into `INTER2.step1_mps` for each component; evaluate the already source-scoped objective-analysis module and validate finite, variable outputs.

### Grid and duration matrix

| Mode | 10 km grid | Horizontal footprint | Observation stations | Duration |
|---|---|---|---|---|
| Short year rollover | 6 × 6 × 9 | 60 × 60 km | 2 SS + 2 US | 48 h across 2023/24 |
| Extended annual stress | **12 × 12 × 9** | **120 × 120 km** | 2 SS + 2 US | **8,784 h, full leap-year 2024** |
| Regression only | 50 × 50 × 9 | 5 × 5 km, 100 m | 1 SS + 1 US | synthetic WIND1 selected comparison |

Expanded station envelope deliberately places the remote surface station approx. 100 km east and north of the grid SW origin, retaining a positive 10 km margin, with no need to refine grid spacing. The simulations are **synthetic tests**, not site-specific reconstructed meteorology.

### Numeric investigation

G9 first attempt showed `1.4e-4 m/s` maximum discrepancy against the frozen single-pair WIND1 for selected test controls. This was attributed to the multi-site code adding an extra normalization after the original's near-calm `ICALM=0` reweight step. Corrected the pair algebra in the generic kernel (single SS + single US regardless of NX/NY) to preserve the frozen REAL*4 sequence rather than loosening acceptance thresholds. Additional Debug/Release CI includes frozen comparison with strict tolerance. Full original-vendor multi-station numerical parity remains **NOT PASS**.

### Scientific and computational limits

- The G9 WIND1 multi-SS/US algorithm is a **candidate generalization** of the *selected* source-scoped 1+1 branch. Its multi-site, mixed stability, lake/over-water, alternative IEXTRP and station missingness paths have **not** been directly validated against original Fortran CALMET. Its omission from production remains intentional.
- The bridge is still a reduced test configuration: `IEXTRP=-4` neutral SIMILT for all surfaces, uniform synthetic roughness 0.1m, fixed model ZFACE (9 layers), BIAS=0, sample INTER2 radii/weights, and simple gridded synthetic land use.
- Original CALMET WIND1 may apply additional source-specific weighting / iterative logic outside the tested subset. No building downwash, kinematic/frictional terrain corrections, original DIAGNO loop, P6E USTAR/EL/WSTAR or P6F MET.DAT have been accepted.
- No attempt was made to claim that 8,784 independently evaluated INTER2 hours constitute the original full stateful CALMET time stepping; hourly P6D–P6E–P6F coupling must remain ordered once connected.
- The existing validated Hefei 143-hour baseline and original GUI are untouched. All CI uses GitHub-hosted Ubuntu.

### Next acceptance gates

1. Obtain an **original Fortran CALMET 6.5.0 reference** generated from an identical 10 km multi-SS/US, same grid ZFACE and 1h/6h/48h data. Compare each intermediate spatial field and WIND1 station-specific weights to the new C++ implementation at REAL*4 semantics.
2. Trace and implement the missing original WIND1 multi-station route for land/water classes, calm behavior, BIAS and controls; reject unsupported branches explicitly.
3. Chain complete WIND1 → DIAGNO including original terrain adjustment, slope, smoothing and vertical velocity, then source-select P6E stability/heat + P6F output using one sequential scheduler. Validate 1h,6h,48h and 8,784h C++ vs Fortran using same 10km geometry.
4. Keep original 5km×5km/100m/143h Hefei regression on every production-physics change. Unlock G5 GUI's P6 only after real multi-site physics parity and IO.
