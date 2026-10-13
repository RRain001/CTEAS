## Independent OUTHD control provenance (2026-10-13)

[Actions #38376315827](https://github.com/RRain001/CTEAS/actions/runs/38376315827) is a separate independent-header source gate with **PASS**. It parses the frozen `CALMET.DAT` as a target (not a header template), reads only the upstream `met.inp` and `GEO.DAT` as expected-value inputs and validates 49 source-derived conditions across the 176-byte OUTHD run-control record: 143 hourly intervals, independent start/end clocks, UTC zone, grid geometry and spacing, selected station counts, land-use count/range, UTM 50N datum/date, origin and projection numeric terms. Mutated ABTZ, IEHR, NX and GEO UTM-zone inputs are each rejected. The tool is `scripts/p6f_independent_header_control_audit.py`; workflow `.github/workflows/p6f-independent-header-source.yml`; only the repository's GitHub-hosted Ubuntu runner was used. The reference file SHA-256 is pinned by the job before validation.

This is separate from [Actions #38375786650](https://github.com/RRain001/CTEAS/actions/runs/38375786650), which confirmed 12,514 independently GEO/deck-sourced static field values and byte-exact re-serialisation in the frozen-hourly **envelope**. The selected real-case independent native P6D/P6E 143h candidate was re-serialized with the same sourced statics in [Actions #38376244497](https://github.com/RRain001/CTEAS/actions/runs/38376244497), retaining all 45 numeric original comparison PASS.

**Important scope restriction:** the independent OUTHD audit checks the bytes' *content provenance*, not that all controls/comments/field masks are generated without any reference CALMET envelope in the current production writer. The 1,114 original comments/declaration and full per-hour autonomous writer invocation remain separate completion gates. A 45/45 real-case numeric pass and static source reconstruction do not establish a fully self-sufficient P6F implementation or generic CALMET acceptance. PR #53 must remain Draft.

---

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
- **Accepted variants**: `wind1_*`, `inter2_*`, `blend_*`, `slope_corrected_*`. The producer renamed
  the pre-tail INTER2 field from `blend_*` to `inter2_*` and reused `blend_*` for the closed DIAGNO
  field (`INTER2 -> SMOOTH -> DIVCEL`), which is the variant P6D recommends for a full wind product;
  `inter2_*` stays available as the diagnostic stage whose verdict is anchored by the unchanged
  original comparator. Each name carries its own verdict in `metadata.json` (`blend.verification`,
  `inter2.verification`), and the adapter records whichever it finds verbatim. `windbc_*` is refused
  with an explicit message: those planes are stitched lateral boundaries (`NZ*2*NY` / `NZ*2*NX`,
  level-major), not a gridded field.
- **What the verdict does and does not do**: the adapter refuses a variant only when the producer
  marks it with the `NOT_SOURCE_VERIFIED` prefix (a "do not use" marker, observed on the earlier
  `rw11` blend). Any other verdict string - including a partial one such as
  `PARTIAL_LEVELS_8_9_PASS_LEVELS_1_7_NEUTRAL_SIMILT_GAP` or a degraded
  `MIXING_HEIGHT_NOT_SUPPLIED_ALL_LEVELS_NEUTRAL_SIMILT` - is written and recorded verbatim in the
  CLI report (`blend_verification`, `variant_source_verified`, `product_partial`, `product_status`);
  the writer never upgrades a producer's caveat into a pass of its own.
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
  used for the earlier case C run (`/home/lhy/build/p6d/rw11`, read 2026-10-12) carries
  `blend.verification = "NOT_SOURCE_VERIFIED: the 143-hour comparison against the unchanged original
  INTER2 FAILED (worst 10.36 m/s) …"`, and `write-from-p6d --variant blend` on it was **refused**
  (fail-closed observed on real metadata). The regenerated directory `rw12` carries
  `PASS_REALCASE_BLEND_ORIGINAL_INTER2` (143 hours, 6,435,000 components, worst 4.22e-05 m/s at a
  2e-4 m/s tolerance, zero mismatches) and the same command accepts `--variant blend` with no code
  change - which is what the case C row records. The refusal path and the explicit
  `--accept-unverified-variant` opt-in are covered by the adapter test.
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
| C | `write-from-p6d --variant blend` on the **real P6D** product `/home/lhy/build/p6d/rw17` (the recommended closed-DIAGNO blend, 1,865 files, 143 labels, tree sha256 `7d34bcc9525f19dd1d207a48a40b5aff87501b2ac26315d11db7152e9cc23cfd`, `metadata.json` sha256 `a36acd862174f2837c28eb1be482447e942eb996ec6984d2213ae10e882d6b6a`, `blend.verification = PARTIAL_LEVELS_8_9_PASS_LEVELS_1_7_NEUTRAL_SIMILT_GAP: …`, `inter2.verification = PASS_REALCASE_BLEND_ORIGINAL_INTER2`, `w_face_*` FRADJ-pending) plus the case's 2-D fields | 143 hours, no fatal marker, finite non-negative concentrations (mean 0.030236, max 5.2611, 31,701 non-zero cells) on a 64,762,556-byte composed file |
| D | a structurally broken file (the frozen file truncated by 10 bytes) | **rejected**: the original aborts (`Error termination. Backtrace:`, return code 2) instead of producing a clean successful run |

The build is pinned in the report: GNU Fortran 11.4.0, the SHA-256 values of the unchanged
`modules.for`/`calpuff.for` and the produced binary (the run documented here built
`f248b09a76a1792c…`; a rebuild in another directory yields a different binary hash because the build
path is embedded, while the pinned sources and flags are the same). One case takes about 20 s. The
test also fails if the original changes a byte of the weather file it read (it opens `METDAT`
read-write).

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


## 2026-10-13: independently sourced P6F static records and numeric control fields

The earlier `write-from-p6d --statics-from` CLI is a **source-dependent integration adapter** and must not be represented as native P6F static-source completion. Following P6D 27/27 and P6E 18/18 strict 143h scientific PASS, a new separate fail-closed source-provenance boundary was built:

- `native_cpp/modules/p6_calmet/p6f/src/p6f_geo_static_provenance_main.cpp` uses the previously vetted independent `cteas::calmet::p6::read_geo_50` reader on **GEO.DAT v2.1** and the original model's **met.inp** numeric deck. It never derives an expected static value from the CALMET.DAT candidate. The candidate is only parsed to compare observations and to retain non-independent envelope portions as declared below.
- Independently rebuilt: 2,500 cells each of land use `ILANDU`, terrain `ELEV`, roughness `Z0`, and leaf-area index `XLAI`; 10 `ZFACE` levels; the four surface/upper-air station X/Y coordinates; 2,500 one-station `NEARS` values. **12,514** comparisons, 0 disagreements with the unchanged original static values. The source-derived numerical run header includes begin/end dates, hourly interval count, grid dimensions/origin/spacing, time zone, station counts, run type, projection key and UTM zone, `NLU/IWAT1/IWAT2`, and `LCALGRD`.
- Exact source-coordinate fidelity: the CALMET OUTHD station static arrays are **grid-origin-relative metres**, whereas SS1/US1 in the deck use absolute UTM kilometres. Original REAL*4 operation order is `float32(station_km)*1000f - float32(origin_km)*1000f`, **not** `(station_km-origin_km)*1000f`. The false latter conversion produced 6 mm of error for upper-air station X, rejected by the audit.
- Optional `--emit-source-statics OUTPUT` rewrites static values and the numerical controls from GEO.DAT/met.inp into the legal `CALMET.DAT` 2.1 file envelope. The original frozen 143h reference remains byte-identical (SHA-256 `b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30`) after replacing those records with independent inputs, and intentionally mutated ZFACE is rejected. [Actions #38376041857](https://github.com/RRain001/CTEAS/actions/runs/38376041857) PASS.
- **Real native 143h integration**: the same independent upstream static regeneration was applied to the actual C++ P6D→P6E G3/G4 PASS output archived from [Actions #38372382572](https://github.com/RRain001/CTEAS/actions/runs/38372382572). Rewritten native candidate is byte-identical to the native candidate before static replacement, and the source-locked original 45-field numeric comparator reports `total_exceedances=0`. [Actions #38376244497](https://github.com/RRain001/CTEAS/actions/runs/38376244497), PASS. This does **not** inject original winds into the native candidate.

**Outstanding P6F work (not accepted):** the complete `OUTHD` comment block, declaration text, and projection auxiliary date field are still held from the candidate envelope. Unsupported multi-station and precipitation-station static paths are not regenerated here; `NSSTA=1, NUSTA=1, NPSTA=0` is the admitted source-emission scope. Full independent native 143h file assembly should use explicit `Calmet21Header + Calmet21Statics + P6D/P6E hourly arrays` without parsing the reference as its starting envelope. Do not label `P6F_COMPLETE` or `P6_COMPLETE` based only on this selected-case source-static gate.


## 2026-10-13: P6F independently generated OUTHD + native 143h hourly fields

This milestone **supersedes the earlier pending independently rebuilt header comment block** for the *locked 50×50×9 Hefei 2022 case only*.

- New C++ P6F tool mode: `cteas_p6f_geo_static_provenance --geo SOURCE_GEO.DAT --deck SOURCE_MET.INP --candidate NATIVE_P6D_P6E_CALMET.DAT --emit-native-header OUTPUT.DAT`. It creates a **new** `Calmet21File`, derives the complete `OUTHD` declaration, 1,090 132-column comments, date/zone/run control, UTM projection date from GEO.DAT, and all admitted static arrays from the GEO/deck source. Only the **hourly fields** are transferred from the native P6D/P6E file. Earlier `--emit-source-statics` remains byte-identity reference-envelope validation, not this new mode.
- Original Fortran `TFERCF` copies each control-deck line as a comment, but the frozen binary has 1,114 comment records whereas the tracked updated `met.inp` supplies a different native comment count (1,090 including the two native provenance comments). Header **byte identity is neither claimed nor desired**. Original CALMET file layout is preserved, not a copied reference text block.
- In [GitHub Actions #38378460648](https://github.com/RRain001/CTEAS/actions/runs/38378460648), selected native 143h file with newly generated header passed C++ binary validation, P7 independent 143h record decoder, and frozen original source numeric comparator **45/45 fields, zero exceedances** with unchanged per-field tolerances. The independent header must differ in bytes from the source candidate, and its first comment must identify CTEAS.
- In [GitHub Actions #38378766675](https://github.com/RRain001/CTEAS/actions/runs/38378766675), **unmodified vendor CALPUFF 7.2.1** was compiled with the original source build recipe and read the CTEAS source-generated header file for all 143 hours. Original CALPUFF generated finite, nonnegative, nonzero concentrations; no original numerical equivalence of P7 is inferred.
- Separate paired-run evidence from [GitHub Actions #38379020835](https://github.com/RRain001/CTEAS/actions/runs/38379020835) directly compares unmodified CALPUFF's concentration output when its input is the original native-file header versus the independent P6F source-header version. Record its final job result before treating byte-equivalent P7 impact as accepted.

**Remaining explicitly open:** the *P6E per-hour producer* still reads an original/reference `CALMET.DAT` envelope for some setup/statics; this downstream P6F **writer independence does not prove upstream P6E input-reference elimination**. No generic multi-surface-station, precipitation-station, non-UTM, variable dimensions, or end-to-end original-source-free meteorological production is claimed. The C++ CALMET time authority remains process-backed, not an in-process `step_diagno/step_comp` kernel. Do not yet mark the overall P6 milestone PASS.


## Original CALPUFF 7.2.1 consumes independent native OUTHD (2026-10-13)

**PASS**, frozen Hefei selected 143h run, [GitHub Actions #38379522205](https://github.com/RRain001/CTEAS/actions/runs/38379522205).

The new [`verify_original_calpuff_native_independent_header.py`](../native_cpp/modules/p6_calmet/p6f/tests/verify_original_calpuff_native_independent_header.py) builds unmodified original CALPUFF 7.2.1 from pinned `modules.for`/`calpuff.for`, and performs a control-variable test using the **same C++ P6D/P6E 143h weather payload** twice:
1. a native CALMET.DAT merged by the C++ clock whose existing header contains 3 documentation lines;
2. a source-generated independent P6F OUTHD with 1,090 comments derived from `met.inp` and `GEO.DAT`, with all numeric control fields and 12,514 static values explicitly rebuilt by the native source generator.

The test checks that **all 6,446 non-comment Fortran records**—including the entire 143h weather field inventory—are byte identical between (1) and (2), independent of comment count and record offsets. Both original CALPUFF runs completed all 143 hours. The 143h concentration-grid `float32` payload SHA-256 is identical between the two outputs. The full `puff.con` file hashes are **not** identical (439 non-hourly/non-concentration data records differ; first differing record index 1), so do not claim full-file byte equivalence or prove the meaning of those changed records without separate decoding.

The earlier [Actions #38378460648](https://github.com/RRain001/CTEAS/actions/runs/38378460648) established independent numerical OUTHD and static provenance + original 45-field scientific comparison + P7 structural decoding. This final control-variable CALPUFF test closes the **unmodified P7 read/143h concentration preservation gate**.

Scope and remaining hold:
- This validates the selected Hefei 2022 one-surface/one-upper-station 50×50×9 case. `--emit-native-header` currently uses an already-assembled native P6D/P6E CALMET.DAT as the *hourly field carrier* (not as the source for header and statics). A fully stand-alone P6F writer should receive the per-hour native P6D/P6E field arrays directly, construct `Calmet21File` without parsing a previously assembled CALMET.DAT, and validate source provenance/hashes at the interface.
- CALPUFF concentration-grid byte equivalence for this one case is not CALPUFF source-physics numerical equivalence for every deposition, output mode, or input branch. Do not claim P6 generic G5 or complete all-case P6F acceptance.
- No CALMET scientific tolerances, P6D/P6E physics, reference data or Fortran sources were modified by the OUTHD acceptance test.
