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
