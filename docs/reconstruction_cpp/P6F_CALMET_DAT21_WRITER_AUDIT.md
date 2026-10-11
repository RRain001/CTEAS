# P6F CALMET.DAT 2.1 writer — byte-identity contract

Date: 2026-10-11. Unchanged reference is CALMET 6.5.0 L150223 (original vendor Git blob
`437f43bb7f0e51a1919e56664663e1a046986ed0`). New module: `native_cpp/modules/p6_calmet/p6f/`
(branch `codex/p6f-calmet-dat21-writer-20261011`). Roadmap row: P6F `CALMET.DAT` writer in
`P6_A_TO_G_SEQUENTIAL_EXECUTION_LEDGER.md`.

## Implemented

A standalone C++20 writer that turns **explicit field arrays** into a legal CALMET.DAT 2.1 file —
the record layout the unmodified CALPUFF reads.

1. **Library** `cteas::p6f` (`include/cteas/p6f/calmet_dat21_writer.hpp`, header-declared /
   compiled in `src/`). `Calmet21File` carries the declaration record, the comment section, the
   run control block (`IBYRN..IESECN`, `AXTZ`, `IRLG`, `IRTYPE`, `NX/NY/NZ`, `DGRID`, origins,
   `IWFCOD`, `NSSTA/NUSTA/NPSTA/NOWSTA`, `NLU`, `IWAT1/IWAT2`, `LCALGRD`) plus the OUTHD
   projection block (`PMAP/DATUM/DATEN/FEAST/FNORTH/UTMHEM/IUTMZN/RNLAT0/RELON0/XLAT1/XLAT2`),
   and the explicit arrays: `ZFACE`, `XSSTA/YSSTA`, `XUSTA/YUSTA`, `XPSTA/YPSTA`,
   `Z0`, `ILANDU`, `ELEV`, `XLAI`, `NEARS`, and per interval the per-level `U-LEV`/`V-LEV`/
   `WFACE` sequence, the `T-LEV` block and `IPGT/USTAR/ZI/EL/WSTAR/[RMM]/TEMPK/RHO/QSW/IRH/[IPCODE]`.
   `serialize_calmet_dat21` / `validate_calmet_dat21` / `parse_calmet_dat21` / `read_calmet_dat21` /
   `write_calmet_dat21` / `field_inventory` are the public surface.

2. **Layout facts** are taken from the unchanged original sources, not from a decoder:
   `OUTHD` L12448–L12537 (declaration, comment section, controls word order, static record order and
   widths, `ncom=ncommout+2`), `OUTHR` L13067–L13237 (hourly order: `U/V/[W]` inside the level loop,
   then `[T-LEV]`, then `IPGT USTAR ZI EL WSTAR [RMM] TEMPK RHO QSW IRH [IPCODE]`,
   `clabel(6:8)` I3 level digits), `WRTR1D` L36789–L36816, `WRTR2D` L36829–L36881,
   `WRTI2D` L36734–L36788 (24-byte record header: `char*8` label + begin/end `YYYYJJJHH` + seconds,
   then `NX*NY` words), `map.met` L5–L7 (`character*8 datum,pmap`, `character*12 daten`,
   `character*4 utmhem`).

3. **Explicit semantics**, all enforced fail-closed:
   - *time*: packed `YYYYJJJHH` + explicit seconds; every interval exactly 3600 s; the original writes
     the end clock as the same packed hour with seconds advanced by 3600; intervals are contiguous;
     `AXTZ` is an 8-character zone label; the declared run window (`IBYRN..IESECN`, written as
     year/month/day/hour plus seconds *inside* that hour — the frozen case is on the hour, so its
     seconds words are zero) must equal the first interval begin and the last interval end as
     instants.
   - *units*: every field carries its SI unit and its OUTHR/OUTHD mask condition
     (`hourly_field_specs`, `static_field_specs`); `check` prints them next to the denominators.
   - *masks*: the field set is exactly a function of `IRTYPE`/`LCALGRD`/`NPSTA`. `RMM`/`IPCODE` with
     `NPSTA=0` are rejected (`cannot appear when NPSTA=0`), internal CALMET state with no file
     representation (`ZICONV`, `DPTT`, `QH`, `USTAROLD`) is rejected by name, a missing field of a
     declared interval is rejected, and `NEARS` supplied with `NSSTA<1` is rejected instead of being
     dropped.
   - *extents/kind*: grid, station, static and per-field value counts must match the declared
     dimensions; `IPGT`/`IRH` must be stored as int32 and every other field as real32; real values
     must be finite; `ZFACE` must be strictly increasing with `NZ+1` faces; `IWAT1<=IWAT2`.
   - *missing*: the original sentinel `XMISS=9999.` (real) / `9999` (integer) is a legal value and is
     preserved bit-exactly; `field_inventory` counts it and excludes it from the number of defined
     cells.

4. **CLI** `cteas_p6f_calmet_dat21_writer`: `roundtrip --file [--out] [--expect-sha256] [--report]`
   (parse → serialise → byte compare, with the first differing record reported),
   `check --file [--report]` (layout validation + per-field inventory with units, masks and the
   `ILANDU` statistics), `emit-fixture --out … [--irtype 0|1] [--with-missing]` (deterministic,
   explicitly labelled synthetic layout fixture that exercises the writer from explicit arrays) and
   `write-from-p6d …` (the P6D wind-array entry point, section **P6D input adapter** below).

   Supported modes: `IRTYPE=0` (winds only) and `IRTYPE=1` (winds plus the 2-D meteorological
   fields), both with `IWFCOD=1`, `LCALGRD` taken from the header, and `NPSTA>=0`. `NPSTA=-1`
   (MM5 precipitation) is refused with an explicit message: it selects a precipitation input whose
   `RMM`/`IPCODE` mask semantics this writer does not implement, so a caller must supply observed
   precipitation stations or none at all. `IRTYPE=0` is derived from `OUTHR` L13160–L13180 (the
   routine returns right after the U/V/[W] loop) and is covered by synthetic round trips and an
   independent structural walk; no frozen original `IRTYPE=0` file exists in this repository, so the
   byte-identity proof above does not transfer to it.

## Byte-identity evidence against the frozen real case

Frozen truth: `project/cases/hefei5km_2022_reference_case_20261011/run/met/met.dat`,
64,762,556 B, sha256 `b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30`,
7,562 Fortran records, 1,114 comment records, 143 intervals × 45 fields = 6,435 field records,
`NX=NY=50, NZ=9`, `IRTYPE=1`, `IWFCOD=1`, `LCALGRD=T`, `NSSTA=1`, `NUSTA=1`, `NPSTA=0`.

Parsing that file into the explicit arrays and serialising it back produces a **byte-identical**
image (`cmp` clean, output sha256 equal to the frozen digest). The same file is then read back by
three independent consumers:

- the writer's own fail-closed parser (self-consistency only);
- an **independent Python record walk** in `tests/verify_real_reference_roundtrip.py`
  (framing, 96-byte declaration, 1,114×132-byte comments, 176-byte controls, the ten static records
  with zero clock words, the OUTHR order, the packed time labels, contiguity, and an aligned XMISS
  count per label) whose inventory must equal the writer's `check` report field by field;
- the **P7 decoder** `native_cpp/modules/p7_calpuff/reference/original_decode_calmet_hourly.py`
  (read-only), which must report `PASS_ORIGINAL_CALMET_EXPLICIT_TIME_AND_FIELD_LAYOUT` on the
  re-serialised file and produce a report **identical** to the one it produces for the frozen
  original, including its own sha256 of the file it read;
- the **E22 contract reader** `cteas::p6e::read_calmet_dat21` (P6E module, read-only include), which
  must parse the re-serialised real case and agree on all 6,435 field records cell by cell
  (bitwise float comparison), and vice versa (the P6F parser must read what the E22 writer produces).

Observation about the frozen artifact: the hourly field values of `met.dat` contain **no** `XMISS`
sentinel (0/15,372,500 real and 0/715,000 integer values of 16,087,500 stored cells), so the
sentinel path of the inventory is exercised only by the synthetic fixtures. This says nothing about
precipitation: the P6C record of the same case already documents `IPCODE=9999` 143/143 in `SURF.DAT`
and `NPSTA=0`.

## Gate

```
python3 scripts/local_gate.py --track p6f     # LOCAL_GATE_P6F=PASS
```

Delivered as PR [#26](https://github.com/RRain001/CTEAS/pull/26) (base `main`); the script writes
its own machine-readable evidence under `$CTEAS_GATE_ROOT/evidence/p6f-<sha12>.json`.

The gate configures, builds (`g++-12`, `-Wall -Wextra -Wpedantic -Werror`) and runs the module's
CTest targets, plus one dedicated step for the original-CALPUFF read test:

| CTest | What it proves |
|---|---|
| `p6f_calmet_dat21_writer_unit` | explicit arrays → records → read back, per-field bit equality, both output modes, the calendar/time helpers over 1600–2400, XMISS preservation, and the ~55-case fail-closed matrix |
| `p6f_calmet_dat21_synthetic_cli_roundtrip` | CLI end to end on four synthetic variants (including `NSSTA=0` and `NPSTA=3`) plus an `IRTYPE=0` and a sub-hourly (`00:30`) start, with an independent Python walk of every emitted record and the CLI-level fail-closed options |
| `p6f_calmet_dat21_real_reference_round_trip` | the frozen byte identity + SHA-256 pin + independent inventory agreement (including the `ILANDU` statistics) + P7 decoder cross-read + six fail-closed negatives |
| `p6f_calmet_dat21_e22_contract_cross_read` | E22 reader on the re-serialised frozen case (6,435 records) and P6F reading an E22-written file |
| `p6f_calmet_dat21_p6d_array_input` | the P6D contract end to end (202,500 values mapped and verified) plus eight fail-closed cases |
| `p6f_calmet_dat21_real_scale_xmiss` | 50x50x9 x 143 intervals densified with XMISS (167,310 sentinel values), independent per-label counts, byte-exact round trip, P7 decoder acceptance, NaN rejected |
| `p6f_calmet_dat21_original_calpuff_reads_written_calmet` | rebuilds the unmodified original and runs cases A-D of the table above (excluded from the plain `ctest` invocation and run as its own step, so a missing Fortran toolchain is reported BLOCKED instead of silently skipped, with `--no-tests=error` preventing a silent skip) |

The real-case tests are registered fail-closed: `CMakeLists.txt` raises a configure error if the
frozen reference file or the P7 decoder is absent, so the byte-identity proof can never be silently
skipped, and the gate's own step declares the Fortran toolchain, the vendor sources and the tracked
reference outputs in `requires`.

## P6D input adapter (the P6D -> P6F -> P6E pipeline entry)

The P6D producer `cteas_p6_realcase_wind_field` (module `native_cpp/modules/p6_calmet`) writes, per
hour, one raw little-endian float32 plane per field plus `metadata.json`, `produced_hours.txt`,
`temp2d_*`, `qh_*` and four `terset_*` planes. One documented command turns that product into a legal
CALMET.DAT 2.1:

```
cteas_p6f_calmet_dat21_writer write-from-p6d \
    --fields <p6d-dir> --variant wind1 --w-field w_face \
    --statics-from <legal CALMET.DAT 2.1> \
  [ --irtype 1 --met-fields-from <legal CALMET.DAT 2.1> ] \
    --out <CALMET.DAT> [--report <json>]
```

- **Read, never guess**: `metadata.json` supplies `grid.nx/ny/nz`, `grid.grid_spacing_m`,
  `grid.layout` (must be the documented `i_fastest` index `i + nx*(j + ny*k)`), `grid.dtype`
  (`float32`) and `grid.byte_order` (`little`), `levels.zface_m` (must equal the `ZFACE` of
  `--statics-from`), `missing_sentinel` (scalar), `hour_label` (`END` or `START`, so the interval
  begin is `label - 3600 s` or the label itself) and the hourly label list/count. Any other spelling
  is rejected with the value it found.
- **W faces**: `w_face_*.f32` holds `NZ+1` faces per column (face 1 is the ground face); CALMET.DAT
  writes W at the *top* face of each layer, so face `k+1` becomes `WFACE(k)` and the ground face is
  not written.
- **No invented values**: the declaration, the static records, the station tables and the projection
  block are case input and come from `--statics-from`; the declared run window and `IRLG` are derived
  from the intervals actually written. The P6D contract contains only U/V/W, so `IRTYPE=0` is the
  default product; `IRTYPE=1` requires `--met-fields-from`, which takes `T-LEV/IPGT/USTAR/ZI/EL/
  WSTAR/TEMPK/RHO/QSW/IRH` verbatim from another legal file (P6E's product, or the case's own records
  when composing for integration).
- **Verification status is honoured**: the adapter keys on the verdict P6D publishes in
  `metadata.json` (`blend.verification`) instead of on a hard-coded list. A variant whose verdict
  starts with `NOT_SOURCE_VERIFIED` is refused unless `--accept-unverified-variant` is passed, and
  the report always records `blend_verification`, `fradj_status`, `variant_source_verified` and
  `w_face_source_verified`. This was exercised on real metadata, not only in tests: the directory
  used for the case C run (`/home/lhy/build/p6d/rw11`, read 2026-10-12) carries
  `blend.verification = "NOT_SOURCE_VERIFIED: the 143-hour comparison against the unchanged original
  INTER2 FAILED (worst 10.36 m/s) …"`, and `write-from-p6d --variant blend` on it was refused; the
  documented run therefore uses `--variant wind1`, whose verdict is
  `PASS_REALCASE_STEP_ONE_ORIGINAL_WIND1_PLUS_SIMILT` (worst 2.53e-05 m/s over 6,435,000 components).
  P6D report a producer-side fix at their head `d300a9326` (blend PASS, worst 4.22e-05 m/s); when a
  regenerated directory carries that verdict in its metadata the same command accepts `--variant
  blend` with no code change, which is exactly what the adapter test covers (refusal, plus the
  explicit `--accept-unverified-variant` opt-in path).
- **What the composition is not**: `w_face_*` is SLOPE-compensated but not FRADJ-compensated
  (`FRADJ: PENDING_GAMMA` in the P6D metadata) until `GAMMA`/`CGAMMA` lands, so it is QA/plumbing
  data; and composing a reconstruction with the *original* 2-D fields is an integration step, not a
  claim that those fields were reconstructed.

Evidence: `tests/verify_p6d_array_input.py` builds a directory of exactly this shape and requires
every U/V/W value of the produced file to equal the corresponding plane value (202,500 values for
three hours at 50x50x9), the byte-exact round trip, the `IRTYPE=1` composition path, and eight
fail-closed cases (missing plane, wrong plane size, unsupported `grid.layout`, unsupported
`hour_label`, non-finite value, non-contiguous hours, `IRTYPE=1` without a met source, grid
mismatch). Run against the **real** P6D product (`/home/lhy/build/p6d/rw8`, 143 hourly labels, 1,291
files) the same command writes a 64,762,556-byte, 143 x 45 file, and the unmodified original CALPUFF
reads it end to end (case C below).

## Original CALPUFF reads what we write

The roadmap clause for this stage is "C++ 完整合法输出 v2.1，原版 P7 读取". Our own decoders are not
enough, so `tests/verify_original_calpuff_reads_written_calmet.py` rebuilds the **unmodified original
CALPUFF 7.2.1** from the tracked vendor sources with the repository's own reference flags
(`scripts/capture_hefei_2022_reference_case.py`, `PUFF_FFLAGS`) and runs it in a temporary directory
(nothing is ever written inside `project/`):

| Case | File the original read | Result |
|---|---|---|
| A | the writer's re-serialisation of the frozen `met.dat` (byte-identical, pinned SHA-256) | 143 hours; `puff.con` **byte-identical** to the tracked original reference output and `puff.lst` identical beyond its two wall-clock lines; the listing echoes our header (`metfm=1`, `irlg=143`, `nsecdt=3600`, `abtz=UTC+0000`, `ibdathr=202200100`, `iedathr=202200623`, `zface = 0.00000000 … 3000.000`) and ends at `Year: 2022 Month: 1 Day: 6 Julian day: 6 Hour: 23 Second: 0` |
| B | the same file with **changed values** from `cteas_p6f_calmet_dat21_perturb` (wind x1.25, ZI x1.5, TEMPK +5 K; structure, masks, units and time labels untouched) | 143 hours, no fatal marker, identical time-window echo, and different finite non-negative concentrations (mean 0.032848 → 0.029406, max 5.1697 → 4.6693, non-zero cells 42,685 → 42,670) - legal *and* read, not merely hash-equal |
| C | `write-from-p6d` on the **real P6D** product (`/home/lhy/build/p6d/rw11`: `wind1_*` U/V whose P6D verdict is `PASS_REALCASE_STEP_ONE_ORIGINAL_WIND1_PLUS_SIMILT`, `w_face_*`, plus the case's 2-D fields) | 143 hours, no fatal marker, finite non-negative concentrations (mean 0.031191, max 7.8652, 52,911 non-zero cells) |
| D | a structurally broken file (the frozen file truncated by 10 bytes) | **rejected**: the original aborts (`Error termination. Backtrace:`, return code 2) instead of producing a clean successful run |

The build is pinned in the report: GNU Fortran 11.4.0, the `modules.for`/`calpuff.for` SHA-256 values
and the produced binary SHA-256 (`8a2200798567ebcaddaf54f88f8d81e73377294abd1b81201f718481f89a4f49` for
the run documented here). One case takes about 20 s. The test also fails if the original changes a
byte of the weather file it read (it opens `METDAT` read-write).

Two findings worth keeping: the PE `calpuff_v7.2.1.exe` from the distribution directory cannot
execute on the Linux build host (no Wine is installed), and a pre-built binary from outside the
repository that looks like the original did **not** reproduce the tracked reference output (it halted
in `GETCELL` at hour 25 and printed differently formatted listings). This test therefore builds the
original from the repository's own tracked sources - the only reproducible authority - and proves the
build by reproducing the tracked reference `puff.con` byte for byte.

## Hard limits and open dependencies

- **The original-CALPUFF evidence is layout/readability evidence.** Case A additionally shows that
  the original computes exactly what it computed from the frozen file (byte-identical concentration
  output), and cases B/C show that changed values are read and used - none of that makes the
  reconstructed fields scientifically correct, which stays a P6G obligation.
- **This is a serialiser, not a physics result.** Byte identity proves the record layout, time
  semantics, units and masks. It does **not** show that reconstructed 2022 fields equal the original
  CALMET values; the writer stores whatever value words the caller supplies, including wrong ones.
  The P6F acceptance clause "物理值正确" therefore remains **open** and belongs to P6G, which must
  compare the P6D/P6E chains' fields with the frozen original under the pre-registered tolerances.
  Status of this work: writer implemented, layout + byte identity PASS, **field values NOT accepted**.
- `IRTYPE=0` is supported and tested synthetically but has no frozen original reference in this
  repository; `NPSTA=-1` (MM5 precipitation) stays refused (see above).
- The projection block is only byte-preserved and typographically validated; only the frozen case's
  UTM/WGS-84 block (`UTM`, `WGS-84`, `02-21-2003`, `N   `, `IUTMZN=50`) has been exercised end to end.
- `ILANDU` is written verbatim and never range-rejected: the frozen case stores category 20 on all
  2,500 cells while `NLU=14`, so `check` reports `ilandu_max`, `ilandu_over_nlu_cells` and
  `ilandu_negative_cells` instead of failing (the independent cross-check in the byte-identity test
  reproduces `ilandu_max=20` and `ilandu_over_nlu_cells=2500`). Land-use validity remains a
  caller/upstream concern.
- The P6D U/V products are written as delivered and carry their own original-source verdicts in the
  metadata this adapter reads (`wind1_*`: original `WIND1`+`SIMILT`; `blend_*`: as published by P6D,
  which was still `NOT_SOURCE_VERIFIED` in the directory used here). `slope_corrected_*` and
  `w_face_*` are SLOPE-compensated but not FRADJ-compensated (`FRADJ: PENDING_GAMMA`): they are
  QA/plumbing values and are labelled as such in the report.
- The E22 header is consumed only as an independent decoder through an explicit include path, and the
  P6D contract is consumed through its published `metadata.json`/plane files: the P6F library and CLI
  include no P6E/P6D/P7 C++ header. If E22 or the P6D contract changes, the corresponding cross-read
  test must be re-run.
- The writer does not close P6F as a stage milestone: it closes the writer/layout half of it.
