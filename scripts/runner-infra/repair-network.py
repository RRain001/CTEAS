#!/usr/bin/env python3
"""Apply only proxy/recovery settings to the three original services."""
from pathlib import Path
import runnerctl as ctl

if not ctl.PROXY:
    raise SystemExit('Explicit deployment.json proxy required')
for name, directory in [('lhy-desktop', '/home/lhy/actions-runner'),
                        ('lhy-desktop-2', '/home/lhy/actions-runner-2'),
                        ('lhy-desktop-3', '/home/lhy/actions-runner-3')]:
    remote = next((r for r in ctl.inventory() if r['name'] == name), None)
    if not remote or remote['busy']:
        print(f'{name}: missing or busy; network change deferred')
        continue
    unit = f'actions.runner.RRain001-CTEAS.{name}.service'
    wd = ctl.run(['systemctl', 'show', unit, '-p', 'WorkingDirectory', '--value']).strip()
    if wd != directory:
        raise SystemExit('Service directory mismatch; refusing change')
    content = f'''[Unit]
Wants=network-online.target
After=network-online.target
StartLimitIntervalSec=0
[Service]
Environment="http_proxy={ctl.PROXY}" "https_proxy={ctl.PROXY}" "HTTP_PROXY={ctl.PROXY}" "HTTPS_PROXY={ctl.PROXY}" "no_proxy=localhost,127.0.0.1,::1" "NO_PROXY=localhost,127.0.0.1,::1"
Restart=always
RestartSec=10
'''
    path = Path('/etc/systemd/system') / (unit + '.d') / 'cteas-network.conf'
    if path.exists() and path.read_text() == content:
        print(f'{name}: network configuration already applied')
        continue
    ctl.write_root(path, content)
    ctl.run(['sudo', '-n', 'systemctl', 'daemon-reload'])
    ctl.run(['sudo', '-n', 'systemctl', 'restart', unit])
    print(f'{name}: explicit proxy and recovery configured; resources untouched')
