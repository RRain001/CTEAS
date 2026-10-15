#!/usr/bin/env python3
"""Fail closed if any executable CTEAS Actions job needs a retired local runner."""
from __future__ import annotations

from pathlib import Path
import sys
import yaml

root = Path(".github/workflows")
paths = sorted((*root.glob("*.yml"), *root.glob("*.yaml")))
failures: list[str] = []
count = 0
for path in paths:
    workflow = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not isinstance(workflow, dict):
        failures.append(f"{path}: missing YAML workflow mapping")
        continue
    jobs = workflow.get("jobs")
    if not isinstance(jobs, dict):
        failures.append(f"{path}: missing jobs mapping")
        continue
    for name, job in jobs.items():
        count += 1
        if not isinstance(job, dict):
            failures.append(f"{path}:{name}: job must be a mapping")
            continue
        runner = job.get("runs-on")
        if runner is None and "uses" in job:  # legitimate reusable workflow
            continue
        if not isinstance(runner, str) or not runner.startswith(
            ("ubuntu-", "windows-", "macos-")
        ):
            failures.append(
                f"{path}:{name}: non-GitHub-hosted/unknown runs-on={runner!r}"
            )
if len(paths) < 70:
    failures.append(f"Expected >=70 tracked workflows, found {len(paths)}")
if failures:
    for failure in failures:
        print("HOSTED_RUNNER_POLICY_BLOCKED " + failure, file=sys.stderr)
    raise SystemExit(1)
print(
    f"CTEAS_GITHUB_HOSTED_RUNNER_POLICY=PASS WORKFLOWS={len(paths)}"
    f" JOBS={count} SELF_HOSTED=0"
)
