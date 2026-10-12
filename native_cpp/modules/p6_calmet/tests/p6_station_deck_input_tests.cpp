#include "cteas/p6/calmet_station_deck.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using cteas::calmet::p6::original_calmet_station_deck;

static void expect(bool value,const char* msg) {
    if(!value)throw std::runtime_error(msg);
}
template<class F> static void must_fail(F&& f,const char* msg) {
    bool caught=false;
    try { f(); } catch (const std::invalid_argument&) { caught=true; }
    expect(caught,msg);
}

int main(int argc,char** argv) {
 try {
    const std::string deck=R"(
! SS1 ='S1'    583210    513.385   3533.769  0 10.0 !
! US1 ='U1'    58424     513.384   3533.772  0 !
)";
    const auto xy=original_calmet_station_deck(deck,510.885f,3531.269f);
    expect(xy.surface.id==583210&&xy.upper.id==58424,"station ids");
    expect(xy.surface.local_x_m==2500.f&&xy.surface.local_y_m==2500.f,
           "surface local geometry");
    expect(std::abs(xy.upper.local_x_m-2498.9624f)<1e-4f,
           "upper single-precision local x; not equirectangular");
    expect(std::abs(xy.upper.local_y_m-2502.9297f)<1e-4f,
           "upper single-precision local y; not equirectangular");
    const float from_deck=513.384f-510.885f;
    expect(std::bit_cast<std::uint32_t>(xy.upper.local_x_m)==
           std::bit_cast<std::uint32_t>(from_deck*1000.f),
           "single precision subtraction must precede metre conversion");
    const auto shifted=original_calmet_station_deck(
        "! SS1 ='A' 1 510.200 3530.100 0 10.0 !\n"
        "! US1 ='B' 2 509.950 3530.050 0 !\n",
        510.000f,3530.000f);
    expect(shifted.surface.local_x_m>0.f&&shifted.upper.local_x_m<0.f,
           "valid outside-grid station must not be clamped");
    must_fail([&]{(void)original_calmet_station_deck(
        "! SS1 ='S' 1 1 2 0 10 !",0.f,0.f);},"missing US1 not rejected");
    must_fail([&]{(void)original_calmet_station_deck(
        deck+"\n! SS1 ='duplicate' 583210 513 3533 0 10 !",510.f,3530.f);
    },"duplicate SS1 not rejected");
    must_fail([&]{(void)original_calmet_station_deck(
        "! SS2 ='S' 1 1 2 0 10 !\n! US1 ='U' 2 1 2 0 !",
        0.f,0.f);},"unsupported SS2 not rejected");
    must_fail([&]{(void)original_calmet_station_deck(
        "! SS1 ='S' 1 1e999 2 0 10 !\n! US1 ='U' 2 1 2 0 !",
        0.f,0.f);},"nonfinite station not rejected");
    if(argc==2) {
        std::ifstream in(argv[1]);
        if(!in)throw std::runtime_error("cannot read frozen met.inp");
        std::ostringstream s;s<<in.rdbuf();
        const auto real=original_calmet_station_deck(s.str(),510.885f,3531.269f);
        expect(real.surface.id==583210&&real.upper.id==58424,
               "frozen input station identities");
        expect(real.surface.local_x_m==2500.f&&real.surface.local_y_m==2500.f,
               "frozen surface UTM station position");
        expect(std::abs(real.upper.local_x_m-2498.9624f)<1e-4f,
               "frozen upper UTM x position");
        expect(std::abs(real.upper.local_y_m-2502.9297f)<1e-4f,
               "frozen upper UTM y position");
    }
    std::cout<<"PASS_CALMET_DECK_STATIONS_FLOAT32\n";
    return 0;
 } catch(const std::exception& e) {
    std::cerr<<"FAIL_CALMET_DECK_STATIONS: "<<e.what()<<"\n";
    return 1;
 }
}
