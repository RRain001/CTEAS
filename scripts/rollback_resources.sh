#!/usr/bin/env bash
set -euo pipefail
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# Refuse rollback while controlled tasks are running/queued; never interrupt science.
python3 - <<'PY'
import json,pathlib,sys
for p in pathlib.Path('/home/lhy/gate-managed/requests').glob('*/state.json'):
 s=json.loads(p.read_text())
 if s['phase'] in ('running','queued','submitted'):
  sys.exit('BLOCKED: Gate task active; wait for completion before rollback')
PY
for i in 1 2 3; do python3 "$script_dir/legacy_runner_resources.py" rollback --runner "$i"; done
python3 - <<'PY'
import json,pathlib,subprocess
p=pathlib.Path('/etc/systemd/system/cteas-gate.slice');b=json.loads(pathlib.Path('/home/lhy/cteas-resource-audit/gate-slice-before.json').read_text())
subprocess.run(['sudo','-n','systemctl','stop','cteas-gate.slice'],check=True)
if b['content'] is None:
 if p.exists():subprocess.run(['sudo','-n','unlink',str(p)],check=True)
else:subprocess.run(['sudo','-n','tee',str(p)],input=b['content'],text=True,stdout=subprocess.DEVNULL,check=True)
subprocess.run(['sudo','-n','systemctl','daemon-reload'],check=True)
PY
# Runtime launcher remains fail-closed; no hidden fallback to unbounded Gate execution.
printf '%s\n' 'Resources restored; Gate launcher now refuses execution until pool controls are installed.'
