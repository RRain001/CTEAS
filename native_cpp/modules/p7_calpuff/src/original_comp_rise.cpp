#include "cteas/puff/original_comp_rise.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace cteas::puff {
namespace {
void require(bool v,const char* message) {
    if(!v)throw std::invalid_argument(message);
}
float r4(double x,const char* what) {
    require(std::isfinite(x),what);
    const float y=static_cast<float>(x);
    require(std::isfinite(y),what);
    return y;
}
}
void OriginalCompRisePointDriver::admit(
    int id,const NewPuffInputs& source,const InjectionInputs& injection) {
    require(id>0 && !rise_.contains(id),"duplicate/invalid rising puff");
    require(source.source_type==1 && source.downwash_code==0 &&
            source.stack_height_m>0.0 &&
            source.effective_height_m>source.stack_height_m &&
            source.x_final_m>0.0 &&
            source.exit_velocity_ms>=0.0 &&
            source.diameter_m>=0.0 &&
            (source.buoyancy_flux>0.0 || source.momentum_flux>0.0) &&
            source.wake_radius_m==0.0 &&
            source.building_height_m==0.0 &&
            source.wake_xshift_m==0.0,
            "original rise point requires native PRFIN, no PRIME/building");
    require(injection.gaussian_distribution==1 &&
            r4(injection.effective_height_m,"injection height")==
              r4(source.effective_height_m,"rise final height"),
            "rise POINTS1 and INJECT source height or Gaussian flag differs");
    RiseSource state;
    state.x_final_m=r4(source.x_final_m,"XFINAL");
    auto& p=state.physics;
    p.source_type=1;
    p.downwash_code=0;
    p.met_period=1;
    p.rise_method=1;
    p.shear_flag=0;
    p.final_rise_m=r4(source.effective_height_m,"ZFINAL");
    p.stack_height_m=r4(source.stack_height_m,"HT0");
    p.full_final_rise_m=r4(
       source.effective_height_m-source.stack_height_m+
       source.stack_tip_downwash_m,"ZFRISE");
    p.diameter_m=r4(source.diameter_m,"DIAM");
    p.exit_velocity_ms=r4(source.exit_velocity_ms,"EXITW");
    p.stack_wind_speed_ms=r4(source.stack_wind_speed_ms,"WS0");
    p.stability_class=source.stability_class;
    p.sqrt_stability_s=r4(source.sqrt_stability_s,"SQRTS");
    p.momentum_flux=r4(source.momentum_flux,"FM");
    p.buoyancy_flux=r4(source.buoyancy_flux,"FB");
    p.x_final_momentum_m=r4(source.x_final_momentum_m,"XMFIN");
    p.z_final_momentum_m=r4(source.z_final_momentum_m,"ZMFIN");
    p.x_final_buoyancy_m=r4(source.x_final_buoyancy_m,"XBFIN");
    p.z_final_buoyancy_m=r4(source.z_final_buoyancy_m,"ZBFIN");
    p.stack_tip_downwash_m=r4(source.stack_tip_downwash_m,"STIPDW");
    // Catch invalid rise data and unsupported source branches before adding
    // to either physical store.
    auto probe=p;
    probe.x_m=0.0;
    (void)gradual_plume_height(probe);
    auto m=motion_;
    auto v=vertical_;
    m.introduce_point_source(id,source);
    v.admit_point(id,source,injection);
    motion_=std::move(m);
    vertical_=std::move(v);
    rise_.emplace(id,state);
}
RiseCompStep OriginalCompRisePointDriver::step_pg_rise(
    int id,const OriginalMetHour& met,const Calmet21Static& statics,
    const NeutralCompControls& control,const DispersionSetup& pg,bool urban) {
    const auto found=rise_.find(id);
    if(found==rise_.end())throw std::out_of_range("unknown rise puff");
    require(pg.mdisp==3 && !pg.heffter_sigma_z &&
            !pg.roughness_adjustment &&
            !control.dispersion.calm &&
            std::isfinite(control.seconds) && control.seconds>0.0F &&
            std::isfinite(control.met_spacing_m) && control.met_spacing_m>0.0F &&
            std::isfinite(control.source_period_s) && control.source_period_s>0.0F &&
            std::isfinite(control.max_sample_distance_cells) &&
            control.max_sample_distance_cells>0.0F &&
            control.max_sampling_steps==1 && control.release_number==1,
            "native rising COMP only supports MDISP3 single noncalm point step");

    // Full state copy: failed wind, GRISE, PG, SETPUF or VMASS must not
    // leave a partial trajectory, mass partition or sampling clock.
    auto m=motion_;
    auto v=vertical_;
    auto state=found->second;
    const auto before=m.state(id);
    const auto vs=v.state(id);
    require(vs.completed_substeps==state.completed &&
            vs.committed_lid_steps==state.completed,
            "native rising puff sigma/mass clock drift");
    VendorSubstepOrder order;
    const SamplingCadence cadence=sampling_cadence({
        .new_puff=state.completed==0,
        .source_type=1,
        .emission_period_s=control.source_period_s,
        .step_seconds=control.seconds,
        .release_number=control.release_number,
        .wind_speed_ms=state.previous_wind_ms,
        .x_sam_len=control.max_sample_distance_cells,
        .grid_spacing_m=control.met_spacing_m,
        .max_sampling_steps=control.max_sampling_steps
    });
    require(cadence.sampling_steps==1 && cadence.start_substep==1,
            "multi-sample COMP not yet integrated");
    order.completed(VendorSubstepPhase::SamplingCadence);
    order.completed(VendorSubstepPhase::LoadPreviousPuffState);
    const auto cell=original_met_cell(
        before.x_cells,before.y_cells,met.nx,met.ny);
    order.completed(VendorSubstepPhase::ResolveMetCell);
    const auto sampled=v.sample(
        id,met,statics,before.x_cells,before.y_cells,before.z_m);
    order.completed(VendorSubstepPhase::EvaluatePuffDz);
    require(sampled.cell.offset_0based==cell.offset_0based,
            "rise METFM1 GETCELL drift");
    order.completed(VendorSubstepPhase::EvaluateAdvect);

    // Original COMP passes DZB (begin vertical top minus centre) and the
    // current puff travel distance to RISEWIND, and the latter itself
    // invokes GRISE at XBEG and XEND in its NCOUNT=2 REAL4 iteration.
    OriginalRiseMetInputs inputs;
    inputs.x_cells=before.x_cells;
    inputs.y_cells=before.y_cells;
    inputs.start_distance_m=before.total_distance_m;
    inputs.final_distance_m=state.x_final_m;
    inputs.step_seconds=r4(cadence.sampling_step_s,"rise step time");
    inputs.rise_half_depth_m=r4(
        std::max(0.0,sampled.extent.z_top_m-before.z_m),"DZB");
    inputs.rise_iterations=2;
    inputs.plume=state.physics;
    const auto rise=original_rise_wind_met(met,statics,inputs);
    const float u=r4(rise.adjusted.u_ms,"RISEWIND U");
    const float w=r4(rise.adjusted.v_ms,"RISEWIND V");
    const float ws=r4(rise.adjusted.wind_speed_ms,"RISEWIND WS");
    require(ws>=static_cast<float>(kRiseWindCalmSpeedMs),
            "rise COMP refuses calm-advection treatment");
    order.completed(VendorSubstepPhase::ApplyRiseWindAndCalm);

    const Substep substep=substep_length({
        .sampling_step_s=cadence.sampling_step_s,
        .wind_speed_ms=ws,.puff=true,.free_slug=false,
        .subsampling=cadence.subsampling,
        .grid_spacing_m=control.met_spacing_m,
        .x_sam_len=control.max_sample_distance_cells,
        .substep_index=1,.sampling_steps=1
    });
    require(!substep.split &&
            r4(substep.step_s,"native rise TSAMP")==control.seconds,
            "rise COMP cannot silently shorten a sampling step");
    order.completed(VendorSubstepPhase::ChooseSubstepLength);

    const float distance=r4(ws*r4(substep.step_s,"time"),"rise distance");
    auto end_state=state.physics;
    end_state.x_m=r4(before.total_distance_m+distance,"RISE x end");
    const float end_height=r4(
        gradual_plume_height(end_state).height_m,"native GRISE ZE");
    const auto moved=m.advance(id,{
        .uadv_ms=u,.vadv_ms=w,.ws_ms=ws,
        .tsamp_s=r4(substep.step_s,"rise TSAMP"),
        .met_spacing_m=control.met_spacing_m,
        .end_height_m=end_height,
        .settling_velocity_ms=0.0F,.plume_tilt=0
    });
    order.completed(VendorSubstepPhase::AdvancePosition);

    const StepBookkeeping bookkeeping=step_bookkeeping({
        .total_distance_m=before.total_distance_m,
        .total_time_s=before.travel_time_s,
        .step_distance_m=moved.distance_m,
        .step_seconds=substep.step_s,
        .km_per_m=0.001,.wind_speed_ms=ws,
        .calm_wind_threshold=0.0,.downwash_code=0,
        .x_final_m=state.x_final_m,
        .dispersion_y_option=3,.dispersion_z_option=3
    });
    require(!bookkeeping.calm,"rise COMP refuses calm SETPUF");
    const SigmaCurves curves(dispersion_environment(
        pg,sampled.stability_class,urban,ws));
    const SigmaYCurve sigma_y=[&curves](double first,double delta,double time) {
        const auto s=curves.sigma_y(first,{delta,time});
        return SigmaCurvePoint{s.sigma_y_m,s.virtual_time_y_s,
                               s.virtual_distance_y_km};
    };
    const SigmaZCurve sigma_z=[&curves](double first,double delta,
                                        double time,double height) {
        const auto s=curves.sigma_z(first,height,{delta,time});
        return SigmaCurvePoint{s.sigma_z_m,s.virtual_time_z_s,
                               s.virtual_distance_z_km};
    };
    auto dispersion=control.dispersion;
    dispersion.step.puff_code=vs.puff_code;
    dispersion.step.sigma_y_start_m=vs.sigma_y_m;
    dispersion.step.sigma_z_start_m=vs.sigma_z_m;
    dispersion.step.puff_height_m=before.z_m;
    dispersion.step.mid_time_s=bookkeeping.mid_time_s;
    dispersion.step.mid_distance_km=bookkeeping.mid_distance_km;
    const auto sigma=v.commit_sigma(id,dispersion,sigma_y,sigma_z);
    const auto mass=v.commit_vmass_and_lid(
        id,sampled.mixing_height_m,before.z_m,moved.state.z_m,
        r4(sigma.adjust.sigma_z_end_m,"SIGZE"),
        sampled.stability_class,0);
    order.completed(VendorSubstepPhase::CommitSetpuf);
    order.finish();
    const auto post=v.state(id);
    require(post.completed_substeps==state.completed+1 &&
            post.committed_lid_steps==state.completed+1,
            "native rising COMP partial vertical commit");
    state.previous_wind_ms=ws;
    ++state.completed;
    RiseCompStep outcome{
        before,moved.state,post,sampled,rise,
        cadence,substep,bookkeeping,sigma,mass,end_height
    };
    motion_=std::move(m);
    vertical_=std::move(v);
    found->second=std::move(state);
    return outcome;
}
OriginalPuffMotionState OriginalCompRisePointDriver::motion(int id)const{
    return motion_.state(id);
}
NativeVerticalPuffState OriginalCompRisePointDriver::vertical(int id)const{
    return vertical_.state(id);
}
std::size_t OriginalCompRisePointDriver::steps(int id)const{
    const auto it=rise_.find(id);
    if(it==rise_.end())throw std::out_of_range("unknown rising puff");
    return it->second.completed;
}
std::size_t OriginalCompRisePointDriver::active_count()const noexcept{
    return rise_.size();
}
void OriginalCompRisePointDriver::retire(int id) {
    require(rise_.contains(id),"unknown native rise puff retire");
    auto m=motion_;
    auto v=vertical_;
    m.retire(id);
    v.retire(id);
    motion_=std::move(m);
    vertical_=std::move(v);
    rise_.erase(id);
}
} // namespace cteas::puff
