#pragma once
// P6G G8 SELECTED-KERNEL exercise, not the complete CALMET DIAGNO.
// Source-backed SIMILT (surface neutral profile), VERTAV/PREPDI (upper
// profiles) and INTER2 (all stations) operate for a single UTC hour.
// WIND1 first-guess uses a controlled zero field here: no original
// multi-station WIND1, terrain FRADJ/SLOPE, P6E or P6F acceptance claim.
#include "cteas/p6/observation_input_contract.hpp"
#include "cteas/p6/inter2_selected.hpp"
#include "cteas/p6/similt_profile.hpp"
#include "cteas/p6/upper_hourly_wind.hpp"
#include "cteas/p6/vertical_wind_average.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace cteas::calmet::p6g {
namespace p6 = cteas::calmet::p6;

struct PreparedUpper {
    std::vector<p6::Upper2022> profiles;
    std::vector<std::vector<std::vector<p6::LayerWind>>> layers;
};
inline PreparedUpper prepare_upper_original_selected(
    const std::vector<UpperSeries>& upper,
    const std::vector<float>& zface) {
    if(upper.empty()||zface.size()!=10)
        throw std::invalid_argument("G8 selected bridge supports 9 source-proven layers");
    PreparedUpper result;
    for(const auto& station:upper) {
        p6::Upper2022 u;
        u.station_id=station.station_id;
        u.pstop_hpa=station.pstop_hpa;
        u.soundings=station.soundings;
        std::vector<std::vector<p6::LayerWind>> stations_layers;
        stations_layers.reserve(u.soundings.size());
        for(const auto& sounding:u.soundings)
            stations_layers.push_back(p6::calmet_vertav(sounding,zface));
        result.profiles.push_back(std::move(u));
        result.layers.push_back(std::move(stations_layers));
    }
    return result;
}

struct SelectedBridgeGrid {
    int nx=0,ny=0;
    float spacing_km=0.0f;
    std::array<float,10> zface_m{};
    std::vector<int> landuse; // Read from P6G READGE GEO.DAT, not hardcoded.
};
struct SelectedBridgeHour {
    std::chrono::sys_seconds end{};
    float center_u_mps{},center_v_mps{};
    float upper_u_mps{},upper_v_mps{};
    std::size_t observed_contributions{};
};
inline SelectedBridgeHour selected_multisite_hour(
    const StationSet& deck,const SurfaceHour& surface,
    const InputHour& input,const PreparedUpper& prepared,
    const SelectedBridgeGrid& grid) {
    if(grid.nx<2||grid.ny<2||grid.nx>400||grid.ny>400||
       grid.spacing_km<=0.f||!std::isfinite(grid.spacing_km)||
       surface.end!=input.end||surface.by_station.size()!=deck.surface.size()||
       prepared.profiles.size()!=deck.upper.size()||
       prepared.layers.size()!=deck.upper.size()||
       input.upper.size()!=deck.upper.size()||
       grid.landuse.size()!=static_cast<std::size_t>(grid.nx)*
                            static_cast<std::size_t>(grid.ny))
        throw std::invalid_argument("G8 bridge source-hour/grid/array mismatch");
    for(std::size_t i=1;i<grid.zface_m.size();++i)
        if(!(grid.zface_m[i]>grid.zface_m[i-1]))
            throw std::invalid_argument("G8 invalid ZFACE");
    const std::size_t stations=deck.surface.size()+deck.upper.size();
    if(stations<2||stations>479)
        throw std::invalid_argument("G8 original INTER2 station count unsupported");
    p6::Inter2SelectedInput inter;
    inter.nx=grid.nx;inter.ny=grid.ny;inter.nz=9;
    inter.nstations=static_cast<int>(stations);
    inter.dx_m=grid.spacing_km*1000.f;
    inter.dy_m=inter.dx_m;
    inter.rmin_km=0.5f;
    // Synthetic driver settings. Do NOT reuse for production CALMET physics.
    inter.rmax1_km=150.f;inter.rmax2_km=150.f;inter.rmax3_km=150.f;
    inter.r1_km=50.f;inter.r2_km=80.f;
    inter.nintrp_per_layer.assign(9,inter.nstations);
    const std::size_t cells=static_cast<std::size_t>(grid.nx)*
                            static_cast<std::size_t>(grid.ny);
    inter.landuse=grid.landuse;
    inter.station_x_km.reserve(stations);
    inter.station_y_km.reserve(stations);
    for(const auto& s:deck.surface){
        inter.station_x_km.push_back(s.local_x_m*0.001f);
        inter.station_y_km.push_back(s.local_y_m*0.001f);
    }
    for(const auto& s:deck.upper){
        inter.station_x_km.push_back(s.local_x_m*0.001f);
        inter.station_y_km.push_back(s.local_y_m*0.001f);
    }
    const std::size_t levels=9;
    std::vector<float> su(levels*stations,0.f),sv(levels*stations,0.f);
    for(std::size_t s=0;s<deck.surface.size();++s){
        const auto& o=surface.by_station[s];
        if(!o.wind_valid)
            throw std::invalid_argument("G8 selected test needs real SURF wind, not invented");
        const float rad=0.0174533f*o.wind_direction_deg;
        const float u=-o.wind_speed_mps*std::sin(rad);
        const float v=-o.wind_speed_mps*std::cos(rad);
        p6::SimiltProfileInput profile;
        profile.station_lat_deg=31.94f; // synthetic UTM50N test case only
        profile.anemometer_height_m=deck.surface[s].anemometer_height_m;
        profile.roughness_m=0.1f;
        profile.anemometer_u_mps=u;profile.anemometer_v_mps=v;
        profile.obukhov_length_m=10000.f;
        profile.mixing_height_m=10000.f;
        profile.minimum_mixing_height_m=0.f;
        profile.zface_m=grid.zface_m;
        const auto winds=p6::original_similt_profile(profile);
        if(winds.levels_filled!=levels)
            throw std::invalid_argument("G8 surface source profile not complete");
        for(std::size_t k=0;k<levels;++k){
            su[k*stations+s]=winds.u_mps[k];
            sv[k*stations+s]=winds.v_mps[k];
        }
    }
    for(std::size_t s=0;s<deck.upper.size();++s){
        const auto& station=prepared.profiles[s];
        const auto hr=p6::calmet_prepdi_hour(
            station,input.end,prepared.layers[s]);
        const auto& bracket=input.upper[s];
        if(hr.prior_sounding!=bracket.earlier||
           hr.following_sounding!=bracket.later||
           hr.past_seconds!=bracket.past_seconds||
           hr.to_go_seconds!=bracket.future_seconds)
            throw std::invalid_argument("G8 COMP/PREPDI station bracket disagreement");
        const std::size_t index=deck.surface.size()+s;
        for(std::size_t k=0;k<levels;++k){
            su[k*stations+index]=hr.winds.at(k).u_mps;
            sv[k*stations+index]=hr.winds.at(k).v_mps;
        }
    }
    // Only exercise the SOURCE-SELECTED INTER2 kernel. WIND1 first-guess,
    // observation weights and land-use here are synthetic controls.
    inter.step1_mps.assign(cells*levels,0.f);
    inter.observation_weight.assign(cells*levels,1.f);
    inter.station_component_mps=std::move(su);
    const auto u=p6::original_inter2_selected(inter);
    inter.station_component_mps=std::move(sv);
    const auto v=p6::original_inter2_selected(inter);
    const auto center=static_cast<std::size_t>(grid.ny/2)*
                      static_cast<std::size_t>(grid.nx)+
                      static_cast<std::size_t>(grid.nx/2);
    SelectedBridgeHour out;
    out.end=input.end;
    out.center_u_mps=u.component_mps[center];
    out.center_v_mps=v.component_mps[center];
    out.upper_u_mps=u.component_mps[8*cells+center];
    out.upper_v_mps=v.component_mps[8*cells+center];
    for(const auto n:u.contributing_station_count)
        out.observed_contributions+=static_cast<std::size_t>(n);
    if(out.observed_contributions==0||
       !std::isfinite(out.center_u_mps)||!std::isfinite(out.center_v_mps)||
       !std::isfinite(out.upper_u_mps)||!std::isfinite(out.upper_v_mps))
        throw std::invalid_argument("G8 source INTER2 nonfinite/empty selected field");
    return out;
}
} // namespace cteas::calmet::p6g
