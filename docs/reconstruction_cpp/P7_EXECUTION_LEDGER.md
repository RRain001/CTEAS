# P7 CALPUFF 7.2.1 — isolated reconstruction ledger

Date: 2026-10-11. Branch: codex/cteas-p7-calpuff-20261011.
Base: P1–P5 integrated foundation 9b17dc65dbe736cdfa205619003cce085cf05d0b.
P6 branches and original engine/vendor/src are NOT modified.

## Dedicated CI

P7-only workflow: .github/workflows/p7-calpuff-scoped-ci.yml.
Runner routing is [self-hosted, lhy-desktop-3], with RUNNER_NAME == lhy-desktop-3
fail-closed enforcement. A runner name alone is NOT an automatically installed
GitHub Actions custom label; label presence, online assignment and HTTPS checkout
must be verified by an actual job. Jobs for P6 are not changed.

## Work packages (gated)

- P7A: Source blob SHA1/sha256 audit, routine and active-control inventory:
  IMPLEMENTED_FOR_CI, no runner-success claim. Original unchanged Fortran build,
  real Hefei 2022 CALPUFF run and frozen CONC.DAT are PENDING.
- P7B: First scoped native C++20 control-file reader and strict sequential
  unformatted Fortran record framing reader (4/8-byte markers, endian choice,
  trailer/size/truncation checks and synthetic fixtures): IMPLEMENTED_FOR_CI,
  NOT CLAIMED PASS until CTest. Active !KEY=value! vs inactive *KEY=value*
  syntax, repeated lists (ZFACE, CSPEC), exact numeric checks and fail-closed
  selected dry reference option validation. Original CALMET.DAT binary reader,
  date/CRS compatibility and full source parser are PENDING.
- P7C: Puff release / 3D advection: NOT STARTED.
- P7D: Sigma evolution / Briggs plume rise / terrain: NOT STARTED.
- P7E: Receptor concentration / CONC.DAT writer: NOT STARTED.
- P7F: Dry+wet deposition, decay and inventories: NOT STARTED.
- P7G: Native P6 to P7 end-to-end: BLOCKED ON P6 FINAL MET OUTPUT.

## Baselines and scientific constraints

Original source authority: engine/vendor/src/CALPUFF_v7.2.1_L150618.
Initial bundled CALPUFF.INP is an ORIGINAL SAMPLE, not the 2022 Hefei
50x50x9 at 100 m configuration. Historical project/cases/smoke5d_coldstart_20260701
is a different 2023 case and cannot be relabeled as 2022 reference.

The desired 2022 Hefei reference has only 143 confirmed CALMET intervals,
not blindly 144 output records. P6 currently has IPCODE=9999 at 143/143 hourly
records and NPSTA=0. P7 wet scavenging cannot obtain a scientific pass from
these dry-only observations.

Original source audit is only identity/inventory. P7B current parser is only
an input contract, NOT a binary CALMET.DAT parser nor CALPUFF physical model.
Fortran original parity, mass conservation and real-case full-model numerical
comparisons are required before any P7 PASS_REFERENCE/PASS_REAL_CASE status.

## Next gates

1. Verify the runner custom label, online assignment, network transport and CTest.
2. Freeze actual 2022 CALPUFF.INP and per-hour source conversion from annual Bq.
3. Execute original CALPUFF 7.2.1 on frozen original 2022 MET.DAT in job-private
   scratch, record original compiler, hashes, log, 1/6/all-143-hour CONC.DAT.
4. Implement original MET.DAT record-level read/parity before adding puff physics.

## 2026-10-11 P7B second increment — MET.DAT v2.1 field reader

An isolated C++20 Calmet21Reader now handles the original selected
unformatted CALMET.DAT 2.1 OUTHD/OUTHR, WRTR2D and WRTI2D fields.
It parses run metadata (time zone, 50x50x9 geometry, spacing, station counts),
the 4-byte Fortran field prefix (8 ASCII + 4 ints), all 4*Nx*Ny float/int
grids per field, expected 4*Nz+9 label ordering, exact 3600-second intervals,
cross-field time consistency, contiguous intervals, and an optional frozen
Hefei 2022 gate demanding 143 intervals. Integer fields IPGT and IRH are
explicitly separated from float fields. The callback is streaming and bounded
to a single field in memory. Synthetic corruption/time/NaN fail-closed tests
were added; P7 CI still must run before a PASS_SOFTWARE claim.

This is a decoded field I/O contract, not a scientific demonstration of
CALMET/CALPUFF numerical equivalence. Full raw real-2022 comparison and
original CALPUFF reference runs remain PENDING. Original 2022 precipitation
is missing; wet deposition cannot be validated.

## P7A/P7B independent reference tool preparation

- reference/build_original_calpuff.py: compile original UNMODIFIED
  modules.for + calpuff.for with gfortran in a job-private directory and
  persist source/compiler SHA provenance. This is NOT a successful build
  until actually run, and does not execute the 2022 original reference.
- reference/original_decode_calmet_hourly.py: copied P6 original-readout
  oracle unchanged from repository branch codex/cteas-p6-calmet-hefei-100m;
  copy sourced from the P6 implementation as a separate reference harness.
- reference/compare_frozen_met21.py: opt-in compare selected 2022 original
  MET.DAT structure and timestamps against new C++ CALMET21 reader. Does NOT
  certify wind physics or CALPUFF original full-case concentration parity.
- No baseline data is fabricated or read from the historical 2023 case.

## 2026-10-11 P7B Runner-3 build failure corrective patch

Observed: Runner `lhy-desktop-3` was assigned (P7 CI 38121776970,
38121935433, 38122015862); first two failed while default compiler
GNU 9.5.0 lacked std::bit_cast and std::chrono calendar. Latest CI
38122015862 did not reach C++ due to github.com:443 transient timeout.
P7B replaces bit_cast with standards-conforming memcpy and chrono calendar
with integer Gregorian-to-epoch conversion; CI selects newer installed
g++-12/g++-11 when available and retries HTTPS preflight. These corrections
are **not accepted** until a new job builds and all CTests run successfully.

## P7A GNU Fortran toolchain diagnostic

Runner-3 job 38122867540 proved P7B 4/4 CTest PASS with GCC 12.3.
The legacy default `/usr/bin/gfortran` does not support
`-fallow-argument-mismatch` (first actual compiler diagnostic).
Original reference CI now prefers installed `gfortran-12`, then
`gfortran-11`, before falling back to the default binary, logging exact
compiler version; reference build status stays UNVERIFIED until execution.

## P7A explicit compiler compatibility check

The P7A job now prints all `gfortran-12`, `gfortran-11` and default
`gfortran` executable paths/versions and uses a minimum GNU Fortran 10
gate when only the default binary is available. An incompatible old
compiler fails with a toolchain-specific message before trying CALPUFF.
Previous successful existing Linux CTEAS builds do not establish the
compiler selected inside this independent P7 runner job. The most recent
P7 CI 38122937191 attempt 2 failed on github.com HTTPS before compilation;
therefore the new compiler selection has not yet been proven by CI.

## 2026-10-11 GNU Fortran 11 fixed-column source diagnosis

Actual Runner-3 job [38123324977](https://github.com/RRain001/CTEAS/actions/runs/38123324977)
selected `/usr/bin/gfortran-11` (GNU Fortran 11.4.0), so the old
gfortran 4.7.4 problem has been resolved. Original `calpuff.for` contains
legacy fixed-format sequence identifiers in columns 73–80; the accidental
`-ffixed-line-length-none` option made those identifiers parse as Fortran
tokens, causing cascaded unexpected declarations/duplicate labels/junk
after function declaration. Build flag corrected to explicit
`-ffixed-line-length-72`. This is compilation behavior correction,
**NOT a change to the original Fortran source** and not a claim of
successful numerical/reference validation.

## P7A original 2022 reference execution gate scaffold

`reference/run_original_calpuff_reference.py` stages independently supplied
2022 CALPUFF controls + untouched original CALMET.DAT inside a case-private
working tree. It refuses invalid wet mode, mismatched study geometry/date,
paths escaping scratch, overwrites, and unpinned source hashes before any
original numerical run. Original Fortran `--execute` must be explicitly
requested with *two* frozen SHA256 inputs and the independently compiled
unchanged binary. It saves original binary/control/met SHA256, runtime log,
listing and concentration file identities; original execution success is
NOT C++ CALPUFF physics parity. Even if reference run is successful,
P7A source/physics fidelity is limited to the supplied, audited control.
Synthetic staging/unit test is not an original model run.

Pending actual source files from 2022; the old 2023 case is excluded.

## P7A frozen original real-case CI routing

P7-only CI has an optional real ORIGINAL CALPUFF execution gate;
it requires repository Actions variables with explicit `MET.DAT` and
CALPUFF control paths, plus both SHA256 hashes:
`P7_FROZEN_2022_MET_DAT`, `P7_FROZEN_2022_PUFF_INP`,
`P7_FROZEN_2022_MET_SHA256`, `P7_FROZEN_2022_PUFF_SHA256`.
All absent => `BLOCKED_NO_FROZEN_2022_CASE`; partial => error, never a
success. When present, the runner hashes the supplied source files,
runs the untouched original Fortran binary with a bounded timeout,
checks listing and concentration output, and writes provenance.
Successful real original run still does not certify new C++ physics.

## P7 dedicated Runner-3 HTTPS source retrieval hardening

Due to repeated `github.com:443` timeouts after verified Runner assignment,
the P7 workflow now probes both Git and authenticated private GitHub API.
When Git transport is unavailable or checkout fails but REST is available,
it bootstraps only the P7 submodule and unmodified original CALPUFF vendor
source files from the **exact PR merge commit Git tree** using the Actions
read-only token. Every blob is SHA1-verified with canonical Git object framing,
stale module/vendor trees are removed, and required files are checked before
any source audit/Fortran execution. Unavailable Git and REST transports
remain hard failures with no invented C++/Fortran pass. P6 and other runners
are not modified. CI verdict pending.

## P7 independently frozen 2023 CALMET/CALPUFF historical reference

The ORIGINAL case `project/cases/smoke5d_coldstart_20260701` is verified
in Git to contain immutable blobs:
- `run/met/met.dat`: 53,927,996 bytes, Git blob `cd0af6fed77639a5e7205bc1fc4f9e4421b41926`.
- `run/puff/puff.inp`: 83,305 bytes, Git blob `67dcdb856c0af92d30de346d4877af3b7d80db6f`.
- `run/puff/puff.con`: 1,653,584 bytes, Git blob `2a0c1a0dabf50e1e61badaa526780cc623150dc3`.

This is **Jan 1–5, 2023, 50×50 at 1 km**, not Jan 1–6, 2022 at
100 m. It can validate a real CALMET.DAT reader, run unchanged original
CALPUFF and yield genuine reference concentration evidence **without P6**.
The case's original control has `MWET=0, NDECAY=0`: it is not a
wet-deposition or tritium radioactive-decay parity benchmark.

The separate read-only `legacy_2023_case.py` script enforces original
Git blob SHA1 identities and freezes original source/control details.
It runs the compiled untouched Fortran original in an isolated scratch
directory. The historical `puff.con` comparison reports exact bytes
if present; record-envelope differences remain explicitly unverified
scientifically, and only a future C++ concentration output can establish
P7E numerical parity. The native `Calmet21Reader` must decode real
2023 hourly fields to pass. No historical data can satisfy the P7 2022 gate.

## 2026-10-11 first historical real-case diagnostic and exact fix

[CI #38125878790](https://github.com/RRain001/CTEAS/actions/runs/38125878790)
reliably downloaded all three historical immutable Git blobs (53.9 MB
MET.DAT, 83.3 KB original puff.inp, 1.65 MB historical puff.con), ran
all **6/6 scoped software CTest PASS**, compiled original Fortran
**PASS**, and executed untouched original CALPUFF with real historical
output present. The C++ MET21 reader then failed on header label
`Z0` before the hourly `U-LEV` sequence. Source-backed reason:
CALMET 2.1 static OUTHD header grid records may have the same length
as hourly 2D wind fields but have zero begin/end times. The reader
now gates on these static timestamps as well as label and size; the
synthetic fixture adds a same-size `Z0` record. Do not accept P7B real
historical decoding until this fix is validated by the next run.

Preserve the new historical original-reference JSON, log and
`puff.con` as a short-lived GitHub Actions artifact on private PR.

## 2026-10-11 P7C first source and transport implementation

The P7C C++20 prototype implements:
- Strict single constant original `NPT1=1` source `X` parsing. UTM
  coordinates are converted km -> m; stack altitude is geometric base +
  stack height, **WITHOUT** Briggs plume rise.
- Independent emitted species mass array (as provided in original
  source units, no arbitrary tritium Bq/g conversion), deterministic
  sequential puff IDs and one emission per configured time step.
- Original CALMET i-fastest [z][j][i] 3D wind storage; cell-centred
  horizontal bilinear and flat-terrain layer-centre vertical interpolation.
- Explicit midpoint RK2 advection; out-of-domain puffs are classified
  as outflow, not silently clamped or lost.
- Per-species equality: release = active inventory + domain outflow.
  Negative/NaN rates, malformed geometry and fields fail closed.

These are **P7C software contracts only**. Original CALPUFF release
cadence and source term units/physical momentum and wind/terrain
transformations still require original Fortran numerical comparison.
The 2023 original CONC.DAT does not establish P7C puff trajectory
parity, since P7D dispersion and P7E concentration have not been
reconstructed. CI result pending.

## P7C second increment — actual original CALMET hourly replay

Added `met_replay.cpp` and `cteas_p7c_met_replay`: reads the full
verified original 2023 CALMET.DAT 2.1 twice (metadata integrity, then
hourly 3-D wind stream), stores only one meteorological hour at a time,
and moves deterministic prototype packets with bilinear/vertical wind
interpolation and midpoint RK2. Each source `NSECDT` substep releases
all configured constant-source species in input units. A per-species
mass closure gate verifies emitted = active inventory + outflow.

The 2023 original case requires **119 consecutive one-hour intervals**
(2023-01-01 00 to 2023-01-05 23) with 1-km spacing. Original
CALPUFF uses its own puff split/merge/cadence, plume rise, sigma,
terrain and receptor averaging; this initial P7C replay is explicitly
**NOT a claim of source-faithful puff position or CONC.DAT parity**.
Run the real integration gate via scoped P7 CI; verdict pending.

## 2026-10-11 original CALMET 2023 endpoint representation

Actual P7 CI [38127081928](https://github.com/RRain001/CTEAS/actions/runs/38127081928)
established `CALMET.DAT 2.1`: NX=50, NY=50, NZ=9, DX=1000m,
119 hourly field groups, `FIRST_UNIX_SEC=1672531200`,
`END_UNIX_SEC=1672959600`, i.e. Jan 1 00:00 through Jan 5
23:00 UTC. The encoded original **final** Fortran field time is
`2023 / Julian day 5 / hour 22 / seconds 3600`, which is exactly
the same instant as 23:00. Our earlier P7C condition erroneously
required raw hour==23; P7B correctly normalizes seconds with 3600.
P7C now gates on normalized absolute Unix seconds, not raw Fortran
packed-hour bytes. This fixes the actual observed real source mismatch
without weakening the historic original input time interval.

## 2026-10-11 P7 2022 Hefei 50x50/100 m case execution ledger

`cteas_p7_2022_case_ledger` (library `cteas_p7_case_ledger`) is the
machine-checked execution ledger of the frozen 2022 reference case. It
takes `--met MET.DAT --control CALPUFF.INP` plus optional
`--concentration`, `--listing`, `--report` and
`--expect-{met,control}-sha256` pins, and prints deterministic JSON
(`gate=P7_HFEI_2022_CASE_EXECUTION_LEDGER`) to stdout; the human summary
line goes to stderr.

Statuses and exit codes are fail closed:
- `ACCEPTED_CANONICAL_2022_PENDING_ORIGINAL_EXECUTION` / exit 0: every
  frozen-spec and cross-file assertion passed and no original
  concentration file is present, so the original 2022 run is still pending.
- `ACCEPTED_CANONICAL_2022_ORIGINAL_OUTPUT_PRESENT` / exit 0: same, with an
  original concentration artifact pinned for provenance only.
- `BLOCKED_MISSING_INPUTS` / exit 3: mandatory MET.DAT/control (or an
  explicitly requested artifact) is absent. No ledger rows are invented and
  nothing is asserted about the case.
- `REJECTED_NOT_CANONICAL_2022` / exit 4: any refusal row failed. It covers
  absent sign/contract, non-50x50x9 grid, spacing other than 100 m, non
  `UTC+0000` zone, zero surface/upper or nonzero precipitation stations,
  interval count other than 143, endpoints other than 2022-01-01 00:00Z and
  2022-01-06 23:00Z, MET origin outside the +-3.5 km 2022 crop window,
  control date/grid/`ABTZ`/single-point-source departure, missing grid
  origin, control grid outside the MET domain, MET control-time mismatch,
  and any caller SHA-256 pin mismatch.

Refusal rows record `expected` and `observed`, so a foreign or historical
case cannot be relabelled: the immutable 2023 case
`project/cases/smoke5d_coldstart_20260701` is refused with
`DX_M=1000.000`, `INTERVALS=119`, `FIRST_UNIX_SEC=1672531200`,
`END_UNIX_SEC=1672959600` and a MET origin 38.4 km/18.3 km away, while its
control still resolves `METDAT` to the identical immutable blob. The
2022 case itself reports MET.DAT `origin=510884.644/3531268.927` as the
selected domain lower face and passes every row.

CTest:
- `cteas_p7_2022_case_ledger_contract`: synthetic CALMET.DAT 2.1 fixtures
  written by the test (canonical 143-interval 100 m fixture accepted, plus
  in-place header/field mutations, a truncated file, a 1 km 119-interval
  2023-like fixture, control mutations, pin mismatch, blocked inputs) and
  SHA-256 FIPS 180-4 vectors. Exit 0 requires the accept rows to pass and
  every refusal row to fail exactly where expected.
- `cteas_p7_2022_ledger_real_2023_refusal`: real-data gate over the
  immutable 2023 case; skips with an explicit line when the case is absent
  (sparse CI checkout) and is executed with `--require` by
  `.github/workflows/p7-calpuff-scoped-ci.yml`.

The Gregorian date helper is now shared by the CALMET.DAT reader and the
ledger (`include/cteas/puff/gregorian.hpp`) instead of being duplicated.

This ledger is **input identity and provenance only**: it makes no CALMET
or CALPUFF physics claim, no dispersion or concentration parity claim, and
no tritium-release or wet-deposition claim (`MWET=0` and zero precipitation
stations are part of the selected scope). `P7_FROZEN_2022_MET_DAT`,
`P7_FROZEN_2022_PUFF_INP` and both SHA256 repository variables remain
unset, so the 2022 50x50/100 m case is still
`BLOCKED_NO_FROZEN_2022_CASE`, and P7D/P7E/P7F/P7G stay unchanged.

## 2026-10-11 P7D MDISP=3 dispersion coefficients (PG rural + MP urban)

The selected real deck `project/cases/smoke5d_coldstart_20260701/run/puff/puff.inp`
carries `MDISP=3`, `MDISP2=3`, `MTURBVW=3`, `MTAULY=0`, `MHFTSZ=0`, `MROUGH=0`,
`AVET=60.`, `PGTIME=60.`, `SYTDEP=5.5E2` and `MDRY=1`. For MDISP=3 the vendor
documentation states: "3 = PG curves (rural), MP (urban)" (`csigma.puf`), so
`MDISP2`, `MTURBVW` and `MTAULY` are recorded but **not effective** for this
case (they enter IDOPT 1, 2 and 5 only). `MDISP=3` is therefore the real-case
dispersion path and the one now reconstructed; wet scavenging (`MWET=0`),
chemistry (`MCHEM=0`), slug mode (`MSLUG=0`) and gravity settling/tilt
(`MTILT=0`) remain out of this path, and dry deposition (`MDRY=1`) is the next
physical path, not yet implemented.

New native module (`include/cteas/puff/dispersion.hpp`, `src/dispersion.cpp`,
probe `src/dispersion_probe.cpp`):

- SIGTY equivalent: rural Pasquill-Gifford-Turner sigma-y with the ISC 6-8
  C/D pair per class, averaging-time factor `(AVET/PGTIME)**0.2`, per-cell
  roughness factor `YZ0FAC`, the secant pseudo-distance inversion for a
  non-zero start-of-step sigma (CRIT = 1e-4, 50 iteration limit, 0.85 step
  restriction) and the Heffter (1965) time-based growth beyond `SYH`, with
  `THFTY` obtained exactly as SETCSIG does (`SIGTY` with `SIGY0 = SYH`).
- SIGTZ equivalent: rural ISC multi-segment sigma-z (segment A/B pairs per
  class, segment selection on the SIGSET junctions, optional roughness
  adjustment `A' = A*AZ0FAC`, `B' = B-BZ0TRM` when `BZ0TRM > -999`), and urban
  McElroy-Pooler/Briggs sigma-z with the ISC 5000 m cap. Urban sigma-y uses
  Briggs `AYURB`/`XIYURB`; `XVY`/`XVZ` pseudo-distances are ported verbatim
  (including the cubic branch for classes A and B).
- Tables from the vendor `BLOCK DATA`/`SIGTY` DATA statements
  (`AYURB`, `XIYURB`, `AZURB`, `XIZURB`, `NZBRUR`, `XZBRUR`, `AZRUR`, `BZRUR`,
  `AYT`, `AZT`, `AYPGT`, `BYPGT`, `SYH`, `SZH`, ISC `C`/`D`), plus the SIGSET
  junction recomputation from the A/B continuity condition.
- Original semantics reproduced: the stability class is conditioned to 6 above
  6 (SETCSIG), small negative increments are reset to zero and reported while
  larger ones fail closed at the original `ZEROSEC(2)`/`ZEROKM(2)` thresholds,
  and `SYTDEP` is clipped to `NINT(AVEFAC*100)*1000`.
- Fail closed (deviation from the original, which silently propagates or
  halts): negative or non-finite sigma inputs, non-finite results, negative or
  non-finite setup values, unsupported `MDISP`, unsupported `MROUGH`/`MHFTSZ`
  values, `MROUGH=1` (gridded CALMET z0 plumbing not reconstructed; use
  `roughness_adjustment(z0)` per cell) and `MHFTSZ=1` (HEFTRAN transition time
  not reconstructed).

Not yet reconstructed: the `SIGMA()` nearest-known-sigma restart wrapper,
`HEFTRAN` slug geometry, `MROUGH=1` gridded roughness plumbing, and MDISP 1, 2,
4 and 5. No advection, concentration, deposition or chemistry is claimed.

### Source anchor

`reference/sigma_source_anchor.py` slices the exact line spans of the vendor
units out of the unmodified `calpuff.for` (no edits), pins the vendor file and
all vendor `*.puf` includes by SHA-256 in `reference/sigma_source_anchor.json`,
and stages the spans plus the hand-written driver
`reference/sigma_oracle_driver.f` for compilation. Vendor source:
`sha256 da40372cab1c181158d0ee5faac16f678e876fc14415073c38c8e3bdabfbc568`,
`BLOCK DATA` L1999-2424, `SIGSET` L52303-52502, `SIGTY` L50797-51215,
`SIGTZ` L51263-51752, `XVY` L51217-51261, `XVZ` L51754-51838,
`WARN` L79648-79797. Any drift fails the `--check` mode.

### CTest evidence (local worktree, macOS gfortran 16.2.0, Debug -Werror)

- `cteas_p7d_dispersion_contract`: hand-derived closed-form values (classes C,
  D urban/rural), monotone growth for all six classes, pseudo-distance round
  trip, Heffter growth, roughness adjustment, averaging factor, control audit
  and every fail-closed path. PASS.
- `cteas_p7d_sigma_source_parity`: vendor anchor + 243 table values bit-exact
  as REAL*4 (including the SIGSET junctions, max recomputation deviation
  1.4e-6 relative) + 612-case matrix against the compiled **unmodified**
  vendor oracle: `MAX_REL=2.596e-07` with acceptance `1e-5` across 7 columns.
  PASS in 4.3 s.
- Local `ctest` 10/11 PASS; the only failure is the pre-existing
  macOS case-insensitive-filesystem failure of
  `cteas_p7_original_case_stage_contract` (reproduced from pristine HEAD, PASS
  on Linux).

The workflow runs the parity test with `--require` (a Fortran compiler is
mandatory there) and audits the real deck with
`cteas_p7d_sigma_probe --audit`, asserting `MDISP=3` is the selected option.

### CI evidence for the P7D increment

Dedicated runner `lhy-desktop-3`, [run 38134496962](https://github.com/RRain001/CTEAS/actions/runs/38134496962)
at commit [4449c84](https://github.com/RRain001/CTEAS/commit/4449c84): `p7-scoped`
SUCCESS, `100% tests passed, 0 tests failed out of 11`. The required P7D steps printed
`P7D_SIGMA_FORTRAN_ORACLE_PARITY=PASS CASES=612 COLUMNS=7 TOLERANCE_REL=1e-05 MAX_REL=0.000e+00`
with `GNU Fortran (Ubuntu 11.4.0) 11.4.0` (bit-exact against the compiled unmodified
vendor routines, versus 2.596e-07 on macOS with gfortran 16.2.0) and
`P7D_DISPERSION_OPTION_AUDIT=PASS MDISP=3 SELECTED_OPTION_IS_REAL_CASE_PATH=true
MDISP2_EFFECTIVE=false MTURBVW_EFFECTIVE=false MTAULY_EFFECTIVE=false` for the real
2023 deck. `MDRY=1` dry deposition is the next physical path and is not implemented.

## 2026-10-11 P7F MDRY=1 dry deposition (computed gas, real-case species)

The locked Hefei 2022 deck
(`project/cases/hefei5km_2022_reference_case_20261011/run/puff/puff.inp`)
carries `MDRY=1`, the default `RCUTR=30` s/cm, `RGR=10` s/cm, `REACTR=8.0`,
`NINT=9`, `IVEG=1`, `ZFACE` with a 20 m first layer (so `ZREF=10 m`), and two
species in input group 3a:

| species | modeled | emitted | dry deposition | group 7 parameters |
|---|---|---|---|---|
| `H-3` | 1 | 1 | `1` = COMPUTED-GAS | `DIFFUSIVITY .1509` cm**2/s, `ALPHA STAR 1.0`, `REACTIVITY 8.0`, `MESOPHYLL 0.0` s/cm, `HENRY 0.04` |
| `HE-3` | 1 | 0 | `0` = no dry removal | none |

So the real dry-deposition path is the gas resistance model for one species.
`IDRY=0` in input group 5, i.e. **the case writes no dry-flux output**: the
frozen `run/puff/puff.con` (1,896,056 bytes, 3,655 sequential-unformatted
records, 11 distinct record sizes, clean 4-byte envelope) contains
concentration records only.

New native module (`include/cteas/puff/dry_deposition.hpp`,
`src/dry_deposition.cpp`, probe `src/drydep_probe.cpp`):

- Input group 3a species table (hyphenated names, which the generic
  `!KEY=value!` parser cannot express), input group 7 gas parameters and input
  group 9 reference values are parsed from the control text with anchored
  patterns, and cross-checked against `CSPEC`.
- DRYI equivalents: `cm**2/s -> m**2/s` and `s/cm -> s/m` conversions,
  `SC = vair/pdiff`, `RD1 = d1*SC**d2/vk`, `RCUT = RCUTR*REACTR/REACT`,
  `RGG = RGR*REACTR/REACT`, `RGW1 = HENRY/(ALPHAS*d3)`, and
  `ZREF = min(0.5*(ZFACE1+ZFACE2), 10)`.
- VDCOMP equivalent: the aerodynamic resistance (stability-dependent `PSIH`,
  `USMIN` floor, `RAMIN` floor, wind-driven water roughness
  `z0m = 2e-6*w**2.5` with the 1 m/s threshold), the deposition-layer term
  `RD1/u*`, the canopy resistance from stomatal (`PCONST`, `BMIN`/`BMAX`,
  `QSWMAX` radiation interpolation and the 35 C/10 C temperature branches),
  mesophyll, cuticle and ground/water terms, and `vd = 1/(RA+RD+RC)`.
- DRY uniformly-mixed branch: `DIFFK` from `CONK1`/`CONK2`, `u*`, `w*` and the
  mixing height, `VDPVD = DIFFK/(DIFFK+vd*dz)`, and
  `FRACDRY = exp(-VDPVD*vd*dt/lid)`.
- Fail closed: `MDRY != 1`, particle deposition (method 2), user-specified
  diurnal velocities (method 3), illegal method values, a gas species without
  group 7 parameters, group 7 rows for unknown or non-deposited species, a
  species list that disagrees with `CSPEC`, non-positive `RCUTR`/`RGR`/
  `REACTR`/diffusivity/alpha/reactivity, `IVEG` outside 1-3, and non-finite or
  out-of-range environment inputs (lid height, sampling step, mixing height,
  `w*`, temperature, `u*`, `L`, LAI, roughness, vegetation state) all raise
  instead of being ignored.
- Not reconstructed: the Gaussian-puff removal branch (needs the `VCBar`
  vertical coupling coefficient), the `RDVD` input table and the particle
  physics of `DRYI`/`VDP1`.

### Source anchor

`reference/drydep_source_anchor.py` slices the exact spans out of the
unmodified `calpuff.for` and pins them in
`reference/drydep_source_anchor.json`: compiled by the oracle
`VDCOMP` L26293-26565 and `VDP` L26567-26681; anchored read-only `BLOCK DATA`
L1999-2424, `DRY` L26052-26291, `DRYI` L26820-27042, `RDVD` L26683-26818 and
`VDP1` L27044-27208, plus every vendor `*.puf` include. Vendor source
`sha256 da40372cab1c181158d0ee5faac16f678e876fc14415073c38c8e3bdabfbc568`.
The DRYI statements that are ported by hand are asserted to still be present
verbatim in the anchored text, and the /DRYGAS/ constants (`PCONST`, `BMIN`,
`BMAX`, `QSWMAX`, `DCONST1`-`DCONST4`) are read back from the vendor BLOCK DATA
and compared with the values compiled into the module.

### CTest evidence

- `cteas_p7f_dry_deposition_contract`: hand-derived closed forms (inactive
  vegetation, hot/cold stomata, radiation interpolation and clamping, water
  cell with wind-driven roughness, neutral/stable `PSIH` branches, `u*` floor),
  the removal fraction, and every fail-closed path. PASS.
- `cteas_p7f_locked_case_dry_deposition`: the locked Hefei 2022 deck is audited
  in C++ (`MDRY=1`, H-3 gas, HE-3 none, `ZREF=10`, `RCUTR=3000` s/m,
  `RGR=1000` s/m, `REACTR=8`, `RD1=4.9801`). PASS.
- `cteas_p7f_drydep_source_parity`: anchors + 8 vendor constants + 11 quoted
  DRYI statements + **11,520 cases x 2 species against the compiled unmodified
  VDCOMP**: `MAX_REL=1.414e-07` (float32 level) against a fixed `1e-5`
  acceptance bound, plus the real-deck audit
  (`P7F_REAL_DECK_DRYDEP_AUDIT=PASS MDRY=1 GAS_SPECIES=1 NON_DEPOSITED=1
  UNSUPPORTED=0 VEGETATION_STATE=1 ZREF_M=10 RD1_SPECIES_0=4.9801`). PASS in
  1.4 s.
- Local `ctest` 13/14 PASS; the only failure remains the pre-existing macOS
  case-insensitive-filesystem failure of
  `cteas_p7_original_case_stage_contract` (PASS on Linux).

### Real-case comparability statement (no fabricated comparison)

`run/puff/puff.con` is **not yet comparable** with the new C++ deposition
velocities, and no comparison number is claimed. Verified gaps:

1. `IDRY=0` in the locked deck: the artifact has no dry-flux records, and the
   original writes per-step `vd` only behind its debug flag, so a
   case-level dry-flux comparison is impossible without re-running the
   original with debug output (out of scope here).
2. Per-cell dry-deposition inputs are missing: the P7B CALMET.DAT reader
   decodes the hourly typed fields (`USTAR`, `ZI`, `EL`, `QSW`, `TEMPK`, ...)
   but skips the static header records, so the real-case `Z0`, land use and
   `XLAI` are not available yet.
3. No puff-state integration: the `SIGMA()` restart wrapper, `HEFTRAN` and the
   `VCBar` coupling coefficient are not reconstructed, and P7C is still a
   prototype, so the removal fraction cannot be advanced over a real puff
   trajectory.
4. No concentration output: P7E does not exist, so there is nothing to diff
   against the 3,655 concentration records of `puff.con`.

What is comparable and now compared: the deposition velocity and the
time-invariant parameters for the real species configuration, against the
unmodified vendor routines (above), which is the strongest available
original-reference check at this stage.
