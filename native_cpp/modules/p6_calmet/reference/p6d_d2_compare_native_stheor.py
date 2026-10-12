#!/usr/bin/env python3
"""Source-native station STHEOR/ELUSTR2 against independently frozen ELUSTR2
original actual 143h DGM-cell audit. Emits evidence, never injects references.
"""
import argparse
import json
import math
import struct
from pathlib import Path
FIELDS={"EL":"stheor_el","USTAR":"stheor_ustar","QQH":"stheor_qh"}
THRESHOLD={"EL":(0.02,0.0002),"USTAR":(0.0001,0.0002),"QQH":(0.005,0.0002)}
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--original",required=True,type=Path)
    ap.add_argument("--product",required=True,type=Path)
    ap.add_argument("--index",required=True,type=Path)
    ap.add_argument("--out",required=True,type=Path)
    args=ap.parse_args()
    records=json.loads(args.original.read_text())["rows"]
    labels=args.index.read_text().split()
    if len(records)!=143 or len(labels)!=143:raise RuntimeError("frozen original/candidate hour count differs from 143")
    stat={}
    for field,prefix in FIELDS.items():
        row={"n":0,"exceedances":0,"max_abs":0.0,"max_rel":0.0,
             "rms_sum":0.0,"worst":None,"first_exceedances":[]}
        for record,label in zip(records,labels):
            if record["hour_index"]!=row["n"]+1:
                raise RuntimeError("original DGM hour sequencing changed")
            path=args.product/(prefix+"_"+label+".f32")
            data=path.read_bytes()
            if len(data)!=4:raise RuntimeError(f"bad standalone station state: {path}")
            candidate=struct.unpack("<f",data)[0]
            original=float(record[field])
            if not math.isfinite(candidate) or not math.isfinite(original):
                raise RuntimeError("nonfinite native/original STHEOR station")
            error=abs(candidate-original)
            tol=THRESHOLD[field][0]+THRESHOLD[field][1]*abs(original)
            row["n"]+=1;row["rms_sum"]+=error*error
            if error>row["max_abs"]:
                row["max_abs"]=error
                row["worst"]={"hour":label,"original":original,
                              "native":candidate,"abs_error":error,"tolerance":tol}
            row["max_rel"]=max(row["max_rel"],error/max(abs(original),1.0e-20))
            if error>tol:
                row["exceedances"]+=1
                if len(row["first_exceedances"])<8:
                    row["first_exceedances"].append({"hour":label,"original":original,
                          "native":candidate,"abs_error":error,"tolerance":tol})
        row["rms"]=math.sqrt(row.pop("rms_sum")/row["n"])
        stat[field]=row
    passed=all(v["exceedances"]==0 for v in stat.values())
    result={"status":"SOURCE_NATIVE_STHEOR_ELUSTR2_STATION_DIAGNOSTIC",
            "stage":"original calmet.for L6699-6866 before MIXHTST; native observations only",
            "inputs":"native SURF.DAT, met.inp SS1 station XY/anemometer, GEO.DAT, native solar/surfvar/airden",
            "oracle_usage":"NONE_FOR_NATIVE_STHEOR; DGM EL/USTAR/QQH only for after-run external comparison",
            "independent_snapshot":"frozen original DGM-cell 143h source SHA-guarded, not produced by candidate",
            "fields":stat,"scientific_verdict":"NATIVE_STHEOR_STATION_SCOPED_PASS" if passed
              else "NATIVE_STHEOR_STATION_NOT_PASS",
            "production_p6d_verdict":"NOT_PASS_MIXHTST_ZII_CGAMMA_STILL_OPEN"}
    args.out.parent.mkdir(parents=True,exist_ok=True)
    args.out.write_text(json.dumps(result,indent=2)+"\n")
    print("P6D_NATIVE_STHEOR_143H_DIAGNOSTIC="+result["scientific_verdict"])
    print(json.dumps({name:{"n":s["n"],"exceedances":s["exceedances"],
                             "max_abs":s["max_abs"],"worst":s["worst"]}
                     for name,s in stat.items()},indent=2))
if __name__=="__main__":main()
