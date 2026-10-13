#pragma once
// Selected original CALPUFF TURBSET L52039-52190.
// For MDISP=MDISP2=3: TSIGV/TSIGW initialize to -999, neither the
// MDISP=2 SIGWV path nor the MDISP=1/5 observation-profile path executes,
// and the arrays SVMIN(JDSTAB,ILW)/SWMIN(JDSTAB,ILW) supply both.
// Land/water ILW is explicitly required, never guessed from UTM position.
// Source-pinned by original SIGTY/SIGTZ calm inspection workflow.
#include "cteas/puff/control.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
namespace cteas::puff {
struct OriginalTurbsetMdisp3 {
 std::array<float,12> svmin,swmin;
 float tyidr=0.001F,tzidr=0.002F,tzisdr=0.01F; // Original BLOCK DATA L2140
 float ayt=0.5F; // Original BLOCK DATA L2144, not a fitted value
};
struct OriginalTurbsetMinimum {
 float sigv_ms=0.0F,sigw_ms=0.0F;
 int stability=0,land_water=0;
};
[[nodiscard]] inline OriginalTurbsetMdisp3 original_turbset_mdisp3_from_control(
    const ControlFile& control) {
 const auto ok=[](bool q,const char* s){if(!q)throw std::invalid_argument(s);};
 ok(control.required_int("MDISP")==3 && control.required_int("MDISP2")==3 &&
    control.required_int("MTAULY")==0,
    "TURBSET minima route needs MDISP=MDISP2=3, MTAULY=0");
 OriginalTurbsetMdisp3 cfg;
 const auto v=control.required_float_list("SVMIN");
 const auto w=control.required_float_list("SWMIN");
 ok(v.size()==12&&w.size()==12,"TURBSET requires both 12-value minimum arrays");
 for(std::size_t i=0;i<12;++i){
   ok(std::isfinite(v[i])&&std::isfinite(w[i])&&v[i]>0&&w[i]>0,
      "nonphysical SVMIN/SWMIN in original deck");
   cfg.svmin[i]=static_cast<float>(v[i]);
   cfg.swmin[i]=static_cast<float>(w[i]);
 }
 return cfg;
}
[[nodiscard]] inline OriginalTurbsetMinimum original_turbset_mdisp3_minimum(
    const OriginalTurbsetMdisp3& p,int stability,int ilw){
 if(stability<1||stability>6||(ilw!=1&&ilw!=2))
  throw std::invalid_argument("original TURBSET requires 1<=JDSTAB<=6 and ILW=1/2");
 const auto i=static_cast<std::size_t>(stability-1+6*(ilw-1));
 return {p.svmin[i],p.swmin[i],stability,ilw};
}
} // namespace cteas::puff
