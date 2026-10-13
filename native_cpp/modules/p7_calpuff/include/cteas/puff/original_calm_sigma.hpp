#pragma once
// CALPUFF 7.2.1 SIGTY 50962-51022 / SIGTZ 51458-51573.
// Source-derived REAL*4 IDOPTYS=IDOPTZS=1, EL<=0 selected subset.
// Deliberately fail closed for stable EL>0 (iterated logarithmic inverse),
// MHFTSZ=1, or absent actual /CSIGMA/ turbulence parameters.
#include <cmath>
#include <stdexcept>
namespace cteas::puff {
struct OriginalCalmSigmaInput {
 float sy0=0,sz0=0,dt_s=0,dx_km=0,uavg=0,sigv=0,sigw=0;
 float el=0,tyidr=0,tzidr=0,syh=550,ayt=0,thfty=0;
};
struct OriginalCalmSigmaOutput {
 float sy=0,virty=0,virdy=0,sz=0,virtz=0,virdz=0;
};
[[nodiscard]] inline OriginalCalmSigmaOutput original_calm_sigma(
    const OriginalCalmSigmaInput& p) {
    auto need=[](bool b,const char* msg){if(!b)throw std::invalid_argument(msg);};
    const float a[]={p.sy0,p.sz0,p.dt_s,p.dx_km,p.uavg,p.sigv,p.sigw,
                     p.el,p.tyidr,p.tzidr,p.syh,p.ayt,p.thfty};
    for(const float v:a)need(std::isfinite(v),"calm SIGTY/SIGTZ nonfinite");
    need(p.sy0>=0&&p.sz0>=0&&p.dt_s>=0&&p.dx_km>=0&&p.uavg>0&&
         p.sigv>0&&p.sigw>0&&p.tyidr>=0&&p.tzidr>=0&&
         p.syh>0&&p.ayt>0&&p.thfty>=0,
         "calm SIGTY/SIGTZ missing native turbulence or invalid input");
    need(p.el<=0,"stable ELS>0 logarithmic SIGTZ inversion not yet ported");
    constexpr float zero=0.0F,zerosig=0.001F;
    OriginalCalmSigmaOutput r;
    if((p.dt_s>p.thfty&&p.sy0<zerosig)||p.sy0>p.syh) {
        const float tprime=p.syh/p.ayt-p.thfty;
        const float t=p.sy0>p.syh ? (p.sy0/p.ayt-tprime)+p.dt_s : p.dt_s;
        r.sy=p.ayt*(t+tprime);
        r.virty=t;
        r.virdy=p.uavg*t*0.001F;
    } else {
        float t0=zero;
        if(p.sy0>zerosig) {
            const float gama=p.sy0/p.sigv;
            const float beta=gama*(1.0F+0.405F*gama*p.tyidr);
            t0=beta+std::sqrt(beta*beta-gama*gama);
        }
        const float t=t0+p.dt_s;
        r.virty=t;
        r.virdy=p.uavg*t*0.001F;
        if(p.dt_s>zero) {
            const float fy=1.0F/(1.0F+0.9F*std::sqrt(t*p.tyidr));
            r.sy=p.sigv*t*fy;
        } else r.sy=p.sy0;
    }
    float t0=zero;
    if(p.sz0>zerosig){
        const float gama=p.sz0/p.sigw;
        const float beta=gama*(1.0F+0.405F*gama*p.tzidr);
        t0=beta+std::sqrt(beta*beta-gama*gama);
    }
    const float t=t0+p.dt_s;
    r.virtz=t;
    if(p.dt_s>zero){
        const float fz=1.0F/(1.0F+0.9F*std::sqrt(p.tzidr*t));
        r.sz=p.sigw*t*fz;
    } else r.sz=p.sz0;
    r.virdz=t*p.uavg*0.001F;
    need(std::isfinite(r.sy)&&std::isfinite(r.sz)&&
         std::isfinite(r.virty)&&std::isfinite(r.virtz),
         "nonfinite original IDOPT1 turbulence result");
    return r;
}
} // namespace cteas::puff
