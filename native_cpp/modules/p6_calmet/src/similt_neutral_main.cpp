#include "cteas/p6/similt_neutral.hpp"
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){
 try{
  if(argc!=3)throw std::invalid_argument("usage: similt INPUT OUTPUT");
  std::ifstream f(argv[1]);if(!f)throw std::runtime_error("SIMILT input missing");
  cteas::calmet::p6::SimiltNeutralInput in;
  if(!(f>>in.station_lat_deg>>in.anemometer_height_m>>in.roughness_m>>
       in.anemometer_u_mps>>in.anemometer_v_mps>>in.iextrp>>in.ipsifcn))
    throw std::runtime_error("SIMILT input incomplete");
  const auto o=cteas::calmet::p6::original_similt_wind1_neutral(in);
  std::ofstream out(argv[2]);if(!out)throw std::runtime_error("SIMILT output missing");
  out<<std::setprecision(12);
  for(std::size_t k=0;k<9;++k)
    out<<k+1<<' '<<o.u_mps[k]<<' '<<o.v_mps[k]<<'\n';
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
