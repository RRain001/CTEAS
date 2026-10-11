# P6D9A DIAGNO bridge — scoped scientific contract

Date: 2026-10-11. Unchanged reference is CALMET 6.5.0 L150223, original vendor Git blob `437f43bb7f0e51a1919e56664663e1a046986ed0`.

## Implemented

This is the first **single-call C++ bridge** of the selected 50×50×9 diagnostic wind sequence:

1. `WIND1` selected 1-land + 1-upper, neutral `SIMILT`, original `IEXTRP=-4` source-scoped first-guess field (verified separately at [CI #38125704104](https://github.com/RRain001/CTEAS/actions/runs/38125704104), 270,000 original U/V values PASS).
2. Explicit upstream corrected-field injection or **synthetic identity for original-source testing only**. In real simulations `DiagnoD9UpstreamMode::Missing` fails closed; setting the identity mode does **not** mean terrain/heat/slope physics were solved.
3. Original `WINDBC` boundary extraction **from the post-terrain/slope corrected first guess** (the position where original `DIAGNO` refreshes wind boundaries, L5748). It is not extracted from the final INTER2 blend.
4. Independent `INTER2` for U and V at original DIAGNO lines L6055–6061, with caller-supplied measured winds, original radii, valid masks and WO observation weights, preserving component independence.
5. Original `SMOOTH` → `DIVCEL` → `W` at L6119–6148 via preverified `original_smooth_divcel_w`, without imposing unsourced velocity corrections.

## Hard limits and open dependencies

The original full `DIAGNO` also contains `TOPOF2/MINIM`, `FRADJ`, `HEATFX`, `ELUSTR`, `STHEOR` and `SLOPE`, as well as `ADJUST`, lake-breeze branches and conditional O'Brien vertical velocity adjustments. D9A does **not** implement those. `INTER2` observation weights `WO`, observed surface-layer extrapolated U/V, corrected first guess and slope/terrain/heat outputs are all required from independently verified upstream providers. The D9A test intentionally uses a named synthetic identity bypass only; **never promote this mode to a 2022 scientific result**.

`P6D9A_SOFTWARE_GATE` remains **PENDING** until dedicated C++20 CI passes. `P6D9A_FULL_ORIGINAL_FORTRAN_CHAIN` remains **NOT_PASS** until a single composed original Fortran reference explicitly compares step-one/interpolated/final U/V/W for all chosen inputs. `P6D_FULL_2022_UVW=NOT_PASS`; 1h/6h/143h actual Hefei 2022 U/V/W verification pending. P6E thermodynamic inputs, P6F/G and HTO wet precipitation forcing not validated by this bridge.

Independent original source segments previously established: [WIND1 CI #38125704104](https://github.com/RRain001/CTEAS/actions/runs/38125704104), [INTER2 CI #38125137491](https://github.com/RRain001/CTEAS/actions/runs/38125137491), [DIAGNO final W CI #38125137481](https://github.com/RRain001/CTEAS/actions/runs/38125137481). These are **not** yet an end-to-end original-source numerical acceptance for their composition.
