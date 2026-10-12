#!/usr/bin/env bash
set -euo pipefail
source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
sudo -n install -d -m 0755 /usr/local/lib/cteas-gate
for file in gate_worker.py resource_limits.py local_gate.py gate_submit.py; do
  sudo -n install -m 0644 "$source_dir/$file" "/usr/local/lib/cteas-gate/$file"
done
sudo -n install -m 0755 "$source_dir/gate_control.py" /usr/local/sbin/cteas-gate-control
mkdir -p /home/lhy/gate-managed/requests
chmod 700 /home/lhy/gate-managed /home/lhy/gate-managed/requests
if [[ ! -f /home/lhy/cteas-resource-audit/gate-slice-before.json ]]; then
  mkdir -p /home/lhy/cteas-resource-audit
  python3 - <<'PY'
from pathlib import Path
import json
p=Path('/etc/systemd/system/cteas-gate.slice')
b=Path('/home/lhy/cteas-resource-audit/gate-slice-before.json')
b.write_text(json.dumps({'content':p.read_text() if p.exists() else None}));b.chmod(0o600)
PY
fi
sudo -n tee /etc/systemd/system/cteas-gate.slice >/dev/null <<'UNIT'
[Unit]
Description=CTEAS local Gate shared resource budget
[Slice]
CPUAccounting=yes
CPUQuota=600%
MemoryAccounting=yes
MemoryHigh=14G
MemoryMax=16G
MemorySwapMax=0
TasksMax=2048
UNIT
sudo -n systemctl daemon-reload
sudo -n systemctl start cteas-gate.slice
