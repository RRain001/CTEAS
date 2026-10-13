# P6G G5: scenario generalization audit and first UTM handoff

Date: 2026-10-13. Development branch: `codex/p6g-g5-shared-utm-p4-p5-20261013`, stacked on PR #40's P4/P5 input-contract branch, **not** on `main` or PR #53. GitHub-hosted Ubuntu only.

## 1. Scope completed in this patch (CI must be checked at final commit)

- Introduce one projection/boundary authority: `engine/run_met/station_grid.py`. Parse the **published** `grid.cmn` UTM zone/hemisphere/SW origin, spacing, NX/NY, and datum; reject malformed numerical grids. Inclusive projected outer edges use the same 1e-10 km floating boundary buffer as P5.
- Route native P4 IGRA selection through this authority rather than centre-relative degrees/km. Preserve year-qualified candidates, strict >80% original 00/12 UTC sounding coverage, all accepted in-domain stations, external nearest valid virtual proxy, and source vs effective coordinates. The GUI/legacy orchestration path is **not** implicitly migrated here.
- Route native P5 through the identical shared projected boundary and station CRS convention. Preserve the P5 unit-test projection injection seam, original annual NOAA coverage gates, primary/secondary identities, and fallback policies.
- Audit the supported GUI dispatch: `engine/preprocess/bin/run_preprocessing_pipeline.py` invokes the legacy `engine/preprocess/read62_upperair_data/run_upperair_pipeline.py` and `engine/preprocess/smerge_surf_data/run_isd_surface_pipeline.py`. Both legacy **download selectors** now share the same UTM geometry, without converting legacy processing into the native C++ implementation. Legacy P4's implicit reuse of the first year's IGRA station has been eliminated, while explicit user station choice remains respected.
- Tests: annual candidate membership and station changes, two simultaneous P4 stations, grid shift independent of `center_location`, UTM closed edges/corners, both P4/P5 point classification, full mocked P4 integration (including external virtual fallback), and existing P5 selection rules. A GitHub-hosted Ubuntu workflow runs all these tests and blocks approximate selection functions in both scoped codepaths.

**Do not relabel this patch as P6G physical generalization PASS until CI and the independent original CALMET comparisons pass.** The original vendor source, frozen Hefei reference, `met.inp` physics and P6D/P6E/P6F calculations are unchanged.

## 2. Branch and input ownership

- Original GUI/case-definition path should write `case.yaml` (stack longitude/latitude, model length, model width, horizontal grid spacing, vertical layers, start/end) and publish **one** derived `grid.cmn`, `research_time` and source/stack metadata.
- The published `grid.cmn` controls the numerical footprint. `center_location` may be rounded and is not allowed to redefine the domain for station selection.
- Both native P4 `native_cpp/modules/p4_read62/data_prep/prepare_inputs.py` and the legacy GUI P4 runner now select anew each calendar year unless the user explicitly configures one station; this patch fixes the native approximate spatial test and the legacy automatic reuse policy.
- Native P5 `native_cpp/modules/p5_smerge/data_prep/download_isd_by_grid.py` was previously updated by stacked PR #52. Native AND GUI legacy P4/P5 selectors now use the shared UTM geometry. This proves selector routing only; it does **not** prove that the GUI is operating the native C++ P4/P5 data writers or a generic P6 physical computation.
- P4 `stations.csv` physical/effective coordinate and year identity, P5 per-year station manifests, and `met.inp` SSn/USn must reconcile independently and fail closed on duplicates, missing source records, invalid CRS or geometry mismatches.

## 3. Remaining acceptance gates, in priority order

1. **Input/UI route acceptance:** the GUI currently dispatches legacy P4/P5 processing, not the native writers. Verify the GUI-generated `case.yaml`→`grid.cmn`→`met.inp` handoff on a newly configured scenario, and explicitly choose native vs legacy writer migration without silently switching production physics.
2. **Multistation/multiyear physical input chain:** 2+ surface, 2+ upper soundings, cross-year effective coordinate changes; construct original Fortran CALMET input with identical selection and raw observations. Record station provenance, timezone, availability/coverage and exact yearly membership. Do not mislabel cross-region P5 synthetic 2-station SMERGE evidence as same-domain CALMET physics.
3. **Dynamic P6D implementation:** remove 50×50×9, 100 m, one surface/upper station and 143-interval hardcoded guards only after auditing dependent `read_geo_50`, `read_upper_2022`, station-pair deck parsing, topological masks, layer allocations, station buffers, initial/previous-hour COMP state, and time endpoint semantics. Add configuration contracts and memory bounds before changing scientific kernels.
4. **Independent source parity matrix:** include alternate NX/NY/NZ/DGRIDKM, 1 vs 2+ surface/upper stations, UTM zone/hemisphere and year boundary; original CALMET 1h→6h→full-window independent per-field comparisons. Halt on first mismatching stage and preserve the 50×50×9, 100m, 143-interval Hefei reference in PR #53.
5. **Production boundary:** independently generate P6F static/header provenance (not frozen `met.dat`), and integrate GUI→P4/P5→P6D/P6E→P6F. Rain/precipitation modes are separate unaccepted physics gates.

### Status labels

- `G5_SHARED_UTM_CODE=IMPLEMENTED_PENDING_CI`
- `G5_NATIVE_P4_P5_INTERFACES=TESTING`
- `G5_GUI_SELECTOR_ENTRYPOINTS=IDENTIFIED_AND_PATCHED_PENDING_CI`
- `G5_MULTISTATION_3D_CALMET=NOT_ACCEPTED`
- `G5_ARBITRARY_GRID_TIME_LAYER=NOT_ACCEPTED`
- `G5_143H_HEFEI_REGRESSION=UNCHANGED_IN_THIS_PATCH`
