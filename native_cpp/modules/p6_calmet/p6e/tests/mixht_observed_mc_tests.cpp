#include "cteas/p6e/mixht_observed_mc.hpp"
#include <cassert>
#include <cmath>
#include <stdexcept>
using namespace cteas::p6e;
template<class F> void fails(F f){bool threw=false;try{f();}catch(const std::invalid_argument&){threw=true;}assert(threw);}
int main(){
    MixhtObservedMcInput a;
    a.nx=3;a.ny=1;
    a.justa={1,-1};a.iupt=-1;
    a.sounding[0]={{ {0.f,100.f,300.f,550.f,800.f,1500.f},
                     {302.f,301.f,298.f,295.f,292.f,284.f}},
                   { {0.f,100.f,300.f,550.f,800.f,1500.f},
                     {299.f,298.f,295.f,292.f,289.f,281.f}}};
    a.sounding[1]=a.sounding[0];
    a.nearu={1,2,1};a.landuse={12,12,55};
    a.qh={120.f,-0.1f,100.f};
    a.ustar={0.35f,0.23f,0.3f};
    a.el={200.f,30.f,20.f};
    a.rho={1.2f,1.2f,1.2f};
    a.cmech={110.f,110.f,110.f};
    a.fcori={0.00007f,0.00007f,0.00007f};
    a.initial_zi={-7777.f,-7777.f,-7777.f};
    a.initial_ziconv={120.f,120.f,-7777.f};
    a.initial_dptt={0.2f,0.3f,-7777.f};
    auto out=mixht_observed_mc(a);
    assert(out.zi[0]>=a.zimin&&out.zi[0]<=a.zimax);
    assert(out.ziconv[0]>=0.f);
    assert(out.ziconv[1]==0.f&&out.dptt[1]==0.f);
    assert(out.zi[2]==-7777.f&&out.ziconv[2]==-7777.f&&out.dptt[2]==-7777.f);
    a.iupt=2;
    assert(std::isfinite(mixht_observed_mc(a).zi[0]));
    a.iupt=3;fails([&]{return mixht_observed_mc(a);});
    a.iupt=-1;a.nearu[0]=3;fails([&]{return mixht_observed_mc(a);});
    a.nearu[0]=1;
    // IMIXH=2 selects the original Batchvarova-Gryning growth and, unlike
    // MIXHMC, never updates the potential-temperature jump.
    a.imixh=2;a.el[0]=-120.f;
    const auto bg=mixht_observed_mc(a);
    assert(bg.ziconv[0]>0.f&&bg.ziconv[0]<=a.zimax);
    assert(bg.dptt[0]==a.initial_dptt[0]);
    assert(bg.zi[0]>=a.zimin&&bg.zi[0]<=a.zimax);
    // Positive L collapses the convective height exactly, as in the source.
    a.el[0]=120.f;
    assert(mixht_observed_mc(a).ziconv[0]==0.f);
    // The sign of IMIXH only selects the over-water scheme.
    a.imixh=-2;a.el[0]=-120.f;
    assert(mixht_observed_mc(a).ziconv[0]>0.f);
    a.imixh=3;fails([&]{return mixht_observed_mc(a);});
    a.imixh=1;
}
