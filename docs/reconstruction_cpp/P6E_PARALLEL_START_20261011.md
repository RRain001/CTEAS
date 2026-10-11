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

## E8–E9 source-backed selection (2026-10-11)

- [E8 dedicated CI PASS #38121765658](https://github.com/RRain001/CTEAS/actions/runs/38121765658):
  8/8 CTest, 2,160/2,160 original source
  `ELUSTR` daytime/neutral scalar parity, both original
  `PSIUD` and `PSIUC` correction forms, 0 exceedances.
  Nighttime ELUSTR accepted separately at E5.
- [E9 dedicated CI PASS #38121943909](https://github.com/RRain001/CTEAS/actions/runs/38121943909):
  9/9 CTest, 640/640 unchanged original CLOUD3 RH-to-cloud values,
  128 RH>=99% saturated grid cells, max abs error 4.37e-13,
  0 mismatches.

## E10 observed multistation SURFVAR (verification pending)

New source-aligned all-land, 4 observed stations, `NPSTA=0`,
`NOOBS=0`, `ITPROG=0`, `IRHPROG=0`, `IALL=1`, `IAVET=0`
multistation temperature and humidity interpolation.
This expands E3 one-station validation to original `IRAD=1/2`
inverse-distance/exponent weighting, `TRAD` radius, `NUMTS` site
limits, stable distance sorting, station temperature/RH gaps, 80%
RH land default and original integer truncation. Numerical comparator
compiles **unchanged full CALMET SURFVAR**, fixed 48 cases x
5x4 cells x three outputs = **2880 scalar checks**, absolute
T tolerance 0.0003K and exact integer RH/IPCODE. This does
**NOT** simulate measured 2022 hourly COMP or independently validate
IPCODE precipitation; no rain data for 143/143 hours.

State E10 implementation SUBMITTED, NOT PASS until runner CI returns
source evidence. P6E_FULL remains NOT PASS: actual 2022 one/six/143h,
upper-air sounding-based daytime MIXHT, TEMP3D and water paths pending.

## E11 MIXHMC daytime Maul-Carson physical growth — CI pending

A separate original-source aligned `MIXHMC` convective growth kernel
(`ILAPSE=2`) now covers computed potential-temperature gradient
clipping, user-defined buoyancy threshold, binary collapse versus
exponential relaxation (`IZICRLX`), timestep factors, convective
potential-temperature jump `DPTT`, and `ZIMAX` clipping.
Scientific parity harness compiles verbatim pinned original
CALMET 6.5.0 `MIXHMC` subroutine with original `ZIPARM.MET` and
`GEN.MET`, stubbing unreachable prognostic `MIXDT2`. Frozen 160
configurations x five outputs = **800 scalar comparisons** spanning
night/warm, threshold, collapse, growth, relaxation, and time steps.

E11 is **a subcomponent of daytime MIXHT**, not full daytime
`MIXHT` with the actual `MIXDT` temperature-sounding calculation.
Its status remains PENDING until original-source CI returns PASS.

## 2026-10-11 E10 and E11 original-source acceptance

[lhy-desktop-2 run 38122662755](https://github.com/RRain001/CTEAS/actions/runs/38122662755)
completed SUCCESS on immutable branch `d501d933`, with **11/11
CTest**. E10 `SURFVAR` four-station temperature/RH interpolation:
**2880/2880 comparisons PASS**, zero temperature K difference,
identical RH/IPCODE, after replicating the original Fortran
pairwise site ordering (not stable sort). E11 `MIXHMC` provided-lapse
convective growth: **800/800 comparisons PASS** with zero
exceedances, maximum mixing-height absolute difference <5e-9 m.
These remain selected-branch checks, NOT the full 2022 COMP case.

## E12 MIXDT upper-air sounding lapse-rate (pending original CI)

Added independent source-locked 6.5.0 `MIXDT` temperature profile
search and interpolation, preserving 999.89 K missing threshold,
strict above/below brackets, original `0.0098 K/m` environmental
lapse adjustment and `DPTMIN` floor. Unsupported or nonmonotonic
sounding levels fail closed rather than generating invalid fields.
Unmodified original `MIXDT` source + original `GEN.MET` is compiled
with a dual-site 10-level fixture: **96 cases ×3 variables = 288
scalar original comparisons**. The fixture deliberately includes
missing levels, multiple heights/depths and minimum-lapse floors.
E12 remains **NOT PASS** until lhy-desktop-2 numerical CI completes.

Next integration barrier: use the `JUSTA/IUPT/NEARU` AA/BB older
sounding state from full `MIXHT` to feed E12 then E11, compute
mechanical/convective max heights and preserve water/negative QH;
finally `TEMP3D` and selected real 2022 1/6/143h COMP parity.
`P6E_FULL=NOT_PASS`. Rain `IPCODE` is missing 143/143.

## E13 full MIXHT chosen Maul-Carson AA/BB branch (verification pending)

Added standalone grid-based selected CALMET `MIXHT` 6.5.0
`IMIXH=1` original path, composing E12 `MIXDT` and E11
`MIXHMC`: chosen `IUPT` override, `NEARU`, `JUSTA` older
AA/BB sounding selection, air-density sensible buoyancy,
convective and mechanical mixing-height competition, original
`ZIMIN/ZIMAX` bounds, stable branch `DPTT=0` reset, and
unchanged water fields.

Independent source-locked comparison compiles **unmodified**
original `MIXHT + MIXDT + MIXHMC` Fortran subroutines, original
ZIPARM/UPMET/GEN includes and fail-stop unreachable BG/MM5/OUT
routines. Frozen 28 synthetic cases ×20 cells ×3 original fields =
**1,680 numeric checks**, 0.07 m height and 0.004 K jump
absolute thresholds plus 8e-5 relative tolerance. Both AA and
BB, source-choosing IUPT, water and stable cells are covered.

**E13 NOT PASS** until successful source numerical CI. Even once
selected path passes, it does not validate `IMIXH=2`
Batchvarova-Gryning, `COMP` vertical integration, 2022 true
soundings/1h→6h→143h or `TEMP3D`.

## E14 TEMP3D observed surface+upper-air 3D field (verification pending)

New selected all-land original CALMET TEMP3D subroutine path:
`ITPROG=0`, `NOWSTA=0`, 2 upper-air sites, AA/BB 00/12Z
bracket within one UTC day, and `IRAD=1/2` spatial weighting.
Time-interpolates grid-face sounding temperatures, interpolates
upper-air temperature across sites, then forms vertical cell
temperatures with the original three ways of placing `ZICONV`:
above boundary layer, below the whole cell, or crossing a face
within the cell; separately preserves surface T.

Independent source comparator compiles **unchanged full original
TEMP3D 31159-31709**, with exact same-day synthetic time helper
stubs and narrowly mocked COMMON state for land-only:
24 cases × 5×4 cells ×6 levels = **2880 scalar comparisons**,
frozen 0.0005 K absolute and 1e-5 relative thresholds.

This does not qualify `ITPROG` prognostic temperature, water
temperature gradients, observation interpolation across day/year,
source 2022 real COMP selected hourly vertical geometry or full
P6E. E14 CI pending.

## E15 P6C COMP/RDSN to P6E selected-hour read-only bridge

Provides first real typed consumer of **existing P6C headers** from a
P6E-only file `p6c_hourly_bridge.hpp`. It does not mutate P6C
timelines or P6D diagnostic wind. A `CompBufferedHour` from the
buffered P6C COMP and `RdsnHourlyState` from original RDSN source
are joined by exact hour-end identity. The bridge explicitly maps
P6C's chronological `aa_profile/bb_profile` onto original CALMET
*physical* AA/BB slots using `jorder`: when JUSTA=-1, the
P6C `aa_profile` field stores chronological older but original
physical **BB** is older. The resulting physical source temperatures,
original sounding heights, missing masks, precipitation-code observed
status and gap seconds are made available to P6E; the bridge cannot
silently reinterpret `IPCODE=9999` as observed dry `0`.

Synthetic 143-hour P6C `CompBufferedTimeline`+
`RdsnBufferedTimeline` test verifies all transitions through twelve
12-hour soundings (including inverted AA/BB), input time continuity,
and a sampled `MixhtObservedMcInput` assembly. Synthetic fixture
deliberately includes 142 missing precipitation codes and one
**observed zero** to detect accidental conflation.

This is **software integration contract** only, not original Fortran
per-hour COMP/2022 HTO wet forcing/meteorology and not claim of P6D
completion. Dedicated runner now retrieves seven pinned P6C headers
via sparse-checkout or SHA-verified GitHub REST bootstrap; never
changes any P6C/P6D source. E15 PASS awaits CTest on runner.

## E15 read-only real 2022 input CLI

`p6e_p6c_hourly_bridge` now takes actual P5 formatted `SURF.DAT`
and P4 `UP.DAT`, an upper-air station ID, and CSV output path:

`build/p6e/p6e_p6c_hourly_bridge SURF.DAT UP.DAT <UPPER_STATION_ID> p6e-hourly.csv`

This executes P6C's frozen 9-layer/10-face buffered hourly input
pipeline and emits one row per actual 2022 hour (143 total), including
the *physical* AA/BB older/newer mapping, RDSN row and temperature
observations, explicit missing-vs-observed-zero precipitation code,
and 00/12 sounding state. It is a **read-only preparatory trace**, not
CALMET's 3D output and not full original COMP numerical parity.
The runner CI compiles the CLI and tests its core typed mapping using
synthetic 143-hour data. The runner job intentionally does not claim to
have the actual SURF.DAT or UP.DAT payload until those files are
explicitly supplied/generated and traced.

The first E15 CI job #38125910348 exposed default self-hosted
`GNU 9.5.0` incompatible with P6C's `std::chrono::sys_seconds`.
P6E-only CI now selects **GCC >=11** and invalidates the stale CMake
build cache; this is a toolchain correction, not a physics relaxation.

## E16 synthetic 143h P6C→MIXHT→TEMP3D sequential integration

E15 [successful original source CI #38126277567](https://github.com/RRain001/CTEAS/actions/runs/38126277567)
used dedicated `lhy-desktop-2`, selected GCC **12.3.0**
instead of the incompatible host GCC 9.5.0, passed 15/15
software CTests and the existing E1–E14 Fortran source parity suite.
The E15 143-hour P6C buffered/observed **typed boundary test PASS**.
Actual 2022 source `SURF.DAT` and `UP.DAT` were not checked out
into this P6E CI; CLI's real-file behavior is not yet case certified.

New E16 introduces **a one-grid-cell, 143 consecutive hourly
physics flow test**. It consumes P6C's pre-existing COMP AA/BB
buffered observations and RDSN hourly state, reorders original
physical AA/BB using the E15 adapter, then invokes the selected
E13 `MIXHT + MIXDT + MIXHMC` kernel, propagates prior `ZICONV`
and `DPTT` across all hours, and drives E14 `TEMP3D` using
the *offset within each 12-hour sounding bracket*, including at
the 12Z switches. It exercises positive/zero/negative heat flux
and explicitly checks stable convective-memory resets, inverted
AA/BB buffers, surface temperature preservation and finite
3D temperature columns. It uses **synthetic profile and heat
flux inputs** and checks structural/physical invariants rather than
claiming original Fortran 143-hour field equivalence.
`E16_SYNTHETIC_INTEGRATION` awaits CI and must not be confused
with `P6E_REAL_2022_FULL_COMP_PARITY`, which remains blocked on
actual full-scope meteorological/diagnostic inputs and P6D handoff.

## E17 formatted 143h SURF.DAT + UP.DAT → P6E CLI test

Additional file-backed integration test independently generates
**synthetic but original P6C formatted** `SURF.DAT v2.1` with
143 one-hour intervals, and `UP.DAT v2.1` with 13 upper-air
soundings at 00/12 UTC, 10 valid levels per sounding in original
fixed-width 29-char groups. It executes the new actual P6E
`p6e_p6c_hourly_bridge` binary twice, asserts output SHA
determinism and 143 rows, both inverted/normal physical AA/BB
time-ordered mappings, correctly observed zero vs 9999 missing
precipitation masks, gridded upper-air face temperature validity,
and fail-closed wrong station IDs. The test has no numerical
physical source claim: **the meteorological values are all synthetic**.

`E17_FORMATTED_CLI` awaits CTest PASS; the true 2022 SURF/UP
files remain external to the P6E sparse checkout, and the
original-source full CALMET 143h parity gate remains pending.

## E18 pinned actual P6C Hefei 2022 observation replay (source-backed input, provisional mixing)

[Original P6B 2022 CI #38113874913](https://github.com/RRain001/CTEAS/actions/runs/38113874913)
produced retained artifact `11693445092` with actual original 2022
P6C joint observation CSV (SHA256 `400226164f4e17665487ec0845f3ca6025ecdf310178a12fd9bed9d53818554f`)
and independent original COMP/RDSN 143h parity status
`PASS_ORIGINAL_COMP_RUNTIME_AA_BB` (14 original COMP/RDSN scopes,
all 143 hours, 11 physical AA/BB flips, no source mismatch).
Its original runtime-source match was performed in P6B, **not re-run
by P6E**.

E18 stores a compact zlib/base64 trace of **exact original P6C
measured** 143 surface temperature / RH / cloud / 9999 precipitation
states and 13 unique upper sounding temperature-face arrays. The
dataset uses chronological AA/BB indices, `JUSTA`, `AA_SLOT/BB_SLOT`,
sounding time lags and turnover counts preserved from the real
hourly P6C CSV. Canonical JSON SHA256:
`ac45709dec5d5c624e2fbeeede57b4e55c311e384cfcb7a127afa4b5dcc9513b`.
Exact original CSV and original COMP report hashes are pinned in the
validation script, so source evidence is auditable and survives
the expiry of temporary 7-day workflow artifacts.

The E18 runner test replays all **143 actual 2022 observation hours**
into independently already source-validated selected E14 `TEMP3D`
with a *single fictive land grid receptor* at 50m/50m and **three
explicitly synthetic convective-mixing scenarios**
`ZICONV = 0 / 150 / 500 m`; it asserts physical temperature
bounds, exactly original source surface temperature on layer 1,
AA/BB inversion and clocks, zero invented precipitation and
output-byte determinism, with separate 1h/6h/143h QA windows.
No P6D or real original CALMET 2022 P6E full field equivalence is
asserted. **The only actual physical 2022 inputs here are archived
P6C observations; ZICONV is a clearly labeled test parameter.**
E18 remains pending CI until accepted.

## 2026-10-11 local re-verification of the full selected parity suite

Independent local reproduction on the dedicated build host (not a GitHub
job), GCC **12.3.0** + gfortran 11 shim, commit `db14738`/`ad963d3`:

- `cmake -S native_cpp/modules/p6_calmet/p6e -B … -DCMAKE_CXX_COMPILER=$g++-12`
  and `ctest`: **19/19 PASS** (0.25 s) with the new E19 target.
- All **15** source-locked unchanged-Fortran comparators PASS with zero
  exceedances, plus the E18 actual-observation replay PASS.

Explicit E5–E7 numerical verdicts re-confirmed in this reproduction
(these replace the earlier "verdict withheld" state):

- **E5 ELUSTR stable night (`QH<0`)** — `compare_original_elustr_stable.py`,
  30 cases x 600 cells = **1,800 scalars PASS**, 0 mismatches. Cells: 535
  stable land, 65 water. Max absolute errors `QH = 5.000e-11 W/m²`,
  `USTAR = 4.910e-12 m/s`, `L = 5.000e-8 m`; frozen limits 0.001 / 3e-5 /
  0.04 absolute and 5e-5 relative. Water `QH=-0.1` print precision 5e-12;
  water USTAR/L sentinels exact.
- **E6 WSTARR** — `compare_original_wstarr.py`, 30 cases x 20 cells =
  **600 values PASS**, 0 mismatches, max absolute `w*` error
  **4.920064355928844e-12 m/s**; frozen limits 2e-5 m/s absolute and 3e-5
  relative. 411 land positive-heat, 114 land nonpositive, 75 water.
- **E7 MIXHT stable (`QH<0`)** — `compare_original_mixht_stable.py`,
  30 cases x 600 cells x 2 fields = **1,200 scalars PASS**, 0 mismatches,
  514 land / 86 water; max absolute `ZI` error 3.800e-9 m, `ZICONV` exactly
  0.0; frozen limits 0.06 m absolute and 4e-5 relative.

All three still carry the same scope caveats (stable/night branch and
standalone source comparison only; no 2022 COMP coupling).

## E19 original MIXHBG Batchvarova–Gryning convective mixing height

`E18` fixed the convective height to three synthetic values. The remaining
documented gap "unsupported BG `IMIXH=2` path" needed the *other* legal
CALMET convective-growth closure. E19 adds it as an isolated C++20 kernel
`include/cteas/p6e/mixhbg.hpp` derived from **unmodified** CALMET 6.5.0
`calmet.for` L11304–11545 (`Subroutine MIXHBG` + `function fbg`):

- original analytical "false images" pseudo-steady solution with the
  Newton/secant iteration, the `|f| <= 1 s` early exits, the 10-iteration
  cap and the non-positive-argument fallback that averages the last two
  guesses;
- original REAL*4 constants `g=9.81`, `rho=1.2`, `cp=1005`, `vonk=0.4`,
  `ca=0.2`, `cb=2.5`, `cc=8`;
- the threshold flux `WTO = THRESH*H0/(RHO*CP)` and the `IZICRLX` switch:
  full collapse to zero when positive `L` or negative net buoyancy, exact
  previous-height retention when the net buoyancy flux is exactly zero, or
  relaxation `ZICEQ + (H0-ZICEQ)*exp(-NSECDT/TZICRLX)` toward the
  equilibrium height when `0 < WT < WTO`;
- the `GAMMA = max(GAMMA, DPTMIN)` lapse floor and both encroachment limits
  (`EL > -500 m` square-root form, `EL <= -500 m` cube-root form);
- the original trailing clamp `ZICBG = min(h, ZIMAX)` then
  `ZICBG = max(h, ZIMIN)`, both using `h`, reproduced verbatim.

`ILAPSE=2` only: the lapse rate above the previous mixing height is supplied
by the caller (selected AA/BB sounding). The MM5 `MIXDT2` search used by
`ILAPSE=1` is deliberately not implemented and is a fail-stop stub in the
comparison driver.

`reference/compare_original_mixhbg.py` compiles the verbatim original
`MIXHBG` + `FBG` with the real vendor `ZIPARM.MET`/`GEN.MET` includes and
compares **240 cases x 2 fields = 480 scalars**. Coverage gate (fail-closed,
no run starts if unmet): 72 collapse, 5 exact zero-net-buoyancy retention,
24 relaxation, 139 growth, of which **48 deep-limit `EL <= -500 m`** cases;
both `IZICRLX` modes and all `NSECDT` 600/1800/3600 s steps.

Local result on the dedicated host: `status=PASS`, 0 exceedances, max
absolute `ZICBG` difference **2.564e-3 m** (limit 0.07 m + 8e-5 relative),
max absolute `GAMMA` difference **4.90e-15 K/m** (limit 2e-7 K/m).

**Scope / NOT claimed:** this is the standalone BG convective-height kernel
with a *provided* lapse. It does **not** wire `IMIXH=2` selection into the
composed E13 `MIXHT + MIXDT + MIXHMC` grid path (that path still selects the
Maul-Carson closure), does not reproduce the `ILAPSE=1` MM5 `MIXDT2` lapse
search, does not run the `WATER`/`MIXHT2` prognostic callers, and does not
compare 2022 hourly COMP fields. `P6E_FULL` remains **NOT PASS**; 2022
precipitation `IPCODE` is still missing 143/143, so wet forcing stays
unqualified.

## 2026-10-11 E19 CI acceptance (lhy-desktop-2)

Commit **`4995166`** on `codex/cteas-p6e-surface-boundary-layer-20261011`:
`p6e-solar-original` **SUCCESS** on the dedicated runner
(`runner_name=lhy-desktop-2`, GCC **12.3.0** selected by the workflow),
[run 38130128225 / job 114443431983](https://github.com/RRain001/CTEAS/actions/runs/38130128225/job/114443431983).

- **19/19 CTest PASS** (`100% tests passed, 0 tests failed out of 19`),
  including the new `p6e_mixhbg_software`, with `-Wall -Wextra -Wpedantic
  -Werror` C++20.
- All **18** independent original-Fortran comparison steps PASS, including
  the new step *"Original unmodified Batchvarova-Gryning MIXHBG convective
  height source parity"*.
- E19 CI numbers are bit-identical to the local reproduction:
  `status=PASS`, 240 cases / 480 fields, coverage 72 collapse + 5 retention
  + 24 relaxation + 139 growth (48 deep-limit `EL <= -500 m`), 0
  exceedances, max `|dZICBG| = 2.563476840009571e-3 m`, max
  `|dGAMMA| = 4.900160138765486e-15 K/m`, original blob
  `437f43bb7f0e51a1919e56664663e1a046986ed0`, vendor sha256
  `430381b4fb87f15127e20b13cc3e99be8a0e0fccf1d5821110af7f980558d134`,
  unmodified MIXHBG source lines 11304–11557.
- E5/E6/E7 re-confirmed on CI in the same job with exactly the locally
  reproduced numbers (E5 1,800 scalars / 0 mismatches; E6 600 values,
  max `|dw*| = 4.920064355928844e-12 m/s`, 0 mismatches; E7 1,200 fields,
  max `|dZI| = 3.800096237682737e-09 m`, 0 mismatches).
- A duplicate **push**-triggered job for the same commit
  (run 38130123911) shows `cancelled` because the workflow
  `concurrency` group `cancel-in-progress` keeps only the newest run.
  That is superseding, **not** a scientific failure; the authoritative
  job is the SUCCESS above.

`P6E_E1_TO_E14_E19_SELECTED_ORIGINAL_SOURCE_PATHS=PASS`;
`P6E_E15_TO_E18_INTEGRATION=PASS`; **`P6E_FULL=NOT_PASS`** (no
2022 hourly original field parity, no `IMIXH=2` composed-MIXHT wiring,
no `ILAPSE=1` MM5 `MIXDT2`, 143/143 precipitation codes missing).

## E20 original IMIXH=2 selection wired into the composed MIXHT grid path

E19 validated the Batchvarova–Gryning kernel standalone; E20 closes the
documented "unsupported `IMIXH=2` path" gap in the *composed* E13 grid
path. `include/cteas/p6e/mixht_observed_mc.hpp` now selects the original
convective-growth closure exactly as `calmet.for` L10579-10610/L10622-10653
does:

- new `MixhtObservedMcInput::imixh` (`±1` Maul–Carson `MIXHMC`, `±2`
  Batchvarova–Gryning `MIXHBG`); the sign only selects the over-water
  scheme, so the land path dispatches on `|IMIXH|`;
- `MIXHBG` receives `WT = QH/(RHO*996)`, the `MIXDT` lapse rate, `THRESHL`,
  `ZIMAX/ZIMIN`, `USTAR`, `EL`, `HTOLD = ZICONV(old)`, `IZICRLX`,
  `TZICRLX`, `DPTMIN` and `NSECDT`, and — as in the source —
  `TK = tzaa/tzbb(kusta,1)`, the **first face temperature of the selected
  AA/BB sounding**, not `TEMP2D`;
- `MIXHBG` returns the `DPTMIN`-floored lapse rate, which then feeds the
  original mechanical height `hmech = cmech*ustar/bvf` (identical to the
  `MIXHMC` path because `MIXDT` already floors the lapse rate);
- **`DPTT` is left untouched** on the `|IMIXH|=2` path — only `MIXHMC`
  updates the potential-temperature jump — and `ZICONV` is bounded by the
  original `amin1(zimax,ziconv)` after the call, which also neutralizes
  `MIXHBG`'s own trailing `min`/`max` quirk;
- fail-closed: an unselectable `IMIXH` and a non-physical sounding surface
  temperature both throw instead of silently degrading.

`reference/compare_original_mixht_observed_mc.py` now compiles **four**
unmodified original subroutines (`MIXHT`, `MIXDT`, `MIXHMC`, `MIXHBG` +
`FBG`) with the vendor `ZIPARM/UPMET/GEN` includes; the previous `MIXHBG`
fail-stop stub is gone and `imixh` is read per case. Cases grew from 28 to
**48** (28 unchanged `IMIXH=1` + 20 `IMIXH=2`): **960 grid outputs, 2,880
scalar comparisons**, 0 mismatches, same frozen thresholds
0.07 m / 0.07 m / 0.004 K absolute with 8e-5 relative.

Fail-closed `IMIXH=2` coverage gate (no run starts if unmet): 288 daytime
land cells (223 growth, 24 collapse, 8 exact zero-net-buoyancy retention,
33 equilibrium relaxation, of which **48 deep-limit `EL <= -500 m`**),
75 nocturnal land cells, 37 water sentinels, 108 older-AA and 180 older-BB
cells, 3 timestep lengths and both `IZICRLX` modes.

Measured on the dedicated host (GCC 12.3.0) and, unchanged, on CI:
`status=PASS`, 0 mismatches, and the per-mode breakdown proves the
regression is zero:

- `max_abs_error_IMIXH1_MC` = `[4.690036803367548e-9 m, 4.690036803367548e-9
  m, 4.630074101896753e-12 K]` — **bit-identical to the accepted E13 run**,
  i.e. the Maul–Carson path did not move;
- `max_abs_error_IMIXH2_BG` = `[3.464e-3 m ZI, 8.942e-3 m ZICONV,
  4.640e-13 K DPTT]`.

**Scope / NOT claimed:** synthetic cases only, land plus unchanged water
sentinels. Still open: the over-water OCD scheme selected by the `IMIXH`
sign, `WATER`/`WATERP`/`MIXHT2` prognostic callers, `ILAPSE=1` MM5
`MIXDT2`, `MIXHTST`, averaged mixing heights (`IAVEZI`) and the actual
2022 hourly COMP field comparison. `P6E_FULL` remains **NOT PASS**;
2022 precipitation `IPCODE` is still missing 143/143, so wet forcing stays
unqualified.

## E21 real-case CALMET option profile: semantics, 2-D paths, output contract

The reference cold-start deck
`project/cases/smoke5d_coldstart_20260701/run/met/met.inp` selects
`IRTYPE=1`, `IMIXH=1`, `THRESHL=0.0`, `IZICRLX=1`, `ISURFT=-1`, `IUPT=-1`,
`ITPROG=0`, `NOOBS=0`, `NOWSTA=0`, `NM3D=0`, `LCALGRD=T` over 50×50 @1 km
with `NZ=9`. E19/E20's Batchvarova–Gryning work is a **non-critical-path
side branch**: the real deck is Maul–Carson.

- `include/cteas/p6e/realcase_profile.hpp` adds a fail-closed option gate
  (`validate_realcase_profile`) that encodes the validated envelope and
  rejects, with an explicit reason, every combination P6E has not
  source-validated: `ITPROG!=0`, `NM3D!=0` (MM5 `ILAPSE=1` `MIXDT2`),
  `NOOBS!=0`, `NOWSTA!=0` (over-water stations), `ISURFT!=-1`,
  `|IMIXH|∉{1,2}`, `IZICRLX∉{0,1}` and negative threshold fluxes. It
  returns the convective scheme, the upper-air lapse source, the 2-D
  surface-temperature flag, the per-cell lapse-station flag and whether the
  land relaxation branch is reachable at all.
- **The key real-deck consequence is stated and proven**: with `THRESHL=0`
  the net buoyancy flux is `WT-WTO = WT`, so the `IZICRLX=1` relaxation
  `ZIC = ZICEQ + (HTOLD-ZICEQ)*exp(-NSECDT/TZICRLX)` is *unreachable* over
  land. The real deck therefore runs `IZICRLX=1` with no relaxation ever
  applying.

`tests/realcase_options_tests.cpp` (`p6e_realcase_option_profile_contract`)
drives the composed chain MIXHT→TEMP3D on a 2-D 50×50 / NZ=9 field:

- `IZICRLX=0` and `IZICRLX=1` at `THRESHL=0` produce **bit-identical**
  `ZI`/`ZICONV`/`DPTT` (semantic consistency with the real deck);
- `IUPT=-1` really resolves the lapse station per cell — the ≥1,000 cells
  mapped to the steeper sounding grow a strictly taller convective layer
  than the cells mapped to the shallower one;
- `ISURFT=-1` really carries a 2-D spatially varying surface temperature:
  the field is not a single station value (7 distinct values over 2,500
  cells) and TEMP3D preserves it **exactly** on the first model layer for
  every cell, with all 22,500 level-major values finite and physical;
- water sentinels stay untouched, `zimin/zimax` bounds hold, output is
  deterministic, and non-monotonic soundings, all-missing soundings,
  wrong TEMP3D extents and negative convective heights all fail closed.

`reference/compare_original_mixht_observed_mc.py` gains a third fixture
block with the **exact real-case option tuple** (`IMIXH=1`, `THRESHL=0.0`,
`IZICRLX=1`, `IUPT=-1`, 2-D per-cell station assignment). Cases 28 → **60**
(28 `IMIXH=1` + 20 `IMIXH=2` + 12 real-case tuple) = **1,200 grid outputs,
3,600 scalar comparisons**, 0 mismatches, same frozen thresholds
0.07 m / 0.07 m / 0.004 K absolute with 8e-5 relative.

Measured on the dedicated host (GCC 12.3.0), **20/20 CTest**:

- `max_abs_error_IMIXH1_MC` = `[4.690036803367548e-9, 4.690036803367548e-9,
  4.630074101896753e-12]` — still bit-identical to the pre-E20 accepted run;
- `max_abs_error_IMIXH2_BG` = `[3.464e-3 m, 8.942e-3 m, 4.640e-13 K]` —
  unchanged from E20;
- `realcase_option_profile.max_abs_error_ZI_ZICONV_DPTT` =
  `[4.690036803367548e-9 m, 4.690036803367548e-9 m, 4.890254867717658e-13 K]`;
- `relaxation_branch_cells = 0` and
  `original_source_izicrlx_0_vs_1_identical = true` — the unmodified
  original `MIXHT`/`MIXHMC` produce byte-identical output for `IZICRLX=0`
  and `IZICRLX=1` at `THRESHL=0`, which is the source-level proof of the
  inertness claim above.

**Scope / NOT claimed:** synthetic fields; no over-water `OCD` scheme, no
`WATER`/`WATERP`/`MIXHT2`, no `ILAPSE=1` `MIXDT2`, no `MIXHTST`, no
`IAVEZI` averaging, no P6D wind coupling and no actual 2022 hourly COMP
field comparison. `P6E_FULL` remains **NOT PASS**; 2022 precipitation
`IPCODE` is still missing 143/143, so wet forcing stays unqualified.

## 2026-10-11 E20/E21 CI acceptance (lhy-desktop-2)

Both increments were accepted by the dedicated runner
(`runner_name=lhy-desktop-2`, GCC 12.3.0) with numbers identical to the
local dedicated-host reproduction:

- **E20** — commit `204124a`, `p6e-solar-original` **SUCCESS**
  ([run 38132150521 / job 114448526293](https://github.com/RRain001/CTEAS/actions/runs/38132150521/job/114448526293)):
  **19/19 CTest**, all original-Fortran comparison steps PASS, E13 harness at
  48 cases / 2,880 scalars with `max_abs_error_IMIXH1_MC =
  [4.690036803367548e-9, 4.690036803367548e-9, 4.630074101896753e-12]`
  (bit-identical to the pre-E20 accepted run) and
  `max_abs_error_IMIXH2_BG = [3.464e-3 m, 8.942e-3 m, 4.640e-13 K]`.
- **E21** — commit `c3da0cd`, `p6e-solar-original` **SUCCESS**
  ([run 38133191395 / job 114451591561](https://github.com/RRain001/CTEAS/actions/runs/38133191395/job/114451591561)):
  **20/20 CTest** including the new `p6e_realcase_option_profile_contract`,
  E13 harness at 60 cases / 3,600 scalars with 0 mismatches,
  `realcase_option_profile.max_abs_error_ZI_ZICONV_DPTT =
  [4.690036803367548e-9 m, 4.690036803367548e-9 m,
  4.890254867717658e-13 K]`, `relaxation_branch_cells = 0` and
  `original_source_izicrlx_0_vs_1_identical = true`.

Duplicate push-triggered jobs on the same commits show `cancelled` solely
because the workflow `concurrency` group with `cancel-in-progress` keeps the
newest run; those are superseding, not failures.

### P6E gate state after E21

`P6E_E1_TO_E14_E19_E20_E21_SELECTED_ORIGINAL_SOURCE_PATHS=PASS`;
`P6E_E15_TO_E18_INTEGRATION=PASS`;
`P6E_REALCASE_OPTION_TUPLE_SEMANTICS=PASS` (IMIXH=1, THRESHL=0, IZICRLX=1,
ISURFT=-1, IUPT=-1; 2-D paths and output contract exercised at 50x50/NZ=9);
`P6E_BG_IMIXH2=NON_CRITICAL_SIDE_BRANCH_PASS`;
**`P6E_FULL=NOT_PASS`**. Still open: P6D diagnostic-wind coupling, actual
2022 P4/P5 `SURF.DAT`/`UP.DAT` 1 h/6 h/143 h original field comparison,
over-water `OCD`/`WATER` callers, `ILAPSE=1` `MIXDT2`, `MIXHTST`,
`IAVEZI`; 2022 precipitation `IPCODE` missing 143/143, so wet forcing and
P7 wet deposition remain unqualified.

## E22 CALMET.DAT 2.1 output contract and real-file truth-comparison harness

The P6E micrometeorology chain had no defined *output-side* contract. E22 adds
one, derived from the unmodified original `OUTHD` (L12327-12516), `OUTHR`
(L13067-13237) and `WRTR1D/WRTR2D/WRTI2D` (L36734-36830) sources, and then
proves it against the frozen real reference case.

### Contract (`include/cteas/p6e/calmet_dat21.hpp`)

- Fortran sequential-unformatted framing (4-byte little-endian length before
  and after every payload), file declaration record (96 B), comment count,
  NCOM x 132 B comments, and the **176-byte controls record** — the original
  comment says 39 words but the real file has 44, including the ten-word run
  clock `IBYRN..IESECN`, `AXTZ` at byte 40, `IRLG/IRTYPE/NX/NY/NZ`, `DGRID`,
  the origin, `IWFCOD/NSSTA/NUSTA/NPSTA/NOWSTA/NLU/IWAT1/IWAT2/LCALGRD` and a
  60-byte `PMAP/DATUM/DATEN/FEAST/FNORTH/UTMHEM/IUTMZN/RNLAT0/RELON0/XLAT1/
  XLAT2` tail that is preserved verbatim.
- Static records (`ZFACE`, `XSSTA/YSSTA`, `XUSTA/YUSTA`, `XPSTA/YPSTA`, `Z0`,
  `ILANDU`, `ELEV`, `XLAI`, `NEARS`) with the same 24-byte header shape and
  four zero clock words.
- Hourly records of `24 + 4*NX*NY` bytes: 8-byte label, packed begin date
  `YYYYJJJHH`, begin seconds, packed end date, end seconds, then `NX*NY`
  words. `IRLG` intervals of exactly 3600 s, contiguous, with the original
  end-clock convention (same packed hour, seconds advanced to 3600).
- **Exact OUTHR label order.** OUTHR writes U, V and W *inside one loop over
  levels*, so the order is `U-LEV 1, V-LEV 1, WFACE 1, U-LEV 2, ...`,
  `T-LEV 1..NZ`, then `IPGT, USTAR, ZI, EL, WSTAR, [RMM], TEMPK, RHO, QSW,
  IRH, [IPCODE]`. The first P6E implementation grouped the wind components and
  was rejected by the real file; the mis-grouping is now pinned in the CTest by
  a literal expected sequence, independent of the reader/writer pair.
- Missing values use the original `XMISS = 9999.` for real fields and `9999`
  for the two integer fields (`IPGT`, `IRH`); missing cells are excluded from
  the truth-comparison denominators.

### Field ownership (what the real case actually needs)

| file field | unit | owner | real case |
|---|---|---|---|
| `USTAR` | m/s | P6E E5/E8 ELUSTR | yes |
| `ZI` | m | P6E E7/E13/E20 MIXHT (max of convective and mechanical) | yes |
| `EL` | m | P6E E5/E8 ELUSTR | yes |
| `WSTAR` | m/s | P6E E6 WSTARR | yes |
| `TEMPK` | K | P6E E3/E10 SURFVAR 2-D surface temperature | yes |
| `RHO` | kg/m3 | P6E air-density input | yes |
| `QSW` | W/m2 | P6E E4 HEATFX | yes |
| `IRH` | percent (int) | P6E E3/E10 SURFVAR | yes |
| `IPGT` | code (int) | P6E E2 PGTSTB | yes |
| `T-LEV 1..NZ` | K | P6E E14 TEMP3D (layer 1 equals `TEMPK` exactly) | yes |
| `U-LEV`, `V-LEV`, `WFACE` (27 fields) | m/s | **P6D**, not implemented in P6E | yes, in the real file |
| `RMM`, `IPCODE` | - | case input, written only when `NPSTA!=0` | **absent** (NPSTA=0) |

`ZICONV`, `DPTT` and `QH` have **no CALMET.DAT representation** — the original
writes neither of them — so `write_calmet_dat21` rejects them as file labels
instead of inventing a field. `ZI` is the field that carries the mixing height;
`ZICONV` and `DPTT` stay internal diagnostics of E13/E20/E21.

### Byte-exact proof against the frozen real reference case

`check --file met.dat --rewrite out.dat` parses the real 64 MB file, validates
every rule above, and re-serialises it. The rewrite is **byte-identical** and
reproduces the frozen SHA-256:

```
143 intervals x 45 fields x 2500 cells, record 10024 B,
header NX=50 NY=50 NZ=9 DGRID=100 IWAT1=50 IWAT2=55 NSSTA=1 NUSTA=1 NPSTA=0
IRLG=143 comments=1114 LCALGRD=T
rewritten sha256 = b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30
                   (identical to the frozen reference)
```

Source-derived invariants evaluated on that real file (357,500 cells per
field-hour, **zero missing values in all 45 fields**): `T-LEV1 == TEMPK`
exactly (0 mismatches), `WSTAR >= 0`, `ZI >= 0`, `USTAR >= 0.05 m/s` over
land, `IRH` in 25..100, `IPGT` in 2..6.

### Truth-comparison harness

`reference/compare_calmet_dat21.py` is the one-command gate for a real
`met.dat` plus its frozen SHA-256 and a P6E-produced candidate:

```
python3 native_cpp/modules/p6_calmet/p6e/reference/compare_calmet_dat21.py \
  --reference project/cases/hefei5km_2022_reference_case_20261011/run/met/met.dat \
  --reference-sha256 b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30 \
  --candidate <P6E CALMET.DAT 2.1> --out stats.json
```

It reports an explicit `BLOCKED_REFERENCE_MISSING`,
`BLOCKED_REFERENCE_SHA256_MISSING`, `BLOCKED_CANDIDATE_MISSING`,
`FAIL_REFERENCE_SHA256` or `FAIL_LAYOUT` status (exit 3/3/3/2/2) instead of a
silent pass, and emits per-field per-cell statistics: denominator, missing
counts on each side, maximum absolute and relative error, frozen per-field
limits and the first exceedances with hour, cell and both values.

Full-scale runs on the real file:

- reference == candidate: `PASS`, **sum of field denominators 16,087,500**
  (= 45 x 357,500), 0 exceedances;
- one injected `+0.5 m` perturbation in `ZI` hour 1 cell 1: `FAIL_NUMERIC`,
  `total_exceedances = 1`, `max_abs_error = 0.5 m`,
  `max_rel_error = 2.1649589293390122e-3`, exceedance recorded as
  `hour=1, cell=1, reference=230.95126342773438, candidate=231.45126342773438`.

**Not achieved, explicitly:** there is still **no P6E-produced candidate** for
the 143-hour case, so the real truth comparison reports
`BLOCKED_CANDIDATE_MISSING` — it is never reported as a pass. Producing that
candidate needs the full 143-hour P6E chain driven from the case `met.inp`
inputs (P6D wind fields, HEATFX/ELUSTR/WSTARR/MIXHT over the real grid, P6C
SURF/UP observations) and is outside this increment. 27 of the 45 real file
fields belong to P6D and are listed by the harness as
`not_implemented_in_p6e`.

## E23 real 2022 reference case: P6E candidate fields and truth verdicts

The frozen reference case `project/cases/hefei5km_2022_reference_case_20261011`
(exact merged copy, `run/met/met.dat` sha256
`b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30`, 143
intervals, 50x50x9, DGRID=100 m) was driven with its **real** inputs through the
P6E chain as far as the source dependency graph allows.

### Real case inputs actually consumed

- `met/surface/final/surf.dat` (P5 `SURF.DAT 2.1`, station 583210) and
  `met/upperair/final/up_58424.dat` (P4, station 58424) through the existing
  E15 CLI `p6e_p6c_hourly_bridge`, run read-only on the case tree:
  `hours=143 missing_precip_hours=143 observed_zero_precip_hours=0
  reversed_AABB_hours=72` — the real observation/AA-BB contract holds on the
  real payload.
- `run/met/met.lst` resolved options: `IMIXH=1 THRESHL=0 IZICRLX=1 ISURFT=-1
  IUPT=-1 ITPROG=0 NOOBS=0 NOWSTA=0 IWFCOD=1 LCALGRD=T NSSTA=1 NUSTA=1 NPSTA=0
  **IAVET=1 IFRADJ=1 IAVEZI=1 IRAD=1 NUMTS=5 TRADKM=500 ICOARE=10 IRHPROG=0**
  NZ=9 DGRID=100`.
- Grid geometry and time labels are copied from the reference file's *static
  records and hourly clocks* — those are run **inputs** (ZFACE, station
  coordinates, Z0/ILANDU/ELEV/XLAI/NEARS) and labels, not results. The case
  grid is entirely land: `ILANDU = 20` for all 2500 cells, `Z0 = 0.25 m`,
  `ELEV` 25.0-59.5 m, `IWAT1/IWAT2 = 50/55`, `NPSTA = 0`.

### Per-field verdicts against the frozen `met.dat`

`p6e_calmet_dat21 emit-realcase` writes a **partial** candidate: only the fields
the original computes without the diagnostic wind field are produced; every
other field carries the original `9999.` missing sentinel, so the harness
reports it as not produced instead of comparing invented numbers.

| field | roadmap status | denominator | missing ref/cand | max abs | max rel | frozen limit | verdict |
|---|---|---|---|---|---|---|---|
| `TEMPK` | `PASS_REAL_CASE` | 357,500 | 0 / 0 | **1.28174e-3 K** | 4.72702e-6 | 3e-4 K + 1e-5 rel | 0 exceedances |
| `T-LEV 1` | `PASS_REAL_CASE` | 357,500 | 0 / 0 | **1.28174e-3 K** | 4.72702e-6 | 5e-4 K + 1e-5 rel | 0 exceedances |
| `IRH` | `FAIL` (numeric) | 357,500 | 0 / 0 | **1** | 4.0e-2 | exact | **15,796 exceedances (4.42 %)** |
| 42 remaining fields | `BLOCKED` / `P6D_OWNED` | 0 | 0 / 357,500 each | – | – | – | not produced |

`TEMPK` is the surface temperature field from SURFVAR. The residual
1.28e-3 K is the float noise of the original `AVETMP` spatial averaging
(`IAVET=1`, which uses the wind direction) on an otherwise uniform
single-station field, and stays inside the frozen gate.

### IRH discrepancy: diagnosed, not a physics disagreement

The reference `IRH` is *never above* the reported station humidity and is
**exactly one below it** in 15,796 of 357,500 cells (`reference - observed`
distribution: `{0: 341704, -1: 15796, >0: 0}`); for example hour 1 has
`observed = 46` with reference values `{45, 46}`. The original source is
`rhsurf = rhsurf/sumwt` followed by `irh2d(i,j) = int(rhsurf)`
(SURFVAR L12996-13025): with a single station the ratio is algebraically the
station value, but in REAL*4 the product-then-divide rounds one ulp below the
integer for some cell distances, and `int()` truncates to 44/45/51/... The P6E
kernel normalises exactly and therefore returns the observed integer.

So the P6E value equals the observation while the reference carries the
truncation artifact. **The frozen IRH tolerance (`exact`) was deliberately not
relaxed**; the field is reported `FAIL` with the numbers above, and a
bit-exact match would require reproducing the original's REAL*4 distance and
weighting rounding order (an open item, listed below).

### Dependency gap list (why 42 fields cannot be produced yet)

Every remaining field needs the **P6D diagnostic wind field** `U/V` at the
surface/lower levels, either directly or through the surface-temperature
smoothing (`IAVET=1` -> `AVETMP(...,u,v,temp2d)`):

| field | original producer (calmet.for COMP) | blocking input |
|---|---|---|
| `U-LEV/V-LEV/WFACE` (27) | `diagno` (P6D) | P6D wind field itself |
| `IPGT` | `call pgtstb(u,v,...)` L4626 | 10 m wind components |
| `USTAR`, `EL` | `call elustr(z0,u,v,zmid(1),...)` L4662 | wind at the first level |
| `ZI` | `call mixht(el,ustar,qh,...)` L4692 | `ustar`/`el` from ELUSTR |
| `WSTAR` | `call wstarr(ziconv,qh,...)` L4744 | `ziconv` from MIXHT |
| `QSW` | `call heatfx(sinalpc,nears,temp2d,...)` L4583 | `AVETMP`-smoothed `temp2d`, grid solar geometry |
| `RHO` | `call airden(pres,temp2d,rho)` L4641 | 2-D surface pressure + smoothed `temp2d` |
| `T-LEV 2..9` | `call temp3d(temp2d,ziconv,...)` L4766 | `ziconv` from MIXHT |
| `RMM`, `IPCODE` | written only when `NPSTA!=0` | absent for this case (`NPSTA=0`) |

Required external deliverables to close this: the P6D diagnostic `U/V/W` field
for the same 143 hours, and a 2-D surface-pressure field for `RHO`. Neither is
available in the case tree today, so the full 143-hour comparison stays
`BLOCKED`.

### Reproduction

```
python3 native_cpp/modules/p6_calmet/p6e/reference/compare_calmet_dat21.py \
  --reference project/cases/hefei5km_2022_reference_case_20261011/run/met/met.dat \
  --reference-sha256 b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30 \
  --candidate <p6e_calmet_dat21 emit-realcase output> --out comparison.json
```

Evidence artifacts from this run (lhy-desktop, GCC 12.3.0):
candidate sha256 `5fd5ea1c4dffeee0b2d7210c0d925c0844cd727d9241969cc62b8813f808e42f`,
`compare` output sha256 `f1861624bc0e8d32786a9e5e2d72f89ead8ba70c134ff1aedf25e5db3a22bcaa`,
candidate report sha256 `859990feffffe73a6ba429e511408ebeb43dea30234ce5dfc4ba3e50d34ce394`.

**Summary:** `P6E_E22_OUTPUT_CONTRACT=PASS_REFERENCE`;
`P6E_TEMPK_REAL_CASE=PASS_REAL_CASE`; `P6E_TLEV1_REAL_CASE=PASS_REAL_CASE`;
`P6E_IRH_REAL_CASE=FAIL` (rounding artifact, tolerance not relaxed);
`P6E_USTAR_EL_ZI_WSTAR_QSW_RHO_IPGT_TLEV2_9_REAL_CASE=BLOCKED` on the P6D wind
field; `P6E_FULL=NOT_PASS`.

## E24 IRH bit-exact fix and real-case QSW solar geometry

Two follow-ups from E23, both closed with real-case verdicts.

### IRH: root cause found and fixed (now bit-exact)

E23 reported `IRH` failing with `reference - observed = {0: 341704, -1: 15796}`.
The cause was **not** in the interpolation arithmetic but in the station units
fed to it: `met.dat`/`/MET1/` stores `XSSTA/YSSTA` in **metres relative to the
SW corner of grid cell (1,1)** — the same metres as the stored `DGRID = 100.0`
and consistent with the centre-of-domain value `2500.0` — while the E23
emitter scaled them by 1000 as if they were kilometres. That put the single
station 2500 km away, i.e. **beyond `TRAD = 500 km`**, so the kernel took the
original's "no station within the radius of influence, use the nearest one"
shortcut (`SURFVAR` L12998-12999, `rhsurf = itmprh(stlist(1))`) and never ran
the truncating inverse-distance path at all.

With the units corrected, the P6E kernel reproduces the original expression
chain exactly — `r2 = (xxx*dgridi - xc)**2 + (yyy*dgridi - yc)**2`,
`tdist = sqrt(r2)`, `wt = 1.0` when `tdist < 1` else `1.0/tdist`,
`sumwt = sumwt + wt`, `rhsurf = rhsurf + wt*itmprh(...)`,
`rhsurf = rhsurf/sumwt`, `irh2d(i,j) = int(rhsurf)` (SURFVAR L12971-13025) —
including the REAL*4 rounding that leaves the ratio one ulp below the integer
for some cell distances. Result over the real case: **`IRH` max absolute
difference 0 and 0 exceedances over 357,500 cells** (`TEMPK` is unaffected
because a single station weights the temperature identically either way; its
residual below is the `AVETMP` term, which is a different, wind-dependent
step).

### QSW: grid solar geometry implemented, real-case verdict PASS

The short-wave field is the first field closed that does not need the P6D wind
field at all: `HEATFX` L8066-8069 computes
`qsw = (ha1*sinalp + ha2)*(1. + hb1*ccfrac**hb2)` (clipped at 0), so it needs
only the per-cell solar elevation, the cloud fraction and the Holtslag-van
Ulden constants. E24 implements that chain:

- **Projection**: the case grid is UTM zone 50N / WGS-84 (`met.lst`: `PMAP=UTM
  IUTMZN=50 UTMHEM=N DATUM=WGS-84 DGRIDKM=0.1 XORIGKM=510.885
  YORIGKM=3531.269`). Cell centres are `XORIG + (i-0.5)*DGRID` and `YORIG +
  (j-0.5)*DGRID`; an inverse UTM series (WGS-84) yields per-cell latitude and
  east longitude.
- **Solar**: the already source-validated E1 `SOLAR` kernel (26 half-hour
  slots from 23:30 the previous day) with `njul` = the Julian day of the
  hour's beginning and `ibtz` parsed from the file's `AXTZ` (`UTC+0000` -> 0).
- **Mid-timestep selection**: the COMP hourly branch
  `sinalpc(i,j) = sinalp(i,j,nhrb+2)` (L3862-3872), i.e. the half-hour slot at
  the middle of the beginning hour.
- **Cloud**: `MCLOUD=1` selects the station-cloud branch
  (`ccfrac = 0.1*icc(nears)`); the real case has `ICC = 0` for all 143 hours,
  so the fraction is 0. A cloud value outside 0..10 tenths (including the
  missing marker) fails closed rather than inventing cloud.
- **Constants**: the case's own echoed values from `met.lst`
  (`ha1=990, ha2=-30, hb1=-0.75, hb2=3.4`), overridable on the CLI.

**Real-case verdict**: `QSW` denominator 357,500, **max absolute difference
7.24792e-4 W/m²**, max relative 6.36252e-5, **0 exceedances** against the
frozen 0.02 W/m² + 3e-5 relative limits. The residual is the difference between
the local inverse-UTM series and the original COORDLIB transform (a few 1e-7
deg in longitude), far below the gate.

### Updated per-field verdicts (143 intervals x 2500 cells = 357,500 per field)

| field | roadmap status | max abs | max rel | frozen limit | exceedances |
|---|---|---|---|---|---|
| `TEMPK` | `PASS_REAL_CASE` | 1.28174e-3 K | 4.72702e-6 | 3e-4 K + 1e-5 rel | 0 |
| `T-LEV 1` | `PASS_REAL_CASE` | 1.28174e-3 K | 4.72702e-6 | 5e-4 K + 1e-5 rel | 0 |
| `IRH` | `PASS_REAL_CASE` (bit-exact) | **0** | **0** | exact | **0** |
| `QSW` | `PASS_REAL_CASE` | 7.24792e-4 W/m2 | 6.36252e-5 | 0.02 W/m2 + 3e-5 rel | 0 |
| 41 remaining fields | `BLOCKED` / `P6D_OWNED` | - | - | - | not produced |

Overall harness status for the real case: **PASS**, `total_exceedances = 0`
(candidate sha256 `74f625eedc08d48fc177be5f54aa7383b8a7d8ee1fcf04ad4992cfb5e5d1e415`,
comparison sha256 `197a53293ab8cec982bd0047b5a03f40553cd0c47c4e456cadc41782eae2ce86`).

### Remaining gap list (41 fields, each needing the P6D diagnostic wind)

Unchanged from E23 apart from the two closures above: `U-LEV/V-LEV/WFACE`
(27, produced by `diagno`), `IPGT` (`pgtstb(u,v,...)` L4626), `USTAR`/`EL`
(`elustr(z0,u,v,zmid(1),...)` L4662), `ZI` (`mixht(el,ustar,qh,...)` L4692),
`WSTAR` (`wstarr(ziconv,qh,...)` L4744), `RHO` (`airden(pres,temp2d,rho)`
L4641, which also needs a 2-D surface-pressure field), `T-LEV 2..9`
(`temp3d(temp2d,ziconv,...)` L4766). `TEMPK`'s remaining 1.28e-3 K is the
`AVETMP(...,u,v,temp2d)` spatial-averaging term (`IAVET=1`), so even that field
would become bit-exact only once the wind field is available.

**Status summary:** `P6E_IRH_REAL_CASE=PASS_REAL_CASE` (bit-exact, was FAIL);
`P6E_QSW_REAL_CASE=PASS_REAL_CASE` (was BLOCKED); `P6E_TEMPK_REAL_CASE` and
`P6E_TLEV1_REAL_CASE` remain `PASS_REAL_CASE`;
`P6E_REMAINING_41_FIELDS_REAL_CASE=BLOCKED`; `P6E_FULL=NOT_PASS`.

## E25 `AVETMP` source parity, real-case `RHO`, and the cross-track interface

### `AVETMP` (IAVET=1) implemented with original-source parity

`subroutine avetmp` (calmet.for L2356-2589) is the upwind-weighted spatial
averaging of the 2-D surface temperature that SURFVAR calls when `IAVET=1`
(L12878-12881). `include/cteas/p6e/avetmp.hpp` ports it verbatim: the fixed
`DTIME=3600 s`, `DEGRAD=0.01745329`, the Fortran `INT` truncation of the
integer displacement, the `+/-MNMDAV` square shortcut (which still uses the
upwind-displaced weight, only the local cell keeps `wt=1`), the upwind cone
test with `AMOD`-wrapped bearings, the causal-frontier test on the squared
distance, and the weight `1/(dela**2 + (1+|delc|)**2)`.

`reference/compare_original_avetmp.py` compiles the verbatim original together
with a driver and compares **40 cases / 685 values** covering zero wind,
uniform wind, a rotating field and a single hot cell over 6 grid shapes,
`MNMDAV` 1-3, `HAFANG` 30-90 deg and `DGRID` 100/1000/10000 m:
`status=PASS`, **max absolute difference 5.0e-10 K, 0 mismatches**, original
blob `437f43bb7f0e51a1919e56664663e1a046986ed0`, unmodified source lines
2356-2589. The comparator runs as the CTest
`p6e_avetmp_original_source_parity`, registered automatically whenever a
Fortran compiler is found.

Note for the real case: with `NSSTA=1` the pre-`AVETMP` temperature field is
uniform, so the term cannot change the physical value — it only adds the
REAL*4 rounding that produces the ~1e-3 K spread observed in the reference
`TEMPK`. `p6e_avetmp_software` also checks the zero-wind fallback, the 1x1
grid, downstream advection for both wind signs, determinism and fail-closed
validation.

### `RHO` from the real station pressure: PASS_REAL_CASE

`airden` (L2052-2096) is `rho(i,j) = 0.3484321*PRES(nears(i,j))/TEMP2D(i,j)`,
where `PRES` is the **station** pressure in mb from the RDSN observation (the
source comment asks for a 2-D pressure field but the implementation uses the
nearest station). The E15 bridge now emits the `pressure_mb` column
(`P6CObservedHour::surface_pressure_mb`, from `read_surface.hpp`'s
`obs.pressure_mb`), and `emit-realcase` produces `RHO`.

Real-case verdict: denominator 357,500, **max absolute difference
6.31809e-6 kg/m3**, max relative 4.78145e-6, **0 exceedances** against the
frozen 1e-5 + 1e-5 relative limits.

### Cross-track interface answers (for P6D)

All three answers are code in `p6e/**`, not prose:

1. **`IALL`** — `surfvar_one_station` (now `include/cteas/p6e/surfvar_observed.hpp`)
   gained an explicit `iall` field. The original SURFVAR only ever conditions
   temperature at `if(iall.ne.2)temp2d(i,j)=tprog(i,j,1)` (ITPROG=2 path) and
   `if(iavet.eq.1.and.iall.ne.2)call avetmp(...)` (L12878-12881) and then
   `if(iall.eq.0)return` (L12882-12883). **IALL=0 and IALL=1 therefore produce
   identical TEMP2D**; IALL=0 merely returns before RH/IPCODE (the kernel now
   returns empty RH/IPCODE vectors for IALL=0, and the test asserts the TEMP2D
   equality). The real-case path is **IALL=1** (COMP L4567); DIAGNO's IALL=0
   call (L5610) is the terrain pre-step.
2. **`IAVET`** — `surfvar_one_station` now implements `IAVET=1`: when
   `average_temperature` is set it calls the source-parity `avetmp` kernel with
   caller-supplied `u_ms`/`v_ms` (and `dgrid_m`/`mnmdav`/`hafang_deg`), fails
   closed when they are missing, and skips the average for `iall==2` exactly as
   the source does. The real deck's `IAVET=1` is therefore wired.
3. **`HEATFX` `sun` + cloud contract** — new stable header
   `include/cteas/p6e/surface_solar.hpp` exporting
   `surface_solar_grid(nx,ny,utm_zone,xorigin_m,yorigin_m,dgrid_m)` (per-cell
   latitude/east longitude by inverse UTM, WGS-84),
   `sinalpc_grid(grid,julian_day,begin_hour,base_timezone_hours)` (per-cell
   dimensionless signed sine of solar elevation, `(j*nx+i)` layout, the
   original `SINALPC=SINALP(NHRB+2)` mid-hour selection),
   `timezone_hours_from_axtz("UTC+0000") -> 0`, `heatfx_cloud_fraction(tenths)`
   (the `MCLOUD=1` station branch `0.1*ICC(NEARS)`, tenths 0..10, missing or
   out of range fails closed) and `heatfx_shortwave(sinalpc,cloud_fraction,
   ha1,ha2,hb1,hb2)` (HEATFX L8066-8069, defaults 990/-30/-0.75/3.4).
   `emit-realcase` was refactored onto this header, so the interface is the
   same code path that produced the real-case `QSW` PASS.

### Real-case verdicts after E25 (143 x 2500 = 357,500 per field)

| field | roadmap status | max abs | max rel | exceedances |
|---|---|---|---|---|
| `TEMPK` | PASS_REAL_CASE | 1.28174e-3 K | 4.72702e-6 | 0 |
| `T-LEV 1` | PASS_REAL_CASE | 1.28174e-3 K | 4.72702e-6 | 0 |
| `IRH` | PASS_REAL_CASE (bit-exact) | 0 | 0 | 0 |
| `QSW` | PASS_REAL_CASE | 7.24792e-4 W/m2 | 6.36252e-5 | 0 |
| `RHO` | **PASS_REAL_CASE (new)** | 6.31809e-6 kg/m3 | 4.78145e-6 | 0 |
| 40 remaining | BLOCKED / P6D_OWNED | - | - | not produced |

Harness status **PASS**, `total_exceedances = 0`; candidate sha256
`fa72a4fb7701264691c41a5b075b9fb7b39383cb52ecc31b2182198e356eff9d`, comparison
sha256 `5a25b0ccb36ffa68ab7b79cec4520f2378dfd6fb034eae5a4e21ee1db8efb26c`.

### One-command rerun

```
# 1. real observations -> bridge CSV (P5 SURF + P4 UP, exact source rows)
p6e_p6c_hourly_bridge <surf.dat> <up_58424.dat> 58424 hourly.csv
# 2. candidate with every field P6E can currently produce
p6e_calmet_dat21 emit-realcase --reference <met.dat> --observations hourly.csv \
    --out candidate.dat --report candidate.json
# 3. truth comparison against the frozen reference
python3 reference/compare_calmet_dat21.py \
    --reference <met.dat> --reference-sha256 <MET_SHA> \
    --candidate candidate.dat --out comparison.json
```

The 40 remaining fields need the P6D diagnostic wind field, which is the
declared input contract: a CALMET.DAT 2.1 carrying `U-LEV/V-LEV/WFACE` for the
same 143 intervals (same header, same time labels, `9999.` where absent).
Feeding it into step 2 is the remaining orchestration work (ELUSTR ->
`USTAR`/`EL`, PGTSTB -> `IPGT`, HEATFX -> `QH`, MIXHT -> `ZI`, WSTARR ->
`WSTAR`, TEMP3D -> `T-LEV 2..9`); all six kernels already exist and are
source-validated, so this is wiring rather than new physics.

**Status summary:** `P6E_AVETMP_SOURCE_PARITY=PASS_REFERENCE`;
`P6E_RHO_REAL_CASE=PASS_REAL_CASE` (was BLOCKED);
`P6E_TEMPK/TLEV1/IRH/QSW_REAL_CASE=PASS_REAL_CASE`;
`P6E_REMAINING_40_FIELDS_REAL_CASE=BLOCKED`; `P6E_FULL=NOT_PASS`.

## E26 original-wind-driven full-field run: five fields bit-exact

The frozen `met.dat` itself carries the **original** `U-LEV/V-LEV/WFACE`
fields. Feeding them back as the wind input isolates P6E's own physics from
P6D's wind reconstruction: the comparison then measures the P6E kernels, not
the wind regeneration.

### New orchestration

- `p6e_calmet_dat21 emit-realcase --wind-file <CALMET.DAT 2.1>` reads a wind
  source with the **same** grid, time zone, output mode, interval count and
  hourly time labels, and fails closed on any `9999.`/nonfinite wind value.
- With a wind source present the per-hour chain follows the original COMP
  order: `SURFVAR` -> **`AVETMP`** (IAVET=1 with the first-level wind,
  `MNMDAV=1`, `HAFANG=30`, L12878-12881) -> `HEATFX` short wave ->
  `airden` -> **`PGTSTB`** (first-level wind, MCLOUD=1 station cloud/ceiling,
  case land-use grid, L13295-13498).
- The bridge CSV gained `ceiling_hundreds_ft`
  (`P6CObservedHour::ceiling_hundreds_ft` from the P6C surface state), which
  PGTSTB's night branch needs.

### Per-field verdicts, original wind driving (143 x 2500 = 357,500 per field)

| field | denominator | max abs | max rel | exceedances | status |
|---|---|---|---|---|---|
| `TEMPK` | 357,500 | **0** | 0 | 0 | **bit-exact** |
| `T-LEV 1` | 357,500 | **0** | 0 | 0 | **bit-exact** |
| `IRH` | 357,500 | **0** | 0 | 0 | **bit-exact** |
| `RHO` | 357,500 | **0** | 0 | 0 | **bit-exact** |
| `IPGT` | 357,500 | **0** | 0 | 0 | **bit-exact** |
| `QSW` | 357,500 | 7.24792e-4 W/m2 | 6.36252e-5 | 0 | PASS (known UTM residual) |
| `U-LEV/V-LEV/WFACE` (27) | 0 | - | - | - | not produced (P6D-owned; used as *input*) |
| `T-LEV 2..9` | 0 | - | - | - | BLOCKED on `ZICONV` |
| `USTAR`, `EL` | 0 | - | - | - | BLOCKED on `QH` |
| `ZI` | 0 | - | - | - | BLOCKED on `USTAR`/`EL` |
| `WSTAR` | 0 | - | - | - | BLOCKED on `ZICONV` |
| `RMM`, `IPCODE` | - | - | - | - | absent (`NPSTA=0`) |

Harness status **PASS**, `total_exceedances = 0`.

**This closes the AVETMP question quantitatively**: E24/E25 measured `TEMPK`
0.00128 K from the reference; with the real `AVETMP` applied it is **exactly
0**, confirming that the whole residual was the AVETMP REAL*4 rounding. `RHO`
benefited identically (6.32e-6 -> 0) because `airden` divides by the
AVETMP-averaged `TEMP2D`. `IPGT` was produced for the first time and is
**bit-exact** on 357,500 cells.

### Exact remaining gap (39 fields)

`QH` is the missing link: `HEATFX`'s land energy budget needs the per-land-use
albedo, Bowen ratio and soil-heat parameters, which CALMET reads from
`LU.DAT` (`geo/ctgproc/lu.dat`, CTGPROC `LU.DAT 2.1`). With `QH` available the
chain continues through `ELUSTR` (`USTAR`, `EL`; `zm=zmid(1)=10 m`,
`z0`/`landuse` from the statics, `ICC` from the observations), then `MIXHT`
(`ZI`; `CMECH=100*CONSTB=141`, `FCORI` from the cell latitude, `IUPT=-1`,
`NEARU=1`, soundings from `up_58424.dat`), then `WSTARR` (`WSTAR`) and
`TEMP3D` (`T-LEV 2..9` from `ZICONV`). All five kernels already exist and are
source-validated; the outstanding work is the `LU.DAT` land-use property
table reader plus that wiring.

**Status summary:** `P6E_TEMPK/TLEV1/IRH/RHO/IPGT_REAL_CASE=PASS_REAL_CASE`
(bit-exact); `P6E_QSW_REAL_CASE=PASS_REAL_CASE`;
`P6E_USTAR_EL_ZI_WSTAR_TLEV2_9_REAL_CASE=BLOCKED` on the `LU.DAT` tables;
`P6E_U_V_W=P6D_OWNED`; `P6E_FULL=NOT_PASS`.

## E27 GEO.DAT properties, the HEATFX -> ELUSTR chain, and `USTAR` passing

### Anchored GEO.DAT reader

`include/cteas/p6e/geo_landuse.hpp` reads the MAKEGEO `GEO.DAT 2.1` land-use
file following the original `readge`/`fillgeo` sequence (calmet.for
L27790-28380, `fillgeo` L6983-7030): the `IOPT(1)` marker, `NLU/IWAT1/IWAT2`,
`ILUCAT(1..NLU)`, the `ILANDU` grid, the terrain factor, the `ELEV` grid, then
one marker plus payload per parameter (`Z0`, `ALBEDO`, `BOWEN`, `HCG`, `QF`,
`XLAI`) with `IOPT` 0 (built-in table), 1 (category table) or 2 (gridded
field). The case file uses `IOPT=2` for all of them, so the property grids are
read directly.

Fail-closed: unsupported options, truncated payloads, a grid size that does
not match the case, a category count that disagrees with `NLU`, and
out-of-range values (albedo outside 0..1, Bowen outside (-1,100), soil-heat
parameter outside 0..1, negative anthropogenic flux) all raise instead of
falling back to a default. The parsed `ILANDU` is cross-checked against the
reference file's own static record and the water range against the header.

### Chain wiring

With `--geo-file` (and a wind source) `emit-realcase` now runs, per hour, in
the original COMP order: `SURFVAR` -> `AVETMP` -> **`HEATFX`** (L7999-8122 with
the case's gridded albedo/Bowen/soil-heat/anthropogenic fields and
`hc1=5.31e-13, hc2=60, hc3p1=1.12` from the case's own `met.lst`) ->
`airden` -> `PGTSTB` -> **`ELUSTR`** (L6333-6579; per-cell branch selection:
`QH<0` uses the stable kernel, `QH>=0` the daytime kernel with `IPSIFCN=0`
because `ICOARE=10`, `zm=zmid(1)=10 m`, `z0`/`landuse` from the statics,
`ICC` from the observations; cells of the other branch are marked as water so
each kernel skips exactly the ones the original would).

### Results, original wind driving (143 x 2500 = 357,500 per field)

| field | denominator | max abs | max rel | exceedances | status |
|---|---|---|---|---|---|
| `TEMPK` | 357,500 | 0 | 0 | 0 | bit-exact |
| `T-LEV 1` | 357,500 | 0 | 0 | 0 | bit-exact |
| `IRH` | 357,500 | 0 | 0 | 0 | bit-exact |
| `RHO` | 357,500 | 0 | 0 | 0 | bit-exact |
| `IPGT` | 357,500 | 0 | 0 | 0 | bit-exact |
| **`USTAR`** | 357,500 | **5.51343e-7 m/s** | 5.07242e-6 | **0** | **PASS (new)** |
| `QSW` | 357,500 | 7.24792e-4 W/m2 | 6.36252e-5 | 0 | PASS, known UTM residual |
| **`EL`** | 357,500 | - | up to 3.2e-2 | **630 (0.18 %)** | **FAIL: confined to hours 8 and 32** |

`USTAR` closing at 5.5e-7 m/s with zero exceedances validates the whole
`GEO -> HEATFX -> ELUSTR` chain. The `EL` residual is **not** a branch error:
it is confined to exactly two hours (8 and 32: 350 + 280 cells) with relative
errors 5.4e-5 .. 3.2e-2; the other 141 hours pass. The mechanism is the known
solar-geometry residual: `QSW` differs by 7.24792e-4 W/m2 because the local
inverse-UTM series does not reproduce COORDLIB exactly, that difference enters
`HEATFX`'s net radiation, hence `QH`, hence
`EL = -253.8226*RHO*TSF*USTAR**3/QH`, and in those two hours the amplification
is large enough to break the frozen 0.04 m + 5e-5 relative gate. Aligning the
projection (a COORDLIB-equivalent transform) is the fix; it is recorded as a
bounded residual, not as a pass.

### Still open

`ZI`, `WSTAR` and `T-LEV 2..9` remain unproduced: `MIXHT` and `TEMP3D` need the
full 10-level AA/BB sounding profiles per hour, while the bridge CSV currently
carries only the level-1 and face-2 temperatures. Extending the bridge with
the complete profiles plus `ZICONV` is the next step, after which `MIXHT`
(`CMECH=CONSTB/sqrt(FCORI)`, `FCORI=max(|2*(2*pi/86400)*sin(lat)|,0.25e-4)`
computed per cell, `IUPT=-1`, `NEARU=1`, `THRESHL=0`, `IZICRLX=1`), `WSTARR`
and `TEMP3D` plug in directly. The 27 `U-LEV/V-LEV/WFACE` fields stay P6D-owned.

**Status summary:** five fields bit-exact; `USTAR` PASS; `QSW` PASS with a
bounded residual; `EL` FAIL on 630 of 357,500 cells (two hours) traced to the
solar-geometry residual; `ZI`/`WSTAR`/`T-LEV 2..9` BLOCKED on the sounding
profiles; `U-LEV/V-LEV/WFACE` P6D-owned; `P6E_FULL=NOT_PASS`.

## E28 sounding-profile transport, MIXHT/AVEMIX/WSTARR wiring: produced but not yet accepted

### What landed

- The E15 bridge now transports the **full** upper-air profiles: the real
  `up_58424.dat` soundings have **77 levels** (not 10), so the earlier
  fixed-width column scheme was misaligned. The bridge emits two packed
  columns (`aa_profile`, `bb_profile`) as `height:temperature;...` plus two
  face-temperature columns (`aa_face_t`, `bb_face_t`), so the column set no
  longer depends on the level count. Verified on the real file: `0 m:278.6 K,
  96 m:279.9 K, ...` and 20 face temperatures.
- `emit-realcase` now runs `MIXHT` (IMIXH=1, `ZIMIN=50`, `ZIMAX=3000`,
  `THRESHL=0`, `IZICRLX=1`, `TZICRLX=800`, `DPTMIN=0.001`, `DZZI=200`,
  `ONEDTE=4140`, `TWODTE=1080`, `IUPT=-1`, `NEARU=1`; `FCORI` per cell from the
  cell latitude and `CMECH=CONSTB/sqrt(FCORI)`), then `AVEMIX` (COMP L4715,
  `IAVEZI=1`, the same upwind weighting applied to `ZI` and `ZICONV` with the
  `ILEVZI=1` wind), then `WSTARR`.

### Results, original wind driving (143 x 2500 = 357,500 per field)

| field | max abs | max rel | exceedances | verdict |
|---|---|---|---|---|
| `TEMPK`, `T-LEV 1`, `IRH`, `RHO`, `IPGT` | 0 | 0 | 0 | bit-exact (unchanged) |
| `USTAR` | 5.51343e-7 m/s | 5.07242e-6 | 0 | PASS (unchanged) |
| `QSW` | 7.24792e-4 W/m2 | 6.36252e-5 | 0 | PASS (unchanged) |
| `EL` | - | 3.2e-2 | 630 (2 hours) | FAIL (unchanged) |
| **`ZI`** | **113.944 m** | **0.142657** | **221,549 (62 %)** | **FAIL - NOT ACCEPTED** |
| **`WSTAR`** | **0.0262769 m/s** | **0.019062** | **4,880 (1.4 %)** | **FAIL - NOT ACCEPTED** |

`AVEMIX` measurably improved both fields (`ZI` exceedances 248,197 -> 221,549,
`WSTAR` 57,210 -> 4,880) and taking the ELUSTR-updated `QH` (the original
overwrites the `/HFLUX/` array in place before MIXHT) changed nothing, which
means the remaining `ZI` residual is systematic rather than a missing
averaging step.

**Honest status:** round 9 did **not** reach its goal of 18 accepted
non-wind fields. `ZI` and `WSTAR` are produced and compared but **fail** the
frozen gates (0.07 m / 8e-5 relative for `ZI`, 2e-5 m/s / 3e-5 for `WSTAR`)
on a large fraction of cells, so they must not be reported as passing. The
remaining audit targets, in order of likelihood: (a) the AA/BB *physical
slot* mapping between the E15 bridge (`physical_aa/physical_bb`) and MIXHT's
`JUSTA` older/newer selection, (b) the `DPTT`/`ZICONV` hour-to-hour memory
initialisation, (c) the `USTAR`/`EL`/`RHO` provenance inside the MIXHT grid
path. `T-LEV 2..9` remain unproduced: the E14 `TEMP3D` kernel models only the
same-day 00Z/12Z bracket (`current_hour_utc` in 0..12), while 11 of the 143
real hours need the 12Z -> next-day-00Z bracket, so it needs the original's
`DELTSEC`-based `JTOGO/JPAST` generalisation first.

**Status summary (unchanged where not listed):** five fields bit-exact;
`USTAR` PASS; `QSW` PASS with a bounded residual; `EL` FAIL on 630 cells;
`ZI`/`WSTAR` produced but **FAIL** (not accepted); `T-LEV 2..9` BLOCKED on the
TEMP3D time-bracket generalisation; `U-LEV/V-LEV/WFACE` P6D-owned;
`P6E_FULL=NOT_PASS`.

## E29 exported chain state for P6D's `FRADJ` (ZICONV / ZI, 143 hours)

P6D's `FRADJ` (COMP L5643) consumes `GAMMA` from `CGAMMA` (L3031), which reads
the `ZICONV` state that `MIXHT` writes. P6D correctly refused to substitute the
reference file's `ZI` field for it, so the P6E chain now exports the state it
already computes.

**Entry point** (the same command that produces the candidate):

```
p6e_calmet_dat21 emit-realcase --reference <met.dat> \
    --observations <hourly.csv> --wind-file <CALMET.DAT 2.1> \
    --geo-file <GEO.DAT> --out <candidate.dat> \
    --state-prefix /path/prefix
```

writes

| file | contents |
|---|---|
| `<prefix>.ziconv.f32` | 143 x 2500 `float32` little-endian, hour-major, i fastest |
| `<prefix>.zi.f32` | same layout for `ZI` |
| `<prefix>.state.json` | metadata: shape `[143,50,50]`, units m, dtype, layout, water sentinel `-7777`, missing sentinel `9999`, per-hour time labels (packed `YYYYJJJHH` + seconds + year/julian/hour), wind and GEO provenance, and the difference from the `met.dat` `ZI` field |

Measured on the real case (original wind driving): `ziconv` 0.0000 .. 1162.2113 m,
`zi` 49.9999 .. 1162.2113 m, 357,500 values per file, 143 hour labels from
`202200100` to `202200622`.

**Provenance, stated in the metadata and here**: these arrays are
**chain-internal `MIXHT` state advanced hour by hour**, not fields copied from
the reference file. They are *not* the same quantity as the `met.dat` `ZI`
field: `ZI` is the maximum of the convective and mechanical heights *after*
the `AVEMIX` averaging (`IAVEZI=1`), while `ZICONV` is the internal convective
height that `CGAMMA` consumes. Artifacts: `ziconv` sha256
`8f03d2e86ce30453cf96e6fdbba1d303ad4a2e5a748576f9d014cf5bf56b65b1`, `zi`
sha256 `6d354a69a3dfb0da034450dd41b189d5e67e143292d8090611193d84bb3f275d`,
metadata sha256 `9de625adc5f826d1a1697e9cdfc4e5ad6be51e143a0646eb8f60024efd45afa8`.

**Accuracy caveat carried in the metadata**: the `ZI` field produced by this
chain still disagrees with the frozen reference on 62 % of cells (the E28 audit
is open), so P6D should treat the export as the correct *interface* with a
pending accuracy question rather than a validated field. `ZICONV` has no
reference column in `CALMET.DAT` and therefore no independent verdict yet.

## E30 `FCORIOL` root cause: the ZI residual falls from 62 % to 1.26 %

### The audit result, with numbers

The three candidates from E28 were tested:

**(c) MIXHT grid-path input provenance - CONFIRMED and fixed.** The case deck
sets `FCORIOL = 1.0E-04` (`run/met/met.lst` L725). The original rule
(`readcf` L9606-9616) is
`if (fcoriol.gt.998.) f = 2*(2*pi/86400)*sin(lat) else f = fcoriol`, then
`FCORI = max(|f|, 0.25e-4)` and `CMECH = CONSTB/sqrt(FCORI)`. The E28 code
always derived `f` from the cell latitude, giving `7.68e-5` instead of the
deck's `1.0e-4`, which made `CMECH = 160.9` instead of `141` (**14 % high**).

Evidence, per cell and per branch:

- night, hour 70: reference `USTAR = 0.10845`, `EL = 12.745`, so
  `ZI2 = 0.4*sqrt(USTAR*EL/FCORI)` is `47.0` with `FCORI = 1e-4` (clip to
  `ZIMIN = 50` -> reference `ZI = 50.0` exactly) but `53.7` with the
  latitude-derived `7.68e-5` - and the pre-fix code produced exactly `53.6`;
- daytime, hour 0: reference `ZI = 230.9`, pre-fix `263.3`; the ratio
  `263.3/230.9 = 1.140` equals the `CMECH` ratio `160.9/141 = 1.141`, i.e. the
  mechanical height carried the whole error.

After honouring the deck constant (`--fcoriol`, default `1.0e-4`; values above
998 still select the latitude-derived field per the source):
**`ZI` exceedances fall from 221,549 (62 %) to 4,488 (1.26 %)**, max absolute
difference from 113.944 m to 51.0623 m.

**(a) AA/BB physical-slot mapping - not indicated.** With the dominant error
removed, the daytime `ZI` means agree with the reference to 0.006-0.2 m in most
hours; a swapped older/newer sounding would change the lapse rate by tens of
percent and would not leave 98.7 % of cells inside a 0.07 m / 8e-5 gate.

**(b) `DPTT`/`ZICONV` memory initialisation - partly explained away.** The
large first-hour deviations (hour 0 +32 m, hour 1 +56 m) are reproduced by the
same `CMECH` factor of 1.14, so they were the `FCORIOL` error, not a memory
initialisation error. No separate evidence of a memory fault was found; the
initial `ZICONV = 0 / DPTT = 0` remains an assumption of the same kind the
original SETUP makes.

### Current verdicts, original wind driving (143 x 2500 = 357,500 per field)

| field | max abs | max rel | exceedances | verdict |
|---|---|---|---|---|
| `TEMPK`, `T-LEV 1`, `IRH`, `RHO`, `IPGT` | 0 | 0 | 0 | bit-exact |
| `USTAR` | 5.51343e-7 m/s | 5.07242e-6 | 0 | PASS |
| `QSW` | 7.24792e-4 W/m2 | 6.36252e-5 | 0 | PASS (bounded UTM residual) |
| `EL` | - | 3.2e-2 | 630 (2 hours) | FAIL (bounded) |
| `ZI` | 51.0623 m | 5.61028e-2 | 4,488 (1.26 %) | FAIL - close, not accepted |
| `WSTAR` | 0.0262769 m/s | 1.9062e-2 | 4,880 (1.4 %) | FAIL - not accepted |

`WSTAR` is unchanged by the `FCORIOL` fix (identical maxima and counts), which
is itself informative: `WSTAR` consumes the **convective** height, and in
daytime that comes from `MIXHMC`, which does not use `CMECH`/`FCORI`. Both
remaining residuals therefore point at the convective-growth branch
(`MIXHMC` inputs: the `MIXDT` lapse rate from the sounding bracket, or the
`DPTT` memory), not at the mechanical path.

**Exported-state caveat updated:** the E29 `ZICONV`/`ZI` export is still
interface-ready, but its `ZI` accuracy statement improves from "62 % of cells
outside the gate" to **1.26 %**, which is also what the JSON metadata will
carry when it is regenerated.

**Status summary:** six fields bit-exact or PASS; `EL` FAIL on 630 cells;
`ZI` FAIL on 1.26 %; `WSTAR` FAIL on 1.4 %; `T-LEV 2..9` BLOCKED on the
TEMP3D time-bracket generalisation; `U-LEV/V-LEV/WFACE` P6D-owned;
`P6E_FULL=NOT_PASS`.
