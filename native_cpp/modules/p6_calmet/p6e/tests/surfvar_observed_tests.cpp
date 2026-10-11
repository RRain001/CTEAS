#include "cteas/p6e/surfvar_observed.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <stdexcept>
using namespace cteas::p6e;
template<class F>void bad(F f){
    bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}
    assert(caught);
}
namespace {
ObservedSurface surface(std::size_t nx,std::size_t ny,float temp_k,int rh_pct){
    ObservedSurface s;
    s.nx=nx;s.ny=ny;s.station_temp_k=temp_k;s.station_rh_percent=rh_pct;
    return s;
}
}
int main(){
    auto s=surface(5,4,289.125f,65);
    auto r=surfvar_one_station(s);
    assert(r.temp_k.size()==20 && r.rh_percent.size()==20);
    for(std::size_t i=0;i<20;++i){
        assert(r.temp_k[i]==289.125f);
        assert(r.rh_percent[i]==65);
        assert(r.ipcode[i]==0);
    }
    s.station_rh_percent=9999;
    for(int v:surfvar_one_station(s).rh_percent)assert(v==80);
    s.npsta=1;bad([&]{(void)surfvar_one_station(s);});s.npsta=0;
    s.land_only=false;bad([&]{(void)surfvar_one_station(s);});s.land_only=true;
    s.average_temperature=true;bad([&]{(void)surfvar_one_station(s);});
    s.average_temperature=false;s.nowsta=1;
    bad([&]{(void)surfvar_one_station(s);});s.nowsta=0;
    s.station_temp_k=9999.f;bad([&]{(void)surfvar_one_station(s);});
    s.station_temp_k=289.f;s.station_rh_percent=-1;
    bad([&]{(void)surfvar_one_station(s);});
    // IALL semantics: 0 and 1 give the same TEMP2D, but IALL=0 returns before
    // filling RH/IPCODE, exactly as the original routine does.
    {
        auto base=surface(4,3,290.5f,55);
        const auto one=surfvar_one_station(base);
        auto zero_input=base;zero_input.iall=0;
        const auto zero=surfvar_one_station(zero_input);
        assert(zero.temp_k==one.temp_k);
        assert(zero.rh_percent.empty()&&zero.ipcode.empty());
        auto invalid=base;invalid.iall=3;
        bad([&]{(void)surfvar_one_station(invalid);});
    }
    // IAVET=1 is the real deck setting: it requires the diagnostic u/v field
    // and, for the uniform single-station field, is a float-rounding no-op.
    // IALL=2 bypasses the average, as in `iavet.eq.1.and.iall.ne.2`.
    {
        auto base=surface(4,3,290.5f,55);
        const auto without=surfvar_one_station(base);
        auto with=base;with.average_temperature=true;
        bad([&]{(void)surfvar_one_station(with);});
        const std::size_t n=12U;
        with.u_ms.assign(n,3.f);with.v_ms.assign(n,-1.f);
        const auto averaged=surfvar_one_station(with);
        assert(averaged.temp_k.size()==n);
        for(std::size_t k=0;k<n;++k)
            assert(std::fabs(averaged.temp_k[k]-without.temp_k[k])<1e-3f);
        auto bypass=with;bypass.iall=2;
        const auto skipped=surfvar_one_station(bypass);
        assert(skipped.temp_k==without.temp_k);
        assert(skipped.rh_percent.size()==n);
    }
}
