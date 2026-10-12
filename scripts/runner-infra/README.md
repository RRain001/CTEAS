# CTEAS Runner infrastructure

Host: lhy-desktop, Ubuntu x86_64, cgroups v2. Repo: private RRain001/CTEAS.
Existing runners 1–3 retain their registrations, labels, directories and resource
settings. Their network drop-ins now use the existing Mac-shared proxy.

## Layout and resource budgets

| Runner | Pool label | CPU time quota | MemoryMax | Account |
|---|---|---:|---:|---|
| 4–11 | cteas-light | 200% each | 3 GiB each | cteas-r04 … cteas-r11 |
| 12–14 | cteas-compute | 600% each | 12 GiB each | cteas-r12 … cteas-r14 |
| 15–16 | cteas-reserve | 400% each | 6 GiB each | cteas-r15 … cteas-r16 |

Each directory is `/opt/cteas-runners/runner-NN`; `_work` is private (0700).
Service: `actions.runner.RRain001-CTEAS.lhy-desktop-N.service`.
Custom labels: `cteas-managed`, pool label, `cteas-rNN`, `Linux`, `X64`.
Default `self-hosted` label is deliberately omitted: old generic workflows must
not accidentally take new pool resources. These are logical label pools, not
organization runner groups.

Total new quota: 42 CPU-seconds per second (logical CPU time, **not** 42 reserved
physical cores), 72 GiB hard memory ceiling. MemorySwapMax=0, MemoryHigh one GiB
below ceiling, TasksMax=512, OOMPolicy=kill, KillMode=control-group.
Dedicated unprivileged accounts have no sudo rights; NoNewPrivileges and
read-only cgroup access prevent normal jobs from moving to other service groups.
Compute 12/14 prefer NUMA node 0; 13 prefers node 1, with AllowedCPUs taken from
actual sysfs topology. Quota still limits usage within the selected node.

`CMAKE_BUILD_PARALLEL_LEVEL`, `CTEST_PARALLEL_LEVEL`, `OMP_NUM_THREADS`,
`OPENBLAS_NUM_THREADS`, `MKL_NUM_THREADS`, `MAKEFLAGS` match each CPU quota.
Explicit `cmake --parallel 24` overrides the hint. Cgroups still enforce CPU and
memory limits, but oversubscription can slow such commands. For new workflows
use `cmake --build build --parallel "$CMAKE_BUILD_PARALLEL_LEVEL"`.

## Maintenance on the server

Installed scripts: `/home/lhy/cteas-runner-infra` (outside scientific checkouts).
Requires Python >=3.10, gh, sudo -n, systemd, an authenticated repository admin.
`gh` is `/usr/local/bin/gh` on this host. Never run installer as root; it creates
system accounts and services via narrowly scoped sudo commands.

```bash
cd /home/lhy/cteas-runner-infra
./install-runners.sh --start 4 --end 16 --retries 3
./install-runners.sh --runner 12
./check-runners.sh
./restart-runners.sh --runner 12
./remove-runner.sh --runner 16
python3 repair-network.py
```

Only numbers 4–16 are accepted by install/restart/remove. Busy runners are not
restarted/removed. Existing directory/registration mismatches fail closed; there
is no --replace and no recursive deletion. Registration precedes service creation;
a service setup failure disables/stops that new service. Retrying recovers a
valid registration without overwriting it. Uninstall preserves work, logs,
installation and dedicated account. Do not delete these yourself if outputs are
still needed. Repository runner APIs are paginated.

Official latest Linux x64 release is selected at install time and SHA256 checked
against the official release asset digest. Download is cached once. Running
runners keep GitHub's default auto-update enabled; rerunning installation does
not force upgrades. A non-root installer lock prevents overlapping deployments.
Registration/removal tokens are obtained automatically from GitHub and passed
through stdin to a child process, then to Runner's supported masked input env;
never placed in command-line arguments or deployment logs. Auth credentials
remain in gh's credential store, separate from this directory.

Deployment logs: `logs/deploy-YYYYMMDD.log`; snapshots: `logs/status-*.json`.
The archive cache is not source code and should not be committed.

## Proxy and authentication

`deployment.json` is explicit local configuration:

```json
{"proxy": "http://127.0.0.1:7897"}
```

Use an empty string to request direct networking on a different host. This
installation uses the already existing reverse SSH tunnel to the Mac proxy;
no new proxy daemon or system-wide network modification is introduced.
HTTP(S)_PROXY and their lowercase counterparts are set in service drop-ins;
no_proxy covers localhost, 127.0.0.1 and ::1. Listener auto-reconnect plus
Restart=always/RestartSec=10 and disabled start-rate limits allow recovery when
the proxy returns. All runner units are enabled for boot. Runner 1–3's network
repair is a separate script and does not alter their resource settings.

The Mac must stay awake, connected, and provide its existing proxy/SSH tunnel.
This cannot provide GitHub connectivity while the Mac/tunnel is unavailable.
The server does not need a browser; device authorization can be completed on Mac.
On this headless host the desktop keyring blocked even --insecure-storage because
of gh's active-user keyring lookup. With explicit user approval, authentication
used --insecure-storage and an invalid DBUS_SESSION_BUS_ADDRESS **only for the
login process**; credentials are owned by lhy with mode 0600. This is plaintext
storage accessible to lhy/root, not an encrypted keyring. Never print hosts.yml.

## Scheduling and acceptance

Actual API Actions permission (2026-10-12): enabled=true; do not toggle it.
`origin/main` local_gate.py says Actions is disabled; this comment is stale.
Existing P6D/P6E/P7 labels/workflows remain unchanged. Existing general workflows
retain their original runners until a separate reviewed migration is adopted.
New workflows should use `[cteas-managed, cteas-light, Linux, X64]`, or replace
light by compute/reserve. Their pools have disjoint accounts and service limits,
so light jobs cannot occupy reserved runner slots. Keep finite timeout-minutes
and concurrency keyed by workflow + PR/head branch; avoid duplicate push/PR
triggers. Labels do not provide preemption or a fairness guarantee within a pool.

The infrastructure branch changes only the existing smoke workflow's pool and
adds a manually selected `runner_acceptance=true` mode. Its 13 jobs target
individual `cteas-rNN` labels to check all new runners. It checks directory/user
isolation, compiler smoke, private cgroup ceilings, network and no sudo, and
keeps jobs occupied for 30 seconds to measure real overlap. It does not checkout
or run scientific code in acceptance mode. The ordinary smoke remains available
with acceptance=false. Commits use [skip ci] to avoid unrelated automatic
regressions; manual workflow_dispatch runs are explicitly requested. The PR is
left unmerged.

Direct Codex/ChatGPT SSH commands and `scripts/local_gate.py` do **not** run in
runner service cgroups automatically. This deployment neither modifies local_gate
nor constrains existing user sessions. Concurrent local gates still need explicit
resource-controlled execution and distinct output roots. Do not assume extra
GitHub runners themselves create a local gate scheduler. Existing runners 1–3
remain unlimited, so the new limits alone cannot guarantee global OOM protection
against those old services or unrelated local workloads.

For a fresh copy, copy `deployment.example.json` to `deployment.json` and review
its proxy endpoint before installation. `python3 check-network.py` performs
three bounded rounds against API, login and OAuth endpoints and writes
`logs/network-latest.json`; OAuth HTTP 404 from an empty POST is a connection
response, not proof of an authenticated OAuth operation.

Maintenance creates root-owned systemd unit files directly using GitHub's
required `runsvc.sh` entry point; it never executes runner-writable `svc.sh` as
root. Remove disables/unlinks only the requested new unit and unregisters that
runner; source/work/log data remain intact. Syntax and protected-range guards
were checked; successful registration exercised the install path. Destructive
unregister/removal was not exercised on production runners.

Acceptance 2026-10-12: Actions run 38189867178, 13/13 passed, measured peak 13
simultaneous jobs. Explicit test compilers: GCC 12.3.0 and GNU Fortran 11.4.0;
CMake 3.22.1. Default g++ 9.5/Fortran 4.7 are retained for unrelated workloads;
workflows must use the repository's isolated toolchain action or explicit CXX/FC.
First acceptance attempt failed because it used default g++ with -std=c++20;
this was corrected only on the infrastructure branch, and the full run repeated.
