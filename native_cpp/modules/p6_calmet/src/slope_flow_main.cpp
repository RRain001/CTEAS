// Text-only fixed 50x50/9-level Fortran numerical audit driver.
// Input order: terrain, hmax, hmin, disthmax, disthmin, qh, temp2d, ilandu.
#include "cteas/p6/slope_flow.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc,char** argv){
  try{
    if(argc!=4)throw std::invalid_argument("usage: slope INPUT OUTPUT ISURFT");
    using namespace cteas::calmet::p6;
    SlopeFlowInput in;
    in.nx=50;in.ny=50;in.nz=9;in.dx_m=100;in.dy_m=100;
    in.tinf_k=285;in.isurft=std::stoi(argv[3]);
    in.water_first=50;in.water_last=55;
    in.zface_m={0,20,40,80,160,320,640,1200,2000,3000};
    std::ifstream fin(argv[1]);
    if(!fin)throw std::runtime_error("cannot read input");
    auto readf=[&](std::vector<float>& dst) {
       dst.resize(2500);
       for(auto& x:dst)if(!(fin>>x))throw std::runtime_error("truncated float field");
    };
    readf(in.terrain_m);readf(in.highest_m);readf(in.lowest_m);
    readf(in.distance_highest_m);readf(in.distance_lowest_m);
    readf(in.sensible_heat_w_m2);readf(in.surface_temperature_k);
    in.landuse.resize(2500);
    for(auto& x:in.landuse)if(!(fin>>x))throw std::runtime_error("truncated landuse");
    const auto out=slope_flow(in);
    std::ofstream fout(argv[2]);
    if(!fout)throw std::runtime_error("cannot write output");
    fout<<"i,j,k,u_mps,v_mps\n";
    fout<<std::setprecision(12);
    for(int k=0;k<9;++k)for(int j=0;j<50;++j)for(int i=0;i<50;++i){
       const auto n=std::size_t(k)*2500+std::size_t(j)*50+std::size_t(i);
       fout<<i+1<<','<<j+1<<','<<k+1<<','<<out.u_mps[n]<<','<<out.v_mps[n]<<'\n';
    }
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
