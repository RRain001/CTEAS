# P6D — CALMET INTER2 original-source selected path

Date 2026-10-11. Candidate, **original Fortran parity not yet verified**.

Freeze original CALMET v6.5.0 L150223 unchanged source blob `437f43bb7f0e51a1919e56664663e1a046986ed0`; `INTER2` L8124–8616, original `FMINF` L7256 and `XMIT` L36951. Selected control branch `IWFCOD=1` (`I3DCTW=1`), `IPROG=0`, `NBAR=0`, `NOOBS=0`, no prognostic field injection. `LVARY` is an explicitly configured runtime input and tested in both modes, not guessed from CASE INP.

INTER2 computes grid-center km coordinates, per-station distances, enforces RMIN, stable nearest-station tie selection, per-layer NINTRP filtering, land surface/upper vs water radii, skips invalid station-component observations, optionally falls back to nearest valid station under LVARY, and uses double-precision weighting with original REAL*4 output. No synthetic missing observation may be marked valid; unsupported nonzero NBAR/IPROG/NOOBS is fail-closed.

C++20 header `include/cteas/p6/inter2_selected.hpp`, CLI `src/inter2_selected_main.cpp`, CTest `p6_inter2_selected_tests.cpp`. Independent literal Fortran comparison `reference/compare_original_inter2.py` links original extracted INTER2/FMINF/XMIT and a **test-only BARIER stub that is unreachable with NBAR=0**. The original vendor Fortran and P1–P5 are untouched. Four synthetic 50×50×9 scenes assess multi-station observations, missing vertical observations and mixed land/water masks, LVARY nearest-source fallback, duplicate distances and zero observational WO weighting. A numeric PASS requires **90,000 scalar component matches**, nonzero actual blends, exact k/row/i ordering and preregistered `2e-4 m/s` absolute tolerance.

Even success cannot mark `P6D_FULL=PASS`: actual 2022 first-hour/six-hour/143-hour U/V/W requires source-faithful WIND1/SIMILT, real step-1 prognostic/station states, FRADJ/SLOPE and complete DIAGNO interoperation. Never confuse original P6B meteorological output with C++ new output. `NPSTA=0`, surface IPCODE missing 143/143, wet forcing remains unqualified.
