# P5 strict UTM station screening and P6 input acceptance

Scope: frozen Hefei 2022 reference case, P3/P4/P5→P6 Mode A. Developed on top of
[PR #40](https://github.com/RRain001/CTEAS/pull/40), without modifying the
frozen case or the CALMET/SMERGE original Fortran sources.

## Implementation

- **P5**: `download_isd_by_grid.py` reads `grid.cmn` through the same
  `engine.run_met.common.parse_grid_cmn` used by the P6 renderer. It projects
  each NOAA source station with the *configured* UTM zone and hemisphere
  (`lon_lat_to_utm_km`), not an independently chosen zone. Only the
  published `XREFKM/YREFKM` southwestern corner plus `NX/NY × DGRIDKM`
  delimit the grid. UTM outer boundaries and corners are included; latitude/
  longitude boxes and `center_location` do not select stations. Unsupported
  projection, datum, hemisphere, zone and bad spacing fail closed.
- **P4→P6**: the P4 annual effective station latitude/longitude must match the
  `met/upperair/final/stations.csv` effective values, whose UTM projection
  must match `met.inp` `US1` (station ID and three-decimal-kilometre
  publication). Source and relocated effective positions must not be confused.
- **P5→P6**: the P5 annual `station_selection_2022.json` source coordinates
  must have the claimed in-/out-domain classification under the strict UTM
  grid. Its published effective coordinates and station ID must agree with
  `met/surface/final/stations.csv` and `met.inp` `SS1`.
- **P3→P6**: reuse PR #40's `check_case_grid_source_contract.py` to verify
  published `GEO.DAT`, `grid.cmn` and `met.inp` zone, hemisphere,
  datum, SW origin, NX/NY, spacing and vertical ZFACE.

Annual station coverage limits, source download/input identities, nearest
eligible external station fallback, and P4/P5 numerical Fortran kernels are
unchanged. The original three-decimal model-grid origin is preserved rather
than recomputed from the requested geographic centre.

## Frozen original reference policy

`audit_hefei_utm_handoff.py` is read-only. For the study's actual NOAA
history, it compares the old approximate geographic-bbox list and the new
strict UTM list **before downloading data**. It also checks whether the frozen
P5 selection mode can remain the same, and verifies SHA256 of the frozen
surface station table, `SURF.DAT`, `met.inp` and station-selection manifest
against `run_manifest.json`.

If membership or selection mode changes, the gate **fails** and requires a
new versioned P5→CALMET original Fortran reference recapture. It must not
silently substitute the old Fortran `met.dat` as new parity ground truth.

If candidates and frozen inputs do not change, no physical meteorological
data was regenerated, and the existing original reference hashes remain
valid. This does **not** qualify P6D full real-case U/V/W parity.

## Runner acceptance

Workflow: `.github/workflows/p5-utm-p6-input-acceptance.yml` (two parallel
self-hosted Linux jobs).

- P5: Python NOAA policy tests + projected corners/edges + invalid CRS
  rejection + no legacy geographic bbox source.
- P6: restored frozen input/authoritative grid generation, GEO→met.inp,
  P4 effective US1, P5 strict-UTM SS1, full 2022 NOAA history candidate
  comparison and archived scientific input hashes.

This document describes gates, **not a PASS claim**. Record successful
workflow ID and P5 migration JSON evidence before merging either stacked PR.
PR #40's old 143-hour full-field numerical FAIL remains separate and must
not be reclassified by an input-contract check.
