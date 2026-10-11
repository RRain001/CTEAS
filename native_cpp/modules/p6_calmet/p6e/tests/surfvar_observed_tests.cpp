#include "cteas/p6e/surfvar_observed.hpp"
#include <cassert>
#include <stdexcept>
using namespace cteas::p6e;
template<class F>void bad(F f){
    bool caught=false;try{f();}catch(const std::invalid_argument&){caught=true;}
    assert(caught);
}
int main(){
    ObservedSurface s{5,4,289.125f,65};
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
}
