#pragma once
// CALMET 6.5.0 SURFVAR (L12544-13064) highly scoped selected dry path.
// NOOBS=0, ITPROG=0, IRHPROG=0, NPSTA=0, NSSTA=1, NOWSTA=0,
// IALL=1, IAVET=0, land-only temperature interpolation.
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>
namespace cteas::p6e {
struct ObservedSurface {
    std::size_t nx=0,ny=0;
    float station_temp_k=0.0f;
    int station_rh_percent=9999;
    int npsta=0,noobs=0,itprog=0,irhprog=0,nowsta=0;
    bool land_only=true,average_temperature=false;
};
struct SurfaceState {
    std::size_t nx,ny;
    std::vector<float> temp_k;
    std::vector<int> rh_percent,ipcode;
};
inline SurfaceState surfvar_one_station(const ObservedSurface& s) {
    if(s.nx==0||s.ny==0||s.nx>static_cast<std::size_t>(-1)/s.ny||
       s.npsta!=0||s.noobs!=0||s.itprog!=0||s.irhprog!=0||
       s.nowsta!=0||!s.land_only||s.average_temperature)
        throw std::invalid_argument("SURFVAR unsupported selected-case switch");
    if(!std::isfinite(s.station_temp_k)||
       s.station_temp_k<180.0f||s.station_temp_k>350.0f||
       (s.station_rh_percent!=9999 &&
        (s.station_rh_percent<0||s.station_rh_percent>100)))
        throw std::invalid_argument("SURFVAR invalid observed value");
    const auto n=s.nx*s.ny;
    // With exactly one eligible station and no water interpolation,
    // all cells select that station, regardless of radius or IRAD.
    // Missing observed RH invokes original 80% land fallback.
    const int rh=s.station_rh_percent==9999?80:s.station_rh_percent;
    // Original default initialized IPCODE2D=0; NPSTA=0 skips rain input.
    // This is NOT qualified meteorological precipitation forcing.
    return {s.nx,s.ny,std::vector<float>(n,s.station_temp_k),
            std::vector<int>(n,rh),std::vector<int>(n,0)};
}
} // namespace cteas::p6e
