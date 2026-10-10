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

All files above except isd_to_smerge_and_fill.py and download_isd_by_grid.py are byte-identical Git blobs. That copied module has **exactly one explicit integration patch**:
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
