#include "cteas/puff/original_puffct2.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cteas::puff {
OriginalPuffct2Result original_puffct2(
    const OriginalPuffct2Step& s, float frac) {
    for(float x:{frac,s.x_begin_m,s.y_begin_m,s.z_begin_m,
                 s.x_end_m,s.y_end_m,s.z_end_m,s.terrain_fraction}) {
        if(!std::isfinite(x))
            throw std::invalid_argument(
                "original PUFFCT2 requires finite REAL4 /CURRENT/ and FRAC");
    }
    // Original CALPUFF v7.2.1 PUFFCT2 L21070-21093, preserving the
    // REAL*4 evaluation order and the *asymmetric* upslope/downslope
    // FRACZ1 branches. No interpolation shortcuts or FRAC clipping.
    const float x=s.x_begin_m+frac*(s.x_end_m-s.x_begin_m);
    const float y=s.y_begin_m+frac*(s.y_end_m-s.y_begin_m);
    float z=0.0F;
    const float one=1.0F, zero=0.0F;
    if(s.upslope) {
        if(s.terrain_fraction<=zero) {
            z=s.z_begin_m;
        } else {
            float jump=s.z_end_m-s.z_begin_m;
            if(s.terrain_fraction<one)
                jump=jump/s.terrain_fraction;
            z=s.z_begin_m+std::min(frac,s.terrain_fraction)*jump;
        }
    } else {
        if(s.terrain_fraction>=one) {
            z=s.z_begin_m;
        } else {
            float jump=s.z_end_m-s.z_begin_m;
            if(s.terrain_fraction>zero)
                jump=jump/(one-s.terrain_fraction);
            z=s.z_begin_m+std::max(zero,(frac-s.terrain_fraction))*jump;
        }
    }
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z))
        throw std::overflow_error("original PUFFCT2 REAL4 output overflow");
    return {x,y,z};
}
} // namespace cteas::puff
