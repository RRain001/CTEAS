#pragma once
// P6G G9 exploratory multi-station WIND1 first-guess generalization.
//
// Extends the previously source-scoped 1-SS/1-US BIAS, inverse-square and
// calm/missing rules to multiple land SS and US columns. This is a TESTED
// C++ multi-site kernel, NOT yet independent Fortran multi-site equivalence.
// Frozen wind1_firstguess_selected.hpp remains the numerical authority for
// the 50x50x9 single-pair baseline. Do not use this as P6 production physics
// until source CALMET multi-site parity is established.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace cteas::calmet::p6g {
struct Wind1MultiInput {
    int nx=0,ny=0,nz=0;
    float spacing_m=0.f;
    int surface_count=0,upper_count=0,icalm=0;
    // Station ordinal order: all SS first, then all US, from met.inp.
    std::vector<float> station_x_m,station_y_m,surface_speeds_mps;
    std::vector<float> vertical_bias;
    // (k*nstations+l) flattened REAL*4 observed profiles, U/V by layer.
    std::vector<float> station_u_mps,station_v_mps;
    // 1 denotes valid observation; 0 denotes truly missing input.
    std::vector<unsigned char> present;
};
struct Wind1MultiField {
    std::vector<float> u_mps,v_mps;
    int nx=0,ny=0,nz=0;
};
inline Wind1MultiField multi_station_wind1_firstguess(
    const Wind1MultiInput& in) {
    const int n=in.surface_count+in.upper_count;
    if(in.nx<2||in.ny<2||in.nz<1||in.nx>400||in.ny>400||in.nz>12||
       in.surface_count<1||in.upper_count<1||n>479||
       (in.icalm!=0&&in.icalm!=1)||
       !std::isfinite(in.spacing_m)||in.spacing_m<=0.f)
        throw std::invalid_argument("G9 WIND1 grid, source counts or mode invalid");
    const std::size_t cells=static_cast<std::size_t>(in.nx)*
                            static_cast<std::size_t>(in.ny);
    if(cells*static_cast<std::size_t>(in.nz)>4'000'000)
        throw std::invalid_argument("G9 WIND1 cell allocation exceeds 4M budget");
    const auto nc=static_cast<std::size_t>(n);
    const auto nk=static_cast<std::size_t>(in.nz);
    if(in.station_x_m.size()!=nc||in.station_y_m.size()!=nc||
       in.surface_speeds_mps.size()!=static_cast<std::size_t>(in.surface_count)||
       in.vertical_bias.size()!=nk||
       in.station_u_mps.size()!=nk*nc||
       in.station_v_mps.size()!=nk*nc||
       in.present.size()!=nk*nc)
        throw std::invalid_argument("G9 WIND1 profile, flag or coordinate count mismatch");
    for(std::size_t l=0;l<nc;++l)
        if(!std::isfinite(in.station_x_m[l])||
           !std::isfinite(in.station_y_m[l]))
            throw std::invalid_argument("G9 WIND1 invalid local station coordinate");
    for(float speed:in.surface_speeds_mps)
        if(!std::isfinite(speed)||speed<0.f||speed>=998.99f)
            throw std::invalid_argument("G9 WIND1 invalid observed surface speed");
    for(float bias:in.vertical_bias)
        if(!std::isfinite(bias)||bias< -1.f||bias>1.f)
            throw std::invalid_argument("G9 WIND1 BIAS outside [-1,1]");
    for(std::size_t k=0;k<nk;++k)
        for(std::size_t l=0;l<nc;++l){
            const std::size_t z=k*nc+l;
            if(in.present[z]>1)
                throw std::invalid_argument("G9 WIND1 non-boolean validity mask");
            if(in.present[z]&&
               (!std::isfinite(in.station_u_mps[z])||
                !std::isfinite(in.station_v_mps[z])||
                std::abs(in.station_u_mps[z])>=998.99f||
                std::abs(in.station_v_mps[z])>=998.99f))
                throw std::invalid_argument("G9 WIND1 valid profile has missing values");
            if(l<static_cast<std::size_t>(in.surface_count)&&
               in.surface_speeds_mps[l]<1.e-4f&&in.present[z]&&
               (in.station_u_mps[z]!=0.f||in.station_v_mps[z]!=0.f))
                throw std::invalid_argument("G9 WIND1 calm surface with invented profile");
        }
    Wind1MultiField out;
    out.nx=in.nx;out.ny=in.ny;out.nz=in.nz;
    out.u_mps.resize(cells*nk);out.v_mps.resize(cells*nk);
    std::vector<float> raw(nc),base(nc),adjusted(nc);
    const float inv_grid=1.f/in.spacing_m;
    // Mirrors frozen CALMET FIELD1/2 FORTRAN grid order I outer, J inner.
    for(int i=1;i<=in.nx;++i)for(int j=1;j<=in.ny;++j){
        const float xc=static_cast<float>(i)-0.5f;
        const float yc=static_cast<float>(j)-0.5f;
        float total=0.f;
        for(std::size_t l=0;l<nc;++l){
            const float dx=in.station_x_m[l]*inv_grid-xc;
            const float dy=in.station_y_m[l]*inv_grid-yc;
            const float r2=dx*dx+dy*dy;
            raw[l]=r2<1.f?1.f:1.f/r2;
            total+=raw[l];
        }
        if(!(total>0.f)&&std::isfinite(total))
            throw std::invalid_argument("G9 WIND1 empty inverse-square weights");
        if(!std::isfinite(total)||total<=0.f)
            throw std::invalid_argument("G9 WIND1 singular inverse-square total");
        const float inverse=1.f/total;
        float surface_base=0.f,upper_base=0.f;
        for(std::size_t l=0;l<nc;++l){
            base[l]=raw[l]*inverse;
            if(l<static_cast<std::size_t>(in.surface_count))
                surface_base+=base[l];
            else upper_base+=base[l];
        }
        if(surface_base<=0.f||upper_base<=0.f)
            throw std::invalid_argument("G9 WIND1 zero surface/upper class share");
        for(std::size_t k=0;k<nk;++k){
            // For exactly one SS/US, these beta/gamma operations reduce
            // to wind1_firstguess_selected's original selected route.
            const float bias=in.vertical_bias[k];
            float beta=0.f,gamma=0.f;
            if(bias>=0.f){
                beta=1.f-bias;
                gamma=(bias+(1.f-bias)*upper_base)/upper_base;
            }else{
                beta=(-bias+(1.f+bias)*surface_base)/surface_base;
                gamma=1.f+bias;
            }
            float kept=0.f;
            for(std::size_t l=0;l<nc;++l){
                const std::size_t z=k*nc+l;
                const bool surface=l<static_cast<std::size_t>(in.surface_count);
                const bool calm_surface=surface&&k>0&&in.icalm==0&&
                    in.surface_speeds_mps[l]<0.0001f;
                adjusted[l]=(!in.present[z]||calm_surface)?0.f:
                            (surface?beta:gamma)*base[l];
                kept+=adjusted[l];
            }
            // An explicitly BIAS-forced class with no observations is not
            // silently replaced by fabricated winds.
            if((bias>=0.9999f&&upper_base>0.f)&&
               std::none_of(adjusted.begin()+in.surface_count,
                            adjusted.end(),[](float w){return w>0.f;}))
                throw std::invalid_argument("G9 WIND1 forced upper class is missing");
            if((bias<=-0.9999f&&surface_base>0.f)&&
               std::none_of(adjusted.begin(),
                            adjusted.begin()+in.surface_count,
                            [](float w){return w>0.f;}))
                throw std::invalid_argument("G9 WIND1 forced surface class is missing");
            if(!(kept>0.f)||!std::isfinite(kept))
                throw std::invalid_argument("G9 WIND1 no valid station observation");
            // Redistribute missing/calm station weights among available
            // stations; the frozen pair case retains its own acceptance.
            const float inv_kept=1.f/kept;
            float u=0.f,v=0.f;
            for(std::size_t l=0;l<nc;++l){
                const float w=adjusted[l]*inv_kept;
                const std::size_t z=k*nc+l;
                u+=w*in.station_u_mps[z];
                v+=w*in.station_v_mps[z];
            }
            if(!std::isfinite(u)||!std::isfinite(v))
                throw std::invalid_argument("G9 WIND1 nonfinite field");
            const std::size_t idx=k*cells+
                static_cast<std::size_t>(j-1)*static_cast<std::size_t>(in.nx)+
                static_cast<std::size_t>(i-1);
            out.u_mps[idx]=u;out.v_mps[idx]=v;
        }
    }
    return out;
}
} // namespace cteas::calmet::p6g
