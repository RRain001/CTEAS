"""Linux cgroups v2 resource discovery; no global configuration changes."""
from __future__ import annotations
import math
import os
from pathlib import Path


def cpu_set(value: str) -> set[int]:
    result = set()
    for part in value.strip().split(','):
        if not part:
            continue
        lo, sep, hi = part.partition('-')
        result.update(range(int(lo), int(hi) + 1) if sep else [int(lo)])
    return result


def cgroup_path() -> Path | None:
    try:
        relative = next(line[3:] for line in Path('/proc/self/cgroup').read_text().splitlines() if line.startswith('0::'))
        for line in Path('/proc/self/mountinfo').read_text().splitlines():
            if ' - cgroup2 ' in line:
                fields = line.split()
                mount_root, mount = fields[3], Path(fields[4].replace('\\040', ' '))
                if mount_root != '/' and relative.startswith(mount_root + '/'):
                    relative = relative[len(mount_root):]
                candidate = mount / relative.lstrip('/')
                if '..' not in candidate.parts and candidate.is_dir():
                    return candidate
    except (OSError, StopIteration):
        return None
    return None


def detect(request: str | None = None, directory: Path | None = None,
           affinity: set[int] | None = None) -> dict:
    if affinity is None:
        try:
            affinity = set(os.sched_getaffinity(0))
        except (AttributeError, OSError):
            affinity = set(range(4))  # safe default; never trust os.cpu_count()
    available = len(affinity) or 1
    directory = directory if directory is not None else cgroup_path()
    quota = None
    memory = None
    paths = []
    if directory:
        current = directory
        # Ancestor limits also apply. Stop at the cgroup filesystem root.
        while (current / 'cgroup.controllers').exists():
            paths.append(str(current))
            try:
                q, period = (current / 'cpu.max').read_text().split()
                if q != 'max':
                    value = int(q) / int(period)
                    quota = value if quota is None else min(quota, value)
            except (OSError, ValueError, ZeroDivisionError):
                pass
            try:
                allowed = cpu_set((current / 'cpuset.cpus.effective').read_text())
                if allowed:
                    available = min(available, len(affinity & allowed) or 1)
            except (OSError, ValueError):
                pass
            for name in ('memory.high', 'memory.max'):
                try:
                    value = (current / name).read_text().strip()
                    if value != 'max':
                        memory = int(value) if memory is None else min(memory, int(value))
                except (OSError, ValueError):
                    pass
            current = current.parent
    cap = min(available, max(1, math.floor(quota))) if quota is not None else available
    memory_jobs = max(1, (memory - 512 * 1024**2) // (768 * 1024**2)) if memory is not None else None
    if memory_jobs is not None:
        cap = min(cap, memory_jobs)
    requested = None
    if request is not None:
        requested = int(request)
        if requested < 1:
            raise ValueError('CTEAS_BUILD_JOBS must be a positive integer')
    jobs = min(requested, cap) if requested is not None else (min(cap, 8) if quota is None else cap)
    return dict(build_jobs=jobs, requested_jobs=requested, cpu_quota=quota,
                affinity_cpus=available, memory_budget_bytes=memory,
                memory_jobs=memory_jobs, cgroup=str(directory) if directory else None,
                ancestors=paths)


def metrics(directory: Path | None = None) -> dict:
    directory = directory or cgroup_path()
    out = {}
    if directory:
        for name in ('cpu.max', 'memory.max', 'memory.high', 'memory.peak', 'cpu.stat', 'memory.events'):
            try:
                out[name] = (directory / name).read_text().strip()
            except OSError:
                out[name] = None
    return out
