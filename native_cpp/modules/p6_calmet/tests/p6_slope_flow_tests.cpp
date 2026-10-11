#include "cteas/p6/slope_flow.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace cteas::calmet::p6;
static SlopeInput make_input() {
    SlopeInput x;
    x.nx=4; x.ny=4; x.nz=3; x.dx_m=100; x.dy_m=100;
    x.water_first=50; x.water_last=55;
    x.isurft=-1; x.domain_temperature_k=285;
    x.zface_m={0,20,40,80};
    for (int j=0;j<4;++j) for(int i=0;i<4;++i) {
        float z=100.0f+i*15.0f+j*5.0f;
        x.terrain_m.push_back(z);
        x.hmax_m.push_back(z+800.0f);
        x.hmin_m.push_back(z-800.0f);
        x.disthmax_m.push_back(900.0f);
        x.disthmin_m.push_back(900.0f);
        x.sensible_heat_w_m2.push_back(-70.0f);
        x.surface_temperature_k.push_back(280.0f);
        x.landuse.push_back(10);
    }
    return x;
}
template<class F> void expect_invalid(F f) {
    bool threw=false;
    try { f(); } catch(const std::invalid_argument&) { threw=true; }
    assert(threw);
}
int main() {
    const auto x=make_input();
    const auto w=compute_original_slope_scoped(x);
    assert(w.u_mps.size()==48 && w.v_mps.size()==48);
    assert(std::hypot(w.u_mps[5],w.v_mps[5])>0.01f);
    assert(std::abs(w.u_mps[5]-w.u_mps[16+5])<1e-5f);
    assert(w.u_mps[32+5]==0.0f && w.v_mps[32+5]==0.0f);
    {
        auto water=x; water.landuse[5]=52;
        const auto wind=compute_original_slope_scoped(water);
        for(int k=0;k<3;++k) {
            assert(wind.u_mps[16*k+5]==0.0f);
            assert(wind.v_mps[16*k+5]==0.0f);
        }
    }
    {
        auto uphill=x; uphill.sensible_heat_w_m2[5]=110.0f;
        const auto wind=compute_original_slope_scoped(uphill);
        assert(wind.u_mps[5]*w.u_mps[5]<0.0f ||
               wind.v_mps[5]*w.v_mps[5]<0.0f);
    }
    {
        auto flat=x; for(auto& z:flat.terrain_m) z=100.0f;
        const auto wind=compute_original_slope_scoped(flat);
        for(float u:wind.u_mps) assert(u==0.0f);
        for(float v:wind.v_mps) assert(v==0.0f);
    }
    {
        auto bad=x; bad.disthmax_m.pop_back();
        expect_invalid([&] { (void)compute_original_slope_scoped(bad); });
    }
    {
        auto bad=x; bad.surface_temperature_k[0]=std::numeric_limits<float>::quiet_NaN();
        expect_invalid([&] { (void)compute_original_slope_scoped(bad); });
    }
    {
        auto bad=x; bad.zface_m[2]=bad.zface_m[1];
        expect_invalid([&] { (void)compute_original_slope_scoped(bad); });
    }
    {
        auto bad=x; bad.hmax_m[3]=bad.terrain_m[3]-1.0f;
        expect_invalid([&] { (void)compute_original_slope_scoped(bad); });
    }
}
