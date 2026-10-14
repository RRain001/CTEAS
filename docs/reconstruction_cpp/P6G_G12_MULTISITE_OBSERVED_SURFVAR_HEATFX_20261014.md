# P6G G12 — multi-station original-selected SURFVAR→HEATFX hourly fields to 10km DIAGNO

Date 2026-10-14. PR #61 Draft, stacked on PR #60 and earlier G5–G10 Draft PRs.

## Why a multi-site acceptance chain is required

One station's original CALMET numerical equivalence does not by itself establish multi-station equivalence. The correct strategy is **not** to duplicate every standalone subroutine test: retain source-accepted numerical unit gates and frozen 100m 143h Hefei regression. Re-evaluate each station-count-dependent component and every hourly coupling boundary under multiple surface/upper stations and dynamic grids, then compare the complete ordered reference model.

Current granular status (selected cases only):

| Function | Scoped original standalone parity | Multi-site selected 10km integration | Whole-model original multi-SS/US parity |
|---|---|---|---|
| G6 GEO and P4/P5 station boundary, G7 SURF/UP | Dynamic reader + strict identity/time controls | PASS selected 2SS/2US synthetic records | NOT ACCEPTED |
| G9 WIND1 U/V original weighting | Literal source Fortran 2SS+2US 12x12x9 PASS | PASS in annual 10km chain | NOT ACCEPTED |
| INTER2 observational blend | Selected Fortran scope, frozen regression | PASS C++ chained original-selected kernel | NOT ACCEPTED |
| G10 DIAGNO SMOOTH/DIVCEL/W | Literal source Fortran selected tail 12x12x9 PASS | PASS C++ 48h + 8784h | NOT ACCEPTED |
| G11 TERSET/FRADJ/SLOPE | Literal source Fortran combined 12x12x9 PASS | PASS in annual 10km chain | NOT ACCEPTED |
| G12 P6E SURFVAR observed TEMP/RH | Full selected multistation Fortran scope has existing 4SS 5x4 tests | **PASS 2SS 10km input-dependent C++ hourly interface** | NOT ACCEPTED |
| G12 P6E HEATFX radiation to QH | Original single-subroutine Fortran selected scope existing | **PASS 10km, day/night, GEO properties and station cloud coupling** | NOT ACCEPTED |
| Full CGAMMA/lapse, radiative solar provenance, prognostic thermal state | Independent narrow CGAMMA and solar modules only | **EXTERNAL, SYNTHETIC** — not claimed | NOT ACCEPTED |
| P6E EL/USTAR/WSTAR, 3D TEMP and mixing state | Frozen 143h selected acceptance exists | NOT ACCEPTED | NOT ACCEPTED |
| P6F output MET.DAT/real geophysical station data | Frozen output only | NOT ACCEPTED | NOT ACCEPTED |

## New G12 code / data contract

`include/cteas/p6/p6g_observed_thermal_hour.hpp` introduces `source_selected_g12_thermal_hour`. It consumes one **decoded** hourly `SURF.DAT v2.1` record *plus the complete SURF station IDs*, the original met.inp SSn deck, original dynamically decoded `GEO.DAT v2.1`, and an **explicit** independent solar/heat/CGAMMA-control bundle.

- Enforces met.inp SSn ordinal order matches SURF.DAT station list exactly; station count 2..6 in this accepted selected SURFVAR kernel, UTC-hour bounds, observable temperature at one or more real stations and valid cloud observations at each site. No data interpolation outside supported original selected mode.
- `cteas::p6e::surfvar_multistation` computes full 2D `TEMP2D` from the decoded site-level SURF station **observations** and applies original multi-site distance weights/ordering. Temperature is no longer a hardcoded `280K` horizontal plane in the yearly G11 fixture.
- Original-selected `cteas::p6e::heatfx` consumes that computed TEMP2D, site cloud via explicit nearest-station map (`MCLOUD=1`), explicit solar sine, explicit original radiative coefficients, and actual GEO albedo, Bowen ratio, soil heat fraction and anthropogenic heat. This calculates the actual source-selected QH formula for both night and daylight, leaving water initial QH unchanged. A correctly sized but unconsumed 2D cloud array is allocated as required by the source-selected HEATFX input contract; it **does not replace** observed station cloud.
- 2D lapse GAMMA is still an explicit independent input. Supplying no radiation/nearest station/coefficients/temperature/GAMMA, missing station clouds, mismatched SURF SSn order or shifted UTC hour fails. The metadata value `ObservedSurfvarSyntheticSolarFixture` unambiguously blocks claims of independently accepted P6E full physics.
- G11 consumes G12's `TerrainHourG11` directly (temperature, QH, lapse); WIND1→FRADJ→SLOPE→INTER2→SMOOTH/DIVCEL/W remains in exact selected stage order.

## Cases and testing

1. Isolated **12×12 10km, 2-site** multi-SURF TEMP2D/HEATFX test includes station-temperature perturbation, spatial temperature variation, day/night QH, preserved water QH, and explicit fail-closed corrupt/missing/shifted source controls.
2. Original-format 2SS+2US 48h cross-year GEO/SURF/UP on **60×60km 6×6×9**.
3. 2024 leap-year **8,784 hours** at **10km 120×120km 12×12×9**, separated 2SS + 2US station sites. Test surface temperature now changes with hour and location; resulting computed QH changes with solar hour and GEO data. Individual hours are *consecutive selected source computations*, not an accepted full CALMET stateful forecast.
4. Frozen G9 source-selected WIND1, G11 terrain and original DIAGNO selected regressions remain in strict GitHub-hosted Ubuntu22.04 Debug and Release build.

**Limitations:** Original 10km full Fortran CALMET is **not** run here. The G12 solar sine, radiative coefficients, nearest-station cell IDs and spatial GAMMA are test-controlled inputs; SUN/MCLOUD/CGAMMA original multi-station source production chain is not yet validated. Thus **source-selected computed QH is not an observed or production-validated heat flux**. QH at water cells is an explicit input, not solved. This does not justify unlocking G5 production P6, assigning real P6E heat/stability parity, or merging to main. The prior 2022 Hefei 50×50×9 100m/143h physical reference and original GUI remain unchanged.

Next numerical gate: source-lift original multi-SS SURFVAR 2SS **10km 12×12** (instead of only the earlier 4SS 5×4 scope), then connect genuine source solar/CGAMMA controls into the P6E stateful loop; only then do selected full Fortran run 1h→6h→48h→8784h parity.
