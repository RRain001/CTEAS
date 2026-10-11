#pragma once
// P6D9A selected original CALMET 6.5.0 DIAGNO bridging contract.
//
// Source call order: WIND1 [L5590], WINDBC [L5599], then optional kinetic,
// Froude [L5643-5644], radiative/heat, slope corrections and final WINDBC
// [L5748]; INTER2 U/V [L6055-6061]; SMOOTH+DIVCEL+W [L6119-6148]; optional
// IOBR=1 O'Brien W adjustment [L6153-6159] and MINIM [L6180].
//
// D9A deliberately DOES NOT simulate the intervening heat/terrain physics.
// It requires the caller to explicitly designate a synthetic-identity
// audit route or supply an externally calculated, audited post-correction
// step-one field. No silent fallback and no claim of actual DIAGNO parity.
#include "cteas/p6/wind1_selected_firstguess.hpp"
#include "cteas/p6/inter2_selected.hpp"
#include "cteas/p6/diagno_segment.hpp"
#include "cteas/p6/diagno_obrien_minim.hpp"
#include "cteas/p6/froude_adjust.hpp"
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cteas::calmet::p6 {

enum class DiagnoD9UpstreamMode {
    Missing=0,
    SyntheticIdentityForSourceAudit=1,
    ExternallyCompletedUpstreamPhysics=2
};
struct DiagnoD9FroudeBoundary {
    // Original FRADJ inputs beyond the step-one winds: gridded terrain,
    // terrain envelope HMAX, lapse rate GAMMA, 2-D surface temperature and
    // the ISURFT selector. All 2500-element fields are k-less 50x50 planes.
    int isurft=-1;
    float tinf_k=0.f;
    float critfn=0.f;
    std::vector<float> terrain_m,maximum_terrain_m;
    std::vector<float> lapse_rate_k_per_m,surface_temperature_k;
};
struct DiagnoD9SelectedInput {
    Wind1SelectedStationHour stations;
    Wind1FirstGuessSelectedControls first_guess;
    Inter2SelectedInput inter2_u, inter2_v;
    std::vector<int> smoothing_passes;
    std::vector<float> layer_thickness_m;
    DiagnoD9UpstreamMode upstream_mode=DiagnoD9UpstreamMode::Missing;
    // If externally completed, replace WIND1 original step-one winds with
    // the caller's terrain/heat/slope-corrected UG/VG. Size 50*50*9.
    std::vector<float> corrected_first_guess_u_mps,corrected_first_guess_v_mps;
    bool lake_breeze=false;
    // Original IFRADJ=1 stage L5643-L5644, applied to the step-one UG/VG
    // before the L5748 WINDBC snapshot and the L6055/L6059 INTER2 calls.
    bool froude_adjustment=false;
    DiagnoD9FroudeBoundary froude;
    // Original IOBR=1 tail: O'Brien W adjustment L6153-L6159 then MINIM
    // L6180. CELLZB(1) is the original lowest face height, NITER and DIVLIM
    // are the original namelist MINIM controls.
    bool obrien_adjustment=false;
    float cellzb_origin_m=0.f;
    int minim_iterations=0;
    float minim_divergence_limit_per_s=0.f;
};
struct DiagnoD9SelectedOutput {
    Wind1FirstGuessSelectedFields raw_first_guess;
    std::vector<float> prepared_ug,prepared_vg;
    std::vector<float> blended_u,blended_v;
    FinalWindSegment final_fields;
};

inline DiagnoD9SelectedOutput integrate_diagno_d9_selected(
    const DiagnoD9SelectedInput& request) {
    if(request.upstream_mode==DiagnoD9UpstreamMode::Missing)
        throw std::invalid_argument("D9 upstream terrain/radiative/slope stage not provided");
    if(request.lake_breeze)
        throw std::invalid_argument("D9 lake breeze branch not verified");
    if(request.first_guess.nx!=50||request.first_guess.ny!=50||
       request.first_guess.nz!=9)
        throw std::invalid_argument("D9 only supports frozen 50x50x9 case");
    if(request.smoothing_passes.size()!=9||request.layer_thickness_m.size()!=9)
        throw std::invalid_argument("D9 missing nine-level SMOOTH/DZ controls");
    for(float dz:request.layer_thickness_m)
        if(!(std::isfinite(dz)&&dz>0.f))
            throw std::invalid_argument("D9 invalid DZ");
    const auto first=wind1_firstguess_selected(request.stations,request.first_guess);
    auto u=first.ug_mps,v=first.vg_mps;
    if(request.upstream_mode==DiagnoD9UpstreamMode::ExternallyCompletedUpstreamPhysics){
        if(request.corrected_first_guess_u_mps.size()!=22500||
           request.corrected_first_guess_v_mps.size()!=22500)
            throw std::invalid_argument("D9 missing upstream corrected UG/VG");
        u=request.corrected_first_guess_u_mps;
        v=request.corrected_first_guess_v_mps;
        for(float x:u)if(!std::isfinite(x))
            throw std::invalid_argument("D9 corrected U contains missing values");
        for(float x:v)if(!std::isfinite(x))
            throw std::invalid_argument("D9 corrected V contains missing values");
    }else if(!request.corrected_first_guess_u_mps.empty()||
             !request.corrected_first_guess_v_mps.empty()) {
        throw std::invalid_argument("D9 synthetic identity cannot silently apply corrections");
    }
    if(request.froude_adjustment) {
        if(request.upstream_mode!=DiagnoD9UpstreamMode::SyntheticIdentityForSourceAudit)
            throw std::invalid_argument("D9 Froude stage cannot re-adjust an external corrected field");
        const auto& f=request.froude;
        if(f.terrain_m.size()!=22500||f.maximum_terrain_m.size()!=22500||
           f.lapse_rate_k_per_m.size()!=22500||
           f.surface_temperature_k.size()!=22500)
            throw std::invalid_argument("D9 missing original FRADJ 50x50 fields");
        std::vector<float> zface(10);
        zface[0]=request.cellzb_origin_m;
        for(std::size_t k=0;k<9;++k)zface[k+1]=zface[k]+
            request.layer_thickness_m[k];
        WindUV step_one{std::move(u),std::move(v)};
        auto adjusted=froude_adjust(
            50,50,9,100.f,100.f,f.terrain_m,f.maximum_terrain_m,
            f.lapse_rate_k_per_m,f.surface_temperature_k,
            f.isurft,f.tinf_k,f.critfn,zface,std::move(step_one));
        u=std::move(adjusted.u);v=std::move(adjusted.v);
    }

    // Original DIAGNO refreshes step-one WINDBC after terrain and slope
    // modifications at L5748, BEFORE the L6055/6059 INTER2 calls.
    const auto nx=std::size_t(50),ny=std::size_t(50),plane=nx*ny;
    std::vector<float> ub(9*2*ny),vb(9*2*nx);
    for(std::size_t k=0;k<9;++k)for(std::size_t j=0;j<ny;++j){
        ub[k*2*ny+j]=u[k*plane+j*nx];
        ub[k*2*ny+ny+j]=u[k*plane+j*nx+nx-1];
    }
    for(std::size_t k=0;k<9;++k)for(std::size_t i=0;i<nx;++i){
        vb[k*2*nx+i]=v[k*plane+i];
        vb[k*2*nx+nx+i]=v[k*plane+(ny-1)*nx+i];
    }

    auto au=request.inter2_u,av=request.inter2_v;
    // Original INTER2 has independent observed station values, one
    // component at a time; never copy U into the V observation slot.
    auto check_inter2=[&](const Inter2SelectedInput& field){
        if(field.nx!=50||field.ny!=50||field.nz!=9||
           field.nstations!=2||field.iprog!=0||field.nbar!=0||
           field.noobs!=0||field.dx_m!=100.f||field.dy_m!=100.f)
            throw std::invalid_argument("D9 original selected INTER2 control mismatch");
        if(field.station_x_km.size()!=2||field.station_y_km.size()!=2)
            throw std::invalid_argument("D9 missing station locations");
        const float sx=request.first_guess.surface_x_relative_m*0.001f;
        const float sy=request.first_guess.surface_y_relative_m*0.001f;
        const float ux=request.first_guess.upper_x_relative_m*0.001f;
        const float uy=request.first_guess.upper_y_relative_m*0.001f;
        constexpr float eps=0.000001f;
        if(std::abs(field.station_x_km[0]-sx)>eps||
           std::abs(field.station_y_km[0]-sy)>eps||
           std::abs(field.station_x_km[1]-ux)>eps||
           std::abs(field.station_y_km[1]-uy)>eps)
            throw std::invalid_argument("D9 WIND1/INTER2 station position mismatch");
    };
    check_inter2(au);check_inter2(av);
    // WO weights may legitimately differ between U and V in original
    // CALMET configurations. Preserve independently supplied arrays.
    au.step1_mps=u;av.step1_mps=v;
    const auto ou=original_inter2_selected(au);
    const auto ov=original_inter2_selected(av);
    SmoothInput smooth;
    smooth.nx=50;smooth.ny=50;smooth.nz=9;
    smooth.lake_breeze=false;
    smooth.passes_per_layer=request.smoothing_passes;
    smooth.u_mps=ou.component_mps;
    smooth.v_mps=ov.component_mps;
    smooth.ub_west_east_mps=std::move(ub);
    smooth.vb_south_north_mps=std::move(vb);
    auto final=original_smooth_divcel_w(
        std::move(smooth),100.f,100.f,request.layer_thickness_m,false);
    if(request.obrien_adjustment) {
        DiagnoObrienControls controls;
        controls.cellzb_origin_m=request.cellzb_origin_m;
        controls.minim_iterations=request.minim_iterations;
        controls.minim_divergence_limit_per_s=request.minim_divergence_limit_per_s;
        final=original_diagno_obrien_minim(
            std::move(final),request.layer_thickness_m,100.f,100.f,controls);
    }
    DiagnoD9SelectedOutput output;
    output.raw_first_guess=first;
    output.prepared_ug=std::move(u);
    output.prepared_vg=std::move(v);
    output.blended_u=ou.component_mps;
    output.blended_v=ov.component_mps;
    output.final_fields=std::move(final);
    return output;
}

} // namespace cteas::calmet::p6
