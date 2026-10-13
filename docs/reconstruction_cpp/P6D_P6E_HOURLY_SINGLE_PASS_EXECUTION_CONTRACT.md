# P6D/P6E: original CALMET single-pass hourly scheduler, integration contract

Date: 2026-10-13. Target: original CALMET 6.5.0, frozen Hefei 2022, 50×50×9, 143 hourly intervals, HTFAC=1. Physical status: **NOT PASS**.

## Main finding (do not conflate algorithm parity with callgraph parity)

The old `scripts/run_p6_end_to_end.py` runs the complete 143h P6D producer, writes a partly original-backed CALMET.DAT with P6F, and runs the complete 143h P6E producer; an optional follow-up consumes its earlier P6E state on a subsequent P6D pass. This is a **forensic multistage replay**, not original CALMET's one-pass COMP/DIAGNO model.

The old script now explicitly records `scientific_acceptance=NOT_PASS` and `native_single_pass_comp_diagno=false`. `--require-native-single-pass` rejects that route without running the scientific comparison. Its legacy `status` remains a *diagnostic comparator exit status only*. Do not elevate a green workflow to G2/G3/G4 acceptance.

Reference reads in P6F `--statics-from` and `--met-fields-from` may assist investigations, but must never be used as native physical outputs in an accepted solver.

## Selected-case original source order to port, without replays

The original source callgraph is recorded in `P6D_CALLGRAPH_COMPLETENESS_AUDIT_20261013.md` on PR #51. Source anchors: COMP calls DIAGNO near L4511; PREPDI near L4424; DIAGNO FRADJ L5643; HEATFX/AIRDEN/ELUSTR/SLOPE L5685–5732; first WINDBC(UG,VG) L5748; STHEOR/ELUSTR2/MIXHTST near L5943–5952; INTER2 L6055/6059; **HTFAC=1 second WINDBC(U,V)** L6092–6101; SMOOTH/DIVCEL/W L6119–6148; COMP MIXHT L4692–4699, AVEMIX L4715, WSTARR L4744, TEMP3D L4766.

Native single-pass contract:

1. Before hour `n`, own one committed `REAL*4` state (ZI, ZICONV and /TJMP/ DPTT), initialised according to original SETUP. PREPDI/CGAMMA for hour `n` reads *hour n−1* ZICONV, not current hour ZI.
2. For hour `n`, consume real SURF/UP observations and run native PREPDI then DIAGNO; distinguish WIND1 neutral SIMILT station profile from the STHEOR→ELUSTR2→MIXHTST→SIMILT station calculation. Honor original missing-data sentinels and REAL*4 conversions.
3. Execute FRADJ with the native CGAMMA, HEATFX/AIRDEN/ELUSTR, then SLOPE using these same-hour native QH/TEMP2D. Capture first WINDBC(UG,VG), then INTER2, **then second WINDBC(U,V) for HTFAC=1**, then SMOOTH→DIVCEL→W. Do not keep only the first boundary snapshot.
4. Station STHEOR/ELUSTR2/MIXHTST updates /TJMP/ DPTT *before* COMP grid MIXHT in this same hour. Pass this mutated state to COMP (not the old DPTT from hour n−1).
5. Run COMP's native grid MIXHT/AVEMIX/WSTARR/TEMP3D and commit the full updated grid state only after all hour-n native stages and native output are complete. Failed hour must leave prior state and clock unchanged.
6. Serialize the independently generated complete hour to the C++ CALMET.DAT writer. Do not read original met.dat except in the separate comparator.

## Code delivered in this branch

`include/cteas/p6/hourly_comp_scheduler.hpp` provides a strict C++20 single-pass hour authority with immutable preceding-hour memory, 17 source-ordered hooks, selected HTFAC=1 double-WINDBC ordering, grid state validation, all required native output arrays, provenance rejection for original Fortran oracle or frozen reference backfill, and rollback of state/clock on an aborted hour.

`tests/p6_hourly_comp_scheduler_tests.cpp` uses a **test-only deterministic adapter** to validate 143 consecutive transitions and rejection of state leaks, gaps, wrong inputs, reference injection, missing W and partial-case PASS claims. The GitHub-hosted workflow builds this scheduler on ubuntu-22.04. This contract test is *not* original physics equality; it adds no isolated formulas.

**Unimplemented and blocking:** the existing monolithic `realcase_wind_field_main.cpp` and P6E `calmet_dat21_tool_main.cpp` loops still need to be extracted into *real native per-hour kernel adapters*. Neither is yet wired to the new callback interface. A mock or stub MUST NEVER count as successful scientific execution. No native 45-field output is yet claimed.

## Next code changes — prioritize integration, NOT more stand-alone physics tests

- **G2a**: factor the existing P6D whole-period driver into a reusable native `step_diagno`, holding true COMMON-equivalent memory. Bind native PREPDI/WIND1/FRADJ/SLOPE/INTER2/tail into the first 12 scheduler stages. Retain the selected-case original order without changing validated kernels.
- **G2b**: factor existing P6E `emit_realcase` internal for-loop into native `step_comp` called immediately after that hour’s DIAGNO, with station DPTT mutation and same-hour wind. Replace independent `previous_zi/ziconv/dptt` loops with the scheduler’s single committed state owner.
- **G2c**: run **one** ordered native 143h loop from inputs. Source-instrument original CALMET at PREPDI, WIND1, two WINDBC snapshots, post-INTER2, post-SMOOTH, COMP post-MIXHT and TEMP3D. Compare state at hour 1, hour 6, **first divergence hour 12**, and all 143 hours. Gate real source states, not only stage order.
- **G3**: strict independent C++ 27 U/V/W fields × 143h numeric acceptance; reject any omitted level, change to original tolerance, or Fortran-injected value.
- **G4**: full native 45-field CALMET.DAT output, original met.dat **comparator only**, no `statics-from`/`met-fields-from` as physics. P6F format parity remains a separately proven capability.
- **G5**: only after frozen Hefei succeeds, broaden grids/stations/options. Precipitation and wet deposition stay explicitly deferred.

The existing numeric failures in PR #51 (low-layer INTER2 deviation first at hour 12, WFACE continuity and real SLOPE drivers) remain blockers even if G2 scheduler contract CI passes. Correct them against actual native state propagation rather than hiding them behind oracle feeds.

## Merge policy / branch topology

- #9 is the accepted SLOPE kernel; #10 stays closed, not merged.
- #52 is merged to #40 (input acceptance), but #40 remains Draft until its own downstream CI/science is resolved.
- #46 is merged to #42 (independent 143h WIND1/PREPDI evidence); #49 is merged to #11 (P6E E42 evidence). Neither belongs directly in main yet.
- #44's first WINDBC correction cannot be used alone as source-order completeness: PR #51 also needs the HTFAC=1 **second** WINDBC.
- Preserve separate historical #23/#38/#39 and source-forensic #43/#45/#47/#48/#50 until code ownership and ancestry are reconciled. Do not blindly squash or rebase validated reference traces.
- Use this branch as an integration staging PR **targeting #51**. Keep Draft while real native DIAGNO/COMP adapters, strict 143h parity and independent writer are absent.
