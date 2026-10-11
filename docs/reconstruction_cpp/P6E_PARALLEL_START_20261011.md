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
