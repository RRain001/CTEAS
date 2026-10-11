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
