// G17B1: source-computed 2SS TEMP2D and solar with native HEATFX,
// no externally manufactured sunshine; P6E ancillary values remain
// explicit TEST fixtures, not accepted CALMET physics/production.
#include "cteas/p6/p6g_g17b_observed_solar_heat.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace p6=cteas::calmet::p6;
namespace p6g=cteas::calmet::p6g;
using namespace std::chrono;
static void check(bool condition,const char* why) {
    if(!condition)throw std::runtime_error(why);
}
template<class F> static void reject(F&& call,const char* why) {
    bool fails=false;
    try{call();}catch(const std::exception&){fails=true;}
    check(fails,why);
}
static p6::DeckStation station(int id,float x,float y) {
    p6::DeckStation s{};
    s.id=id;s.local_x_m=x;s.local_y_m=y;return s;
}
struct Fixture {
    p6g::GeoGrid geo;
    p6g::StationSet deck;
    p6g::SurfaceSeries surface;
    p6g::G17BExplicitAncillaryFixture anc;
    explicit Fixture(sys_seconds start) {
        geo.nx=12;geo.ny=12;geo.grid_km=10.f;
        geo.zone=50;geo.hemisphere='N';
        geo.xorigin_km=510.885f;geo.yorigin_km=3531.269f;
        geo.iwat1=50;geo.iwat2=55;
        geo.landuse.assign(144,20);geo.landuse[0]=50;
        geo.terrain_m.assign(144,200.f);
        geo.albedo.assign(144,0.2f);
        geo.bowen.assign(144,0.5f);
        geo.hcg.assign(144,0.2f);
        geo.qf.assign(144,0.f);
        deck.surface={station(58321,10000.f,10000.f),
                      station(58322,100000.f,100000.f)};
        surface.station_ids={58321,58322};
        surface.begin=start;surface.end=start+hours{1};
        p6g::SurfaceHour h;
        h.start=start;h.end=surface.end;
        h.by_station.resize(2);
        h.by_station[0].temperature_k=281.f;
        h.by_station[0].temperature_valid=true;
        h.by_station[0].humidity_pct=60;
        h.by_station[0].humidity_valid=true;
        h.by_station[0].cloud_valid=true;
        h.by_station[0].opaque_cloud_tenths=2;
        h.by_station[1].temperature_k=302.f;
        h.by_station[1].temperature_valid=true;
        h.by_station[1].humidity_pct=73;
        h.by_station[1].humidity_valid=true;
        h.by_station[1].cloud_valid=true;
        h.by_station[1].opaque_cloud_tenths=7;
        surface.hourly.push_back(h);
        anc.explicitly_test_only=true;
        anc.irad=1;anc.numts=2;anc.trad_grid=20.f;
        anc.isurft=-1;anc.domain_temperature_k=285.f;anc.critfn=10.f;
        anc.coefficients={990.f,-30.f,-0.75f,3.4f,0.f,0.f,1.f};
        anc.nearest_surface.resize(144);
        for(int j=0;j<12;++j)for(int i=0;i<12;++i)
            anc.nearest_surface[static_cast<std::size_t>(j*12+i)]=i<6?1:2;
        anc.initial_water_qh_wm2.assign(144,0.f);
        anc.initial_water_qh_wm2[0]=7.f;
        anc.lapse_k_per_m.assign(144,0.002f);
    }
    p6g::G17BObservedSolarResult calculate()const{
        return p6g::observed_solar_heat_selected(
            geo,deck,surface,surface.hourly.front(),anc);
    }
};
static void verify_real_solar(const Fixture& f) {
    const auto r=f.calculate();
    check(r.end==f.surface.end &&
          r.sun_sine.size()==144&&r.observed.temp2d_k.size()==144&&
          r.sensible_heat_wm2.size()==144,"G17B source array geometry");
    check(r.fradj_fixture.temperature_k==r.slope_fixture.temperature_k &&
          r.fradj_fixture.temperature_k==r.observed.temp2d_k,
          "G17B actual SURFVAR TEMP2D differs between FRADJ and SLOPE");
    check(r.fradj_fixture.provenance==
              p6g::TerrainHourProvenance::SyntheticReferenceFixture &&
          r.slope_fixture.provenance==
              p6g::TerrainHourProvenance::SyntheticReferenceFixture,
          "G17B incorrectly promoted unaccepted ancillaries to production");

    // Independent direct SURFVAR invocation, same immutable source kernel.
    cteas::p6e::SurfvarMultistationInput si;
    si.nx=12;si.ny=12;si.dgrid_m=10000.f;
    si.irad=1;si.numts=2;si.trad_grid=20.f;
    si.stations={{10000.f,10000.f,281.f,60},
                 {100000.f,100000.f,302.f,73}};
    const auto observed=cteas::p6e::surfvar_multistation(si);
    check(observed.temp2d_k==r.observed.temp2d_k&&
          observed.rh2d_pct==r.observed.rh2d_pct,
          "G17B SURFVAR station source numerical mismatch");
    check(r.observed.temp2d_k[13]<r.observed.temp2d_k[130],
          "G17B does not honor two distinct surface stations");

    // Source SOLAR computes the grid and the BEGIN-HOUR mid-interval slot;
    // external fabricated solar forcing is not an input to G17B.
    const auto coords=cteas::p6e::surface_solar_grid(
        12,12,50,f.geo.xorigin_km*1000.f,
        f.geo.yorigin_km*1000.f,10000.f);
    const auto day=floor<days>(f.surface.begin);
    const auto yr=sys_days{year_month_day{day}.year()/January/1};
    const int julian=static_cast<int>((day-yr).count())+1;
    const int hour=static_cast<int>(
        duration_cast<hours>(f.surface.begin-day).count());
    const auto solar=cteas::p6e::sinalpc_grid(coords,julian,hour,0);
    check(solar==r.sun_sine,"G17B original SOLAR midhour slot mismatch");
    for(float v:r.sun_sine)check(std::isfinite(v)&&v>=-1.f&&v<=1.f,
                               "G17B solar physical range");

    // HEATFX uses the REAL source-calculated solar and SURFVAR, explicit
    // unaccepted ancillary inputs, original MCLOUD=1 station-nearest branch.
    cteas::p6e::HeatFluxInput hi;
    hi.nx=12;hi.ny=12;
    hi.mcloud=1;hi.water_first=50;hi.water_last=55;
    hi.ha1=990.f;hi.ha2=-30.f;hi.hb1=-0.75f;hi.hb2=3.4f;
    hi.hc1=0.f;hi.hc2=0.f;hi.hc3p1=1.f;
    hi.station_cloud={2,7};hi.nearest=f.anc.nearest_surface;
    hi.landuse=f.geo.landuse;hi.sun=solar;
    hi.temp_k=observed.temp2d_k;
    hi.cloud_grid.assign(144,0.f);
    hi.albedo=f.geo.albedo;hi.bowen=f.geo.bowen;
    hi.soil_heat_fraction=f.geo.hcg;
    hi.anthropogenic_wm2=f.geo.qf;
    hi.initial_qh=f.anc.initial_water_qh_wm2;
    const auto direct=cteas::p6e::heatfx(hi);
    check(direct.shortwave_wm2==r.shortwave_wm2&&
          direct.sensible_wm2==r.sensible_heat_wm2,
          "G17B HEATFX differs from original selected kernel");
    check(r.sensible_heat_wm2[0]==7.f,
          "G17B source HEATFX did not preserve water QH");
    for(float v:r.sensible_heat_wm2)
        check(std::isfinite(v),"G17B nonfinite source HEATFX flux");
}
int main() {
    try {
        const auto noon=sys_days{year{2024}/January/1}+hours{4};
        Fixture winter_day(noon);
        verify_real_solar(winter_day);
        Fixture winter_night(noon+hours{12});
        verify_real_solar(winter_night);
        const auto daylight=winter_day.calculate();
        const auto nighttime=winter_night.calculate();
        check(daylight.sun_sine[20]>nighttime.sun_sine[20]&&
              daylight.shortwave_wm2[20]>nighttime.shortwave_wm2[20],
              "G17B real solar model has no diurnal forcing");
        check(nighttime.sun_sine[20]<0.f&&
              nighttime.sensible_heat_wm2[20]==-0.1f,
              "G17B original HEATFX night branch not followed");
        auto altered=winter_day;
        altered.surface.hourly.front().by_station[0].temperature_k=291.f;
        check(altered.calculate().observed.temp2d_k[13]>
              daylight.observed.temp2d_k[13],
              "G17B updated real station temperature did not propagate");
        altered=winter_day;
        altered.surface.hourly.front().by_station[0].opaque_cloud_tenths=9;
        check(altered.calculate().shortwave_wm2[20]<
              daylight.shortwave_wm2[20],
              "G17B source station cloud did not enter HEATFX");
        altered=winter_day;altered.anc.explicitly_test_only=false;
        reject([&]{(void)altered.calculate();},
               "G17B allowed externally controlled P6E ancillaries as production");
        altered=winter_day;altered.geo.hemisphere='S';
        reject([&]{(void)altered.calculate();},
               "G17B used unsupported UTM south source geometry");
        altered=winter_day;altered.surface.station_ids={58322,58321};
        reject([&]{(void)altered.calculate();},
               "G17B accepted reversed original SSn identity");
        altered=winter_day;altered.surface.hourly.front().by_station[0].cloud_valid=false;
        reject([&]{(void)altered.calculate();},
               "G17B fabricated missing station cloud");
        altered=winter_day;altered.anc.nearest_surface[30]=3;
        reject([&]{(void)altered.calculate();},
               "G17B accepted invalid external NEARS site");
        altered=winter_day;altered.anc.coefficients.hc3p1=0.f;
        reject([&]{(void)altered.calculate();},
               "G17B silently supplied missing source heat coefficients");
        altered=winter_day;altered.anc.initial_water_qh_wm2.pop_back();
        reject([&]{(void)altered.calculate();},
               "G17B substituted original water QH");
        altered=winter_day;altered.anc.lapse_k_per_m.pop_back();
        reject([&]{(void)altered.calculate();},
               "G17B accepted incomplete original lapse input");
        std::cout<<"PASS_P6G_G17B_OBS_SURFVAR_COMPUTED_SOLAR_HEATFX "
                    "2SS_10KM_REAL_UTM UTC_DAY_NIGHT "
                    "SOURCE_KERNEL_ELEMENTWISE_EQUAL "
                    "EXPLICIT_ANCILLARIES=FIXTURE_ONLY "
                    "FULL_P6E_AND_45_FIELD_PARITY=NOT_YET_ACCEPTED\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<"FAIL_P6G_G17B "<<e.what()<<"\n";return 1;
    }
}
