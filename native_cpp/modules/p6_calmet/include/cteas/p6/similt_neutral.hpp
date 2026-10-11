#pragma once
// CALMET 6.5.0 L150223 SIMILT L30097–30370 selected neutral extrapolation.
// WIND1 IEXTRP=-4 calls SIMILT with EL=ZI=ZIMINI=10000 m.
// This isolated kernel does NOT implement WIND1, turbulent stability, or
// Southern Hemisphere/overwater modes: fail-closed outside selected scope.
#include <array>
#include <cmath>
#include <stdexcept>
namespace cteas::calmet::p6 {
struct SimiltNeutralInput {
    float station_lat_deg=0.f;
    float anemometer_height_m=0.f;
    float roughness_m=0.f;
    float anemometer_u_mps=0.f;
    float anemometer_v_mps=0.f;
    int iextrp=-4;
    int ipsifcn=0;
};
struct SimiltNeutralOutput {
    std::array<float,9> u_mps{};
    std::array<float,9> v_mps{};
};
inline SimiltNeutralOutput original_similt_wind1_neutral(
    const SimiltNeutralInput& in) {
    constexpr std::array<float,10> zface{0.f,20.f,40.f,80.f,160.f,
        320.f,640.f,1200.f,2000.f,3000.f};
    constexpr float factor=57.2957795f;
    constexpr float deg2rad=0.0174533f;
    constexpr float dh200=12.f;
    constexpr float d1=1.58f,d2=1.f,ztbl2=200.f;
    const auto finite=[](float x){return std::isfinite(x);};
    if(in.iextrp!=-4||(in.ipsifcn!=0&&in.ipsifcn!=1)||
       !finite(in.station_lat_deg)||in.station_lat_deg<=0.f||
       !finite(in.anemometer_height_m)||in.anemometer_height_m<=0.f||
       !finite(in.roughness_m)||in.roughness_m<=0.f||
       in.roughness_m>=in.anemometer_height_m||
       in.roughness_m>=10.f||
       !finite(in.anemometer_u_mps)||!finite(in.anemometer_v_mps))
        throw std::invalid_argument("SIMILT: unsupported or invalid WIND1 neutral station");
    const float speed=std::sqrt(in.anemometer_u_mps*in.anemometer_u_mps+
                                in.anemometer_v_mps*in.anemometer_v_mps);
    // WIND1 skips SIMILT for speed < 0.0001. Never invent calm profile.
    if(!finite(speed)||speed<0.0001f)
        throw std::invalid_argument("SIMILT: WIND1 does not extrapolate calm station");
    const float angle=270.f-std::atan2(in.anemometer_v_mps,
                                      in.anemometer_u_mps)*factor;
    float wd1=std::fmod(angle,360.f);
    if(wd1==0.f)wd1=360.f;
    const float base_log=std::log(in.anemometer_height_m/in.roughness_m);
    if(!finite(base_log)||base_log<=0.f)
        throw std::invalid_argument("SIMILT: singular log-profile");
    const float dzan=dh200*d1*(1.f-std::exp(-d2*in.anemometer_height_m/ztbl2));
    SimiltNeutralOutput out;
    for(std::size_t k=0;k<9;++k){
        // Original SIMILT loops K=2..NZP1 and writes US(K-1,NS);
        // EL=10000 => ELINV=0, PSIM1=PSIM=0, ZI=ZIMINI=10000.
        const float mid=(zface[k]+zface[k+1])*0.5f;
        const float xlnzz0=std::log(mid/in.roughness_m);
        const float ws=speed*xlnzz0/base_log;
        const float dwd=dh200*d1*(1.f-std::exp(-d2*mid/ztbl2));
        float wd=wd1+dwd-dzan;
        if(wd>360.f)wd-=360.f;
        const float wdrad=deg2rad*wd;
        out.u_mps[k]=-ws*std::sin(wdrad);
        out.v_mps[k]=-ws*std::cos(wdrad);
        if(!finite(out.u_mps[k])||!finite(out.v_mps[k]))
            throw std::invalid_argument("SIMILT: nonfinite extrapolated wind");
    }
    return out;
}
} // namespace cteas::calmet::p6
