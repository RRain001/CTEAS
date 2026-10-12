#pragma once
// Original CALMET v6.5.0 STHEOR -> ELUSTR2 land-station diagnostics.
// calmet.for L6699-6866: compute the *station-specific* heat flux,
// friction velocity and Monin-Obukhov length consumed by SIMILT L6894.
// This is separate from the DIAGNO grid ELUSTR L6333-6578.
//
// Deliberately source-scoped: no mixing height is computed in this
// subroutine (original ELUSTR2 calls MIXHTST *after* these values).
#include "cteas/p6e/elustr_daytime.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace cteas::calmet::p6 {
struct StheorStationMeteo {
    float surface_u_mps{},surface_v_mps{},roughness_m{};
    float zmid1_m{},anemometer_m{},temperature_k{},rho_kgm3{};
    float solar_sine{},ha1{},ha2{},hb1{},hb2{},hc1{},hc2{},hc3p1{};
    float albedo{},bowen{},soil_heat_fraction{},anthropogenic_wm2{};
    int cloud_tenths=0,ipsifcn=0;
};
struct StheorStationResult {
    float qh_wm2{},ustar_mps{},el_m{};
};
inline StheorStationResult original_stheor_elustr2_station(
    const StheorStationMeteo& a) {
    if(!(a.zmid1_m>0.f&&a.anemometer_m>0.f&&
         a.roughness_m>0.f&&a.roughness_m<a.zmid1_m&&
         a.temperature_k>0.f&&a.rho_kgm3>0.f&&a.hc3p1>0.f&&
         a.bowen>=0.f&&a.bowen<1.e4f&&
         a.cloud_tenths>=0&&a.cloud_tenths<=10&&
         (a.ipsifcn==0||a.ipsifcn==1)))
        throw std::invalid_argument("STHEOR original ELUSTR2 invalid inputs");
    const float ws=std::sqrt(a.surface_u_mps*a.surface_u_mps+
                             a.surface_v_mps*a.surface_v_mps);
    if(!std::isfinite(ws)||ws<0.001f)
        throw std::invalid_argument("STHEOR original refuses calm/missing station wind");
    // ELUSTR2 L6705 uses first CALMET level, but PSIUD L6778
    // uses the *actual anemometer height*. Do not conflate them.
    const float xlnzz0=std::log(a.zmid1_m/a.roughness_m);
    const float ccfrac=0.1f*static_cast<float>(a.cloud_tenths);
    float qsw=(a.ha1*a.solar_sine+a.ha2)*
               (1.f+a.hb1*std::pow(ccfrac,a.hb2));
    qsw=std::max(qsw,0.f);
    float qqh=-0.1f;
    if(a.solar_sine>0.f) {
        const float tk2=a.temperature_k*a.temperature_k;
        const float tk4=tk2*tk2;
        const float tk6=tk4*tk2;
        const float qstar=((1.f-a.albedo)*qsw+a.hc1*tk6-
                          5.67e-8f*tk4+a.hc2*ccfrac)/a.hc3p1;
        qqh=a.bowen*(qstar*(1.f-a.soil_heat_fraction)+
                     a.anthropogenic_wm2)/(1.f+a.bowen);
    }
    float ustar=0.f,el=0.f;
    if(qqh>=0.f) {
        // Original L6751-6805, Holtslag/van Ulden unstable branch.
        el=-9.9e9f;
        ustar=std::max(0.4f*ws/xlnzz0,0.05f);
        if(qqh>0.f) {
            float xlold=el;
            for(int it=0;it<5;++it) {
                el=-253.8226f*a.rho_kgm3*a.temperature_k*
                    (ustar*ustar*ustar)/qqh;
                float psizl=0.f,psiz0l=0.f;
                if(a.ipsifcn==0) {
                    const double z=static_cast<double>(a.anemometer_m/el);
                    const double z0=static_cast<double>(a.roughness_m/el);
                    psizl=static_cast<float>(cteas::p6e::original_psiud_unstable(z));
                    psiz0l=static_cast<float>(cteas::p6e::original_psiud_unstable(z0));
                }else{
                    psizl=cteas::p6e::original_psiuc_unstable(a.anemometer_m,el);
                    psiz0l=cteas::p6e::original_psiuc_unstable(a.roughness_m,el);
                }
                ustar=std::max(0.4f*ws/(xlnzz0-psizl+psiz0l),0.05f);
                if(std::abs((el-xlold)/el)<0.005f)break;
                xlold=el;
            }
        }
    }else{
        // Original L6809-6866, Venkatram / Weil & Brower stable branch.
        const float stable_ws=std::max(ws,0.5f);
        const float jcc=static_cast<float>(a.cloud_tenths);
        const float theta1=0.09f*(1.f-5.e-3f*(jcc*jcc));
        const float cdn=0.4f/xlnzz0;
        const float theta2=a.temperature_k*cdn*stable_ws*stable_ws/
                           (184.428f*a.zmid1_m);
        float thetas=std::max(std::min(theta1,theta2),1.e-9f);
        float u02=46.107f*a.zmid1_m*thetas/a.temperature_k;
        float cu2=std::max(0.f,stable_ws*stable_ws-4.f*u02/cdn);
        ustar=std::max(0.5f*cdn*(stable_ws+std::sqrt(cu2)),0.05f);
        if(ustar*thetas>0.05f){
            const float toler=1.05f*0.05f;
            for(int it=0;it<3;++it){
                thetas=0.05f/ustar;
                u02=46.107f*a.zmid1_m*thetas/a.temperature_k;
                cu2=std::max(0.f,stable_ws*stable_ws-4.f*u02/cdn);
                ustar=0.5f*cdn*(stable_ws+std::sqrt(cu2));
                if(ustar*thetas<=toler)break;
            }
        }
        qqh=-996.f*a.rho_kgm3*ustar*thetas;
        el=-253.8226f*a.rho_kgm3*a.temperature_k*
            (ustar*ustar*ustar)/qqh;
    }
    if(!std::isfinite(el)||!std::isfinite(qqh)||!std::isfinite(ustar))
        throw std::runtime_error("STHEOR ELUSTR2 produced nonfinite physical state");
    return {qqh,ustar,el};
}
} // namespace cteas::calmet::p6
