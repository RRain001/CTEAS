#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

# 固定使用清华镜像源
PIP_INDEX="https://pypi.tuna.tsinghua.edu.cn/simple"

if ! command -v python3 >/dev/null 2>&1; then
    echo "[ERROR] python3 not found."
    exit 1
fi

if command -v uv >/dev/null 2>&1; then
    echo "[INFO] Using uv to create/update .venv"
    if [ ! -x .venv/bin/python ]; then
        uv venv .venv
    fi
    uv pip install --python .venv/bin/python -i "$PIP_INDEX" -r requirements.txt
else
    echo "[INFO] uv not found, fallback to python venv + pip"
    if [ ! -x .venv/bin/python ]; then
        python3 -m venv .venv
    fi
    if ! .venv/bin/python -m pip -V >/dev/null 2>&1; then
        .venv/bin/python -m ensurepip --upgrade || true
    fi
    .venv/bin/python -m pip install --upgrade pip
    .venv/bin/python -m pip config set global.index-url "$PIP_INDEX"
    .venv/bin/python -m pip install -i "$PIP_INDEX" -r requirements.txt
fi

echo "[INFO] Environment ready:"
.venv/bin/python -V
.venv/bin/python -m pip -V
echo "[INFO] Test imports:"
.venv/bin/python -c "import yaml, numpy, pandas; print('ok')"