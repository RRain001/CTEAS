# P5 surface meteorological preparation — frozen CTEAS Python snapshot

Source authority: RRain001/CTEAS main at 66e30ffb1ff4638a97af8a060748ce30b7578350 (2026-10-10). Original files stay untouched.

| Destination | Original under engine/preprocess/smerge_surf_data | Git blob SHA |
|---|---|---|
| download_isd_by_grid.py | download_isd_by_grid.py | aaf522a97639a05139182c5d4a0261443a2eb63d |
| isd_to_smerge_and_fill.py | isd_to_smerge_and_fill.py | 990886d8d0fe549f41708d635b04850619c27de4 |
| interpolate.py | interpolate.py | 4f417df5ea8c90e933165333690e2e992df473f2 |
| clean_merge_filled_by_research_time.py | clean_merge_filled_by_research_time.py | a2b0e235fe2315fa4f481a7ef77093f2ea04de7c |
| convert_cleaned_to_smerge_ready.py | convert_cleaned_to_smerge_ready.py | 0e2eb707e254edae47b60b35fa3469da84900240 |
| generate_smerge_input_sections.py | generate_smerge_input_sections.py | 3251a0c0512f402698eb403aee31eb08fde1bf10 |
| upstream_README.md | README.md | 8db934cd7207b1b8557127b9ecd45f9b75076074 |

The above Git blob hashes are **source snapshot provenance**, not current file hashes. Files deliberately edited in this scoped branch are download_isd_by_grid.py, isd_to_smerge_and_fill.py, interpolate.py, generate_smerge_input_sections.py, and prepare_inputs.py. Original source files under engine/preprocess/smerge_surf_data remain untouched. The copied run_smerge_and_monitor.py is additionally adapted for effective station coordinates, and is NOT invoked by the default preparation runner. Remaining upstream snapshots are byte-identical. That copied module has **exactly one explicit integration patch**:
- upstream: SCRIPT_DIR = engine_path("preprocess", "smerge_surf_data")
- copy: SCRIPT_DIR = Path(__file__).resolve().parent
This ensures subprocess invocation of interpolate.py uses **the copied script**, rather than silently routing to the original engine/preprocess script. No original file modified. All other algorithms remain unchanged.

From repo root:
- Dry run: python3 -m native_cpp.modules.p5_smerge.data_prep.prepare_inputs --expected-year 2022
- Download and prepare for the configured 2022 case: append --execute
- Existing NOAA input: --skip-download --execute
- Optional legacy SMERGE control generation: --write-legacy-control --execute (modifies project/current/run/smerge/smerge.inp; deliberately off by default)

Pipeline: NOAA NCEI Global Hourly CSV -> converted GENERIC -> *copied* interpolation -> research-time trim and merge -> id-specific ready GENERIC CSV. Original SMERGE input generation is optional; original run_smerge_and_monitor.py is not invoked and no SURF.DAT is published.

Dependencies: existing engine.core.paths, engine/resources/isd/isd-history.txt and non-Git project/current case directory; pandas and numpy for interpolation; external NOAA network to download. Actual 2022 station files are **not** tracked. Data paths remain project/current/met/surface/{isd_raw,isd2smerge_and_fill,isd2smerge_cleaned,isd2smerge_ready}. Default runner is dry-run to prevent overwriting existing case outputs unintentionally.

Important scientific audit items:
- Original interpolate.py uses wind forward-fill and a 0.5 m/s computation speed for selected calm hours, and other field imputation. These are **derived** and not direct observations. Keep *_wind_audit* evidence and original raw CSV hashes; compare derived vs raw separately.
- Quality checks and precipitation conversion are not yet proven equivalent to Fortran SMERGE. The copied inputs are a preprocessing reproducibility path, not validation of a C++ SURF.DAT writer.
- Real 2022 data and original Fortran SURF.DAT output have not been captured on GitHub, so P5 status remains BLOCKED_FORTRAN_REFERENCE.

## 2026-10-10 NOAA endpoint migration
NOAA official June 23, 2026 notice: NCEI ISD CSV HTTPS endpoint retired July 31, 2026. Current official public NODD CSV location: https://noaa-global-hourly-pds.s3.amazonaws.com/ (same ISD CSV station-year keys). The copied download_isd_by_grid.py is intentionally patched to use this endpoint, and validates STATION/DATE/WND headers before writing any downloaded bytes. Previous 200-OK AWS S3 Explorer HTML is rejected. This changes download transport only, not scientific processing.
Reference: https://www.nesdis.noaa.gov/news/service-location-change-integrated-surface-data-global-hourly

## P5 station selection and prior-hour-only reconstruction (2026-10-10)

User-approved policy implemented ONLY in this P5 copy:

- Define the research rectangle from grid/center_location + grid/grid.cmn. For each target calendar year intersect with grid/research_time. All NOAA metadata stations spanning the relevant study interval **inside** that rectangle must be selected; coordinates/elevation stay at observed values.
- If zero eligible inside-domain stations, require explicit stack coordinates via --stack-lat and --stack-lon, or grid/stack_location in the same latitude/longitude YAML-style format. It is not valid to assume the grid center is the stack.
- Rank external stations by great-circle distance from the stack; download in increasing order; compute the **original unique observation hours for the entire calendar year** from the CSV DATE field (not filled/predicted hours). Require >=80% of 8,760 hours (>=7,008 in common years) or >=80% of 8,784 in leap years (>=7,028). Select exactly ONE nearest passing station. Rejected candidates can be downloaded for audit but are explicitly excluded from the processed manifest.
- For a virtual station, the *effective* lat/lon in surf_station_list_YEAR.txt is the stack lat/lon. Original source lat/lon, source ID, unmodified source elevation, relocation flag, coverage, and numeric IFSTN are persisted in station_selection_YEAR.json. Station elevation method is unchanged.
- Legacy six-digit numeric USAF station IDs are preserved if unique. Alphanumeric USAF and distinct WBAN records sharing one USAF receive deterministic seven-digit integer aliases, with collision detection that fails closed. The original 11-character NOAA identity is always preserved in filenames and provenance; scripts that generate SMERGE control blocks and final station metadata use the same alias.
- Temperature (DegC), humidity (%), and pressure (mb): after hourly indexing, each gap uses the preceding valid hour with **no duration cap**. Before the first valid hour, data remains missing (9999), rather than fabricating a future observation. Wind direction/speed already use fieldwise unbounded forward fill; **calm handling is unchanged**.
- Precipitation (AA1-AA4/one-hour only) and cloud-cover missing-data pathways remain unchanged at the user's request. Neither is silently included in the new previous-hour filler.
- Selection and raw station filenames are enforced; rejected, out-of-case or stale station intermediates are not allowed to feed downstream SMERGE implicitly. Published final SURF.DAT numerical equivalence is NOT demonstrated.

Run (from repository root, use a clean research case):
python3 -m native_cpp.modules.p5_smerge.data_prep.prepare_inputs --expected-year 2022 --stack-lat 31.939774444 --stack-lon 117.141610855
Above is a dry-run. Add --execute for network/file writes.

## NOAA manual source verification

- Official NODD **CSV** bucket: https://noaa-global-hourly-pds.s3.amazonaws.com/
- Direct 2022 example (Luogang/Xinqiao, station 58321099999):
  https://noaa-global-hourly-pds.s3.amazonaws.com/2022/58321099999.csv
- Former retired endpoint: https://www.ncei.noaa.gov/data/global-hourly/access/2022/58321099999.csv
- NOAA migration notice:
  https://www.nesdis.noaa.gov/news/service-location-change-integrated-surface-data-global-hourly
- NOAA ISD CSV bucket registry: https://registry.opendata.aws/noaa-isd/

The copied downloader verifies full semantic header names STATION, DATE, WND, TMP, DEW, SLP and that every STATION cell agrees with the filename. Parsing uses csv.DictReader keyed by **column name**, never ordinal column position. WND is a five-part direction/QC/type/speed/QC compound; TMP and SLP likewise have independent compound decoding. Independent regression tests reorder the source columns, prove 25 C does not become wind, and verify missing headers fail closed. GF1, CIG and AA1-AA4 are optional input variables: absent/invalid observations retain legacy missing treatment. This is a schema guard, not a claim that all NOAA records are scientifically complete.

## Unchanged calm-wind algorithm — decision pending

The separate fill_wind_forward_calm() remains exactly as in upstream.
1. Decode wind direction and speed independently using original ISD WND quality flags.
2. Missing wind direction and speed are each ffill() independently, unlimited duration; leading missing hours remain missing.
3. Define calm where *filled* speed < 0.5 m/s. If a calm run is bracketed by two non-calm hours, replace its direction with the shortest circular arc between the bracket directions, spaced uniformly; a precisely 180-degree gap uses clockwise.
4. If a calm run is at the start/end, keep its forward-held direction and log CALM_UNBRACKETED_KEEP_FILLED_DIRECTION.
5. Keep 0.0 and other subthreshold filled speed in detailed wind-audit statistics, but set **calculation speed = 0.5 m/s** for calm hours in the main GENERIC/SMERGE path.
6. Wind-audit *_wind_audit.csv and *_wind_audit.json retain raw, predecessor timestamp, filled speed, calm flag, and calculation speed.

No decision has been made to change this step; discuss with user before edits. Alternatives: (A) preserve observed 0.0 in primary input and handle calm in downstream CALMET explicitly; (B) continue numeric 0.5 guard but maintain transparent calm provenance; (C) separate source speed/calm flag from *internal solver minimum* without overwriting archived input. Each needs comparative reference testing and explicit CALMET interface semantics.
