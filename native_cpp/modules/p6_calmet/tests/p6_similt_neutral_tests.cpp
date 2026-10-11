#include "cteas/p6/similt_neutral.hpp"
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
int main(){
 try{
  using namespace cteas::calmet::p6;
  SimiltNeutralInput in{31.939774f,10.f,0.1f,3.f,4.f,-4,0};
  const auto a=original_similt_wind1_neutral(in);
  if(!(std::isfinite(a.u_mps[0])&&std::isfinite(a.v_mps[8])))
    throw std::runtime_error("SIMILT finite expected");
  const float scale=std::hypot(a.u_mps[0],a.v_mps[0]);
  if(!(scale>4.f&&scale<6.f))throw std::runtime_error("SIMILT first level speed");
  if(!(std::hypot(a.u_mps[8],a.v_mps[8])>scale))
    throw std::runtime_error("SIMILT neutral vertical shear missing");
  bool rejected=false;
  in.station_lat_deg=-30.f;
  try{(void)original_similt_wind1_neutral(in);}catch(const std::invalid_argument&){rejected=true;}
  if(!rejected)throw std::runtime_error("SIMILT southern station unsupported");
  rejected=false;in.station_lat_deg=31.f;in.anemometer_u_mps=0.f;in.anemometer_v_mps=0.f;
  try{(void)original_similt_wind1_neutral(in);}catch(const std::invalid_argument&){rejected=true;}
  if(!rejected)throw std::runtime_error("SIMILT unexpected calm extrapolation");
  rejected=false;in.anemometer_u_mps=3.f;in.roughness_m=10.f;
  try{(void)original_similt_wind1_neutral(in);}catch(const std::invalid_argument&){rejected=true;}
  if(!rejected)throw std::runtime_error("SIMILT singular logarithm accepted");
  std::cout<<"P6D SIMILT NEUTRAL SOFTWARE PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
