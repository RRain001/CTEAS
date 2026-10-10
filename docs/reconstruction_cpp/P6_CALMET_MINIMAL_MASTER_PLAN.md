# P6 CALMET 6.5.0 — 合肥 100 m 科研最小重构总计划
Date: 2026-10-10. Status: **P6A_SOURCE_AND_CONTRACT_DESIGN_IN_PROGRESS / P6_ORIGINAL_NUMERIC_REFERENCE_BLOCKED**.
Authority: user-chosen research-specific **minimal reconstruction**; no claim of full CALMET feature parity.
Repository: `RRain001/CTEAS`; immutable foundation `60b7b1d93866fff772926b6ac32dc0af7235552b`; implementation branch `codex/cteas-p6-calmet-hefei-100m`. Do not edit any existing P1–P5 branch, source or the original `engine/vendor/src`.

## 0. Current evidence, critical distinction

- Original P6: `engine/vendor/src/CALMET_v6.5.0_L150223/calmet.for`, Git blob `437f43bb7f0e51a1919e56664663e1a046986ed0`, ~1.46 MB; `calmet.inp`, `cpl.txt`, relevant common-block `*.met`. The compiler's routine inventory in `cpl.txt` and original control text are initial anchors. **This is an inventory, not a completed line-by-line physical-code audit.**
- The committed `calmet.inp` is a **legacy example**: 2000 dates, `NX=NY=100`, `DGRIDKM=1`, generic `SS1`/`US1` placeholders and a 2023 path. **It is not a real 2022 Hefei 50×50 100 m runnable control file.** Never use the sample to certify the study.
- P3 [PR #5](https://github.com/RRain001/CTEAS/pull/5): real and synthetic scoped `GEO.DAT 2.1` eight fields, 20,000/20,000 printed field matches against original Fortran. Its legacy output writer is independent; no P1/P2 modern standalone full pipeline yet.
- P4 [PR #6](https://github.com/RRain001/CTEAS/pull/6): current FSL/READ62 scoped input/parser, **UP.DAT writer and original Fortran 2022 equivalence not certified**.
- P5 [PR #7](https://github.com/RRain001/CTEAS/pull/7): current GENERIC/SMERGE scoped input/parser, **SURF.DAT writer and original Fortran 2022 equivalence not certified**.
- No user-approved complete site meteorology (surface+upper-air+precip) at P6. **No 2022 CALMET original full-run results or numerical comparison exist.** Synthetic checks are software, never real-case or physics equivalence.

## 1. Frozen study geometry, subject to Fortran-control verification

- Hefei stack center: 31.939774444 N, 117.141610855 E. Entire model domain **5,000 m × 5,000 m** centered on stack, **not 5 km radius**. Cartesian `50×50` at `0.100 km`, grid origin SW *cell corner*, i-fastest / j increasing northwards; UTM zone 50 N, WGS84. Use the P1/P2/P3 independently verified case origin rather than recomputing/rounding stack coordinates.
- Nine CALMET cell layers, 10 interfaces: `ZFACE=[0,20,40,80,160,320,640,1200,2000,3000] m`. Do not confuse interface heights, layer-center heights, heights AGL, MSL, or sensor heights.
- 2022-01-01 00:00 through 2022-01-06 23:00, 144 nominal hourly labels. **`ABTZ`, `NSECDT`, observation-time conventions, beginning/ending-of-hour labels, and Fortran `CALMET.DAT` record semantics must be proven, not inferred.** No automatic assumption that nominal 144 labels = 144 saved records.
- Retain surface roughness, elevation, albedo, Bowen ratio, soil heat flux, anthropogenic heat flux, LAI, dominant land-use / water mask from verified P3; suppress unsupported inputs fail-closed.
- Stack emission/physical source parameters belong to P7 CALPUFF, **not** P6 CALMET input (prevent scope leakage).

## 2. Research-critical physics: mandatory vs optional

The first end-to-end *usable* P6 must support a selected original-observations pathway:
`NOOBS=0`, `IWFCOD=1` diagnostic wind, `IPROG=0`, `IRTYPE=1`, gridded non-snow GEO, surface station winds/temperature/humidity/cloud, upper-air profiles, and meteorological variables used by CALPUFF. These are **proposed study choices**, not asserted final original controls. Station counts and precipitation mode remain pending user data.

| Workstream | Original CALMET routines (initial anchors from compiler listing) | Original control levers | Research importance / gate |
|---|---|---|---|
| Time, geometry, consistency, upper/surface/geophysical readers | `SETUP`, `READCF`, `READGE`, `READHD`, `RDHD`, `RDHDU`, `RDS`, `RDUP`, `RDP` | Groups 0–4: `GEODAT`, `SRFDAT`, `UPDAT`, `IBYR..`, `IEYR..`, `ABTZ`, `NSECDT`, `PMAP`, `NX`, `NY`, `NZ`, `ZFACE`, `NOOBS`, `IFORMS`, `NPSTA` | Mandatory; no fabricated inputs or invisible time shifts |
| Terrain gradients and initial diagnostic adjustments | `TERSET`, `TOPOF2`, `SLOPE`, `FRADJ`, `WIND1`, `PREPDI`, `DIAGNO`, `DIAGI`, `DIAG2`, `BARIER` | Group 5 `IWFCOD`, `IFRADJ`, `ISLOPE`, `IKINE`, `IOBR`, `TERRAD`, `CRITFN`, `ALPHA`, `NBAR` | **Must retain every effect enabled in the frozen real control.** The example currently has Froude and slope ON, kinematic and O'Brien OFF. Terrain-following geometry must be source-verified. |
| Observational wind reconstruction and blending | `INTERP`, `INTP`, `INTER2`, `PROGRD`, `WIND1`, `SMOOTH`, `MINIM`, `DIVPR`, `DIVCEL`, `ADJUST`, `SIMILT` | `IEXTRP`, `ICALM`, `BIAS`, `RMIN2`, `RMIN`, `RMAX1/2/3`, `R1`, `R2`, `DIVLIM`, `NITER`, `NSMTH`, `NINTR2` | Mandatory selected route: surface/profile heights, calm wind, objective weights, Step 1/Step 2 fields, divergence metrics; mapping of each routine to physical phase subject to full source audit |
| Temperature/humidity, radiation and surface fluxes | `TEMP3D`, `AVETMP`, `SURFVAR`, `SOLAR`, `HEATFX`, `PGTSTB`, `ELUSTR`, `AIRDen` | `IRHPROG`, `ITPROG`, `TRADKM`, `IRAD`, `NUMTS`, `IAVET`, `TGDEFB`, `TGDEFA` | Mandatory for `IRTYPE=1` CALPUFF chain, not an optional cosmetic field |
| Boundary-layer and stability / mixing-height | `MIXHT`, `MIXHT2`, `MIXHTST`, `MIXHMC`, `AVEMIX`, `HEATFX`, `WSTARR` | `IMIXH`, `CONSTB`, `CONSTE`, `CONSTN`, `FCORIOL`, `IAVEZI`, `MNMDAV`, `HAFANG`, `IZICRLX`, `TZICRLX`, `ZIMIN`, `ZIMAX` | Mandatory mixed-layer / Monin-Obukhov / friction velocity and convection parameters for CALPUFF |
| Precipitation and cloud | `RDP`, `RDPN`, `RDS`, `RDCLD`, `CLOUD3`, `CLOUD4` | `NPSTA`, `IFORMP`, `ICLOUD`, `NFLAGP`, `SIGMAP`, `CUTP` | **Separate forcing blocker:** P5 SURF.DAT alone is not proof of valid CALMET precipitation station input; a verified precip path is required for wet-deposition studies |
| CALMET output for downstream CALPUFF | `OUTFIL`, `OUTHD`, `OUTHR`, `PACK`, `OUTPC`, `WRT*` | `METDAT`, `LSAVE`, `IFORMO`, `IRTYPE=1`, `LCALGRD` | Preserve original file version/record order/time, arrays, masked values, units and provenance; downstream P7 read test required |

**Optional legacy branches not targeted initially**: `NOOBS=1/2` prognostic-only; `IPROG` CSUMM/MM4/MM5/WRF/3D meteorology; `IGFMET` coarse-CALMET input; `NOWSTA`/SEA.DAT, overwater `COARE`/OCD flux, wave/SST handling, `LLBREZE` lake-breeze boxes; gridded CLOUD.DAT prognostic modes and other output/QA modes. **Only exclude after final 2022 control and landuse/water characterization confirm they are unused**. For any requested off-branch mode, fail closed; document original procedure and switch rather than silently ignore.

### Why ordinary “wind-only” / simplified power law is insufficient
- `IRTYPE=0` produces wind-only fields; it is **not** a CALPUFF-ready replacement for the selected CALMET chain.
- Simple inverse-distance station winds are not equivalent to terrain-adjusted `IWFCOD=1` with `IFRADJ=1` and `ISLOPE=1`; Step 1/2 differences and vertical divergence must be compared.
- No missing station data should be silently filled, no scientific constants retuned to fit output, no option silently defaulted.

## 3. Dependencies and ownership

P3 supplies `GEO.DAT v2.1` as a *pinned input*. P4 must supply independently validated, original-compatible `UP1.DAT` (with station ID, sounding layers, units, validity); P5 must supply original-compatible `SURF.DAT` (with hourly station observations and validity). Verify P4/P5 file-format choices match P6 `IFORMS` and time-zone controls exactly.

**Current P4/P5 public meteorological capture workflows are not substitute evidence of `UP.DAT` / `SURF.DAT` Fortran parity.** The P6 branch must not change P4/P5 source or tests or “repair” their output implicitly. Interface adapters must be explicit, local to P6, and versioned when provided. The authors of P4/P5 retain authority over those branches.

One P6 owner controls P6 option contracts/physics; parallel tasks allowed only after interfaces are frozen: (a) input readers/time QA; (b) wind diagnostics/terrain; (c) surface/BL physics; (d) independent output parser + reference comparison. Avoid multiple agents modifying the same files or shared branch concurrently.

## 4. Execution plan and stop/go criteria

| Substage | Deliverable | Data prerequisites | Exit criterion |
|---|---|---|---|
| **P6A** — inventory/control contract | Source version/blob inventory; original control / routine path matrix; case geometry/time/units/validity schema; strict fail-closed C++20 module + CTest | No weather needed | PASS_SOFTWARE_CONTRACT_ONLY; intentionally **no physics equivalence** |
| **P6B** — original input/output capture | Freeze actual P3 GEO.DAT, P4 UP1.DAT, P5 SURF.DAT and PRECIP.DAT path or justified absence; hash real 2022 input, control and compiled original CALMET; isolated original run | **Blocks on station data + format agreement** | Clean original setup, first hour, multi-hour; original CALMET.DAT record/field format decoded; provenance preserved |
| **P6C** — geometry and observation preprocessing | Terrain-following height transformations, station spatial mapping, surface-height adjustment, upper profiles, height/UTC interpolations, masks | Real or artificial snapshots; golden original P6 intermediate states | Unit/geometry/synthetic tests; then Fortran comparison per stage |
| **P6D** — diagnostic 3D wind | Selected slope and Froude effects; Step 1 wind, Step 2 objective analysis, smoothing, divergence minimization, vertical wind as enabled | Frozen P6B, independent selected original reference | `u/v/w`, mask/calm/station weights, divergence, near-terrain boundary behavior compared on 1h→6h→144h; no post hoc tolerance |
| **P6E** — complete surface and boundary-layer physics | Radiation, friction velocity, sensible heat, stability, mixing depth, 3D temperature/RH, precip/cloud | P6D + verified surface upper/precip | Independent reference per output and intermediate field on day/night/stable/convective/rain |
| **P6F** — `CALMET.DAT` writer | Original versioned header/record and binary/text formats, correct time indexing, field ordering, SI/display conversions | P6D/E physics | Original reader and CALPUFF original P7 reader accept C++ product; checksums/record count and numeric comparison |
| **P6G** — full-scoped scientific acceptance | Repeat 144h, compare Fortran and modern P6 under frozen identical inputs; audit per field time/layer/cell and cases | P6B–F; independent reviewers | `PASS_REFERENCE_SCOPED_P6_HEFEI_100M` and separate physical observation validation clearly distinguished |

Important: a software PASS, synthetic field or C++ writer's successful self-read **never establishes original Fortran parity**. Fortran source-to-C++ physical formulas, array ordering and original implicit `REAL*4` behavior are audited at each phase. Do not loosen tolerance after inspecting failures.

## 5. Reference comparison and QC protocol to freeze before numerics

1. **Provenance:** original Fortran source SHA/commit, compiler and options, input and control hashes; preserve isolated `stdout/stderr`, `CALMET.LST`, `CALMET.DAT` and key intermediate diagnostics. Original sources read-only; record any unavoidable original compatibility shims outside vendor tree.
2. **Dimensions and time:** compare `NX×NY×NZ×record_count` with explicit layer-face and local/UTC policy. For each field record valid/missing denominators, NaN/Inf, masked positions, exact categorical/QC flags, unsupported options and unit conversions.
3. **Physics fields:** step-1 and final `u/v/w` (m/s), flow/stability/terrain flags, friction velocity `u*`, Monin–Obukhov length, convective `w*`, `zi` (m), temperature (K), RH (%), precip (mm/h), cloud, surface flux (W/m²). Detailed availability and units must be confirmed from the selected original `CALMET.DAT` version before freezing comparisons.
4. **Error metrics:** for floats: MAE, RMSE, max absolute, max relative (only where magnitude is meaningful), p99, counts over predeclared absolute + relative tolerances and first offending coordinates/time/layer; for integers/masks exact. Tolerances must account for original output quantization and source precision **before** running the candidate C++ comparison.
5. **Stress tests:** flat terrain; analytic slope/ridge/saddle; near-zero and opposing winds; finite calm, dry/rain, low-/high-stability, sharp vertical gradients, missing surface/upper observations; terrain roughness and 100 m grid sensitivity, and a 144h repeatability/cross-platform test.
6. **Regulatory / use bound:** no claim that the 5×5 km science-domain wind field reproduces regional processes using only 5 km weather. CALMET influence radii can exceed domain size by orders of magnitude; station availability, representativeness and region-boundary physics need explicit assessment.

## 6. P6A intentionally limited change list

Allowed now: `docs/reconstruction_cpp/P6_CALMET_MINIMAL_MASTER_PLAN.md`, isolated `native_cpp/modules/p6_calmet/` contracts/tests, P6-only CI/PR. No P6 Fortran physics port before source-specific routine audit, actual forcing and original baseline; no code editing in `engine/vendor/src`, P4/P5, P1/P2/P3 PRs or shared foundation.

**Current scientific status (2026-10-10): P6_SOURCE_MAP_IN_PROGRESS, P6A_CONTRACTS_CAN_BE_SOFTWARE_TESTED, P6_REFERENCE_BLOCKED_METEO_INPUTS, P6_WIND_PHYSICS_NOT_STARTED, P6_CALPUFF_READY=FALSE.**
