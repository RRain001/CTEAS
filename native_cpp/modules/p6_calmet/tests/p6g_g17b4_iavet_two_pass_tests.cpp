// G17B4 source IAVET=1: two distinct SURFVAR calls across FRADJ using
// original selected SURFVAR+AVETMP kernels. Actual 45-field multi-site
// physics is NOT accepted; WIND1/FRADJ arrays are controlled stage fixtures.
#include "cteas/p6/p6g_g17b4_iavet_two_pass.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
namespace p6=cteas::calmet::p6;
namespace p6e=cteas::p6e;
namespace p6g=cteas::calmet::p6g;
using namespace std::chrono;
static void check(bool yes,const char* msg) {
    if(!yes)throw std::runtime_error(msg);
}
template<class F>static void rejects(F&& f,const char* why) {
    bool denied=false;
    try{f();}catch(const std::exception&){denied=true;}
    check(denied,why);
}
static p6::DeckStation station(int id,float x,float y){
    p6::DeckStation s{};s.id=id;s.local_x_m=x;s.local_y_m=y;return s;
}
struct Input {
    p6g::GeoGrid grid;
    p6g::StationSet station_set;
    p6g::SurfaceSeries series;
    p6g::G17B4SurfaceControls controls;
    explicit Input(const p6g::G17B4SurfaceControls& source,
                   sys_seconds begin) : controls(source){
        grid.nx=12;grid.ny=12;grid.grid_km=10.f;
        grid.iwat1=50;grid.iwat2=55;grid.landuse.assign(144,20);
        station_set.surface={station(58321,10000.f,10000.f),
                             station(58322,100000.f,100000.f)};
        series.begin=begin;series.end=begin+hours{1};
        series.station_ids={58321,58322};
        p6g::SurfaceHour hour;
        hour.start=begin;hour.end=series.end;
        hour.by_station.resize(2);
        hour.by_station[0].temperature_valid=true;
        hour.by_station[0].temperature_k=281.f;
        hour.by_station[0].humidity_valid=true;
        hour.by_station[0].humidity_pct=60;
        hour.by_station[1].temperature_valid=true;
        hour.by_station[1].temperature_k=302.f;
        hour.by_station[1].humidity_valid=true;
        hour.by_station[1].humidity_pct=73;
        series.hourly.push_back(hour);
    }
    p6g::G17B4TwoPassSurfvar make() const{
        return {grid,station_set,series,series.hourly.front(),controls};
    }
};
static std::vector<float> winds(int component,bool corrected) {
    std::vector<float> out(144*9);
    for(int k=0;k<9;++k)
        for(int j=0;j<12;++j)
            for(int i=0;i<12;++i) {
                const auto p=static_cast<std::size_t>(k*144+j*12+i);
                // The post-FRADJ field has a different horizontal direction.
                const float base=corrected ? (component==0?-8.f:2.f)
                                           : (component==0?8.f:-2.f);
                out[p]=base+static_cast<float>((i+2*j)%5)*0.03f;
            }
    return out;
}
static void verify(const p6g::G17B4SurfaceControls& controls,
                   sys_seconds begin){
    Input f(controls,begin);
    auto two=f.make();
    auto pre_u=winds(0,false),pre_v=winds(1,false);
    auto post_u=winds(0,true),post_v=winds(1,true);
    check(two.end()==f.series.end&&!two.first_ready()&&!two.second_ready(),
          "G17B4 IAVET source state not initially empty");
    rejects([&]{(void)two.result();},"G17B4 returned final field without stages");
    rejects([&]{two.after_fradj_before_heatfx(
        f.series.end,post_u,post_v);},
        "G17B4 accepted second IAVET before first wind");
    rejects([&]{two.after_wind1_before_fradj(
        f.series.end+hours{1},pre_u,pre_v);},
        "G17B4 accepted stale first model hour");
    auto short_wind=pre_u;short_wind.pop_back();
    rejects([&]{two.after_wind1_before_fradj(
        f.series.end,short_wind,pre_v);},
        "G17B4 accepted incomplete source 3D WIND1");
    two.after_wind1_before_fradj(f.series.end,pre_u,pre_v);
    check(two.first_ready()&&!two.second_ready(),
          "G17B4 source first SURFVAR did not commit");
    rejects([&]{(void)two.final_temperature();},
            "G17B4 invented post-FRADJ temperature before second call");
    rejects([&]{two.after_wind1_before_fradj(
        f.series.end,pre_u,pre_v);},
        "G17B4 silently duplicated source first SURFVAR");
    rejects([&]{two.after_fradj_before_heatfx(
        f.series.end+hours{1},post_u,post_v);},
        "G17B4 accepted next-hour post-FRADJ winds");
    // Independent numerical check against immutable #53 P6E AVETMP kernel.
    p6e::SurfvarMultistationInput base;
    base.nx=12;base.ny=12;base.dgrid_m=10000.f;
    base.irad=controls.source.irad;base.numts=controls.source.numts;
    base.trad_grid=controls.source.trad_km/10.f;
    base.stations={{10000.f,10000.f,281.f,60},
                   {100000.f,100000.f,302.f,73}};
    const auto raw=p6e::surfvar_multistation(base);
    check(two.raw_observed_temperature()==raw.temp2d_k,
          "G17B4 observation weighting differs from original SURFVAR");
    auto direct=[&](const std::vector<float>& u,
                    const std::vector<float>& v) {
        p6e::AvetmpInput a;
        a.nx=12;a.ny=12;
        a.mnmdav=controls.mnmdav;
        a.hafang_deg=controls.hafang_deg;
        a.dgrid_m=10000.f;
        a.temperature_k=raw.temp2d_k;
        a.u_ms.assign(u.begin(),u.begin()+144);
        a.v_ms.assign(v.begin(),v.begin()+144);
        return p6e::avetmp(a);
    };
    const auto expected_first=direct(pre_u,pre_v);
    check(two.first_temperature()==expected_first,
          "G17B4 pre-FRADJ source AVETMP field differs elementwise");
    two.after_fradj_before_heatfx(f.series.end,post_u,post_v);
    const auto out=two.result();
    const auto expected_final=direct(post_u,post_v);
    check(out.complete&&out.end==f.series.end&&
          out.temperature_before_fradj_k==expected_first&&
          out.temperature_after_fradj_k==expected_final,
          "G17B4 post-FRADJ source AVETMP field differs elementwise");
    std::size_t different=0;
    for(std::size_t p=0;p<144;++p)
        different+=out.temperature_before_fradj_k[p]!=
                   out.temperature_after_fradj_k[p];
    check(different>0,"G17B4 two source wind snapshots produce identical TEMP2D");
    rejects([&]{two.after_fradj_before_heatfx(f.series.end,post_u,post_v);},
            "G17B4 allowed duplicate second SURFVAR");
    auto invalid=f;
    std::swap(invalid.series.station_ids[0],invalid.series.station_ids[1]);
    rejects([&]{(void)invalid.make();},"G17B4 accepted shuffled station IDs");
    invalid=f;invalid.grid.landuse[0]=50;
    rejects([&]{(void)invalid.make();},
            "G17B4 falsely accepted untested water SURFVAR branch");
    invalid=f;invalid.controls.source.iavet=0;
    rejects([&]{(void)invalid.make();},
            "G17B4 accepted IAVET=0 in original two-pass route");
    invalid=f;invalid.series.hourly.front().by_station[0].temperature_valid=false;
    invalid.series.hourly.front().by_station[1].temperature_valid=false;
    rejects([&]{(void)invalid.make();},
            "G17B4 invented missing multi-site TEMP2D observations");
}
int main(int argc,char** argv){
    try{
        check(argc==2,"G17B4 pinned met.inp required");
        std::ifstream file(argv[1]);
        check(static_cast<bool>(file),"G17B4 source met.inp not found");
        const std::string deck((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
        const auto control=p6g::parse_g17b4_surface_controls(deck);
        check(control.source.iavet==1&&control.mnmdav==1&&
              control.hafang_deg==30.f&&control.source.irad==1&&
              control.source.numts==5&&control.source.trad_km==500.f,
              "G17B4 original Hefei wind-averaging controls diverged");
        const auto begin=sys_days{year{2024}/January/1}+hours{2};
        verify(control,begin);
        verify(control,begin+hours{11});
        auto bad=deck;
        const auto p=bad.find("! IAVET = 1");
        check(p!=std::string::npos,"source IAVET control missing");
        bad.replace(p,11,"! IAVET = 0");
        rejects([&]{(void)p6g::parse_g17b4_surface_controls(bad);},
                "G17B4 silently accepted source IAVET=0 alternative");
        std::cout<<"PASS_G17B4_SOURCE_IAVET1_TWO_PASS "
                    "METINP_MNMDAV_HAFANG_REAL4 "
                    "WIND1_FIRST_FRADJ_SECOND DIAGNO_ORDER "
                    "IAVET1_FULL_MULTI_SITE_PHYSICS=NOT_YET_ACCEPTED\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FAIL_G17B4 "<<e.what()<<"\n";
        return 1;
    }
}
