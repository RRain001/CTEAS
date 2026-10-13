#include "cteas/puff/original_calm_sigma.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main(){
 try {
  cteas::puff::OriginalCalmSigmaInput p;
  p.sy0=1;p.sz0=1;p.dt_s=360;p.dx_km=.02F;
  p.uavg=.2F;p.sigv=.45F;p.sigw=.25F;p.el=-100;
  p.tyidr=.001F;p.tzidr=.002F;p.syh=550;p.ayt=.1F;p.thfty=5000;
  const auto z=cteas::puff::original_calm_sigma(p);
  if(!(z.sy>1&&z.sz>1&&z.virty>360&&z.virtz>360))
   throw std::runtime_error("calm sigma did not grow");
  p.dt_s=0;
  const auto same=cteas::puff::original_calm_sigma(p);
  if(same.sy!=p.sy0||same.sz!=p.sz0)
   throw std::runtime_error("zero travel did not retain sigma");
  p.el=120;
  bool failed=false;
  try{(void)cteas::puff::original_calm_sigma(p);}
  catch(const std::invalid_argument&){failed=true;}
  if(!failed)throw std::runtime_error("stable branch silently approximated");
  std::cout<<"P7_CALM_IDOPT1_NATIVE_SUBSET=PASS ELS_LE_ZERO=YES"
           <<" STABLE=FAIL_CLOSED FULL_COMP=NOT_ACCEPTED\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"P7_CALM_IDOPT1_NATIVE_SUBSET=FAIL "<<e.what()<<'\n';return 1;
 }
}
