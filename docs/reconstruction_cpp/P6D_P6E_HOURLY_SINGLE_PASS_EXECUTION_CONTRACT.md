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


## 2026-10-13 implementation follow-up (PR #53, latest)

- Corrected the scheduler's native W vertical array contract to **(NZ+1) × NX × NY** = 10 × 50 × 50 floats per hour; CALMET.DAT writes only the top 9 faces (WFACE1…9). No reduction of native W state is permitted.
- Added `p6e/native_hourly_comp_steps.hpp`: **existing** physically implemented C++ MIXHT, AVEMIX, WSTARR, and TEMP3D are now callable in this original-order hourly frame, with prior-hour ZICONV and current-hour station-cell DPTT consumed in the correct positions. `CompGridMixht` restores the original MIXHT initial-ZI and previous ZICONV/DPTT contract; updates all current-hour MIXHT states and then AVEMIX before WSTARR/TEMP3D.
- A six-hour native COMP integration test passed on GitHub-hosted Ubuntu, with upstream DIAGNO explicitly **a test fixture**. No fabricated Fortran reference comparisons or new physics formulas were introduced.
- The actual existing `p6e_calmet_dat21 emit-realcase` production executable now has an **opt-in** `--native-hour-comp 1` mode which executes these same four native COMP functions with a per-hour state frame and commits thermal history only after TEMP3D. This mode requires `--station-flux-csv` from native P6D; refusing the old approximate refresh if absent. The original E42 path remains unchanged by default.
- Native P6E source-build and frozen original-wind 143h 18-field baseline strict gate passed once after production-source modification: GitHub Actions run `38340366186`. A further hosted gate compares default and opt-in real-COMP paths over all 143 hours, **both fed by the same native P6D station ELUSTR2 inputs and original frozen wind**, and requires candidate CALMET.DAT byte-identical outputs. It is a *migration guard*, not standalone physical acceptance.
- Remaining primary integration task: transform P6D monolithic wind producer into a **true native per-hour DIAGNO adapter** and feed same-hour U/V/W to these COMP steps under `P6SinglePassHourlyScheduler`. Original `met.dat` must then become read-only comparator. Until that is done, `G2/G3/G4=NOT_PASS`.

### One-P6-focus PR dependency ledger (do not claim merged siblings are in this branch)

| PR | Purpose / disposition |
| --- | --- |
| #8 | Older overall P6 draft; not the active integrator. |
| #9 / #10 | #9 canonical merged SLOPE, #10 closed duplicate. |
| #40 / #52 | Original SS1/US1 station geometry and P5 strict UTM: #52 merged **into #40**. #40 does **not** automatically propagate into #53. |
| #42 / #46 | Source-first P6D REAL*4 reconstruction: #46 merged **into #42**. #42 has diverged from #53; reconcile test/source changes deliberately, not blind merge. |
| #11 / #49 | P6E source implementation: #49 merged **into #11**. Its historical merge does **not** imply #53 contains the #49 workflow. |
| #43–#48, #50 | Frozen source and native-CGAMMA forensic lineage already underpinning #51 where applicable; preserve trace provenance. |
| #51 | P6D full-wind scientific blocker and immediate parent of #53, remains Draft. |
| **#53** | **Only active P6D/P6E source-ordered integration branch**; real hourly COMP adapter in code; DIAGNO adapter and science gates remain open. |

Do not merge #53 into #51, #51 into #42, or any incomplete P6 branch into main just because the contract CI is green. Future PR consolidation should occur only with the exact ancestry, reference SHA and 1/6/143-hour gates checked; closing historical drafts before their sources are accounted for risks losing verification evidence.
