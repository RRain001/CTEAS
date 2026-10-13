#include "cteas/puff/original_turbset_mdisp3.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){
 try{
  if(argc!=2)throw std::invalid_argument("usage TURBSET_TEST puff.inp");
  const auto cfg=cteas::puff::original_turbset_mdisp3_from_control(
      cteas::puff::ControlFile::read_file(argv[1]));
  const std::array<float,6> sw={0.20F,0.12F,0.08F,0.06F,0.03F,0.016F};
  std::size_t tested=0;
  for(int ilw=1;ilw<=2;++ilw){
   for(int cls=1;cls<=6;++cls){
    const auto result=cteas::puff::original_turbset_mdisp3_minimum(cfg,cls,ilw);
    if(result.sigv_ms!=(ilw==1?0.50F:0.37F) ||
       result.sigw_ms!=sw[static_cast<std::size_t>(cls-1)] ||
       result.stability!=cls || result.land_water!=ilw)
      throw std::runtime_error("2022 original SVMIN/SWMIN array mismatch");
    ++tested;
   }
  }
  if(cfg.tyidr!=0.001F||cfg.tzidr!=0.002F||
     cfg.tzisdr!=0.01F||cfg.ayt!=0.5F)
    throw std::runtime_error("original BLOCK DATA 2140/2144 changed");
  bool rejected=false;
  try{(void)cteas::puff::original_turbset_mdisp3_minimum(cfg,4,0);}
  catch(const std::invalid_argument&){rejected=true;}
  if(!rejected)throw std::runtime_error("unknown land/water silently defaulted");
  std::cout<<"P7_ORIGINAL_TURBSET_MDISP3_HEFEI_MINIMA=PASS"
           <<" CELLS=EXPLICIT_ILW"
           <<" STABILITY_LANDWATER_CASES="<<tested
           <<" TYIDR=0.001 TZIDR=0.002 TZISDR=0.01 AYT=0.5"
           <<" ACTUAL_COMP_989_CALM_SUBSTEPS=NOT_YET_ACCEPTED\n";
  return 0;
 }catch(const std::exception& e){
  std::cerr<<"P7_ORIGINAL_TURBSET_MDISP3_HEFEI_MINIMA=FAIL "
           <<e.what()<<'\n';return 1;
 }
}
