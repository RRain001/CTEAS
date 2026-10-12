#!/usr/bin/env python3
"""Submit resource-budgeted Gate tasks; SSH loss leaves the service running."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import uuid

ROOT = Path('/home/lhy/gate-managed')
CONTROL = '/usr/local/sbin/cteas-gate-control'


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--track', required=True, choices=['p6d', 'p6e', 'p6f', 'p7', 'shared'])
    parser.add_argument('--ref')
    parser.add_argument('--repo', default='/home/lhy/gate/driver')
    parser.add_argument('--report')
    parser.add_argument('--keep', action='store_true')
    parser.add_argument('--cpus', type=int, choices=[2, 4, 6], default=6)
    parser.add_argument('--memory-gib', type=int, choices=range(2, 16), default=15)
    parser.add_argument('--detach', action='store_true')
    args = parser.parse_args()
    ident = uuid.uuid4().hex
    directory = ROOT / 'requests' / ident
    gate_args = ['--track', args.track, '--repo', str(Path(args.repo).resolve()),
                 '--report', str(directory / 'evidence.json')]
    import re
    import fcntl
    if not (args.ref and re.fullmatch('[0-9a-fA-F]{40}', args.ref)):
        # Fetch and resolve under the same driver's short-lived Git lock.
        with Path('/home/lhy/gate/driver-git.lock').open('a') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            subprocess.run(['/usr/bin/git', '-C', args.repo, 'fetch', '--prune', '--quiet', 'origin',
                            '+refs/heads/*:refs/remotes/origin/*'], check=True, timeout=120)
    if args.ref:
        # Freeze the SHA before submission; moving remote refs never change queued jobs.
        sha = subprocess.check_output(['/usr/bin/git', '-C', args.repo, 'rev-parse', args.ref + '^{commit}'], text=True).strip()
        gate_args += ['--ref', sha]
    else:
        from local_gate import TRACKS
        sha = subprocess.check_output(['/usr/bin/git', '-C', args.repo, 'rev-parse', TRACKS[args.track]['branch'] + '^{commit}'], text=True).strip()
        gate_args += ['--ref', sha]
    request = dict(id=ident, track=args.track, cpus=args.cpus, memory_gib=args.memory_gib,
                   args=gate_args, created=time.time(), report=args.report, keep=args.keep,
                   environment={k:os.environ[k] for k in ('CTEAS_BUILD_JOBS', 'CTEAS_GATE_CXX', 'CTEAS_GATE_FC') if k in os.environ})
    directory.mkdir(parents=True, mode=0o700)
    (directory / 'request.json').write_text(json.dumps(request, indent=2))
    (directory / 'state.json').write_text(json.dumps(dict(phase='submitted', created=request['created'])))
    result = subprocess.run(['sudo', '-n', CONTROL, ident, str(args.cpus), str(args.memory_gib)])
    if result.returncode:
        (directory / 'state.json').write_text(json.dumps(dict(phase='failed', returncode=result.returncode)))
        return result.returncode
    print(f'GATE_SUBMITTED id={ident} state={directory}/state.json evidence={directory}/evidence.json', flush=True)
    if args.detach:
        return 0
    while True:
        try:
            state = json.loads((directory / 'state.json').read_text())
        except (OSError, ValueError):
            time.sleep(1)
            continue
        if state['phase'] in ('queued', 'running'):
            from gate_worker import alive, write
            if not alive(state):
                import fcntl
                with (ROOT / 'queue.lock').open('a') as lock:
                    fcntl.flock(lock, fcntl.LOCK_EX)
                    state = json.loads((directory / 'state.json').read_text())
                    if state['phase'] in ('queued', 'running') and not alive(state):
                        state.update(phase='failed', returncode=125, reason='worker lost or host rebooted')
                        write(directory / 'state.json', state)
        if state['phase'] in ('done', 'failed'):
            print(json.dumps(state), flush=True)
            return state['returncode']
        time.sleep(1)


if __name__ == '__main__':
    sys.exit(main())
