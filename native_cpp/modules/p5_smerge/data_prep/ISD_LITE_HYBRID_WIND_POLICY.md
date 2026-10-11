# P5 hybrid NOAA inputs — frozen scientific policy (2026-10-11)

Original engine/preprocess scripts and vendored SMERGE/CALMET Fortran remain untouched.

## Official input formats

- All nonwind variables (temperature, dewpoint, sea-level pressure, derived RH, precipitation, cloud, ceiling) stay from the full NOAA Global Hourly / ISD CSV.
- Wind direction and speed originate **exclusively** from the same-station/year ISD-Lite gzip file, downloaded from:
  https://www.ncei.noaa.gov/pub/data/noaa/isd-lite/YEAR/USAF-WBAN-YEAR.gz
- Official definitions:
  https://www.ncei.noaa.gov/pub/data/noaa/isd-lite/isd-lite-format.txt
  https://www.ncei.noaa.gov/pub/data/noaa/isd-lite/isd-lite-technical-document.txt
- ISD-Lite provides 12 whitespace-separated integer fields; field 8 is wind direction in whole degrees, field 9 is wind speed in tenths of m/s. Missing -9999. Wind from north = 360 degrees when moving; calm = wind speed 0, wind direction 0 degrees.
- ISD-Lite does not preserve wind-type C/V/N flags or per-record quality-code provenance. P5 makes no attempt to reconstruct them, and does not use the full-ISD WND field for wind.
- Both NOAA files are required for each selected station/year. External virtual-station selection demands >=80% original-hour coverage of each file, plus existing spatial/stack rules. No automatic full-ISD wind fallback.

## Hourly join

Join by UTC whole-hour timestamp, not row number. Use the union of full-ISD hourly report timestamps and ISD-Lite hourly wind timestamps. Missing field remains missing prior to forward holding, never silently borrow full ISD WND. Station/year, source URL, SHA256, raw wind codes and invalid conditions are recorded in a separate source-audit CSV and JSON. The SMERGE GENERIC file has its original schema; source metadata is kept outside that file.

## Missing wind reconstruction (no maximum duration)

1. Both WD and WS present, WS>0: preserve each.
2. Speed only (WD missing), WS>0: use the most recent **noncalm observed** WD, retain current observed WS. Never use a 0-degree encoding from previous calm as an observed north wind.
3. Direction only (WS missing): use this WD with preceding valid WS; if that preceding WS was zero, write calm WD=0, WS=0 in GENERIC and record that the current observed direction was overridden by held calm speed.
4. Both missing: use prior valid WD and WS independently, with their own source timestamps and ages. No arbitrary maximum carry duration.
5. Exact zero-speed calm: encode WD=0, WS=0 only in GENERIC, preserve raw source direction in provenance, do NOT seed future noncalm direction from calm.
6. A nonzero wind speed paired with WD=0 is inconsistent in ISD-Lite (true north is 360). Treat WD as missing; apply direction hold if possible.
7. Leading missing wind fields without a previous observation remain 9999. A positive-speed hour after earlier calm with no historical noncalm WD also retains missing WD; do not fabricate north wind. A partial wind pair is invalidated by original SMERGE GENERIC reader. Strict mode can abort.
8. Direction or speed originally invalid remains independently marked in audit even when subsequent held data allows a numeric wind pair for CALMET.

Per-hour wind audit records original/held/generated values, observed vs derived flags, source UTC timestamps and age hours; summary captures original speed-only, direction-only, both-missing counts, incomplete numeric pairs and maximum hold age. Observation completeness is a separate metric from the original-hour (>=80%) station selection criterion.

## SMERGE/CALMET separation

The original SMERGE v5.7.0 GENERIC JDAT=8 path reads numeric WD and WS, no C/V/N interpretation; missing in either yields missing pair. It preserves exact zero-speed and records exact calm hours. The original CALMET v6.5.0 handles zero winds with different internal guards: 0.0001 and 0.001 m/s in separate extrapolation/safety branches, a 0.5 m/s local minimum for particular stable Ustar/ThetaStar/HQ calculations, and ICALM=0 prevents extrapolated surface calm influence aloft. These are not universal P5 thresholds; Scheme C retains real zero wind. See CALM_POLICY_C_VENDOR_AUDIT.md for Fortran source line ranges.

## Constraints and pending work

- ISD-Lite is a derived subset with fewer QC/provenance details, not a higher-accuracy independently measured wind record. NOAA's official technical note cautions about exact observation time and different selected reports for different meteorological variables.
- A same-hour wind vector and temperature may come from different source reports up to NOAA's selected capture window. Do not interpret that as strict simultaneous observation.
- Annual raw presence >=80% does not prove wind-pair coverage >=80%; separately audit wind-vector validity and carry duration.
- Original ISD-Lite file availability is proven for 583210-99999 / 2022, not every year or nonnumeric station ID; unavailable Lite files fail closed.
- Multi-year leading gaps and cross-year previous-hour carry require an explicit separate implementation; per-year P5 processing does not silently invent values across a year boundary.
- Other variables, their missing-data rules, precipitation, cloud and relocated station elevation remain unchanged at this stage.
- True Fortran SURF.DAT and CALMET meteorological numerical equivalence on the *same hybrid input* has not yet been demonstrated.
