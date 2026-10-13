# P6G G7 — Multi-station SURF/UP input chain and cross-year time brackets

Date: 2026-10-14. PR #56 is stacked on PR #55, itself stacked on independent GUI PR #54. The old GUI, native frozen 2022 single-station input readers and validated Hefei P6D/P6E/P6F 143h physical outputs are **unchanged**.

## Scope and scientific acceptance

This is an **input-layer** change. It does NOT claim complete multi-station WIND1/INTER2/DIAGNO or P6E/COMP physics parity against the original Fortran CALMET. Full P6G remains blocked at the G5 GUI. No source observation is synthesized.

1. `read_surface_dynamic.hpp` independently parses CALMET original `SURF.DAT 2.1` formatted `IFORMS=2` `PMAP=NONE` `UTC+0000`. Hourly data is indexed `[hour][surface station]` in the file's header SSn order. Supports 1–1000 stations, leap years, cross-year UTC midnight `hour=24`, and non-143-hour periods. Original zero-calm winds are valid; `9999` precipitation code remains missing, not an inferred dry hour. Aborts on station duplicates/reorders, invalid dates, gaps, overlaps, surplus/truncated fields, unbounded periods or combined hour×station memory budget.
2. `read_upper_dynamic.hpp` parses a distinct original `UP.DAT 2.1` `READ62 JDAT=2 IFMT=1` file per US station. Supports calendar years 1900–2100, instantaneous `6201` soundings, strict increasing observation clocks, original 29-character RDUPN level groups, WGS-84 times interpreted as explicit UTC, station IDs and per-file PSTOP. Reuses frozen source-tested RDUPN real/height/wind conversions and missing-height rejection.
3. `observation_input_contract.hpp` validates that surface and upper input headers and ordering exactly match `met.inp`'s `SSn`/`USn`. Builds hour-end AA/BB **indices only** for each station using original PREPDI's `upper_bound` rule, with bounded 1–14-hour sounding gaps. Does not interpolate physical winds, update CALMET's previous-hour state or feed production WIND1.
4. Both new readers retain the frozen original `read_surface_2022` and `read_upper_2022` without modification. In frozen-compatible synthetic 2022 inputs, compare every surface observation value, validity mask and interval timestamp for 143h; compare every upper level, derived U/V, source validity bit, height and sounding timestamp to original scoped reader.

## Evidence and negative controls

GitHub-hosted Ubuntu/GCC 11 Debug + Release `-Wall -Wextra -Wpedantic -Werror` test matrix:

- Surface: 1 station × 143 h source-reader equality; 3 stations × 5 hours across 2023-12-31/2024-01-01; 2 stations across 2024-02-29; missing, calm, IPCODE, ID/order, malformed interval, overflow, extra record and combined memory-budget checks.
- Upper: frozen 2022 RDUPN single-station value parity; two different station-ID files crossing 2023/24; 2024 leap day; reject wrong station, reversed sounding order, invalid height, invalid Julian date and sounding allocation exceedance.
- Integrated station/input contract: two surface and two upper stations, five consecutive year-crossing hourly ends; independent 12-hour upper brackets in station ordinal order; fail on missing following sounding, duplicated/out-of-order SSn or USn, >14-hour separation and excessive station-hour slots.

The 1–14-hour bracket policy matches the **existing scoped** PREPDI temporal policy and is a conservative acceptance requirement, not a full proof of every vendor CALMET mode. If a new scenario has gaps, reject it instead of inventing soundings.

## Remaining P6G work (next priority)

1. Data provenance: use G5 native P4/P5 to build a **real** 2+ surface/2+ upper station case across yearly station changes; reconcile year-specific station identity, `SURF.DAT` and each `UP.DAT` against exact original Fortran CALMET inputs.
2. Integrate validated dynamic `READGE`, `RDSN`, `RDUPN`, station deck and hourly upper indices into a generalized P6 driver, without deleting the original `realcase_wind_field_main.cpp` 50×50×9/100 m/143 h guard.
3. Generalize ZFACE, station arrays, source-backed WIND1/INTER2/DIAGNO and P6E hour-to-hour state, followed by independent Fortran CALMET 1h/6h/full-period numerical comparisons.
4. Preserve PR #53 Hefei 143h reference and P6F original-free output regression across each stage; do not lower tolerances to force PASS.
5. Unlock G5 GUI P6 only when alternative dimensions, multiple meteorological stations, time ranges, real input generation and full U/V/W physics are independently accepted.

Status: `G7_MULTI_SURFACE_INPUT=CODED_TESTED`, `G7_MULTI_UPPER_INPUT=CODED_TESTED`, `G7_HOURLY_STATION_TIME_CONTRACT=CODED_TESTED`, `P6G_FULL_MULTISTATION_PHYSICS=NOT_ACCEPTED`.
