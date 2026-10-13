#include "cteas/puff/original_point_prfin.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cteas::puff {
OriginalPointPrfinResult original_point_prfin_metfm1(
    const OriginalMetHour& met,const Calmet21Static& statics,
    const Calmet21Header& grid,const OriginalPointPrfinRequest& in) {
    const auto finite=[](float v){return std::isfinite(v);};
    if(!finite(in.x_cells)||!finite(in.y_cells)||
       !finite(in.stack_height_m)||in.stack_height_m<=0.0F||
       !finite(in.diameter_m)||in.diameter_m<=0.0F||
       !finite(in.exit_velocity_ms)||in.exit_velocity_ms<0.0F||
       !finite(in.stack_temperature_k)||in.stack_temperature_k<=0.0F||
       !finite(in.momentum_flux_factor)||
       !(in.momentum_flux_factor==0.0F||in.momentum_flux_factor==1.0F)||
       !finite(in.calm_threshold_ms)||in.calm_threshold_ms<0.0F||
       !finite(in.max_sample_distance_cells)||
       in.max_sample_distance_cells<=0.0F||
       !finite(in.hour_seconds)||in.hour_seconds<=0.0F||
       in.max_new_puffs<=0||
       !finite(grid.dx_m)||grid.dx_m<=0.0F||
       grid.nx!=met.nx||grid.ny!=met.ny||grid.nz!=met.nz)
        throw std::invalid_argument("POINTS1 PRFIN requires valid original MET and source controls");
    const std::size_t n=static_cast<std::size_t>(met.nx)*
                        static_cast<std::size_t>(met.ny);
    if(met.tempk.size()!=n||met.zi.size()!=n||met.ipgt.size()!=n||
       statics.roughness_m.size()!=n)
        throw std::invalid_argument("POINTS1 PRFIN missing original TEMP2D/ZI/PGT/Z0");
    const auto cell=original_met_cell(
        in.x_cells,in.y_cells,met.nx,met.ny).offset_0based;
    const int pg=met.ipgt.at(cell);
    // Original RLSMET can source SQRTS from PTLAPS for stable PG classes.
    // The selected D-class first hour is independent of that input in PRFIN.
    if(pg<1||pg>4)
        throw std::invalid_argument("POINTS1 stable PRFIN needs native SQRTS/PTLAPS");
    const float temp=met.tempk.at(cell);
    const float pbl=met.zi.at(cell);
    if(!finite(temp)||temp<=0.0F||!finite(pbl))
        throw std::invalid_argument("POINTS1 invalid genuine TEMP2D or DPBL");
    const auto stack=original_release_wind_metfm1(
        met,statics,{in.x_cells,in.y_cells,in.stack_height_m,in.calm_threshold_ms});
    // POINTS1 L15509-15539: source-ordered float32 stack temperature excess,
    // buoyancy, momentum and stack-tip downwash. TEMP2D, not T3D-stack.
    const float excess=std::max(0.0F,in.stack_temperature_k-temp);
    const auto flux=stack_fluxes({
        in.diameter_m,in.exit_velocity_ms,in.stack_temperature_k,
        temp,excess,in.momentum_flux_factor});
    const float tip=static_cast<float>(stack_tip_downwash({
        in.stack_height_m,in.diameter_m,flux.momentum_m4s2,
        in.exit_velocity_ms,stack.wind_speed_ms}));
    // PRFIN classes A-D do not use SQRTS; do not fabricate a stable
    // lapse parameter to support other classes.
    const auto final=final_plume_rise({
        in.exit_velocity_ms,in.diameter_m,flux.momentum_m4s2,
        flux.buoyancy_m4s3,stack.wind_speed_ms,pg,0.0});
    // Original POINTS1 L15562-15588 invokes PRFPP when positive buoyancy
    // and A-D. Its no-penetration branch returns unchanged rise/Fmix=1
    // without reading PTGINV/DPTINV. Fail closed otherwise.
    if(!(flux.buoyancy_m4s3>0.0) || !(pbl>in.stack_height_m) ||
       !(final.z_final_m<=0.67F*(pbl-in.stack_height_m)))
        throw std::invalid_argument(
            "POINTS1 PRFPP depends on unported PTLAPS/penetration branch");
    const auto partial=partial_penetration_rise({
        final.z_final_m,flux.buoyancy_m4s3,stack.wind_speed_ms,
        in.stack_height_m,pbl,temp,0.0,0.0});
    if(partial.final_rise_m!=final.z_final_m ||
       partial.mass_fraction_mixed!=1.0||partial.max_height_m!=pbl)
        throw std::logic_error("POINTS1 no-penetration PRFPP source-order drift");
    const float heff=static_cast<float>(
        in.stack_height_m+partial.final_rise_m-tip);
    if(!finite(heff)||heff<=in.stack_height_m)
        throw std::invalid_argument("POINTS1 PRFIN effective stack height invalid");
    const auto elevated=original_release_wind_metfm1(
        met,statics,{in.x_cells,in.y_cells,heff,in.calm_threshold_ms});
    const float wsnew=std::max(
        stack.wind_speed_ms,elevated.wind_speed_ms);
    const float xtmp1=in.hour_seconds/
        (grid.dx_m*in.max_sample_distance_cells);
    const auto cadence=release_cadence({
        wsnew,xtmp1,in.max_new_puffs,in.hour_seconds});
    OriginalPointPrfinResult result;
    result.ambient_temperature_k=temp;
    result.mixing_height_m=pbl;
    result.stability_class=pg;
    result.stack_wind_ms=stack.wind_speed_ms;
    result.effective_wind_ms=elevated.wind_speed_ms;
    result.release_wind_ms=wsnew;
    result.buoyancy_flux=static_cast<float>(flux.buoyancy_m4s3);
    result.momentum_flux=static_cast<float>(flux.momentum_m4s2);
    result.stack_tip_downwash_m=tip;
    result.x_final_m=static_cast<float>(final.x_final_m);
    result.final_rise_m=static_cast<float>(partial.final_rise_m);
    result.effective_height_m=heff;
    result.mixed_fraction=static_cast<float>(partial.mass_fraction_mixed);
    result.max_layer_height_m=static_cast<float>(partial.max_height_m);
    result.brigg=final;
    result.release=cadence;
    return result;
}
} // namespace cteas::puff
