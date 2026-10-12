#!/usr/bin/env python3
"""Repository-scoped runner management. Credentials remain in gh's store."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import pwd
import subprocess
import sys
import tarfile
import time
import urllib.request

REPO = "RRain001/CTEAS"
ROOT = Path('/opt/cteas-runners')
HERE = Path(__file__).resolve().parent
GH = '/usr/local/bin/gh'
SETTINGS = json.loads((HERE / 'deployment.json').read_text()) if (HERE / 'deployment.json').exists() else {}
PROXY = SETTINGS.get('proxy', '')


def run(args, **kw):
    env = os.environ.copy()
    if PROXY:
        env.update(http_proxy=PROXY, https_proxy=PROXY, HTTP_PROXY=PROXY, HTTPS_PROXY=PROXY,
                   no_proxy='localhost,127.0.0.1,::1', NO_PROXY='localhost,127.0.0.1,::1')
    kw.setdefault('env', env)
    result = subprocess.run(args, capture_output=True, text=True, **kw)
    if result.returncode:
        # Never echo arbitrary subprocess output (registration can contain secrets).
        raise RuntimeError(f'{Path(args[0]).name} failed (exit {result.returncode})')
    return result.stdout


def api(endpoint, method='GET'):
    out = run([GH, 'api', '--method', method, endpoint])
    return json.loads(out) if out.strip() else None


def inventory():
    runners = []
    page = 1
    while True:
        batch = api(f'repos/{REPO}/actions/runners?per_page=100&page={page}')['runners']
        runners.extend(batch)
        if len(batch) < 100:
            return runners
        page += 1


def spec(i):
    if not 4 <= i <= 16:
        raise ValueError('Only newly requested runners 4..16 may be managed')
    pool, cpu, mem = ('light', 2, 3) if i <= 11 else ('compute', 6, 12) if i <= 14 else ('reserve', 4, 6)
    return dict(index=i, name=f'lhy-desktop-{i}', directory=str(ROOT / f'runner-{i:02}'),
                user=f'cteas-r{i:02}', pool=pool, cpu=cpu, memory_gib=mem,
                labels=['cteas-managed', f'cteas-{pool}', f'cteas-r{i:02}', 'Linux', 'X64'])


def service(s):
    return f'actions.runner.RRain001-CTEAS.{s["name"]}.service'


def configured(s):
    d = Path(s['directory'])
    if not (d / '.cteas-managed.json').exists():
        raise RuntimeError(f'{d}: ownership marker absent; refusing changes')
    if json.loads((d / '.cteas-managed.json').read_text()) != s:
        raise RuntimeError(f'{d}: ownership marker mismatch')
    if (d / '.runner').exists():
        r = json.loads((d / '.runner').read_text(encoding='utf-8-sig'))
        if r['agentName'] != s['name'] or r['gitHubUrl'] != f'https://github.com/{REPO}' or r['workFolder'] != '_work':
            raise RuntimeError('Existing runner configuration mismatch')
        return r
    return None


def account(s):
    try:
        p = pwd.getpwnam(s['user'])
        if p.pw_dir != s['directory'] or p.pw_shell != '/usr/sbin/nologin':
            raise RuntimeError('Account already exists with incompatible settings')
    except KeyError:
        run(['sudo', '-n', 'useradd', '--system', '--user-group', '--home-dir', s['directory'],
             '--shell', '/usr/sbin/nologin', s['user']])
    run(['sudo', '-n', 'chown', '-R', f'{s["user"]}:{s["user"]}', s['directory']])


def config_command(s, args, token):
    # Token is provided via stdin to sudo's child, never argv, files or logs.
    script = "import os,subprocess,sys; e=os.environ.copy(); e['ACTIONS_RUNNER_INPUT_TOKEN']=sys.stdin.readline().strip(); p=sys.argv[1]; e.update({'http_proxy':p,'https_proxy':p,'HTTP_PROXY':p,'HTTPS_PROXY':p,'no_proxy':'localhost,127.0.0.1,::1'}) if p else None; sys.exit(subprocess.call(sys.argv[2:],env=e,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL))"
    run(['sudo', '-n', '-u', s['user'], 'python3', '-c', script, PROXY, './config.sh', *args],
        cwd=s['directory'], input=token + '\n', timeout=180)


def write_root(path, content):
    run(['sudo', '-n', 'mkdir', '-p', str(path.parent)])
    run(['sudo', '-n', 'tee', str(path)], input=content)


def limits(s):
    unit = service(s)
    # Separate Unix accounts prevent jobs escaping limits through another runner.
    content = f'''[Unit]
Wants=network-online.target
After=network-online.target
StartLimitIntervalSec=0
[Service]
CPUAccounting=yes
MemoryAccounting=yes
CPUQuota={s['cpu'] * 100}%
MemoryHigh={max(1, s['memory_gib'] - 1)}G
MemoryMax={s['memory_gib']}G
MemorySwapMax=0
TasksMax=512
OOMPolicy=kill
KillMode=control-group
Restart=always
RestartSec=10
NoNewPrivileges=yes
ProtectControlGroups=yes
RestrictSUIDSGID=yes
UMask=0077
Environment=CMAKE_BUILD_PARALLEL_LEVEL={s['cpu']}
Environment=CTEST_PARALLEL_LEVEL={s['cpu']}
Environment=OMP_NUM_THREADS={s['cpu']}
Environment=OPENBLAS_NUM_THREADS={s['cpu']}
Environment=MKL_NUM_THREADS={s['cpu']}
Environment=MAKEFLAGS=-j{s['cpu']}
'''
    if PROXY:
        content += f'Environment="http_proxy={PROXY}" "https_proxy={PROXY}" "HTTP_PROXY={PROXY}" "HTTPS_PROXY={PROXY}" "no_proxy=localhost,127.0.0.1,::1" "NO_PROXY=localhost,127.0.0.1,::1"\n'
    if s['pool'] == 'compute':
        node = (s['index'] - 12) % 2
        cpulist = Path(f'/sys/devices/system/node/node{node}/cpulist')
        if cpulist.exists():
            content += f'AllowedCPUs={cpulist.read_text().strip()}\nNUMAPolicy=preferred\nNUMAMask={node}\n'
    write_root(Path('/etc/systemd/system') / (unit + '.d') / 'resources.conf', content)
    run(['sudo', '-n', 'systemctl', 'daemon-reload'])
    run(['sudo', '-n', 'systemctl', 'enable', '--now', unit])


def package():
    cache = HERE / 'cache'
    cache.mkdir(exist_ok=True)
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({'http': PROXY, 'https': PROXY} if PROXY else {}))
    release = json.load(opener.open('https://api.github.com/repos/actions/runner/releases/latest', timeout=30))
    asset = next(a for a in release['assets'] if a['name'].startswith('actions-runner-linux-x64-'))
    expected = asset.get('digest', '')
    if not expected.startswith('sha256:'):
        raise RuntimeError('Official release missing SHA256 digest')
    target = cache / asset['name']
    if not target.exists():
        temp = target.with_suffix('.partial')
        import shutil
        with opener.open(asset['browser_download_url'], timeout=120) as response, temp.open('wb') as output:
            shutil.copyfileobj(response, output)
        temp.rename(target)
    with target.open('rb') as f:
        digest = hashlib.file_digest(f, 'sha256').hexdigest() if hasattr(hashlib, 'file_digest') else hashlib.sha256(f.read()).hexdigest()
    if expected != 'sha256:' + digest:
        raise RuntimeError('Runner archive checksum mismatch')
    print(f'Official runner {release["tag_name"]}; SHA256 verified', flush=True)
    return target


def install(s, archive):
    d = Path(s['directory'])
    if not d.exists():
        run(['sudo', '-n', 'mkdir', '-m', '0755', str(d)])
        write_root(d / '.cteas-managed.json', json.dumps(s, sort_keys=True))
    local = configured(s)
    remote = next((r for r in inventory() if r['name'] == s['name']), None)
    if local:
        if not remote or remote['id'] != local['agentId']:
            raise RuntimeError('Local/API registration mismatch; refusing replacement')
        if remote['busy']:
            print(f'{s["name"]}: busy, untouched', flush=True)
            return
        if not set(s['labels']).issubset({x['name'] for x in remote['labels']}):
            raise RuntimeError('Registered labels mismatch')
    elif remote:
        raise RuntimeError('Name already registered elsewhere; refusing replacement')
    if not local:
        # Extract as invoking non-root user, then assign to dedicated account.
        run(['sudo', '-n', 'chown', str(os.getuid()), str(d)])
        with tarfile.open(archive) as t:
            for m in t.getmembers():
                if m.name.startswith('/') or '..' in Path(m.name).parts:
                    raise RuntimeError('Unsafe archive path')
            t.extractall(d)
        account(s)
        token = api(f'repos/{REPO}/actions/runners/registration-token', 'POST')['token']
        try:
            config_command(s, ['--unattended', '--url', f'https://github.com/{REPO}',
                               '--name', s['name'], '--work', '_work', '--no-default-labels',
                               '--labels', ','.join(s['labels'])], token)
        except Exception:
            # Service is installed only after successful registration.
            print(f'{s["name"]}: registration failed; no service created; retry supported', flush=True)
            raise
        finally:
            token = None
        run(['sudo', '-n', 'chmod', '0600', str(d / '.credentials'), str(d / '.credentials_rsaparams')])
        run(['sudo', '-n', '-u', s['user'], 'mkdir', '-p', str(d / '_work')])
        run(['sudo', '-n', 'chmod', '0700', str(d / '_work'), str(d / '_diag')])
    # Never execute runner-writable svc.sh as root during maintenance.
    unit_path = Path('/etc/systemd/system') / service(s)
    write_root(unit_path, f'''[Unit]
Description=GitHub Actions Runner ({REPO}.{s['name']})
After=network-online.target
[Service]
Type=simple
ExecStart={d}/runsvc.sh
User={s['user']}
WorkingDirectory={d}
TimeoutStopSec=90
[Install]
WantedBy=multi-user.target
''')
    try:
        limits(s)
    except Exception:
        run(['sudo', '-n', 'systemctl', 'disable', '--now', service(s)])
        print(f'{s["name"]}: service disabled after setup failure; retry supported', flush=True)
        raise
    print(f'{s["name"]}: configured and enabled', flush=True)


def remove(s):
    configured(s)
    remote = next((r for r in inventory() if r['name'] == s['name']), None)
    if remote and remote['busy']:
        raise RuntimeError('Runner busy; refusing removal')
    d = Path(s['directory'])
    unit_path = Path('/etc/systemd/system') / service(s)
    if unit_path.exists():
        run(['sudo', '-n', 'systemctl', 'disable', '--now', service(s)])
    if (d / '.runner').exists():
        token = api(f'repos/{REPO}/actions/runners/remove-token', 'POST')['token']
        try:
            if unit_path.exists():
                run(['sudo', '-n', 'unlink', str(unit_path)])
                run(['sudo', '-n', 'systemctl', 'daemon-reload'])
            # Runner refuses config removal while its service marker remains.
            if (d / '.service').exists():
                run(['sudo', '-n', '-u', s['user'], 'unlink', str(d / '.service')])
            config_command(s, ['remove', '--unattended'], token)
        finally:
            token = None
    # Preserve logs, work and installed files; no recursive deletion.
    print(f'{s["name"]}: unregistered; all files and account preserved')


def main():
    if PROXY and not PROXY.startswith('http://127.0.0.1:'):
        raise ValueError('This deployment only accepts the documented local shared proxy')
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=['install', 'check', 'restart', 'remove', 'audit'])
    parser.add_argument('--start', type=int, default=4)
    parser.add_argument('--end', type=int, default=16)
    parser.add_argument('--runner', type=int)
    parser.add_argument('--retries', type=int, default=3)
    args = parser.parse_args()
    if args.runner is not None:
        args.start = args.end = args.runner
    specs = [spec(i) for i in range(args.start, args.end + 1)]
    if not specs:
        parser.error('Empty range')
    HERE.joinpath('logs').mkdir(exist_ok=True)
    with (HERE / 'runnerctl.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        if args.command == 'audit':
            print(json.dumps({'permissions': api(f'repos/{REPO}')['permissions'],
                              'actions': api(f'repos/{REPO}/actions/permissions'),
                              'runners': inventory()}, indent=2))
            return
        if args.command == 'check':
            remote = inventory()
            report = {'time_utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                      'total_count': len(remote), 'actions': api(f'repos/{REPO}/actions/permissions'),
                      'all_runners': remote, 'new_runners': [], 'failures': []}
            for s in specs:
                r = next((r for r in remote if r['name'] == s['name']), None)
                state = run(['systemctl', 'show', service(s), '-p', 'ActiveState', '-p', 'SubState',
                             '-p', 'UnitFileState', '-p', 'User', '-p', 'CPUQuotaPerSecUSec',
                             '-p', 'MemoryMax', '-p', 'MemorySwapMax', '-p', 'ControlGroup'])
                props = dict(line.split('=', 1) for line in state.splitlines() if '=' in line)
                local = configured(s)
                ok = bool(r and local and local['agentId'] == r['id'] and r['status'] == 'online'
                          and set(s['labels']) == {v['name'] for v in r['labels']}
                          and props.get('ActiveState') == 'active'
                          and props.get('SubState') == 'running'
                          and props.get('UnitFileState') == 'enabled'
                          and props.get('User') == s['user']
                          and props.get('MemoryMax') == str(s['memory_gib'] * 1024**3)
                          and props.get('MemorySwapMax') == '0'
                          and props.get('CPUQuotaPerSecUSec') == f'{s["cpu"]}s')
                work = Path(s['directory']) / '_work'
                ok = ok and work.is_dir() and work.stat().st_uid == pwd.getpwnam(s['user']).pw_uid and (work.stat().st_mode & 0o777) == 0o700
                if not ok:
                    report['failures'].append(s['name'])
                report['new_runners'].append({**s, 'service': service(s), 'github': r, 'systemd': state, 'passed': ok})
            dest = HERE / 'logs' / ('status-' + time.strftime('%Y%m%d-%H%M%S') + '.json')
            dest.write_text(json.dumps(report, indent=2))
            print(json.dumps(report, indent=2))
            if report['failures']:
                raise RuntimeError('Acceptance failed: ' + ', '.join(report['failures']))
            return
        run(['sudo', '-n', 'true'])
        api(f'repos/{REPO}/actions/runners')  # Fail before host mutations on missing auth.
        if args.command == 'install':
            # Verify administrative registration permission without logging the token.
            api(f'repos/{REPO}/actions/runners/registration-token', 'POST')
            run(['sudo', '-n', 'mkdir', '-p', str(ROOT)])
            archive = package()
        for s in specs:
            for attempt in range(1, args.retries + 1):
                try:
                    if args.command == 'install':
                        install(s, archive)
                    elif args.command == 'restart':
                        configured(s)
                        r = next((r for r in inventory() if r['name'] == s['name']), None)
                        if not r or r['busy']:
                            raise RuntimeError('Missing or busy runner; refusing restart')
                        run(['sudo', '-n', 'systemctl', 'restart', service(s)])
                    else:
                        remove(s)
                    break
                except Exception as e:
                    print(f'{s["name"]}: attempt {attempt} failed: {type(e).__name__}: {e}', flush=True)
                    if attempt == args.retries:
                        raise
                    time.sleep(2)


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print(f'ERROR: {type(e).__name__}: {e}', file=sys.stderr)
        sys.exit(1)
