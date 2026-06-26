#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_NAME="cteas-fortran:debian12-arm64"

docker build --platform linux/arm64 -t "$IMAGE_NAME" -f "$ROOT_DIR/docker/fortran/Dockerfile" "$ROOT_DIR"

docker run --rm \
  --platform linux/arm64 \
  -v "$ROOT_DIR:/work" \
  -w /work \
  "$IMAGE_NAME" \
  bash -lc '
set -euo pipefail
mkdir -p bin/linux-arm64
FFLAGS="-std=legacy -ffixed-line-length-none -fno-automatic -fallow-argument-mismatch -O0"

build() {
  local out="$1"
  shift
  echo "[BUILD] $out"
  gfortran $FFLAGS "$@" -o "bin/linux-arm64/${out}.x"
}

build terrel \
  engine/vendor/src/TERREL_v7.0.0_L141010/calutils.for \
  engine/vendor/src/TERREL_v7.0.0_L141010/coordlib.for \
  engine/vendor/src/TERREL_v7.0.0_L141010/dd_dem.for \
  engine/vendor/src/TERREL_v7.0.0_L141010/tiffinfo.for \
  engine/vendor/src/TERREL_v7.0.0_L141010/terrel.for

build ctgproc \
  engine/vendor/src/CTGPROC_v7.0.0_L150211/calutils.for \
  engine/vendor/src/CTGPROC_v7.0.0_L150211/coordlib.for \
  engine/vendor/src/CTGPROC_v7.0.0_L150211/tiffinfo.for \
  engine/vendor/src/CTGPROC_v7.0.0_L150211/readctg.f \
  engine/vendor/src/CTGPROC_v7.0.0_L150211/ctgproc.for

build makegeo \
  engine/vendor/src/MAKEGEO_V3.2_L110401/calutils.for \
  engine/vendor/src/MAKEGEO_V3.2_L110401/makegeo.for

build read62 \
  engine/vendor/src/READ62_v5.661_L110225/calutils.for \
  engine/vendor/src/READ62_v5.661_L110225/read62.for

build smerge \
  engine/vendor/src/SMERGE_v5.7.0_L121203/calutils.for \
  engine/vendor/src/SMERGE_v5.7.0_L121203/presutil.for \
  engine/vendor/src/SMERGE_v5.7.0_L121203/smerge.for

build calmet \
  engine/vendor/src/CALMET_v6.5.0_L150223/calutils.for \
  engine/vendor/src/CALMET_v6.5.0_L150223/coordlib.for \
  engine/vendor/src/CALMET_v6.5.0_L150223/auxutils.for \
  engine/vendor/src/CALMET_v6.5.0_L150223/calmet.for

build calpuff \
  engine/vendor/src/CALPUFF_v7.2.1_L150618/calutils.for \
  engine/vendor/src/CALPUFF_v7.2.1_L150618/coordlib.for \
  engine/vendor/src/CALPUFF_v7.2.1_L150618/api_chem.for \
  engine/vendor/src/CALPUFF_v7.2.1_L150618/isorropia_v2.2.for \
  engine/vendor/src/CALPUFF_v7.2.1_L150618/lwcsubs.for \
  engine/vendor/src/CALPUFF_v7.2.1_L150618/modules.for \
  engine/vendor/src/CALPUFF_v7.2.1_L150618/calpuff.for

chmod +x bin/linux-arm64/*.x
file bin/linux-arm64/*.x
'
