# P6D — original CALMET 6.5.0 SMOOTH candidate audit

Date: 2026-10-11. **Await original-source CI before acceptance.**

Original CALMET source L30606–30681 `SMOOTH`; source git blob `437f43bb7f0e51a1919e56664663e1a046986ed0`. Actual DIAGNO calls `SMOOTH(U,V,UB,VB,NSMTH)` at L6119 on the 2022 selected IWFCOD=1 path.

The original routine is **sequential and in-place**, not double-buffered Jacobi smoothing. Per layer K, it makes exactly `NSMTH(K)` passes scanning J then I, with weights 0.5 center +0.125 each of four neighbors; U uses `UB` for east/west edges and the local U for missing north/south cells, while V uses `VB` north/south and local V for missing east/west. Fortran REAL*4 order and edge rules are retained. The `LLBREZE` (lake-breeze-box) branch is explicitly rejected, not silently skipped. The original Fortran driver sets `LLBREZE=.FALSE.`; test-only external `BOX` stub exists solely for link resolution on this unsupported branch.

C++20 kernel `include/cteas/p6/smooth.hpp`, CLI `src/smooth_main.cpp`, software `tests/p6_smooth_tests.cpp`; independent unchanged Fortran source comparison `reference/compare_original_smooth.py`; workflow `.github/workflows/p6d-smooth-original-parity.yml`. Four independently evaluated 50×50×9 cases cover zero passes, one pass each, heterogeneous 0–3 layer passes, and nontrivial asymmetrical boundary winds. Compare all **180,000** U/V components against original Fortran, predeclared 2e-4 m/s tolerance, precise coordinate/layer order, finite QC, and active nonzero changes.

This isolated kernel **cannot** stand in for WIND1, SIMILT, INTER2/objective analysis or full original 2022 1h/6h/143h U/V/W verification. It does not validate wet forcing: `NPSTA=0`, precipitation codes absent 143/143.
