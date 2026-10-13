# P6G G6 — Dynamic GEO and multistation deck foundations; station-envelope domain budget

Date: 2026-10-13. Draft PR #55 stacked on independent-GUI PR #54. It does not merge into `main`, alter PR #53, or change the frozen P6D 143h real-case executable.

## Scientific and computational distinction

Increasing **study-domain extent** at unchanged horizontal resolution increases grid cells and CALMET cost. Increasing **horizontal grid spacing** (e.g. 0.1 km → 0.5 or 1 km) usually reduces 2D/3D cell counts for a fixed domain at the cost of lost terrain/land-use detail. Those are independent parameters and must not be conflated with `NX`/`NY` or the vertical layer count `NZ`.

Illustration only: a 5 × 5 km domain with 0.1 km cells has 50 × 50 = 2,500 horizontal cells. A 50 × 50 km domain at 0.1 km would require 500 × 500 = 250,000 cells. At 1 km spacing the same 50 × 50 km domain returns to 2,500 horizontal cells. Full runtime is not linear in cell count and cannot be inferred solely from these numbers.

Grid coarsening may change slope flow, terrain influence, roughness, mixing and numerical agreement with CALMET. There is **no physical parity acceptance** for those larger/coarser alternatives yet; do not weaken scientific tolerances to make them PASS. Retain a fine inner-domain benchmark if the intended application needs near-source gradients. A station physically outside the numerical footprint is not automatically unusable for CALMET, so extending the grid is a **case construction option**, not a universal requirement for meteorological data inclusion.

## New native P6 input modules

1. `read_geo_dynamic.hpp`: original `GEO.DAT 2.1` gridded READGE path with dynamic NX, NY and DGRIDKM, configured WGS-84 UTM zone/hemisphere, strict CALMET origin match and 4M-cell allocation safety cap. All eight original fields retain row orientation and float32 conversion. Does not support all vendor `IOPT` branches, grid projections or datum conversions.
2. `calmet_station_set.hpp`: native Group 7/8 `SSn/USn` parser for declared multiple surface/upper sites. Preserves original REAL*4 1-km→m multiply/subtract order, ordinal sorting, strict station identity and count rejection. Does not invent P6D multi-station wind blending physics.
3. `p6_domain.py`: explicit station-envelope planner (one or many surface/upper sites), centre fixed, UTM zone fixed, configurable station buffer and horizontal cell cap, optional coarser grid spacing. Does not fetch or fabricate station coordinates; supports manual station lists and is dry-run only unless `--apply` is provided. `--apply` refuses cases with pre-existing `met.dat`.

### Running the planner

A station list is a JSON array with `id`, `kind` (`surface`/`upper`), `lon`, and `lat`. For example:

```json
[
  {"id":"SURF_A","kind":"surface","lon":117.14,"lat":31.94},
  {"id":"SURF_B","kind":"surface","lon":117.32,"lat":31.95},
  {"id":"UP_A","kind":"upper","lon":117.12,"lat":31.92},
  {"id":"UP_B","kind":"upper","lon":117.38,"lat":31.99}
]
```

Dry run:
```bash
python3 -m engine.gui_g5.p6_domain --case NEW_CASE --stations stations.json --margin-km 5 --max-horizontal-cells 10000
```
Review output/preview first, then explicitly `--apply` to a new unrun G5 case. The resulting case-specific `grid.cmn` is authoritative for native P4/P5 strict UTM station selection. Recreate all downstream terrain/land-use/GEO and meteorological files for changed grids. Do **not** reuse old `GEO.DAT`, `SURF.DAT`, `UP.DAT`, `met.inp` or `met.dat` across geometrically distinct cases.

### Debug/Release acceptance evidence

- Synthetic original GEO format: 1×1, 13×7, 50×50 and 120×64 cells; spacing 0.1, 0.5, 1 and 2 km; missing/extra numeric data, grid size, CRS, hemisphere, origin, spacing and allocation limits all fail closed.
- Frozen-compatible 50×50 test: all original GEO arrays (land-use, terrain, roughness, albedo, Bowen, HCG, QF and LAI) equal to `read_geo_50` in the synthetic original file.
- Single-station station-deck parsing has coordinate equality with `original_calmet_station_deck`, with 3 surface + 2 upper test verifying ordinal reordering and validation.
- Domain planning: multi-site UTM containment after case publication, budget-restricted coarsening, long-range/hemisphere rejection, duplicate IDs and explicit apply protection.

**These are input/software scope tests, not independent numerical parity against original CALMET 3D physics for alternative dimensions.**

## P6G remaining hardcoded components (no premature deletion)

- `realcase_wind_field_main.cpp` rejects all but frozen 50×50×9, 100 m, single SS/US and 143 hours. Its source-scoped WIND1/INTER2/DIAGNO/COMP physics must be converted with an independent reference for each branch.
- `read_surface_2022` is single station, UTC+0000, frozen 2022 and 143 records. A true multi-station, cross-year `SURF.DAT` reader with per-hour validity masks is needed.
- `read_upper_2022` and temporal upper-air buffer/COMP coordination are scoped to the frozen case. Multiple `UP.DAT` inputs must preserve station identity, availability and timestamp association; cross-hour transitions are stateful.
- P6E/P6F coupling, projected site latitudes/solar physics, dynamic ZFACE and original Fortran independent 1h→6h→full-period numerical comparisons remain unaccepted.
- The verified Hefei 143h reference and its source-specific P6D→P6E→P6F control order must stay unchanged until a new-case matrix passes.

### Status
`G6_DYNAMIC_GEO_INPUT=CODED_AND_CI_TESTED`; `G6_MULTISTATION_DECK=CODED_AND_CI_TESTED`; `G6_DOMAIN_FIT=CODED_AND_CI_TESTED`; `G6_ALTERNATIVE_CALMET_3D_PHYSICS_PARITY=NOT_ACCEPTED`; `G6_HEFEI_143H_PHYSICS=UNMODIFIED`.
