// G17B4 source IAVET=1: two distinct SURFVAR calls across FRADJ using
// original selected SURFVAR+AVETMP kernels. Actual 45-field multi-site
// physics is NOT accepted; WIND1/FRADJ arrays are controlled stage fixtures.
#include "cteas/p6/p6g_g17b4_iavet_two_pass.hpp"
#include "cteas/p6/p6g_g17_staged_terrain.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
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
        for(int j=0;j<12;++j)for(int i=0;i<12;++i)
            grid.terrain_m.push_back(100.f+150.f*i+100.f*j);
        station_set.surface={station(58321,10000.f,10000.f),
                             station(58322,100000.f,100000.f)};
        series.begin=begin;series.end=begin+hours{1};
        series.station_ids={58321,58322};
        p6g::SurfaceHour hour;
        hour.start=begin;hour.end=series.end;
        hour.by_station.resize(2);
        hour.by_station[0].wind_valid=true;
        hour.by_station[0].wind_speed_mps=5.f;
        hour.by_station[0].pressure_valid=true;
        hour.by_station[0].pressure_mb=1013.f;
        hour.by_station[0].temperature_valid=true;
        hour.by_station[0].temperature_k=281.f;
        hour.by_station[0].humidity_valid=true;
        hour.by_station[0].humidity_pct=60;
        hour.by_station[1].wind_valid=true;
        hour.by_station[1].wind_speed_mps=6.f;
        hour.by_station[1].pressure_valid=true;
        hour.by_station[1].pressure_mb=997.f;
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
static void verify_source_staged_iavet(
    const p6g::G17B4SurfaceControls& controls,
    const p6g::OriginalWindControls& wind,
    sys_seconds begin){
    Input f(controls,begin);
    auto surface=f.make();
    p6g::G16ObservedProfiles profiles;
    profiles.end=f.series.end;
    profiles.station_ids={58321,58322,58424,58425};
    for(int k=0;k<9;++k) {
        const std::vector<float> u{3.f,-3.f,8.f,9.f};
        const std::vector<float> v{4.f,4.f,-2.f,10.f};
        profiles.wind1_u_mps.insert(
            profiles.wind1_u_mps.end(),u.begin(),u.end());
        profiles.wind1_v_mps.insert(
            profiles.wind1_v_mps.end(),v.begin(),v.end());
    }
    profiles.wind1_present.assign(36,1);
    profiles.inter2_u_mps=profiles.wind1_u_mps;
    profiles.inter2_v_mps=profiles.wind1_v_mps;
    profiles.inter2_present=profiles.wind1_present;
    for(std::size_t k=4;k<9;++k) {
        profiles.inter2_present[k*4]=0;
        profiles.inter2_u_mps[k*4]=999.f;
        profiles.inter2_v_mps[k*4]=999.f;
    }
    const auto terrain=p6g::prepare_g11_terrain(f.grid,35.f);
    p6g::G17FradjSource forcing;
    forcing.end=f.series.end;
    forcing.provenance=p6g::TerrainHourProvenance::SyntheticReferenceFixture;
    forcing.isurft=-1;
    forcing.domain_temperature_k=285.f;
    forcing.critfn=10.f;
    forcing.lapse_k_per_m.assign(144,0.002f);
    // No externally invented TEMP2D. The first G17B4 SURFVAR fills it
    // using precisely G17's native WIND1 output.
    p6g::G17SelectedTerrainFixturePhysics stages(
        std::move(profiles),terrain,std::move(forcing));
    p6g::StationSet all=f.station_set;
    all.upper={station(58424,20000.f,20000.f),
               station(58425,50000.f,50000.f)};
    p6g::InputHour slot{};
    slot.end=f.series.end;slot.upper.resize(2);
    std::vector<p6g::UpperSeries> upper(2);
    upper[0].station_id=58424;upper[1].station_id=58425;
    p6::NativeGridMemory prior;
    prior.zi_m.assign(144,0.f);
    prior.ziconv_m.assign(144,0.f);
    prior.dptt_k.assign(144,0.f);
    p6g::MultiHourFrame frame{
        0,f.grid,all,f.series.hourly.front(),slot,upper,
        &wind,prior,prior,{}};
    rejects([&]{stages.prepare_original_iavet1_before_fradj(surface);},
            "G17B4 allowed first SURFVAR before source WIND1");
    check(stages.run(p6::NativeHourStage::DiagnoWind1,frame).completed,
          "G17B4 staged native WIND1 failed");
    stages.prepare_original_iavet1_before_fradj(surface);
    check(surface.first_stage_wind_matches(
              stages.wind_stages().first_guess.u_mps,
              stages.wind_stages().first_guess.v_mps),
          "G17B4 first SURFVAR did not use actual source WIND1");
    rejects([&]{stages.finish_original_iavet1_after_fradj(surface);},
            "G17B4 allowed second SURFVAR before native FRADJ");
    check(stages.run(p6::NativeHourStage::DiagnoFradj,frame).completed,
          "G17B4 source FRADJ rejected original first TEMP2D");
    stages.finish_original_iavet1_after_fradj(surface);
    check(surface.second_ready(),
          "G17B4 second original SURFVAR not computed after FRADJ");
    const auto second=surface.final_temperature();
    // Independent raw temperature + immutable AVETMP comparison based on
    // this G17 instance's actual FRADJ-adjusted lower-layer winds.
    p6e::AvetmpInput av;
    av.nx=12;av.ny=12;av.dgrid_m=10000.f;
    av.hafang_deg=controls.hafang_deg;av.mnmdav=controls.mnmdav;
    av.temperature_k=surface.raw_observed_temperature();
    const auto& fr=stages.after_fradj();
    av.u_ms.assign(fr.u.begin(),fr.u.begin()+144);
    av.v_ms.assign(fr.v.begin(),fr.v.begin()+144);
    check(second==p6e::avetmp(av),
          "G17B4 second source TEMP2D not based on native FRADJ wind");
    bool changed=false;
    for(std::size_t i=0;i<144;++i)
        changed|=(second[i]!=surface.first_temperature()[i]);
    check(changed,"G17B4 physical G17 FRADJ did not alter source TEMP2D");
    rejects([&]{
        stages.prepare_selected_airden(frame,surface.first_temperature());
    },"G17B4 AIRDEN incorrectly reuses pre-FRADJ TEMP2D");
    stages.prepare_selected_airden(frame,second);
    check(stages.run(p6::NativeHourStage::DiagnoAirden,frame).completed,
          "G17B4 source AIRDEN did not consume final observed TEMP2D");
    const auto& rho=stages.selected_air_density();
    for(std::size_t i=0;i<144;++i)
        check(rho[i]>0.f&&std::isfinite(rho[i]),
              "G17B4 AIRDEN invalid after two original SURFVAR passes");
    p6g::G17SlopeHeatSource heat;
    heat.end=f.series.end;
    heat.provenance=p6g::TerrainHourProvenance::SyntheticReferenceFixture;
    heat.temperature_k=surface.first_temperature();
    heat.sensible_heat_w_m2.assign(144,-120.f);
    rejects([&]{stages.provide_slope_heat(heat);},
            "G17B4 SLOPE accepted obsolete first TEMP2D");
    heat.temperature_k=second;
    stages.provide_slope_heat(heat);
    check(stages.run(p6::NativeHourStage::DiagnoSlope,frame).completed,
          "G17B4 SLOPE failed with original post-FRADJ TEMP2D");
    check(stages.run(p6::NativeHourStage::DiagnoWindbcFirstGuess,frame).completed,
          "G17B4 first WINDBC did not preserve corrected U/V");
    check(stages.run(p6::NativeHourStage::DiagnoInter2,frame).completed,
          "G17B4 original selected INTER2 rejected two-pass thermal fields");
    check(!stages.run(p6::NativeHourStage::CompGridMixht,frame).completed,
          "G17B4 invented native multi-station COMP acceptance");
    check(frame.next.ziconv_m==prior.ziconv_m,
          "G17B4 selected two-pass test modified n-1 accepted P6E state");
}
int main(int argc,char** argv){
    try{
        check(argc==2,"G17B4 pinned met.inp required");
        std::ifstream file(argv[1]);
        check(static_cast<bool>(file),"G17B4 source met.inp not found");
        const std::string deck((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
        const auto control=p6g::parse_g17b4_surface_controls(deck);
        const auto wind=p6g::parse_original_wind_controls(deck);
        check(control.source.iavet==1&&control.mnmdav==1&&
              control.hafang_deg==30.f&&control.source.irad==1&&
              control.source.numts==5&&control.source.trad_km==500.f,
              "G17B4 original Hefei wind-averaging controls diverged");
        const auto begin=sys_days{year{2024}/January/1}+hours{2};
        verify(control,begin);
        verify_source_staged_iavet(control,wind,begin);
        verify(control,begin+hours{11});
        // Accept original Fortran freeform control spacing, not a
        // hardcoded literal (e.g. source uses ! IAVET =   1 !).
        const std::regex control_pattern(
            R"(!\s*IAVET\s*=\s*1\s*!)",std::regex_constants::icase);
        check(std::regex_search(deck,control_pattern),
              "original source IAVET=1 field not found");
        const auto bad=std::regex_replace(
            deck,control_pattern,"! IAVET = 0 !");
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
