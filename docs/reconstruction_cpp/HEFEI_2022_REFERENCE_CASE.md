# Hefei 2022 reference-case contract (P0)

Date: 2026-10-10. STATUS: SOURCE_SPEC_CAPTURED / REAL_REFERENCE_NOT_YET_FROZEN.
Original requirement source: testcase.md, CTEAS/main @ 69180557b97b99a2ffbf7f7a786909e570a06d6b.
No running, calibration, case-data acquisition or scientific comparison is claimed here.

## Known, user-supplied quantities (do not alter)

| Parameter | Value | Units / qualification |
| --- | --- | --- |
| Stack latitude | 31.939774444 | degrees N |
| Stack longitude | 117.141610855 | degrees E |
| Physical stack height | 70.8 | m; must distinguish stack height AGL vs outlet elevation MSL |
| Stack inner diameter | 1.6 | m |
| Stack exit temperature | 25 | degrees Celsius |
| Stack exit velocity | 17.26 | m/s |
| HTO annual release | 2.63E+13 | Bq/year |
| HT annual release | 0 | Bq/year |
| Study range wording | 5 | km around stack; geometry NOT_YET_RESOLVED |
| Horizontal spacing | 50 | m |
| NZ | 9 | layers |
| ZFACE | [0,20,40,80,160,320,640,1200,2000,3000] | m; ten interfaces, vertical datum/AGL role to be verified in original controls |
| Time labels | 20220101:00:00:00 to 20220106:23:00:00 | timezone/record-start-end role not yet resolved |

If interpreted as one hourly instant per inclusive hour, the two timestamp labels span six days / 144 labelled records. Do not silently transform this into an inclusive/exclusive CALMET or CALPUFF time-step contract without checking their respective input conventions. Case is in year 2022 (non-leap year), but no source rate in Bq/s shall be finalized until the annual-vs-short-window emissions policy is frozen.

## Must resolve before reference-capture acceptance

1. Domain geometry: 5-km radius around stack, plus/minus 5-km square, or 5-km total-width square; explicitly lock NX/NY, origin, center/cell convention and projected CRS. A 50-m grid does not alone resolve 100 vs 200 cells per axis.
2. Projection, datum and units: coordinate system/EPSG, map-axis rotation, grid origin, terrain/stack elevation, lake/reservoir shoreline roles, exact vertical-face meaning.
3. Original reference runtime: source commit, executable SHA, full Fortran controls, source/data provenance, compiler/build options and relevant CALMET/CALPUFF flags, whether their actually run inputs match this 2022 case.
4. Data: DEM tiles/resolution/datum; land-use raster and classification tables; surface/upper/optional overwater station inventories and source IDs; temporal coverage, cloud, T, humidity, pressure, precip and heat-budget variables; external dataset licenses and archival constraints.
5. Meteorological times: fixed UTC/local zone and each file's record timestamp convention, all 144 proposed hour labels, missing-hour handling and upper-air temporal support.
6. HTO: continuous or episodic release, time normalization from annual activity (Bq/year), conversion at species/transport interface, phase/chemical form, dry/wet deposition, decay, treatment of background, comparison outputs and units; explicitly mark unsupported CALPUFF chemistry branches.
7. Independent observational checks: reserved monitoring datasets and any assimilation/station reuse; predeclare skill metrics before results are observed.

## Seven-program reference evidence

| Program | Target legacy version | Required frozen evidence | Status |
| --- | --- | --- | --- |
| TERREL | 7.0.0_L141010 | DEM/control, actual terrain products, per-cell values and masks, source and result hashes | PENDING |
| CTGPROC | 7.0.0_L150211 | land-cover/control/class table, actual grid products, class fractions/missing flags, hashes | PENDING |
| MAKEGEO | 3.2_L110401 | TERREL+CTGPROC parents, original GEO output, per-grid land/water/physical-property values | PENDING |
| READ62 | 5.661_L110225 | sounding source/headers/records, station/height/UTC conventions, original UP output | PENDING |
| SMERGE | 5.7.0_L121203 | surface source/QA/fill/policy, original SURF output, full available-hour records | PENDING |
| CALMET | 6.5.0_L150223 | GEO/UP/SURF and optional parents, controls, micrometeorology + diagnostic wind intermediate fields, final meteorological outputs | PENDING |
| CALPUFF | 7.2.1_L150618 | CALMET parent, source and species configs, puff-state/dispersion/transport budgets, receptor concentrations/deposition | PENDING |

For each reference: record execution attempt and failure too; status remains PENDING until independently parsed and source/inputs/results bound. Historical smoke5d and Shanghai experiments are precedent and may support tools, but cannot be relabelled Hefei-2022 reference.

## Validation hierarchy

- T0: input/format and source/metadata completeness. All required grids and hours; no silent fills.
- T1: original Fortran fresh reference, archived with hashes, independent parse and numerical summary; repeat at least twice on Linux if practical.
- T2: one original routine or processing stage, identical inputs and options, explicit expected outputs.
- T3: C++ recreated stage, comparator on physical values and masks with fixed tolerances and full count.
- T4: fixed full six-day case, end-to-end mixed Fortran/C++ switch and all-C++ candidate.
- T5: independent observations and sensitivity study, separately reported from source-behavior equivalence.

No claim of equivalence or valid HTO dose calculation is authorized by this file.

## Runner execution policy

A GitHub Self-hosted Runner is reported as registered for RRain001/CTEAS, but its label list, online status, permissions, workspace data access and existence of usable workflow files have NOT been verified through this audit. First proposed action: a manual, trusted-branch-only, resource-bounded environment and exact-input-inventory preflight. No automatic push/PR execution and no private data/secrets in artifacts.

## Next actions (P0 only)

- Read and index existing engine/preprocess, engine/run_met, engine/run_puff, scripts native build tools and seven vendor input templates.
- Read actual 2022 case configs and source manifests from authorized private data location on Linux; if absent, report exact missing items before deciding case semantics.
- Build source/test/controls/output identity matrix per program and pin a reproducible reference binary.
- Freeze scientific/output comparison denominator and per-variable thresholds BEFORE C++ results are produced.
- Produce a P0 evidence report and independent review. Only after P0 acceptance, start P1/P2.
