// G9 independent source-lift WIND1 reference fixture. Fortran driver builds
// from the actual vendor calmet.for L36204-L36397 (unchanged source lines).
#include "cteas/p6/p6g_multi_wind1_firstguess.hpp"
#include <iomanip>
#include <iostream>

int main() {
    using cteas::calmet::p6g::Wind1MultiInput;
    using cteas::calmet::p6g::multi_station_wind1_firstguess;
    Wind1MultiInput in;
    in.nx=3;in.ny=3;in.nz=3;
    in.spacing_m=10000.f;
    in.surface_count=2;in.upper_count=2;in.icalm=0;
    in.station_x_m={10000.f,30000.f,20000.f,50000.f};
    in.station_y_m={10000.f,40000.f,20000.f,50000.f};
    in.surface_speeds_mps={0.f,5.f};
    in.vertical_bias={0.f,0.3f,-0.2f};
    in.station_u_mps.resize(12);
    in.station_v_mps.resize(12);
    in.present.assign(12,1);
    for(int k=0;k<3;++k){
        in.station_u_mps[k*4+0]=0.f;
        in.station_v_mps[k*4+0]=0.f;
        in.station_u_mps[k*4+1]=3.f;
        in.station_v_mps[k*4+1]=4.f;
        in.station_u_mps[k*4+2]=5.f;
        in.station_v_mps[k*4+2]=-2.f;
        in.station_u_mps[k*4+3]=9.f;
        in.station_v_mps[k*4+3]=10.f;
    }
    in.present[1*4+3]=0;
    in.station_u_mps[1*4+3]=999999.f;
    in.station_v_mps[1*4+3]=999999.f;
    const auto out=multi_station_wind1_firstguess(in);
    std::cout<<std::scientific<<std::setprecision(9);
    for(int k=0;k<3;++k)
        for(int j=0;j<3;++j)
            for(int i=0;i<3;++i){
                const auto idx=static_cast<std::size_t>(k*9+j*3+i);
                std::cout<<"GRID "<<i+1<<" "<<j+1<<" "<<k+1<<" "
                         <<out.u_mps[idx]<<" "<<out.v_mps[idx]<<"\n";
            }
}
