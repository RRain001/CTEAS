#!/usr/bin/env python3
"""Apply/rollback only the three original resource drop-ins; never network config."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

ROOT = Path('/home/lhy/cteas-resource-audit')
NAMES = {1:'lhy-desktop',2:'lhy-desktop-2',3:'lhy-desktop-3'}
CONTENT = '''[Service]
CPUAccounting=yes
MemoryAccounting=yes
CPUQuota=400%
MemoryHigh=7G
MemoryMax=8G
MemorySwapMax=0
TasksMax=512
OOMPolicy=kill
KillMode=control-group
Environment=CTEAS_BUILD_JOBS=4
Environment=CMAKE_BUILD_PARALLEL_LEVEL=4
Environment=CTEST_PARALLEL_LEVEL=4
Environment=OMP_NUM_THREADS=4
'''


def run(args, **kwargs):
    return subprocess.check_output(args, text=True, **kwargs)


def api():
    # Preserve configured transport; this does not change any proxy setting.
    env=os.environ.copy();env.update(HTTPS_PROXY='http://127.0.0.1:7897',HTTP_PROXY='http://127.0.0.1:7897')
    return json.loads(run(['gh','api','repos/RRain001/CTEAS/actions/runners'],env=env))['runners']


def main():
    p=argparse.ArgumentParser();p.add_argument('action',choices=['apply','rollback']);p.add_argument('--runner',type=int,choices=[1,2,3],required=True);a=p.parse_args()
    name=NAMES[a.runner];unit=f'actions.runner.RRain001-CTEAS.{name}.service';path=Path('/etc/systemd/system')/(unit+'.d')/'cteas-resources.conf'
    ROOT.mkdir(exist_ok=True,mode=0o700);backup=ROOT/(name+'-before.json')
    remote=next(r for r in api() if r['name']==name)
    if remote['busy']:raise SystemExit('BLOCKED: runner busy; no restart performed')
    action_prior_disk=path.read_text() if path.exists() else None
    if a.action=='apply':
        if not backup.exists():
            original=path.read_text() if path.exists() else None
            snapshot=dict(id=remote['id'],resource_dropin=original,unit=unit,
                          service=run(['systemctl','cat',unit]),before=run(['systemctl','show',unit,'-p','CPUQuotaPerSecUSec','-p','MemoryHigh','-p','MemoryMax','-p','User','-p','WorkingDirectory']),
                          network_sha256=hashlib.sha256((path.parent/'cteas-network.conf').read_bytes()).hexdigest())
            backup.write_text(json.dumps(snapshot,indent=2));backup.chmod(0o600)
        run(['sudo','-n','mkdir','-p',str(path.parent)])
        run(['sudo','-n','tee',str(path)],input=CONTENT)
    else:
        original=json.loads(backup.read_text())['resource_dropin']
        if original is None:
            if path.exists():run(['sudo','-n','unlink',str(path)])
        else:run(['sudo','-n','tee',str(path)],input=original)
    # Recheck immediately before restart and reverse the disk change if it became busy.
    if next(r for r in api() if r['name']==name)['busy']:
        prior=action_prior_disk
        if prior is None and path.exists():run(['sudo','-n','unlink',str(path)])
        elif prior is not None:run(['sudo','-n','tee',str(path)],input=prior)
        raise SystemExit('BLOCKED: runner became busy; original disk config restored')
    run(['sudo','-n','systemctl','daemon-reload']);run(['sudo','-n','systemctl','restart',unit])
    for _ in range(30):
        r=next(r for r in api() if r['name']==name)
        if r['status']=='online':break
        time.sleep(2)
    assert r['id']==json.loads(backup.read_text())['id'] and r['status']=='online'
    assert hashlib.sha256((path.parent/'cteas-network.conf').read_bytes()).hexdigest()==json.loads(backup.read_text())['network_sha256']
    print(name,a.action,r['status'],r['id']);print(run(['systemctl','show',unit,'-p','CPUQuotaPerSecUSec','-p','MemoryHigh','-p','MemoryMax']))


if __name__=='__main__':main()
