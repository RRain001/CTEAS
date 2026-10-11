# P5 wind data contract — approved policy v2, 2026-10-11

**ACTIVE:** P5_ISD_LITE_WIND_V2_WHOLE_VECTOR_HOLD_CALM_REPLACE.
Older Scheme C (retain original zero wind) and Hybrid V1 (independently
forward-fill direction and speed) are superseded. Original CTEAS scripts
under engine/preprocess and vendor SMERGE/CALMET Fortran remain unchanged.

## Sources and coverage
- Download full NOAA Global Hourly ISD CSV and official same-station/year
  ISD-Lite gzip. Full ISD supplies all nonwind fields; ISD-Lite exclusively
  supplies wind direction and wind speed, without C/V/N or WND fallback.
- Merge by UTC hour, NOT by source row number. Reindex to every calendar-hour
  from Jan 1 00:00 through Dec 31 23:00, including gaps such as 18 -> 20
  (insert 19 with no original source).
- Annual raw coverage: UNIQUE UTC hours with an ISD-Lite record divided by
  calendar hours (8760; leap year 8784), including records where both wind
  fields are missing. Missing entire hours and present-but-invalid wind
  records must be tracked separately. Domain and external fallback selection
  rules stay unchanged: nearest qualifying external source, >=80% raw-time
  coverage on BOTH official input sources; in-domain station collection does
  not silently discard records based on wind quality.
- ISD-Lite field 8 is integer wind direction (degrees, 0..360); field 9
  integer speed in 0.1 m/s units; original missing is -9999. Model input
  must use 9999 missing sentinel. No original C/V/N provenance is inferred.

## Stage 1: 数据清洗和缺测处理完毕
- Complete OBSERVED noncalm wind: WD in (0,360], WS>0 => use as observed.
  This is the ONLY allowable source for later whole-vector forward hold.
- (WD,WS)=(0,0) => standard original calm, retain (0,0).
- (WD,WS)=(360,0) => accepted original calm, normalize to (0,0),
  preserve original source WD and count separately.
- Any other combo (partial missing -9999, both missing, WD=0 WS>0, or
  other invalid), or an entire missing original hour => invalidate the
  ENTIRE wind vector, not just the absent component, then carry the latest
  whole valid OBSERVED noncalm (WD,WS) pair together, without time limit.
  Observed calm must never seed the carry.
- If no previous valid OBSERVED noncalm wind exists, retain both fields as
  9999 (strict mode aborts); never backfill a missing source hour or
  synthesize a bearing.
- Every other meteorological field (temperature, pressure, humidity,
  precipitation and cloud fields) independently carries its own previous
  valid value without a duration cap. Leading missing stays 9999.
- Stage file: USAF-WBAN-YEAR_cleaned.csv with same 3 GENERIC header lines.
  Sidecar USAF-WBAN-YEAR_cleaned.stage.json has exact Chinese completion
  status, without contaminating the fixed SMERGE GENERIC header.

## Stage 2: 静风插值处理完毕
- ONLY observed calm hours from original categories (0,0)/(360,0) are
  replaced; no forward-held synthetic calm is eligible.
- Identify maximal contiguous original calm runs first.
- Interior calm run with REAL observed positive-speed bearings on BOTH
  sides: use one fixed shortest-circular-arc midpoint for the ENTIRE run,
  and one fixed WS=0.5 m/s for EVERY hour in that run.
- All modeled positive-speed bearings are in (0,360], never WD=0.
  For example 350/10 ->360; 170/190 ->180; 360/360 ->360.
- Explicit USER SPECIAL CASE: exactly antipodal endpoints (difference 180)
  always produce WD=180, including 90/270 and 1/181. Opposite endpoints
  have no unique shortest circular arc; 1/181 ->180 is an imposed convention
  and is NOT the mathematical circular midpoint.
- Beginning-of-year calm without real positive prior anchor: copy BOTH
  direction AND speed from the next real positive observed vector for the
  entire run (do not force 0.5). End-of-year calm without next anchor:
  copy BOTH components from preceding real positive vector.
- If a whole year has no positive observed wind endpoint, abort rather than
  invent input.
- Interpolation endpoints must be ORIGINAL valid noncalm observations, never
  forward-held data or previously inserted artificial calm replacements.
- Output file: USAF-WBAN-YEAR_calm_interpolated.csv with corresponding
  sidecar .stage.json bearing Chinese stage status. The downstream
  compatible USAF-WBAN-YEAR_filled.csv is numerically identical to stage2,
  and becomes research-period-clipped isd2smerge_ready/USAF-WBAN.csv.

## Mandatory annual audit
- Total calendar hours, present ISD-Lite raw hours and ratio,
  entire absent source hours and ratio.
- All wind-invalid hours (include absent + present-but-invalid), count/rate.
- All original calms, count/rate, separate (0,0) and (360,0).
- Complete true positive wind hours, whole-pair held hours and longest
  age, stage1/stage2 valid-vector counts, calm run count.
- Interior constant half-mps replaced hours vs boundary copied hours.
- Every individual source/hold/replaced vector; source UTC and real
  calm interpolation endpoint UTC.
- Raw source ISD-Lite gzip SHA256 and station-year metadata.

## SMERGE/CALMET boundary
SMERGE v5.7.0 GENERIC JDAT=8 only reads numeric wind pair, not C/V/N.
Partial pair with 9999 becomes missing. Original CALMET local wind-speed
safety minima are separate from this user-prescribed human calm replacement.
The new P5 results deliberately differ from Scheme C: original calm is
now replaced before model input. **Reference Fortran SURF.DAT and CALMET
numerical validation have NOT yet been demonstrated.** Data completion and
user-chosen artificial calm replacement are not higher-quality observations.
