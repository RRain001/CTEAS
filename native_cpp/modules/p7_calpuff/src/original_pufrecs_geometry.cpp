#include "cteas/puff/original_pufrecs_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cteas::puff {
OriginalSlgfrac original_slgfrac(
    float x,float y,float x1,float y1,float x2,float y2) {
    for(float v:{x,y,x1,y1,x2,y2})
        if(!std::isfinite(v))
            throw std::invalid_argument("SLGFRAC coordinates must be finite");
    const float x12=x1-x2;
    const float y12=y1-y2;
    float length=std::sqrt(x12*x12+y12*y12);
    if(!std::isfinite(length))
        throw std::overflow_error("SLGFRAC projected length overflow");
    if(length>=1.0F) {
        const float inv=1.0F/length;
        const float cosom=y12*inv;
        const float sinom=x12*inv;
        const float xr=x-x2;
        const float yr=y-y2;
        const float along=yr*cosom+xr*sinom;
        const float across=-xr*cosom+yr*sinom;
        return {along,across,along*inv,length,inv};
    }
    // Original small/zero trajectory branch: DXY12=RDXY12=1.0;
    // RHOA=FRACS=0; RHOC=distance from younger end.
    const float xr=x-x2,yr=y-y2;
    return {0.0F,std::sqrt(xr*xr+yr*yr),0.0F,1.0F,1.0F};
}
OriginalPufrecsGeometry original_pufrecs_geometry(
    const OriginalPufrecsGeometryInputs& in) {
    for(float v:{in.x_receptor_m,in.y_receptor_m,
        in.x_begin_m,in.y_begin_m,in.x_end_m,in.y_end_m,
        in.sigma_y_begin_m,in.sigma_y_end_m,
        in.fclip,in.travel_distance_begin_m,in.mean_wind_ms})
        if(!std::isfinite(v))
            throw std::invalid_argument("PUFRECS geometry requires finite REAL4 inputs");
    if(in.mean_wind_ms<=0.0F || in.fclip<0.0F ||
       in.sigma_y_begin_m<0.0F || in.sigma_y_end_m<0.0F)
        throw std::invalid_argument("PUFRECS non-calm UAVG/FCLIP/sigma invalid");
    // Original PUFRECS L4950 passes XE1,YE1 as SLGFRAC old end x1/y1,
    // and XB1,YB1 as the new end x2/y2.
    const auto p=original_slgfrac(
        in.x_receptor_m,in.y_receptor_m,
        in.x_end_m,in.y_end_m,in.x_begin_m,in.y_begin_m);
    float frac=p.fraction,along=p.along_m;
    if(in.clip){
        frac=std::max(frac,-in.fclip*in.sigma_y_begin_m*p.inverse_length_per_m);
        frac=std::min(frac,1.0F+in.fclip*in.sigma_y_end_m*p.inverse_length_per_m);
        along=frac*p.projected_length_m;
    }
    float xtt1=in.travel_distance_begin_m+along;
    if(xtt1<0.0F){
        xtt1=0.0F;
        frac=-in.travel_distance_begin_m*p.inverse_length_per_m;
    }
    return {frac,along,xtt1,xtt1/in.mean_wind_ms,
            frac!=p.fraction};
}
} // namespace cteas::puff
