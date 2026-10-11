# P6E isolated parallel development — 2026-10-11

Development branch: codex/cteas-p6e-surface-boundary-layer-20261011.
Frozen P6C starting point: 490a09c292477f8736efbd7038171faa48e8cbfe.
Concurrent P6D owner: codex/cteas-p6d-slope-original-parity-20261011.

Implemented first independent E1 module: source-backed SOLAR solar-elevation
kernel from unchanged CALMET 6.5.0 calmet.for lines 30871–30990.
REAL*4 constants, east-positive longitude, 26 signed half-hour values,
including negative nighttime sine, strict independent C++20 target and tests.
No shared CMake or P6D source paths modified. C++ software tests do NOT prove
unmodified original-source parity; independent Fortran comparison is required.

Pending before complete P6E PASS: exact original PGTSTB, SURFVAR, HEATFX,
ELUSTR, MIXHT, TEMP3D and selected cloud/RH/radiative-flux processes;
original source unit comparisons on day/night, calm, stable and convective
scenarios; P6D final 3D wind input coupling; true 1h/6h/143h original
comparison with all relevant P6E intermediate/output fields.

P6C = PASS_SCOPED_HEFEI_2022_OBSERVATIONS
P6D = IN_PROGRESS_NOT_PASS
P6E = E1_SOLAR_IMPLEMENTED_SOFTWARE_PENDING_INDEPENDENT_PARITY
P6E_FULL = NOT_PASS
P6_CALPUFF_READY = FALSE
P6_WET_FORCING_VALIDATED = FALSE

2022 precipitation IPCODE missing 143/143, original NPSTA=0: no scientific
wet forcing qualification, no synthesized precipitation.

## Verified E1 evidence (2026-10-11)

Source-locked GitHub Actions original Fortran comparison **PASS**: https://github.com/RRain001/CTEAS/actions/runs/38115313127

- C++20 warning-as-error compilation PASS; independent software CTest 1/1 PASS.
- Original CALMET 6.5.0 SOLAR Fortran extracted without editing, Git blob 437f43bb7f0e51a1919e56664663e1a046986ed0.
- 2,808 / 2,808 original REAL*4 sine-of-elevation values PASS, six Julian days, three base time zones, six spatial points, 26 consecutive signed half-hour slots.
- Max absolute Fortran/C++ difference = 0.0 after 11-decimal-digit scientific formatting; frozen absolute tolerance = 3e-6; no mismatches. Evidence in workflow log and uploaded JSON artifact.
- A second PR-triggered run also PASS: https://github.com/RRain001/CTEAS/actions/runs/38115324902
- State update: P6E_E1_SOLAR_ORIGINAL_SOURCE_PARITY=PASS; FULL_P6E=NOT_PASS. Never generalize E1's standalone results to downstream COMP/SOLAR coupling or the complete P6E micrometeorology.

## E2 PGTSTB implementation (verification pending)

Native CALMET PGTSTB stability classification implemented in an isolated
header and independent CLI/CMake CTest. Original unchanged source is
calmet.for L13295-13498, including the Fortran column-major 12x7 Jstab
matrix, night cloud/ceiling paths, daytime solar thresholds, wind conversion
to integer knots and original water-cell skip. The implementation accepts
observed station ICC/ICEIL or gridded MCLOUD=2/3/4, and returns -999 on
water where original leaves the field untouched. Fail-closed validation of
bad/missing selected inputs is a deliberate modern safety contract, not
source-equivalent input error handling.

The source-locked Fortran harness covers 28 configurations x 20 cells =
560 output cells, checking land stability classes and water sentinels
exactly. E2 is **NOT PASS** until its GitHub original-source workflow
completes; selected 2022 COMP MCLOUD scheduling and P6D integration remain
separate gates.

## Verified P6E E2 and E3 evidence (2026-10-11)

The same [independent original-source CI run](https://github.com/RRain001/CTEAS/actions/runs/38116019206) confirms C++20 warnings-as-errors, **3/3 CTests PASS**, immutable CALMET 6.5.0 vendor source and three independent numerical kernels.

- **E1 SOLAR**: 2,808 / 2,808 values PASS, maximum exported numerical difference 0.0.
- **E2 PGTSTB**: 560 / 560 results PASS (498 land stability classes + 62 water skip masks), exact integer classes, zero mismatches; modes MCLOUD 1–4 covered synthetically.
- **E3 selected SURFVAR**: original *full* SURFVAR subroutine compiled unchanged with temporary mock COMMON declarations; 12 controlled one-station, all-land, observed-dry inputs x 5x4 grid = 240 grid cells x (temperature, RH, IPCODE) = **720 values PASS**. Temperature maximum absolute difference **3.0518e-5 K**, frozen tolerance **3e-4 K**; RH and IPCODE exact. Source stub AVETMP aborts if unexpectedly invoked. This does not validate smoothing, multisite inverse-distance behavior, surface water, prognostic fields, rain-data processing, or 2022 actual joint COMP inputs.

**Authoritative gate**: P6E_E1_SOLAR_SOURCE_PARITY=PASS;
P6E_E2_PGTSTB_SOURCE_PARITY=PASS;
P6E_E3_SURFVAR_ONE_STATION_SOURCE_PARITY=PASS;
P6E_COMPLETE=FALSE; P6D_INTEGRATED=FALSE; P6_CALPUFF_READY=FALSE.

Next physical components requiring full original parity: SURFVAR multistation and real P6C hourly fields, ELUSTR, HEATFX, MIXHT, TEMP3D and related selected cloud/RH/radiation paths. Eventually original 1h/6h/143h micrometeorological field comparison must be performed after P6D gate. Precipitation codes in real 2022 SURF are missing 143/143, original NPSTA=0. Original dry output IPCODE2D=0 must NOT be interpreted as valid observed zero rain; wet forcing remains unqualified.

## E4 HEATFX physical kernel — submitted, pending source-parity CI

Selected original CALMET v6.5.0 HEATFX (calmet.for L7999–8122)
is implemented in an isolated C++20 module. It computes **short-wave
flux at all cells**, even water, before skipping water sensible heat;
sets land nighttime sensible heat to the original -0.1 W/m² marker
without calculating nighttime energy balance; and calculates daytime
land heat flux with the original Holtslag–van Ulden energy-budget
constants, heat storage, anthropogenic flux, albedo and Bowen ratio.

A frozen original-subroutine harness compiles the verbatim source and
original hflux.met layout. Comparison: 24 synthetic cases × 20 grid
cells × 2 fields = 960 components. Includes land vs water, multiple
MCLOUD modes, fractional/observed cloud, twilight, daytime and
nighttime. Predeclared tolerance: 0.02 W/m² absolute + 3e-5 relative
for finite physical fluxes, exact equality for night/water sentinel.
E4 is NOT PASS until the corresponding GitHub action reports PASS.

Remaining: source-backed ELUSTR (u* and L), MIXHT / WSTARR
(convective and mechanical mixing heights and w*), TEMP3D, real
2022 multistation/surface and original 143h coupled 3D verification.
No P6D branch, vendor source, shared P6 CMake, or default main edited.
