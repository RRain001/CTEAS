#pragma once
// Stable P6E surface solar / radiation interface for the real 2022 case.
//
// Answers the three inputs HEATFX needs (calmet.for L7999-8122):
//   1. per-cell mid-timestep sine of solar elevation (SINALPC),
//   2. the MCLOUD=1 station-cloud fraction contract,
//   3. the short-wave formula itself.
//
// Units and dimensions
//   SINALPC      : dimensionless sine of the solar elevation, signed (negative
//                  before sunrise / after sunset), one REAL*4 per cell.
//                  Layout: (j*nx+i), i fastest.
//   cloud cover  : integer tenths 0..10 from the surface observation (RDSN
//                  ICC); fraction = 0.1*ICC, dimensionless.
//   QSW          : W/m**2, clipped at 0.
// Time semantics
//   The SOLAR table has 26 half-hour slots covering 23:30 of the previous day
//   to 00:30 of the next day. For an hourly timestep the original COMP code
//   selects `SINALPC(i,j) = SINALP(i,j,NHRB+2)` (L3862-3872), i.e. the slot at
//   the MIDDLE of the hour that begins at NHRB. `julian_day` and `nhrb` are the
//   beginning time of the interval, matching the CALMET.DAT field clock.
//   `base_timezone_hours` is the AXTZ offset (UTC+0000 -> 0).
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include "cteas/p6e/solar.hpp"
namespace cteas::p6e {
struct SurfaceSolarGrid {
    std::size_t nx=0,ny=0;
    std::vector<float> latitude_deg,longitude_east_deg;
};
// Inverse transverse Mercator for the case projection (UTM north zones,
// WGS-84). Series inverse, accurate to ~1e-9 deg; the P6E QSW real-case
// comparison is insensitive to the remaining difference against COORDLIB.
inline void utm_north_to_latlon(int zone,double easting_m,double northing_m,
                                double& latitude_deg,double& longitude_east_deg) {
    if(zone<1||zone>60)throw std::invalid_argument("UTM zone outside 1..60");
    constexpr double a=6378137.0,flattening=1.0/298.257223563;
    constexpr double k0=0.9996,false_easting=500000.0;
    const double e2=flattening*(2.0-flattening);
    const double ep2=e2/(1.0-e2);
    const double e1=(1.0-std::sqrt(1.0-e2))/(1.0+std::sqrt(1.0-e2));
    const double x=(easting_m-false_easting)/k0;
    const double y=northing_m/k0;
    const double mu=y/(a*(1.0-e2/4.0-3.0*e2*e2/64.0-5.0*e2*e2*e2/256.0));
    const double phi1=mu+(3.0*e1/2.0-27.0*e1*e1*e1/32.0)*std::sin(2.0*mu)+
        (21.0*e1*e1/16.0-55.0*e1*e1*e1*e1/32.0)*std::sin(4.0*mu)+
        (151.0*e1*e1*e1/96.0)*std::sin(6.0*mu)+
        (1097.0*e1*e1*e1*e1/512.0)*std::sin(8.0*mu);
    const double sinphi=std::sin(phi1),cosphi=std::cos(phi1);
    const double tanphi=std::tan(phi1);
    const double n1=a/std::sqrt(1.0-e2*sinphi*sinphi);
    const double r1=a*(1.0-e2)/std::pow(1.0-e2*sinphi*sinphi,1.5);
    const double t1=tanphi*tanphi,c1=ep2*cosphi*cosphi;
    const double d=x/n1;
    const double lat=phi1-(n1*tanphi/r1)*(d*d/2.0-
        (5.0+3.0*t1+10.0*c1-4.0*c1*c1-9.0*ep2)*d*d*d*d/24.0+
        (61.0+90.0*t1+298.0*c1+45.0*t1*t1-252.0*ep2-3.0*c1*c1)*
        d*d*d*d*d*d/720.0);
    const double lon=(d-(1.0+2.0*t1+c1)*d*d*d/6.0+
        (5.0-2.0*c1+28.0*t1-3.0*c1*c1+8.0*ep2+24.0*t1*t1)*
        d*d*d*d*d/120.0)/cosphi;
    constexpr double rad_to_deg=57.2957795130823208768;
    latitude_deg=lat*rad_to_deg;
    longitude_east_deg=(zone-1)*6.0-180.0+3.0+lon*rad_to_deg;
}
// Per-cell latitude / east longitude of the grid cell centres. (XORIG,YORIG)
// is the SW corner of cell (1,1) in the same metres as DGRID.
inline SurfaceSolarGrid surface_solar_grid(std::size_t nx,std::size_t ny,
                                           int utm_zone,float xorigin_m,
                                           float yorigin_m,float dgrid_m) {
    if(nx==0||ny==0||nx>static_cast<std::size_t>(-1)/ny||
       !std::isfinite(dgrid_m)||dgrid_m<=0.f||
       !std::isfinite(xorigin_m)||!std::isfinite(yorigin_m))
        throw std::invalid_argument("surface solar grid extent invalid");
    SurfaceSolarGrid g;
    g.nx=nx;g.ny=ny;
    g.latitude_deg.resize(nx*ny);
    g.longitude_east_deg.resize(nx*ny);
    for(std::size_t i=0;i<nx;++i)
        for(std::size_t j=0;j<ny;++j) {
            const double easting=static_cast<double>(xorigin_m)+
                (static_cast<double>(i)+0.5)*static_cast<double>(dgrid_m);
            const double northing=static_cast<double>(yorigin_m)+
                (static_cast<double>(j)+0.5)*static_cast<double>(dgrid_m);
            double lat=0.0,lon=0.0;
            utm_north_to_latlon(utm_zone,easting,northing,lat,lon);
            g.latitude_deg[j*nx+i]=static_cast<float>(lat);
            g.longitude_east_deg[j*nx+i]=static_cast<float>(lon);
        }
    return g;
}
// AXTZ (character*8 in CALMET.DAT, e.g. "UTC+0000") -> whole-hour offset.
// Sub-hour offsets round to the nearest hour, matching the REAL*4 arithmetic
// of the original IBTZ usage in SOLAR.
inline int timezone_hours_from_axtz(const std::string& axtz) {
    if(axtz.size()!=8U||axtz.compare(0U,3U,"UTC")!=0||
       (axtz[3]!='+'&&axtz[3]!='-'))
        throw std::invalid_argument("unsupported AXTZ "+axtz);
    const int sign=axtz[3]=='-'?-1:1;
    const int hours=std::stoi(axtz.substr(4U,2U));
    const int minutes=std::stoi(axtz.substr(6U,2U));
    return sign*(hours+(minutes>=30?1:0));
}
// Mid-timestep sine of solar elevation for one hourly interval, per cell.
inline std::vector<float> sinalpc_grid(const SurfaceSolarGrid& g,int julian_day,
                                       int begin_hour,int base_timezone_hours) {
    if(begin_hour<0||begin_hour>23)
        throw std::invalid_argument("begin hour outside 0..23");
    Grid solar;
    solar.nx=g.nx;solar.ny=g.ny;
    solar.lat=g.latitude_deg;
    solar.lon_east=g.longitude_east_deg;
    const auto table=calculate(solar,julian_day,base_timezone_hours);
    const auto slot=static_cast<std::size_t>(begin_hour)+1U; // SINALP(NHRB+2)
    std::vector<float> out(g.nx*g.ny,0.f);
    for(std::size_t j=0;j<g.ny;++j)
        for(std::size_t i=0;i<g.nx;++i)
            out[j*g.nx+i]=table.at(slot,i,j);
    return out;
}
// MCLOUD=1 station-cloud branch of HEATFX L8061-8064:
//   ccfrac = 0.1*ICC(NEARS(i,j))
// `nearest_station_index` is 1-based as in the original NEARS array.
// Missing (999) or out-of-range cloud cover fails closed.
inline float heatfx_cloud_fraction(int opaque_cloud_tenths) {
    if(opaque_cloud_tenths<0||opaque_cloud_tenths>10)
        throw std::invalid_argument(
            "HEATFX MCLOUD=1 cloud cover must be 0..10 tenths");
    return 0.1f*static_cast<float>(opaque_cloud_tenths);
}
// HEATFX L8066-8069. Constants are HA1 HA2 HB1 HB2 of /HFLUX/; the real case
// uses the source defaults HA1=990, HA2=-30, HB1=-0.75, HB2=3.4 (met.lst).
inline float heatfx_shortwave(float sinalpc,float cloud_fraction,
                              float ha1=990.f,float ha2=-30.f,
                              float hb1=-0.75f,float hb2=3.4f) {
    if(!std::isfinite(sinalpc)||!std::isfinite(cloud_fraction)||
       cloud_fraction<0.f||cloud_fraction>1.f)
        throw std::invalid_argument("HEATFX short-wave inputs invalid");
    float value=(ha1*sinalpc+ha2)*(1.f+hb1*std::pow(cloud_fraction,hb2));
    if(value<0.f)value=0.f;
    return value;
}
} // namespace cteas::p6e
