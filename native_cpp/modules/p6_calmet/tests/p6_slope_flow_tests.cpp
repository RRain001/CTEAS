#include "cteas/p6/slope_flow.hpp"
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace cteas::calmet::p6;
int main(){
  try{
    SlopeFlowInput in;
    in.nx=4;in.ny=4;in.nz=3;in.dx_m=100;in.dy_m=100;
    in.tinf_k=285;in.isurft=-1;in.water_first=50;in.water_last=55;
    in.zface_m={0,20,40,80};
    for(int j=0;j<4;++j)for(int i=0;i<4;++i){
      const float h=100.f+8.f*i+2.f*j;
      in.terrain_m.push_back(h);
      in.highest_m.push_back(h+100.f);
      in.lowest_m.push_back(h-100.f);
      in.distance_highest_m.push_back(150.f);
      in.distance_lowest_m.push_back(160.f);
      in.sensible_heat_w_m2.push_back(-70.f);
      in.surface_temperature_k.push_back(284.f);
      in.landuse.push_back(10);
    }
    auto a=slope_flow(in);
    if(a.u_mps.size()!=48||a.v_mps.size()!=48)
        throw std::runtime_error("wrong 3D output shape");
    if(!(std::abs(a.u_mps[5])+std::abs(a.v_mps[5])>0.f))
        throw std::runtime_error("negative QH slope flow unexpectedly zero");
    for(int k=1;k<3;++k)for(int p=0;p<16;++p)
      if(a.u_mps[std::size_t(k)*16+p]!=0.f||
         a.v_mps[std::size_t(k)*16+p]!=0.f)
         throw std::runtime_error("flow leaked above thickness");
    in.landuse[5]=52;
    a=slope_flow(in);
    for(int k=0;k<3;++k)
      if(a.u_mps[std::size_t(k)*16+5]!=0.f||
         a.v_mps[std::size_t(k)*16+5]!=0.f)
         throw std::runtime_error("water slope flow not suppressed");
    in.landuse[5]=10;
    for(float& q:in.sensible_heat_w_m2)q=+100.f;
    a=slope_flow(in);
    if(!(std::abs(a.u_mps[5])+std::abs(a.v_mps[5])>0.f))
        throw std::runtime_error("positive QH slope flow unexpectedly zero");
    for(float& h:in.terrain_m)h=100.f;
    for(float& h:in.highest_m)h=100.f;
    for(float& h:in.lowest_m)h=100.f;
    a=slope_flow(in);
    for(float x:a.u_mps)if(x!=0.f)throw std::runtime_error("flat-terrain U nonzero");
    for(float x:a.v_mps)if(x!=0.f)throw std::runtime_error("flat-terrain V nonzero");
    in.sensible_heat_w_m2[0]=std::numeric_limits<float>::quiet_NaN();
    bool rejected=false;
    try{(void)slope_flow(in);}catch(const std::invalid_argument&){rejected=true;}
    if(!rejected)throw std::runtime_error("missing QH accepted");
    std::cout<<"PASS P6D_SLOPE_SOFTWARE_ONLY\n";
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
