#include "cteas/puff/original_release_wind.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace cteas::puff;
namespace {
void require(bool p,const char* reason){
    if(!p)throw std::runtime_error(reason);
}
template<class Fn> void refuses(Fn&& fn) {
    bool threw=false;try{fn();}catch(const std::exception&){threw=true;}
    require(threw,"unsupported RLSMET/WINDSET input was accepted");
}
void synthetic() {
    OriginalMetHour m;
    m.nx=3;m.ny=3;m.nz=3;
    m.u.assign(27,1.0F);
    m.v.assign(27,0.0F);
    // Release is at level 2 with 3-4-5 triangle.
    for(std::size_t i=9;i<18;++i) {m.u[i]=3.0F;m.v[i]=4.0F;}
    Calmet21Static s;
    s.cell_face_m={0,100,200,300};
    s.roughness_m.assign(9,0.25F);
    const OriginalReleaseWindRequest req{.x_cells=1.5F,
        .y_cells=1.5F,.release_height_m=150.0F,.calm_threshold_ms=0.5F};
    const auto native=original_release_wind_metfm1(m,s,req);
    require(native.cell.ix_1based==2 && native.cell.iy_1based==2 &&
            native.layer_1based==2 && native.wind_speed_raw_ms==5.0F &&
            native.wind_speed_ms==5.0F && !native.calm,
            "native original WINDSET did not use RLSMET ZFIND layer");
    ReleaseCadenceInputs schedule;
    schedule.emission_interval=36.0;
    schedule.max_puffs=99;
    schedule.hour_seconds=3600;
    schedule.wind_speed_ms=1111; // must be overwritten by genuine wind
    const auto puff=original_points1_cadence_from_release_wind(
        m,s,req,schedule);
    require(puff.puff_count==99 &&
            puff.emission_duration_s==static_cast<float>(3600.0F/99.0F),
            "native RLSMET WINDSET did not feed POINTS1 WSNEW");
    // wind 1 m/s at level 1 → INT(36)+1=37.
    auto low=req;low.release_height_m=50.0F;
    const auto n=original_points1_cadence_from_release_wind(m,s,low,schedule);
    require(n.puff_count==37,"level-dependent native NPUFFS not applied");
    // A calm zero-wind must retain the original WDCALM and minimum
    // transport speed of 0.001, without forcing a 0.5 m/s substitute.
    m.u.assign(27,0);m.v.assign(27,0);
    const auto calm=original_release_wind_metfm1(m,s,req);
    require(calm.calm && calm.wind_speed_raw_ms==0.0F &&
            calm.wind_speed_ms==0.001F && calm.wind_direction_deg==180.0F &&
            calm.flow_deg==0.0F && calm.vector_direction_deg==0,
            "original RLSMET calm branch diverged");
    auto nonfinite=req;nonfinite.release_height_m=std::nanf("");
    refuses([&]{(void)original_release_wind_metfm1(m,s,nonfinite);});
    s.roughness_m.clear();
    refuses([&]{(void)original_release_wind_metfm1(m,s,req);});
}
struct DgmFirst {std::array<std::int32_t,24> ints;std::array<float,64> floats;};
static_assert(sizeof(DgmFirst)==352);
static_assert(std::endian::native==std::endian::little);
void real_first_hour(const std::string& met_path,const std::string& dgm_path){
    std::ifstream dump(dgm_path,std::ios::binary);
    require(static_cast<bool>(dump),"original 2022 first-hour DGM absent");
    DgmFirst first{};
    require(static_cast<bool>(dump.read(reinterpret_cast<char*>(&first),sizeof(first))),
            "original first DGM record unreadable");
    require(first.ints[0]==1 && first.ints[1]==202200100 &&
            first.ints[12]==0,"wrong original first-hour DGM record");
    std::size_t checked=0;
    OriginalMetHourlyReader::read(met_path,
      [&](const Calmet21Header& header,const Calmet21Static& static_met,
          const OriginalMetHour& hour) {
        if(checked!=0)return;
        require(header.nx==50 && header.ny==50 && header.nz==9,
                "frozen original MET dimensions changed");
        const auto observed=original_release_wind_metfm1(hour,static_met,{
            .x_cells=first.floats[0],.y_cells=first.floats[1],
            .release_height_m=first.floats[2],
            .calm_threshold_ms=0.5F
        });
        const float orig=first.floats[18]; // DGM WSVAR from original COMP
        const auto rel=std::abs(observed.wind_speed_ms-orig);
        std::cout<<"P7_NATIVE_RELEASE_WIND_FIRST_HOUR"
                 <<" ORIGINAL_WSVAR="<<orig
                 <<" NATIVE_WSNEW="<<observed.wind_speed_ms
                 <<" LAYER="<<observed.layer_1based
                 <<" DIFFERENCE="<<rel<<"\n";
        require(rel<0.00002F,
                "first-hour original WSVAR differs: original POINTS1 WSNEW or HEFF/GRID mismatch");
        ReleaseCadenceInputs schedule;
        schedule.wind_speed_ms=-100.0;
        schedule.emission_interval=36.0;
        schedule.hour_seconds=3600.0;
        schedule.max_puffs=99;
        const auto release=original_points1_cadence_from_release_wind(
            hour,static_met,{
              .x_cells=first.floats[0],.y_cells=first.floats[1],
              .release_height_m=first.floats[2],.calm_threshold_ms=0.5F
            },schedule);
        require(release.puff_count==54,
                "native original source wind cannot reproduce first-hour 54 POINTS1 puffs");
        ++checked;
      },true);
    require(checked==1,"frozen first-hour MET record absent");
    std::cout<<"P7_NATIVE_WINDSET_RLSMET_RELEASE=PASS"
             <<" FROZEN_FIRST_HOUR_POINT_COUNT=54"
             <<" ORIGINAL_143H_FULL_RLSMET=NOT_YET_ACCEPTED\n";
}
}
int main(int argc,char** argv){
    try{
        synthetic();
        if(argc==3) real_first_hour(argv[1],argv[2]);
        else if(argc!=1)throw std::invalid_argument("expected [met.dat first-dgm.bin]");
        std::cout<<"P7_NATIVE_WINDSET_METFM1=PASS SOURCE_SELECTED=YES\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"P7_NATIVE_WINDSET_METFM1=FAIL "<<e.what()<<"\n";
        return 1;
    }
}
