#pragma once
// CALMET 6.5.0 L150223 SLOPE (calmet.for L30372-30604).
// Isolated Mahrt slope-flow physics, not a complete 3D diagnostic solver.
// Fortran REAL*4 arithmetic, i-fastest/k-slowest, SI m/s/m/K/Wm-2.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace cteas::calmet::p6 {
struct SlopeFlowInput {
    int nx=0,ny=0,nz=0;
    float dx_m=0,dy_m=0,tinf_k=0;
    int isurft=-1,water_first=0,water_last=-1;
    std::vector<float> zface_m,terrain_m,highest_m,lowest_m;
    std::vector<float> distance_highest_m,distance_lowest_m;
    std::vector<float> sensible_heat_w_m2,surface_temperature_k;
    std::vector<int> landuse;
};
struct SlopeFlow {
    std::vector<float> u_mps,v_mps;
};
// This kernel consumes the original QH meteorological field as an explicit
// prerequisite. QH is not synthesized: P6E surface energy balance remains open.
inline SlopeFlow slope_flow(const SlopeFlowInput& in) {
    if(in.nx<1||in.ny<1||in.nz<1||in.nx>400||in.ny>400||in.nz>12)
        throw std::invalid_argument("SLOPE unsupported grid");
    if(!(in.dx_m>0.f&&in.dy_m>0.f&&in.tinf_k>0.f)||
       !std::isfinite(in.dx_m)||!std::isfinite(in.dy_m)||!std::isfinite(in.tinf_k))
        throw std::invalid_argument("SLOPE invalid grid metric/temperature");
    const auto n=std::size_t(in.nx)*std::size_t(in.ny);
    if(in.zface_m.size()!=std::size_t(in.nz)+1||
       in.terrain_m.size()!=n||in.highest_m.size()!=n||in.lowest_m.size()!=n||
       in.distance_highest_m.size()!=n||in.distance_lowest_m.size()!=n||
       in.sensible_heat_w_m2.size()!=n||in.surface_temperature_k.size()!=n||
       in.landuse.size()!=n)
        throw std::invalid_argument("SLOPE input shape mismatch");
    for(std::size_t k=0;k<in.zface_m.size();++k)
        if(!std::isfinite(in.zface_m[k])||
           (k&&!(in.zface_m[k]>in.zface_m[k-1])))
            throw std::invalid_argument("SLOPE invalid ZFACE");
    for(std::size_t p=0;p<n;++p) {
        if(!std::isfinite(in.terrain_m[p])||!std::isfinite(in.highest_m[p])||
           !std::isfinite(in.lowest_m[p])||
           !std::isfinite(in.distance_highest_m[p])||
           !std::isfinite(in.distance_lowest_m[p])||
           !std::isfinite(in.sensible_heat_w_m2[p])||
           !std::isfinite(in.surface_temperature_k[p])||
           !(in.surface_temperature_k[p]>0.f)||
           in.distance_highest_m[p]<0.f||in.distance_lowest_m[p]<0.f||
           in.highest_m[p]<in.terrain_m[p]||in.lowest_m[p]>in.terrain_m[p])
            throw std::invalid_argument("SLOPE invalid/missing terrain or forcing");
    }
    SlopeFlow out{std::vector<float>(n*std::size_t(in.nz),0.f),
                  std::vector<float>(n*std::size_t(in.nz),0.f)};
    constexpr float rhocp=1229.9f,cdk=0.08f,rpd=0.017453f,dpr=57.296f;
    constexpr float one_third=1.f/3.f;
    const auto idx=[&](int i,int j)->std::size_t {
        return std::size_t(j)*std::size_t(in.nx)+std::size_t(i);
    };
    for(int j=0;j<in.ny;++j)for(int i=0;i<in.nx;++i) {
        const auto p=idx(i,j);
        if(in.landuse[p]>=in.water_first&&in.landuse[p]<=in.water_last)
            continue;
        const float temp=(in.isurft>0||in.isurft==-2)?
            in.tinf_k:in.surface_temperature_k[p];
        const float dxi=0.5f/in.dx_m,dyi=0.5f/in.dy_m;
        const float hi=(in.terrain_m[idx(std::min(i+1,in.nx-1),j)]-
                        in.terrain_m[idx(std::max(i-1,0),j)])*dxi;
        const float hj=(in.terrain_m[idx(i,std::min(j+1,in.ny-1))]-
                        in.terrain_m[idx(i,std::max(j-1,0))])*dyi;
        const float gradient=std::sqrt(hi*hi+hj*hj);
        const float alpha=std::atan(gradient);
        if(std::abs(alpha)<0.009f)continue;
        float sin_alpha=std::sin(std::abs(alpha));
        const float qh=in.sensible_heat_w_m2[p];
        float hdepth=0.f,crest=0.f;
        if(qh<0.f) {
            const float rise=in.highest_m[p]-in.terrain_m[p];
            crest=std::sqrt(rise*rise+in.distance_highest_m[p]*in.distance_highest_m[p]);
            hdepth=0.05f*rise;
        }else{
            const float drop=in.terrain_m[p]-in.lowest_m[p];
            hdepth=0.05f*drop;
            crest=std::sqrt(drop*drop+in.distance_lowest_m[p]*in.distance_lowest_m[p]);
        }
        float valley=0.f;
        if(hdepth>0.05f) {
            if(qh<0.f) {
                sin_alpha=std::min(sin_alpha,(in.highest_m[p]-in.terrain_m[p])/crest);
                const float decay=std::max(-crest*cdk/hdepth,-50.f);
                valley=-std::pow(sin_alpha*(9.81f/temp)*std::abs(qh)*crest/
                                 rhocp/cdk,one_third)*
                        std::pow(1.f-std::exp(decay),one_third);
            }else{
                valley=std::pow((9.81f/temp)*std::abs(qh)*
                                (in.terrain_m[p]-in.lowest_m[p])/rhocp,
                                one_third);
            }
        }
        float theta=0.f;
        if(hi==0.f){
            if(hj==0.f)continue;
            theta=hj<0.f?270.f:90.f;
        }else{
            const float theta_p=std::atan(hj/hi)*dpr;
            if(hi<0.f)theta=theta_p+180.f;
            if(hi>0.f)theta=hj>0.f?theta_p:theta_p+360.f;
        }
        float theta_d=0.f;
        if(theta>=0.f&&theta<=90.f)theta_d=90.f-theta;
        if(theta>90.f&&theta<=360.f)theta_d=450.f-theta;
        const float angle=(270.f-theta_d)*rpd;
        const float du=-std::cos(angle)*valley;
        const float dv=-std::sin(angle)*valley;
        for(int k=0;k<in.nz;++k){
            float ratio=0.f;
            if(hdepth>in.zface_m[std::size_t(k)+1])ratio=1.f;
            else if(hdepth>in.zface_m[std::size_t(k)])
                ratio=(hdepth-in.zface_m[std::size_t(k)])/
                      (in.zface_m[std::size_t(k)+1]-in.zface_m[std::size_t(k)]);
            const auto t=std::size_t(k)*n+p;
            out.u_mps[t]=du*ratio;
            out.v_mps[t]=dv*ratio;
        }
    }
    return out;
}
} // namespace cteas::calmet::p6
