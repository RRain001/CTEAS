#!/usr/bin/env python3
"""Bounded connectivity diagnostics through the explicitly configured proxy."""
import json
import time
import urllib.error
import urllib.request
import runnerctl as ctl

opener = urllib.request.build_opener(urllib.request.ProxyHandler({'https': ctl.PROXY} if ctl.PROXY else {}))
results = []
for round_number in range(1, 4):
    for url, method in [('https://api.github.com/', 'GET'),
                        ('https://github.com/login/device', 'GET'),
                        ('https://github.com/login/oauth/access_token', 'POST')]:
        start = time.monotonic()
        try:
            request = urllib.request.Request(url, method=method, data=b'' if method == 'POST' else None)
            with opener.open(request, timeout=5) as response:
                status = response.status
            connected = True
        except urllib.error.HTTPError as e:
            status, connected = e.code, True  # Invalid unauthenticated OAuth POST should return HTTP error.
        except (urllib.error.URLError, TimeoutError):
            status, connected = None, False
        results.append(dict(round=round_number, url=url, http_status=status, connected=connected,
                            seconds=round(time.monotonic() - start, 3)))
report = dict(proxy=ctl.PROXY, checks=results, passed=all(r['connected'] for r in results))
ctl.HERE.joinpath('logs').mkdir(exist_ok=True)
ctl.HERE.joinpath('logs/network-latest.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
raise SystemExit(0 if report['passed'] else 1)
