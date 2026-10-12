#include "cteas/p6/inter2_selected.hpp"
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
int main(){
 try{
  using namespace cteas::calmet::p6;
  Inter2SelectedInput in;
  in.nx=3;in.ny=3;in.nz=2;in.nstations=2;
  in.dx_m=100;in.dy_m=100;in.rmin_km=0.05f;
  in.rmax1_km=1;in.rmax2_km=1;in.rmax3_km=1;
  in.r1_km=0.6f;in.r2_km=1.2f;
  in.nintrp_per_layer={2,2};in.landuse.assign(9,10);
  in.station_x_km={0.05f,0.25f};in.station_y_km={0.05f,0.25f};
  in.station_component_mps={3,7,5,9};
  in.step1_mps.assign(18,2.f);in.observation_weight.assign(18,1.f);
  const auto out=original_inter2_selected(in);
  if(out.component_mps.size()!=18||out.contributing_station_count.size()!=18)
    throw std::runtime_error("INTER2 3D shape failed");
  if(!(out.component_mps[0]>2.f&&out.component_mps[0]<7.f))
    throw std::runtime_error("INTER2 did not include observation");
  // Original Hefei deck NINTR2=99 with NWIND=2. It is a maximum, not
  // an error condition or a value to rewrite as two.
  in.nintrp_per_layer={99,99};
  const auto uncapped=original_inter2_selected(in);
  for(std::size_t k=0;k<out.component_mps.size();++k)
      if(std::abs(out.component_mps[k]-uncapped.component_mps[k])>1e-7f)
          throw std::runtime_error("INTER2 NINTRP original-maximum parity drift");
  // Original PREPDI/INTER2 missing upper-air wind at level 1 is 999;
  // the original missing threshold is EDITL=998, not a 999999 sentinel.
  in.edit=999.f;in.editl=998.f;
  in.station_component_mps[1]=999.f;
  const auto missing_first=original_inter2_selected(in);
  if(missing_first.contributing_station_count[0]!=1)
      throw std::runtime_error("INTER2 did not exclude original level-1 999 sentinel");
  in.station_component_mps[1]=7.f;
  in.rmax1_km=0.001f;in.rmax2_km=0.001f;in.rmax3_km=0.001f;
  in.lvary=false;
  auto none=original_inter2_selected(in);
  for(float x:none.component_mps)
    if(std::abs(x-2.f)>1e-6f)
      throw std::runtime_error("INTER2 no eligible station fails step1 fallback");
  in.lvary=true;
  auto nearest=original_inter2_selected(in);
  if(nearest.nearest_fallback_used[0]!=1 ||
     !(nearest.component_mps[0]>2.f))
    throw std::runtime_error("INTER2 LVARY fallback absent");
  in.nbar=1;
  bool rejected=false;
  try{(void)original_inter2_selected(in);}catch(const std::invalid_argument&){rejected=true;}
  if(!rejected)throw std::runtime_error("unsupported barrier silently accepted");
  in.nbar=0;in.station_component_mps[0]=std::numeric_limits<float>::quiet_NaN();
  rejected=false;
  try{(void)original_inter2_selected(in);}catch(const std::invalid_argument&){rejected=true;}
  if(!rejected)throw std::runtime_error("NaN observation silently accepted");
  std::cout<<"PASS P6D_INTER2_SELECTED_SOFTWARE_ONLY\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
