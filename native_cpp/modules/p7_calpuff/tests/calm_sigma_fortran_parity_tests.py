#!/usr/bin/env python3
"""Fortran oracle: original SIGTY/SIGTZ IDOPT1, EL<=0; not whole P7."""
import argparse,importlib.util,json,math,shutil,subprocess,tempfile
from pathlib import Path
def main():
 p=argparse.ArgumentParser()
 p.add_argument("--probe",required=True,type=Path)
 p.add_argument("--repo-root",required=True,type=Path)
 a=p.parse_args()
 root=a.repo_root.resolve()
 base=root/"native_cpp/modules/p7_calpuff"
 spec=importlib.util.spec_from_file_location("p7sigma",base/"reference/sigma_source_anchor.py")
 mod=importlib.util.module_from_spec(spec)
 spec.loader.exec_module(mod)
 src=root/mod.VENDOR_SUBDIR
 manifest,spans=mod.extract(src)
 pinned=json.loads((base/"reference/sigma_source_anchor.json").read_text())
 assert manifest==pinned,"vendor SIGTY/SIGTZ anchor changed"
 cases=[]
 for el in [-200.0,0.0,100.0]:
  for dt in [0.0,20.0,300.0,1800.0]:
   for sy in [0.0,0.5,4.0,12.0]:
    cases.append((len(cases)+1,sy,sy*0.8,dt,0.02,0.2,
                  0.45,0.25,el,0.001,0.002,0.001,550.0,0.1,5000.0))
 matrix="\n".join(" ".join(str(x) for x in row) for row in cases)+"\n"
 with tempfile.TemporaryDirectory(prefix="p7-calm-idopt1-") as work:
  stage=Path(work)/"stage"
  mod.stage(src,stage,manifest,spans)
  shutil.copy2(base/"reference/calm_sigma_oracle_driver.f",
               stage/"calm_sigma_oracle_driver.f")
  fc=next((shutil.which(x) for x in
           ("gfortran-12","gfortran-11","gfortran") if shutil.which(x)),None)
  if fc is None:raise RuntimeError("gfortran required for original-source gate")
  oracle=stage/"calm_original"
  cmd=[fc,"-O0","-std=legacy","-ffixed-line-length-72",
       "-fallow-argument-mismatch","-fno-automatic",
       f"-I{stage}","-o",str(oracle)]
  cmd.extend(str(s) for s in sorted(stage.glob("vendor_*.f")))
  cmd.append(str(stage/"calm_sigma_oracle_driver.f"))
  c=subprocess.run(cmd,capture_output=True,text=True)
  if c.returncode:raise RuntimeError(c.stderr[-2500:])
  o=subprocess.run([str(oracle)],input=matrix,cwd=stage,
                   capture_output=True,text=True)
  n=subprocess.run([str(a.probe.resolve())],input=matrix,
                   capture_output=True,text=True)
  if o.returncode or n.returncode:raise RuntimeError(
      "vendor="+o.stderr[-500:]+" native="+n.stderr[-500:])
  ov=o.stdout.splitlines()
  nv=n.stdout.splitlines()
  if len(ov)!=len(cases) or len(nv)!=len(cases):raise RuntimeError(
      f"lost rows oracle={len(ov)} native={len(nv)} expected={len(cases)}")
  worst=0.0
  for i,(left,right) in enumerate(zip(ov,nv),1):
   x=list(map(float,left.split()));y=list(map(float,right.split()))
   if len(x)!=7 or len(y)!=7 or int(x[0])!=i or int(y[0])!=i:
    raise RuntimeError(f"misaligned case {i}")
   for j,(want,got) in enumerate(zip(x[1:],y[1:]),1):
    if not math.isfinite(want) or not math.isfinite(got):
     raise RuntimeError(f"nonfinite {i}:{j}")
    rel=abs(want-got)/max(abs(want),1e-9)
    worst=max(worst,rel)
    if abs(want-got)>max(2e-4,2e-4*abs(want)):
     raise RuntimeError(f"original mismatch {i}:{j}: {want} vs {got} rel={rel}")
  print(f"P7_CALM_IDOPT1_SIGTY_SIGTZ_VENDOR_PARITY=PASS CASES={len(cases)} "
        f"VALUES={len(cases)*6} MAX_REL={worst:.3e}")
  print("P7_STABLE_CALM_ELS_GT_ZERO=SOURCE_PARITY_TESTED")
  print("P7_REAL_989_CALM_SUBSTEPS=NOT_YET_ACCEPTED")
if __name__=="__main__":main()
