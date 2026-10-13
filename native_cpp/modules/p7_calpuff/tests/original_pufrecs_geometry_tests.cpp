#include "cteas/puff/original_pufrecs_geometry.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace cteas::puff;
static void check(bool ok,const char* what){
    if(!ok)throw std::runtime_error(what);
}
int main(){
    try{
        auto a=original_slgfrac(50,1,100,0,0,0);
        check(a.along_m==50 && a.across_m==1 &&
              a.fraction==0.5F && a.projected_length_m==100.0F,
              "original SLGFRAC along/cross/fraction mapping");
        auto small=original_slgfrac(0.5F,0.5F,0.5F,0,0,0);
        check(small.fraction==0 && small.projected_length_m==1 &&
              small.inverse_length_per_m==1,
              "vendor projected-length<1m branch changed");
        auto base=original_pufrecs_geometry({
            .x_receptor_m=200,.x_begin_m=0,.x_end_m=100,
            .sigma_y_begin_m=10,.sigma_y_end_m=20,.fclip=1,
            .travel_distance_begin_m=100,.mean_wind_ms=2,.clip=false});
        check(base.fraction==2 && base.along_m==200 &&
              base.distance_to_receptor_m==300 &&
              base.time_to_receptor_s==150,
              "original PUFRECS must extrapolate when LCLIP false");
        auto clip=original_pufrecs_geometry({
            .x_receptor_m=200,.x_begin_m=0,.x_end_m=100,
            .sigma_y_begin_m=10,.sigma_y_end_m=20,.fclip=1,
            .travel_distance_begin_m=100,.mean_wind_ms=2,.clip=true});
        check(std::abs(clip.fraction-1.2F)<1.0e-6F &&
              std::abs(clip.along_m-120.0F)<1.0e-4F &&
              std::abs(clip.time_to_receptor_s-110.0F)<1.0e-4F && clip.clipped,
              "original PUFRECS FCLIP extension failed");
        auto upstream=original_pufrecs_geometry({
            .x_receptor_m=-500,.x_begin_m=0,.x_end_m=100,
            .sigma_y_begin_m=10,.sigma_y_end_m=20,.fclip=1,
            .travel_distance_begin_m=30,.mean_wind_ms=3,.clip=false});
        check(upstream.distance_to_receptor_m==0 &&
              upstream.time_to_receptor_s==0 &&
              std::abs(upstream.fraction+0.3F)<1.0e-6F,
              "original PUFRECS source-boundary reset failed");
        bool rejected=false;
        try{(void)original_pufrecs_geometry({.mean_wind_ms=0});}
        catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"PUFRECS accepted calm UAVG without original calm branch");
        std::cout<<"P7_ORIGINAL_PUFRECS_GEOMETRY=PASS SLGFRAC=REAL4"
                 <<" FCLIP=YES UPWIND_SOURCE=YES"
                 <<" FULL_PUFRECS_SIGMA_GRISE_TERRAIN=OPEN\\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"P7_ORIGINAL_PUFRECS_GEOMETRY=FAIL "<<e.what()<<"\n";
        return 1;
    }
}
