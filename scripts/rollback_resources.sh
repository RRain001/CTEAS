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
# Refuse to overwrite any work made by another session in the driver.
python3 - <<'PYCODE'
import pathlib,subprocess
repo='/home/lhy/gate/driver'
assert not subprocess.check_output(['/usr/bin/git','-C',repo,'status','--porcelain'],text=True),'BLOCKED: driver has user changes'
branch=subprocess.check_output(['/usr/bin/git','-C',repo,'branch','--show-current'],text=True).strip()
assert branch=='infra/cteas-resource-control-live-20261012','BLOCKED: driver deployment branch changed'
assert pathlib.Path('/home/lhy/cteas-resource-audit/driver-before.txt').exists()
PYCODE
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
python3 - <<'PYCODE'
import pathlib,subprocess
old=pathlib.Path('/home/lhy/cteas-resource-audit/driver-before.txt').read_text().splitlines()[0]
subprocess.run(['/usr/bin/git','-C','/home/lhy/gate/driver','switch','--detach',old],check=True)
PYCODE
printf '%s\n' 'Explicit rollback restored original Runner limits, Gate slice and driver code. Original direct Gate behavior is restored.' 
