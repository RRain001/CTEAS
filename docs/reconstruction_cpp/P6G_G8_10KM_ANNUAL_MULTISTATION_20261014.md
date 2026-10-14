# P6G G8 — 10 km resolution for multi-station full-year/cross-year integration checks

Date: 2026-10-14. Draft PR #57 stacked on PR #56 → #55 → #54. GitHub-hosted Ubuntu 22.04 only.

## Motivation and exact configuration

For long-duration regression, validate temporal station indexing and selected CALMET input/physics kernels without a massive 100 m computational grid. The requested **grid-cell spacing is 10 km (DGRIDKM=10)**, distinct from the overall study-domain length.

| Parameter | G8 test | Frozen Hefei reference |
| --- | --- | --- |
| Horizontal spacing | **10 km** | 0.1 km |
| Size (NX × NY × NZ) | **6 × 6 × 9** (60 × 60 km) | 50 × 50 × 9 (5 × 5 km) |
| Surface stations (SS) | 2 | 1 |
| Upper stations (US) | 2 | 1 |
| Cross-year interval | 2023-12-31 00 UTC → 2024-01-02 00 UTC; 48 h | not a cross-year benchmark |
| Annual interval | 2024-01-01 00 UTC → 2025-01-01 00 UTC; **8,784 h** | 143 h |

The G8 grid has 36 horizontal cells and 324 3D cells. This configuration specifically tests long-time software behavior, not near-source terrain and surface flux resolution. Coarsening changes the numerical problem; results from a 10 km case **must not** be compared directly with the frozen 100 m case as if the physics and grids were identical.

## What is actually exercised

1. **Original-format input files**: synthetic, non-observational `GEO.DAT 2.1` (UTM zone 50N, WGS84; 6×6 × 10 km), `SURF.DAT 2.1` (two station IDs each hour), and two separate `UP.DAT 2.1` (12-hourly upper soundings). All are generated on GitHub's Ubuntu runner; no downloads and no vendor Fortran execution.
2. Native dynamic G6 **READGE**, G7 dynamic **RDSN/RDUPN**, `SSn/USn` source-deck station mapping and each-hour PREPDI bracket contract. Grid roughness/land use are read from the synthetic GEO.DAT file rather than filling the bridge with arbitrary land use.
3. Existing original source-reconstructed **SIMILT** neutral station profiles for both surface sites, **VERTAV/PREPDI** for both upper sites, **INTER2** on all four stations per hour. Station latitude is recovered from source-stored REAL*4 UTM coordinates using the existing source-ported **COORDLIB** inverse; not hardcoded Hefei latitudes.
4. Independently test that each hour uses the expected bracket identities and time fractions, that both layers and fields are finite and nonzero, and that time-variable inputs yield time-variable outputs.
5. `engine.gui_g5.p6_domain` accepts an **explicit, optional** `--spacing-km 10`. It verifies that every selected station remains inside the updated UTM footprint, respects the cell budget and fails closed if the fixed spacing is too fine or the projected domain is too large. All old defaults and legacy GUI remain intact.

The source-selected INTER2 bridge deliberately uses **test-only** first-guess fields, weighting radii, neutral roughness and simplified land-use distribution. The complete original multi-station CALMET `WIND1` first guess, `DIAGNO`, terrain FRADJ/SLOPE, P6E hourly coupled fields, CALMET full MET.DAT and independent original Fortran multi-case U/V/W parity are **not** covered. These must remain explicitly not accepted; do not turn on the G5 GUI's P6/all stage.

## Run

For a newly created (unrun) G5 case, use a JSON array of real, verified station IDs and WGS84 lon/lat. Preview the fit:

```bash
python3 -m engine.gui_g5.p6_domain \
  --case NEW_P6G_STRESS --stations stations.json \
  --spacing-km 10 --margin-km 5 --max-horizontal-cells 10000
```

Only use `--apply` on a disposable/new scenario after reviewing the recommended extent; regenerate all dependent GEO/met.inp/SURF/UP for the revised geometry. No repository reference input data or frozen test case should be edited.

CI: `P6G G8 10km multi-site whole-year selected kernels` on Ubuntu 22.04 (C++20, GCC 11, Debug/Release, `-Werror`). Source-selected 48h cross-year and **8784h full year** plus fixed baseline input contract and original INTER2 software test. Output is a deterministic digest and number of observations contributing; it is **not** an original CALMET field comparison.

## Exit gates for general P6 physics

- Build real 2+ surface / 2+ upper case data from P4/P5 with 10km `GEO.DAT` and corresponding `met.inp`. Original CALMET can then run **the same 10km case** for independent C++ vs Fortran comparison.
- Implement multi-station WIND1, original source-selected distance/bias handling and subsequent INTER2 with actual WIND1 first guess, not the G8 zero source test field.
- Integrate ordered hourly P6D → P6E → P6F state transitions without splitting time into independent runs, and verify first hour, six hours, cross year and full year against Fortran.
- Repeat smaller high-resolution cases to establish real terrain/roughness near-source accuracy after software equivalence is proven.

Status: `G8_10KM_SOURCE_SELECTED_STRESS=TESTED`, `G8_MULTI_SURF_UP_INPUT=TESTED`, `P6G_MULTI_STATION_WIND1_DIAGNO=NOT_ACCEPTED`, `P6G_REAL_10KM_FORTRAN_PARITY=NOT_ACCEPTED`, `HEFEI_143H_BASELINE=UNCHANGED`.
