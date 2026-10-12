#!/usr/bin/python3
"""Root-owned smoke helper: join exactly one existing cgroup, then drop privileges."""
import json
import os
from pathlib import Path
import pwd
import subprocess
import sys
import tempfile

index=int(sys.argv[1]);assert index in (1,2,3) and os.geteuid()==0
name='lhy-desktop'+('' if index==1 else f'-{index}')
cg=Path('/sys/fs/cgroup/system.slice')/f'actions.runner.RRain001-CTEAS.{name}.service'
(cg/'cgroup.procs').write_text(str(os.getpid()))
p=pwd.getpwnam('lhy');os.setgroups([]);os.setgid(p.pw_gid);os.setuid(p.pw_uid)
assert (cg/'cpu.max').read_text().split()==['400000','100000']
assert int((cg/'memory.max').read_text())==8*1024**3
root=Path('/home/lhy/cteas-resource-audit');root.mkdir(exist_ok=True)
d=Path(tempfile.mkdtemp(prefix=f'smoke-{index}-',dir=root))
(d/'hello.cpp').write_text('#include <chrono>\nint main(){std::chrono::sys_seconds s{};(void)s;return 0;}\n')
(d/'hello.f90').write_text('program hello\nprint *, 123\nend program hello\n')
(d/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.16)\nproject(resource_smoke LANGUAGES CXX Fortran)\nset(CMAKE_CXX_STANDARD 20)\nenable_testing()\nadd_executable(cpp hello.cpp)\nadd_executable(fort hello.f90)\nadd_test(NAME cpp20 COMMAND cpp)\nadd_test(NAME fortran COMMAND fort)\n')
with (d/'build.log').open('w') as log:
 for cmd in [['cmake','-S',str(d),'-B',str(d/'build'),'-DCMAKE_CXX_COMPILER=/usr/bin/g++-12','-DCMAKE_Fortran_COMPILER=/usr/bin/gfortran-11'],['cmake','--build',str(d/'build'),'--parallel','4'],['ctest','--test-dir',str(d/'build'),'--output-on-failure']]:
  subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
r={k:(cg/k).read_text().strip() for k in ['cpu.max','memory.high','memory.max','memory.peak','memory.events','cpu.stat']}
(d/'metrics.json').write_text(json.dumps(r,indent=2));print(name,'smoke PASS',str(d));print(json.dumps(r))
