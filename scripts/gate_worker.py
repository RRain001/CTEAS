#!/usr/bin/env python3
"""Per-job worker, run as lhy in root-defined cgroups; no persistent daemon."""
from __future__ import annotations
import fcntl
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
from resource_limits import metrics, cgroup_path

ROOT = Path('/home/lhy/gate-managed')
BOOT = Path('/proc/sys/kernel/random/boot_id').read_text().strip()


def identity(pid: int) -> str:
    return Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()[19]


def alive(state: dict) -> bool:
    try:
        return state['boot'] == BOOT and identity(state['pid']) == state['start_identity']
    except (OSError, KeyError, TypeError):
        return False


def write(path: Path, value: dict) -> None:
    temp = path.with_suffix('.tmp')
    temp.write_text(json.dumps(value, indent=2))
    temp.replace(path)


def admit(active: list[dict], request: dict) -> bool:
    return (all(r['track'] != request['track'] for r in active)
            and sum(r['cpus'] for r in active) + request['cpus'] <= 6
            and sum(r['memory_gib'] for r in active) + request['memory_gib'] <= 15)


def main() -> int:
    ident = sys.argv[1]
    directory = ROOT / 'requests' / ident
    request = json.loads((directory / 'request.json').read_text())
    state = dict(phase='queued', pid=os.getpid(), boot=BOOT, start_identity=identity(os.getpid()),
                 created=request['created'], queued_at=time.time(), **{k:request[k] for k in ('track', 'cpus', 'memory_gib')})
    write(directory / 'state.json', state)
    rc = 1
    try:
        with (ROOT / 'queue.lock').open('a') as lock:
            while True:
                fcntl.flock(lock, fcntl.LOCK_EX)
                active, waiting = [], []
                for path in (ROOT / 'requests').glob('*/state.json'):
                    other = json.loads(path.read_text())
                    if other['phase'] in ('queued', 'running'):
                        if not alive(other):
                            other.update(phase='failed', returncode=125, reason='worker lost or host rebooted')
                            write(path, other)
                        elif other['phase'] == 'running':
                            active.append(other)
                        else:
                            waiting.append((other['created'], path.parent.name))
                # Strict FIFO prevents large regressions starving behind small jobs.
                if waiting and min(waiting)[1] == ident and admit(active, request):
                    state.update(phase='running', started=time.time())
                    write(directory / 'state.json', state)
                    fcntl.flock(lock, fcntl.LOCK_UN)
                    break
                fcntl.flock(lock, fcntl.LOCK_UN)
                time.sleep(1)
        directory.joinpath('resource-before.json').write_text(json.dumps(metrics(), indent=2))
        cg = cgroup_path()
        # Verify leaf and aggregate pool limits before any compiler is launched.
        expected = request['cpus'] * 1000
        quota, period = (cg / 'cpu.max').read_text().split() if cg else ('max', '1')
        if quota == 'max' or int(quota) / int(period) > request['cpus']:
            raise RuntimeError('Effective CPU limit missing')
        if int((cg / 'memory.max').read_text()) != request['memory_gib'] * 1024**3:
            raise RuntimeError('Effective memory limit mismatch')
        pool = Path('/sys/fs/cgroup/cteas.slice/cteas-gate.slice')
        if (pool / 'cpu.max').read_text().split()[0] == 'max' or int((pool / 'memory.max').read_text()) != 16 * 1024**3:
            raise RuntimeError('Shared Gate pool controls missing')
        env = os.environ.copy()
        env.update(request['environment'])
        env.update(CTEAS_GATE_MANAGED=ident)
        command = ['/home/lhy/CTEAS/.venv/bin/python3.11', '/usr/local/lib/cteas-gate/local_gate.py', *request['args']]
        if request['keep']:
            command.append('--keep')
        with (directory / 'gate.log').open('w') as log:
            rc = subprocess.call(command, env=env, stdout=log, stderr=subprocess.STDOUT)
        if request.get('report'):
            # Never overwrite another run's explicit report path.
            target = Path(request['report'])
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('x') as f:
                f.write((directory / 'evidence.json').read_text())
        state.update(phase='done' if rc == 0 else 'failed', returncode=rc)
    except BaseException as e:
        state.update(phase='failed', returncode=rc or 1, error=type(e).__name__, reason=str(e))
    finally:
        state.update(finished=time.time(), resources=metrics())
        write(directory / 'state.json', state)
    return state['returncode']


if __name__ == '__main__':
    # systemd also terminates the entire process tree. The state update releases admission.
    signal.signal(signal.SIGTERM, lambda *_: (_ for _ in ()).throw(KeyboardInterrupt('service stopped')))
    sys.exit(main())
