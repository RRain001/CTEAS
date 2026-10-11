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
