#include "cteas/p6/p6g_selected_hour_bridge.hpp"
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::chrono;
using namespace cteas::calmet::p6g;

static void require_test(bool ok,const char* message) {
    if(!ok)throw std::runtime_error(message);
}
static int day_of_year(sys_seconds instant) {
    const auto d=floor<days>(instant);
    return static_cast<int>((d-sys_days{year_month_day{d}.year()/January/1}).count())+1;
}
static void stamp(std::ostream& out,sys_seconds when,bool hour24=false) {
    if(hour24&&when==floor<days>(when)){
        when-=hours{1};
        const auto ymd=year_month_day{floor<days>(when)};
        out<<int(ymd.year())<<" "<<day_of_year(when)<<" 24 0";
        return;
    }
    const auto ymd=year_month_day{floor<days>(when)};
    out<<int(ymd.year())<<" "<<day_of_year(when)<<" "
       <<duration_cast<hours>(when-floor<days>(when)).count()<<" 0";
}
static void write_surface(const std::string& path,sys_seconds begin,
                          sys_seconds end) {
    std::ofstream out(path);
    require_test(static_cast<bool>(out),"cannot open synthetic SURF.DAT");
    out<<"SURF.DAT        2.1             Hour Start and End Times with Seconds\n"
       <<"1\nG8 10km 2 station synthetic annual driver, no real observations\n"
       <<"NONE\nUTC+0000\n";
    stamp(out,begin);out<<" ";stamp(out,end,true);out<<" 2\n";
    out<<"58321\n58322\n";
    const int total=static_cast<int>(duration_cast<hours>(end-begin).count());
    for(int h=0;h<total;++h) {
        const auto now=begin+hours{h};
        stamp(out,now);out<<" ";stamp(out,now+hours{1},true);out<<"\n";
        const float ws1=3.f+static_cast<float>(h%7)*0.1f;
        const float ws2=4.f+static_cast<float>(h%11)*0.1f;
        const int wd1=90+(h%19),wd2=120+(h%13);
        out<<std::fixed<<std::setprecision(3)
           <<ws1<<" "<<wd1<<" 722 5 283.000 60 1013.000 0\n"
           <<ws2<<" "<<wd2<<" 722 5 285.000 65 1008.000 0\n";
    }
    require_test(static_cast<bool>(out),"write synthetic SURF.DAT failed");
}
static void level(std::ostream& out,float pressure,int z,float temperature,
                  int direction,int speed) {
    out<<"   "<<std::fixed<<std::setprecision(1)<<std::setw(6)<<pressure
       <<"/"<<std::setw(5)<<z
       <<"/"<<std::setw(5)<<temperature
       <<"/"<<std::setw(3)<<direction
       <<"/"<<std::setw(3)<<speed;
}
static void write_upper(const std::string& path,int id,sys_seconds begin,
                        sys_seconds end) {
    std::ofstream out(path);
    require_test(static_cast<bool>(out),"cannot open synthetic UP.DAT");
    out<<"UP.DAT          2.1             Hour Start and End Times with Seconds\n"
       <<"1\nG8 10km original READ62 formatted input fixture\nNONE\nUTC+0000\n";
    stamp(out,begin);out<<" ";stamp(out,end);out<<" 500.0 2 1\nF F F F\n";
    int index=0;
    for(auto when=begin-hours{12};when<=end+hours{12};when+=hours{12},++index) {
        const auto d=year_month_day{floor<days>(when)};
        const int hour=static_cast<int>(
            duration_cast<hours>(when-floor<days>(when)).count());
        out<<"6201 "<<id<<" "
           <<int(d.year())<<" "<<unsigned(d.month())<<" "<<unsigned(d.day())
           <<" "<<hour<<" 0 "
           <<int(d.year())<<" "<<unsigned(d.month())<<" "<<unsigned(d.day())
           <<" "<<hour<<" 0 2 2\n";
        const int direction=90+(id%5)+index%5;
        level(out,1000.0f,120,280.f,direction,5+index%4);
        level(out,500.f,5500,259.8f,direction,12+index%4);
        out<<"\n";
    }
    require_test(static_cast<bool>(out),"write synthetic UP.DAT failed");
}
static std::uint64_t digest(std::uint64_t value,float sample) {
    value^=std::bit_cast<std::uint32_t>(sample);
    return value*1099511628211ULL;
}
static void exercise_period(sys_seconds begin,sys_seconds end,
                            int expected_hours,bool require_year_boundary) {
    const std::string surface_file="p6g_g8_10km_surf.dat";
    const std::string upper1_file="p6g_g8_10km_upper1.dat";
    const std::string upper2_file="p6g_g8_10km_upper2.dat";
    write_surface(surface_file,begin,end);
    write_upper(upper1_file,58424,begin,end);
    write_upper(upper2_file,58425,begin,end);
    const auto surface=read_surface_dynamic(surface_file,{58321,58322});
    const auto upper=read_upper_station_set(
        {upper1_file,upper2_file},{58424,58425});
    const std::string deck_text=
        "! SS2 = 'S2' 58322 545.885 3566.269 0 10.0 !\n"
        "! US2 = 'U2' 58425 540.885 3561.269 0 !\n"
        "! SS1 = 'S1' 58321 515.885 3536.269 0 10.0 !\n"
        "! US1 = 'U1' 58424 520.885 3541.269 0 !\n";
    const auto deck=original_calmet_multi_station_deck(
        deck_text,510.885f,3531.269f,2,2);
    const auto clock=bind_original_calmet_station_inputs(deck,surface,upper);
    require_test(clock.size()==static_cast<std::size_t>(expected_hours),
                 "year duration not present in G8 input chain");
    const SelectedBridgeGrid grid{
        6,6,10.0f,{0,20,40,80,160,320,640,1200,2000,3000}
    };
    // Published conceptual G8 test grid: 6x6 cells, 10km spacing,
    // 60x60 km footprint, 9 Z layers, 4 observation stations.
    require_test(grid.nx*grid.ny==36&&grid.spacing_km==10.f,
                 "G8 must test precisely 10km cells");
    const std::vector<float> zface(
        grid.zface_m.begin(),grid.zface_m.end());
    const auto prepared=prepare_upper_original_selected(upper,zface);
    std::uint64_t hash=1469598103934665603ULL;
    bool varied=false, crossed=false;
    float first_u=0.f;
    auto last=begin;
    std::size_t total_contributions=0;
    for(std::size_t hour=0;hour<clock.size();++hour) {
        const auto& input=clock[hour];
        require_test(input.end==begin+hours{static_cast<int>(hour)+1},
                     "G8 one-hour input clock drifted");
        const auto output=selected_multisite_hour(
            deck,surface.hourly[hour],input,prepared,grid);
        require_test(output.end==input.end,
                     "G8 selected INTER2 returned wrong hour");
        if(hour==0)first_u=output.center_u_mps;
        if(hour>0 && output.center_u_mps!=first_u)varied=true;
        if(year_month_day{floor<days>(output.end)}.year()!=
           year_month_day{floor<days>(last)}.year())
            crossed=true;
        last=output.end;
        total_contributions+=output.observed_contributions;
        hash=digest(hash,output.center_u_mps);
        hash=digest(hash,output.center_v_mps);
        hash=digest(hash,output.upper_u_mps);
        hash=digest(hash,output.upper_v_mps);
    }
    require_test(varied&&total_contributions>static_cast<std::size_t>(expected_hours),
                 "G8 selected multi-observation fields not active/variable");
    require_test(!require_year_boundary||crossed,
                 "G8 did not exercise Gregorian year boundary");
    require_test(hash!=1469598103934665603ULL,"G8 digest unchanged");
    std::cout<<"G8_10KM_SYNTHETIC_SELECTED_INTER2 hours="<<expected_hours
             <<" grid=6x6x9 spacing_km=10 surface=2 upper=2"
             <<" contributions="<<total_contributions
             <<" signature="<<std::hex<<hash<<std::dec<<"\n";
    std::remove(surface_file.c_str());
    std::remove(upper1_file.c_str());
    std::remove(upper2_file.c_str());
}
int main(){
    try{
        // Dec 2023 to Jan 2024 verifies leap-year transition and
        // both two-station readers with the original hour-24 record encoding.
        const auto cross=sys_days{year{2023}/December/31};
        exercise_period(cross,cross+hours{48},48,true);
        // 2024 is a leap year: actual 8784 consecutive UTC hours.
        const auto annual=sys_days{year{2024}/January/1};
        exercise_period(annual,annual+hours{8784},8784,true);
        std::cout<<"PASS P6G_G8_10KM_MULTI_SITE_FULL_LEAP_YEAR_SELECTED_KERNELS\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"P6G_G8_10KM_FAIL "<<e.what()<<"\n";
        return 1;
    }
}
