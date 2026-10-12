#!/usr/bin/env bash
set -euo pipefail
umask 077
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "$script_dir/logs"
python3 "$script_dir/runnerctl.py" install "$@" 2>&1 | tee -a "$script_dir/logs/deploy-$(date -u +%Y%m%d).log"
