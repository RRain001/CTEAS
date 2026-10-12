#!/usr/bin/python3
"""Root-owned fixed launcher: arbitrary root commands/properties are forbidden."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys


def main():
    if os.geteuid() != 0 or len(sys.argv) != 4:
        raise SystemExit('Usage: sudo cteas-gate-control ID CPU MEMORY_GIB')
    ident, cpu, mem = sys.argv[1:]
    if not re.fullmatch('[0-9a-f]{32}', ident) or cpu not in ('2', '4', '6') or not mem.isdecimal() or not 2 <= int(mem) <= 15:
        raise SystemExit('Invalid bounded request')
    directory = Path('/home/lhy/gate-managed/requests') / ident
    if directory.is_symlink() or not directory.is_dir():
        raise SystemExit('Invalid request directory')
    request = json.loads((directory / 'request.json').read_text())
    if request['cpus'] != int(cpu) or request['memory_gib'] != int(mem):
        raise SystemExit('Request/resource mismatch')
    pending = 0
    for path in directory.parent.glob('*/state.json'):
        state = json.loads(path.read_text())
        if state['phase'] in ('submitted', 'queued', 'running'):
            pending += 1
    if pending > 32:
        raise SystemExit('Queue capacity exceeded')
    props = dict(User='lhy', Group='lhy', Slice='cteas-gate.slice', CPUQuota=f'{int(cpu)*100}%',
                 MemoryHigh=f'{max(1,int(mem)-1)}G', MemoryMax=f'{mem}G', MemorySwapMax='0',
                 TasksMax='512', OOMPolicy='kill', KillMode='control-group',
                 NoNewPrivileges='yes', ProtectControlGroups='yes', UMask='0077',
                 RemainAfterExit='yes', TimeoutStopSec='30')
    cmd = ['systemd-run', '--unit=cteas-gate-'+ident, '--quiet']
    for k,v in props.items():
        cmd += ['--property', f'{k}={v}']
    cmd += ['/usr/bin/python3', '/usr/local/lib/cteas-gate/gate_worker.py', ident]
    subprocess.run(cmd, check=True)


if __name__ == '__main__':
    main()
