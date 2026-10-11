# P6E isolated parallel development — 2026-10-11

Development branch: codex/cteas-p6e-surface-boundary-layer-20261011.
Frozen P6C starting point: 490a09c292477f8736efbd7038171faa48e8cbfe.
Concurrent P6D owner: codex/cteas-p6d-slope-original-parity-20261011.

Implemented first independent E1 module: source-backed SOLAR solar-elevation
kernel from unchanged CALMET 6.5.0 calmet.for lines 30871–30990.
REAL*4 constants, east-positive longitude, 26 signed half-hour values,
including negative nighttime sine, strict independent C++20 target and tests.
No shared CMake or P6D source paths modified. C++ software tests do NOT prove
unmodified original-source parity; independent Fortran comparison is required.

Pending before complete P6E PASS: exact original PGTSTB, SURFVAR, HEATFX,
ELUSTR, MIXHT, TEMP3D and selected cloud/RH/radiative-flux processes;
original source unit comparisons on day/night, calm, stable and convective
scenarios; P6D final 3D wind input coupling; true 1h/6h/143h original
comparison with all relevant P6E intermediate/output fields.

P6C = PASS_SCOPED_HEFEI_2022_OBSERVATIONS
P6D = IN_PROGRESS_NOT_PASS
P6E = E1_SOLAR_IMPLEMENTED_SOFTWARE_PENDING_INDEPENDENT_PARITY
P6E_FULL = NOT_PASS
P6_CALPUFF_READY = FALSE
P6_WET_FORCING_VALIDATED = FALSE

2022 precipitation IPCODE missing 143/143, original NPSTA=0: no scientific
wet forcing qualification, no synthesized precipitation.

## Verified E1 evidence (2026-10-11)

Source-locked GitHub Actions original Fortran comparison **PASS**: https://github.com/RRain001/CTEAS/actions/runs/38115313127

- C++20 warning-as-error compilation PASS; independent software CTest 1/1 PASS.
- Original CALMET 6.5.0 SOLAR Fortran extracted without editing, Git blob 437f43bb7f0e51a1919e56664663e1a046986ed0.
- 2,808 / 2,808 original REAL*4 sine-of-elevation values PASS, six Julian days, three base time zones, six spatial points, 26 consecutive signed half-hour slots.
- Max absolute Fortran/C++ difference = 0.0 after 11-decimal-digit scientific formatting; frozen absolute tolerance = 3e-6; no mismatches. Evidence in workflow log and uploaded JSON artifact.
- A second PR-triggered run also PASS: https://github.com/RRain001/CTEAS/actions/runs/38115324902
- State update: P6E_E1_SOLAR_ORIGINAL_SOURCE_PARITY=PASS; FULL_P6E=NOT_PASS. Never generalize E1's standalone results to downstream COMP/SOLAR coupling or the complete P6E micrometeorology.

## E2 PGTSTB implementation (verification pending)

Native CALMET PGTSTB stability classification implemented in an isolated
header and independent CLI/CMake CTest. Original unchanged source is
calmet.for L13295-13498, including the Fortran column-major 12x7 Jstab
matrix, night cloud/ceiling paths, daytime solar thresholds, wind conversion
to integer knots and original water-cell skip. The implementation accepts
observed station ICC/ICEIL or gridded MCLOUD=2/3/4, and returns -999 on
water where original leaves the field untouched. Fail-closed validation of
bad/missing selected inputs is a deliberate modern safety contract, not
source-equivalent input error handling.

The source-locked Fortran harness covers 28 configurations x 20 cells =
560 output cells, checking land stability classes and water sentinels
exactly. E2 is **NOT PASS** until its GitHub original-source workflow
completes; selected 2022 COMP MCLOUD scheduling and P6D integration remain
separate gates.

## Verified P6E E2 and E3 evidence (2026-10-11)

The same [independent original-source CI run](https://github.com/RRain001/CTEAS/actions/runs/38116019206) confirms C++20 warnings-as-errors, **3/3 CTests PASS**, immutable CALMET 6.5.0 vendor source and three independent numerical kernels.

- **E1 SOLAR**: 2,808 / 2,808 values PASS, maximum exported numerical difference 0.0.
- **E2 PGTSTB**: 560 / 560 results PASS (498 land stability classes + 62 water skip masks), exact integer classes, zero mismatches; modes MCLOUD 1–4 covered synthetically.
- **E3 selected SURFVAR**: original *full* SURFVAR subroutine compiled unchanged with temporary mock COMMON declarations; 12 controlled one-station, all-land, observed-dry inputs x 5x4 grid = 240 grid cells x (temperature, RH, IPCODE) = **720 values PASS**. Temperature maximum absolute difference **3.0518e-5 K**, frozen tolerance **3e-4 K**; RH and IPCODE exact. Source stub AVETMP aborts if unexpectedly invoked. This does not validate smoothing, multisite inverse-distance behavior, surface water, prognostic fields, rain-data processing, or 2022 actual joint COMP inputs.

**Authoritative gate**: P6E_E1_SOLAR_SOURCE_PARITY=PASS;
P6E_E2_PGTSTB_SOURCE_PARITY=PASS;
P6E_E3_SURFVAR_ONE_STATION_SOURCE_PARITY=PASS;
P6E_COMPLETE=FALSE; P6D_INTEGRATED=FALSE; P6_CALPUFF_READY=FALSE.

Next physical components requiring full original parity: SURFVAR multistation and real P6C hourly fields, ELUSTR, HEATFX, MIXHT, TEMP3D and related selected cloud/RH/radiation paths. Eventually original 1h/6h/143h micrometeorological field comparison must be performed after P6D gate. Precipitation codes in real 2022 SURF are missing 143/143, original NPSTA=0. Original dry output IPCODE2D=0 must NOT be interpreted as valid observed zero rain; wet forcing remains unqualified.

## E4 HEATFX physical kernel — submitted, pending source-parity CI

Selected original CALMET v6.5.0 HEATFX (calmet.for L7999–8122)
is implemented in an isolated C++20 module. It computes **short-wave
flux at all cells**, even water, before skipping water sensible heat;
sets land nighttime sensible heat to the original -0.1 W/m² marker
without calculating nighttime energy balance; and calculates daytime
land heat flux with the original Holtslag–van Ulden energy-budget
constants, heat storage, anthropogenic flux, albedo and Bowen ratio.

A frozen original-subroutine harness compiles the verbatim source and
original hflux.met layout. Comparison: 24 synthetic cases × 20 grid
cells × 2 fields = 960 components. Includes land vs water, multiple
MCLOUD modes, fractional/observed cloud, twilight, daytime and
nighttime. Predeclared tolerance: 0.02 W/m² absolute + 3e-5 relative
for finite physical fluxes, exact equality for night/water sentinel.
E4 is NOT PASS until the corresponding GitHub action reports PASS.

Remaining: source-backed ELUSTR (u* and L), MIXHT / WSTARR
(convective and mechanical mixing heights and w*), TEMP3D, real
2022 multistation/surface and original 143h coupled 3D verification.
No P6D branch, vendor source, shared P6 CMake, or default main edited.

## E5 stable ELUSTR path — source parity running

Added independent C++20 QH<0 land ELUSTR stable friction velocity
(u*), Monin–Obukhov length (L), and calculated nighttime sensible
heat flux. Original CALMET full ELUSTR verbatim source lines
6333–6578 are compiled for the reference; PSIUD/PSIUC isolated stub
functions stop the run if the daytime branch is accidentally executed.
Frozen independent 30-case x 20-gridpoint x 3-field = 1800 scalar
comparison spans calm/weak/strong winds, clouds, roughness, temperature,
land/water masks and the stable limiting-iteration path.

This is **only** the QH<0 nighttime stable branch. The QH>=0
daytime unstable pathway uses PSIUD/PSIUC and needs separate original
source-backed numerical testing. No claim of full ELUSTR, MIXHT or P6E
acceptance until coupled 2022 source comparison.

## 2026-10-11: switched P6E CI to lhy-desktop

The GitHub-hosted `ubuntu-24.04` runner exhausted private-repository
hosted-job availability; the P6E workflow now requests `self-hosted`
and **fails closed** unless `RUNNER_NAME=lhy-desktop`, OS is Linux,
and CMake/C++, Fortran, Python, Git are installed. GitHub connector
cannot access the administration-gated runner inventory endpoint, so
the actual online state of the registered runner must be established
from the workflow job `runner_name` and executed step logs.
The first redirected runs 38118603445 / 38118601535 entered the
**queued / runner unassigned** state. Do not claim source-parity
acceptance for E5 without actual completed numerical comparison.

## E6 WSTARR (source validation pending)

Implemented isolated CALMET WSTARR convective velocity scale from
unmodified `calmet.for` L36884–36949:
 `w*=pow(9.81*QH*ZICONV/(TEMP2D*RHO*996),0.3333333)`
for positive sensible heat flux, and exactly zero for stable/nonpositive
flux, while leaving overwater WSTAR untouched. The original-source
harness includes 30 synthetic grids of 5x4 cells = 600 outputs,
day/night and water masks, frozen absolute limit 2e-5 m/s and
relative 3e-5; runs independently from unresolved MIXHT and P6D.
No E6 PASS claim until self-hosted numeric workflow completes.

Pending major physics: full daytime unstable ELUSTR; daytime and
nighttime MIXHT; TEMP3D and full 2022 hourly coupled comparison.

## E7 MIXHT nighttime mechanical mixing (pending source parity)

Added original CALMET `MIXHT` stable `QH<0` path: Venkatram
`constn*ustar**1.5` and Zilitinkevich
`0.4*sqrt(ustar*EL/FCORI)`, taking their minimum then enforcing
original user `ZIMIN`/`ZIMAX`; `ZICONV=0` on nocturnal land
while **all water fields remain unchanged**. The standalone compiled
Fortran harness preserves the *entire unmodified* original MIXHT
subroutine and actual vendor ZIPARM/UPMET includes. Unsupported
daytime MIXDT/MIXHMC/MIXHBG and debug OUT call stubs terminate if
mistakenly executed. Frozen 30 configs x 20 grid cells x 2 fields
= **1200 scalar values**, absolute 0.06 m / relative 4e-5 limits.

Neither E7 nor earlier E5/E6 is accepted until the self-hosted
lhy-desktop actually executes and returns original Fortran parity
logs/artifacts. This selected path does not qualify daytime mixing
heights, mixed-layer convective growth, or real AA/BB sounding
time-slicing. **Full P6E remains NOT PASS**.

## E5–E7 completed on dedicated lhy-desktop-2 (2026-10-11)

[Successful original-source CI run 38121546787](https://github.com/RRain001/CTEAS/actions/runs/38121546787) finished with
`runner_name=lhy-desktop-2`, 7/7 CTest PASS and all seven original
source parity steps PASS, including new E5/E6/E7.

- E5 ELUSTR selected **nighttime stable**: 30x20x3 = 1800 values
  PASS, 0 mismatches after fixing QH's output-serialization issue on
  water cells (the binary32-derived `-0.1` value is rendered by
  Fortran 14 digits vs C++ 11 digits). Land physics thresholds
  **not** relaxed; exact USTAR/L water sentinels retained.
- E6 WSTARR: 30x20 = 600 values PASS, zero mismatches, max
  absolute wstar error 4.920064355928844e-12 m/s.
- E7 MIXHT selected **nighttime stable**: 30x20x2 = 1200 values
  PASS, zero mismatches.
- E1–E4 remain PASS in this same run.

`P6E_E1_TO_E7_IMPLEMENTED_SELECTED_ORIGINAL_SOURCE_PATHS=PASS`;
`P6E_FULL=NOT_PASS`. Full daytime physics and 2022 actual hourly
input/field-coupling remain unqualified.

## E8 ELUSTR daytime/neutral — implementation, CI pending

Separate C++20 original-source day-land `ELUSTR` module retains the
original neutral `QH==0` sentinel `L=-9.9e9`, 5-pass convective
iterations, minimum friction speed and both `IPSIFCN=0` COARE
`PSIUD` and `IPSIFCN=1` legacy `PSIUC` functions.
The new independent Fortran harness extracts the unmodified full
ELUSTR and unmodified original PSIUD and PSIUC from pinned vendor
source. Frozen 36x20x3=2160 scalar comparisons span both PSI functions,
neutral/unstable, calm/normal/strong winds, roughness/temperature
and water skip. Do not mark E8 PASS until exact original-source CI runs.
Still pending: daytime MIXHT with real upper-air soundings, TEMP3D,
selected SURFVAR multistation, full 2022 hourly field coupling.

## E9 prognostic CLOUD3 cloud fraction (source comparison pending)

Added isolated original CALMET L30683–30735 RH-based prognostic
`CLOUD3` 850mb cloud-fraction kernel, with original >=99% RH
saturation branch. The source-locked full original Fortran subroutine
comparison fixes 32 cases x 20 cells = 640 scalar values across RH
0–100%, including the 98–99% transition and saturation. This is
not acceptance of MCLOUD3/4 selected hourly 2022 COMP, nor a
substitute for diagnosed station cloud observations.
No claim of E9 PASS until original CI finishes.
