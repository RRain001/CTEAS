#include "cteas/puff/original_flat_calcpf.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cteas::puff {
RiseCompSamplingKernel make_original_flat_calcpf_sampler(
    const ReceptorSet& receptors,float met_spacing_m,
    float averaging_period_s,float impact_sigma_multiple) {
    if(!receptors.gridded || !receptors.discrete.empty() ||
       receptors.mesh_down!=1 || receptors.sampling_nx<=0 ||
       receptors.sampling_ny<=0 || receptors.ibsamp<1 ||
       receptors.jbsamp<1 || !std::isfinite(met_spacing_m) ||
       met_spacing_m<=0.0F || !std::isfinite(averaging_period_s) ||
       averaging_period_s<=0.0F || !std::isfinite(impact_sigma_multiple) ||
       impact_sigma_multiple<=0.0F ||
       !std::isfinite(receptors.spacing_km) ||
       std::abs(static_cast<float>(receptors.spacing_km)*1000.0F -
                met_spacing_m)>1.0e-3F ||
       receptors.grid.size()!=static_cast<std::size_t>(receptors.sampling_nx) *
                              static_cast<std::size_t>(receptors.sampling_ny)) {
        throw std::invalid_argument(
            "flat CALCPF supports only validated MESHDN=1 gridded ground receptors");
    }
    const auto nrecs=receptors.grid.size();
    const auto nx=static_cast<std::size_t>(receptors.sampling_nx);
    const float grid_x=static_cast<float>(receptors.ibsamp)-0.5F;
    const float grid_y=static_cast<float>(receptors.jbsamp)-0.5F;
    return [nrecs,nx,grid_x,grid_y,met_spacing_m,averaging_period_s,
            impact_sigma_multiple](const RiseCompSamplingState& s) {
        if(s.after_setpuf_before_lid.puff_code!=s.vmass.puff_code ||
           !s.after_setpuf_before_lid.vmass_pending ||
           s.vmass.mass_mixed.empty() ||
           s.vmass.mass_mixed.size()!=1 ||
           s.vmass.mass_above.size()!=1 ||
           !std::isfinite(s.step_seconds) || s.step_seconds<=0.0F ||
           s.step_seconds>averaging_period_s) {
            throw std::invalid_argument(
                "flat CALCPF expects one-species post-VMASS, pre-history state");
        }
        const float sy=static_cast<float>(s.sigmas.step.sigma_y_mid_m);
        const float sz=static_cast<float>(s.sigmas.step.sigma_z_mid_m);
        const float end_sy=static_cast<float>(s.sigmas.step.sigma_y_end_m);
        const float lid=s.vmass.reflecting_lid_m;
        if(!std::isfinite(sy)||sy<=0.0F || !std::isfinite(sz) ||
           sz<0.0F || !std::isfinite(end_sy)||end_sy<=0.0F ||
           !std::isfinite(lid)||lid<=0.0F) {
            throw std::invalid_argument("flat CALCPF invalid sigma or VMASS HLID");
        }
        // CALCPF L4191-4197 original sampling-grid coordinates, not UTM.
        const float xo=(s.before.x_cells-grid_x)+1.0F;
        const float yo=(s.before.y_cells-grid_y)+1.0F;
        const float xn=(s.after.x_cells-grid_x)+1.0F;
        const float yn=(s.after.y_cells-grid_y)+1.0F;
        const float midz=0.5F*(s.before.z_m+s.after.z_m);
        const float max_lid=std::max(lid,s.after_setpuf_before_lid.zimax_m);
        const float vfactor=static_cast<float>(vertical_coupling({
            s.vmass.puff_code,0.0,midz,sz,lid,max_lid}));
        if(!std::isfinite(vfactor) || vfactor<0.0F)
            throw std::invalid_argument("flat CALCPF invalid original VCOUP factor");
        std::vector<float> values(nrecs,0.0F);
        if(vfactor==0.0F || s.vmass.mass_mixed[0]<=0.0F)
            return values;
        const float radius=impact_sigma_multiple*end_sy/met_spacing_m;
        const float at_rec=sy/met_spacing_m;
        if(!std::isfinite(radius)||radius<=0.0F ||
           !std::isfinite(at_rec)||at_rec<=0.0F)
            throw std::invalid_argument("flat CALCPF sampling radius overflow");
        const float rec_radius=impact_sigma_multiple*at_rec;
        const float frac=s.step_seconds/averaging_period_s;
        for(std::size_t i=0;i<nrecs;++i) {
            const float xr=static_cast<float>(i%nx)+1.0F;
            const float yr=static_cast<float>(i/nx)+1.0F;
            const auto screen=original_puff_screen(xo,yo,xn,yn,xr,yr,radius);
            if(!screen.has_impact ||
               static_cast<float>(screen.closest_distance_squared)>rec_radius*rec_radius)
                continue;
            const auto ints=puff_sampling_integrals(
                xo,yo,xn,yn,xr,yr,at_rec,screen.closest_distance_squared);
            const double inc=receptor_concentration_increment(ints,{
                .mass_old=s.vmass.mass_mixed[0],
                .mass_new=s.vmass.mass_mixed[0],
                .sigma_y_at_receptor_m=sy,
                .closest_approach_squared_km2=screen.closest_distance_squared,
                .vertical_factor=vfactor,
                .time_fraction=frac,
                .deposition_ratio=1.0,.area_source_factor=1.0
            });
            if(!std::isfinite(inc) ||
               std::abs(inc)>std::numeric_limits<float>::max())
                throw std::overflow_error("flat CALCPF receptor REAL4 overflow");
            values[i]=static_cast<float>(inc);
        }
        return values;
    };
}
} // namespace cteas::puff
