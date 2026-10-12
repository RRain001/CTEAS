#!/usr/bin/env bash
set -euo pipefail
exec /home/lhy/CTEAS/.venv/bin/python3.11 "$(dirname -- "${BASH_SOURCE[0]}")/gate_submit.py" "$@"
