#include "cteas/puff/original_comp_neutral.hpp"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace cteas::puff {
namespace {
void require(bool condition,const char* msg){
    if(!condition)throw std::invalid_argument(msg);
}
float f32(double value) {
    require(std::isfinite(value),"non-finite native COMP calculation");
    const auto f=static_cast<float>(value);
    require(std::isfinite(f),"REAL4 COMP overflow");
    return f;
}
}
void OriginalCompNeutralPointDriver::admit(
    int id,const NewPuffInputs& point,const InjectionInputs& injection) {
    require(running_.find(id)==running_.end(),"duplicate native COMP point");
    // Admission is transactional: error in INJECT or POINTS1 never creates
    // half-admitted motion/vertical state.
    auto m=motion_,v=vertical_;
    m.introduce_point_source(id,point);
    v.admit_point(id,point,injection);
    motion_=std::move(m);vertical_=std::move(v);
    running_.emplace(id,RunningPuff{});
}
NeutralCompResult OriginalCompNeutralPointDriver::step(
    int id,const OriginalMetHour& met,const Calmet21Static& statics,
    const NeutralCompControls& control,
    const SigmaYCurve& yc,const SigmaZCurve& zc) {
    const auto entry=running_.find(id);
    if(entry==running_.end())throw std::out_of_range("unknown native COMP puff");
    require(std::isfinite(control.seconds) && control.seconds>0.0F &&
            std::isfinite(control.source_period_s) &&
            control.source_period_s>0.0F &&
            std::isfinite(control.met_spacing_m) &&
            control.met_spacing_m>0.0F &&
            std::isfinite(control.max_sample_distance_cells) &&
            control.max_sample_distance_cells>0.0F &&
            control.max_sampling_steps==1 && control.release_number==1,
            "native COMP only supports one neutral sampling step");

    // Copy-on-commit: both independent state authorities advance only once
    // EVERY component of the substep has succeeded. Any exception retains
    // both previous states, the prior-wind cadence and sequence count.
    auto m=motion_,v=vertical_;
    const auto running=entry->second;
    VendorSubstepOrder order;
    SamplingCadenceInputs sampling;
    sampling.new_puff=running.steps==0;
    sampling.source_type=1;
    sampling.emission_period_s=control.source_period_s;
    sampling.step_seconds=control.seconds;
    sampling.release_number=control.release_number;
    sampling.wind_speed_ms=running.previous_ws_ms;
    sampling.x_sam_len=control.max_sample_distance_cells;
    sampling.grid_spacing_m=control.met_spacing_m;
    sampling.max_sampling_steps=1;
    const auto cadence=sampling_cadence(sampling);
    require(cadence.sampling_steps==1 && cadence.start_substep==1,
            "native neutral COMP cadence unexpectedly requires multiple steps");
    order.completed(VendorSubstepPhase::SamplingCadence);

    const auto before=m.state(id);
    const auto existing=v.state(id);
    require(existing.completed_substeps==existing.committed_lid_steps &&
            existing.completed_substeps==running.steps,
            "motion and vertical COMP sequencing diverged");
    order.completed(VendorSubstepPhase::LoadPreviousPuffState);

    const auto cell=original_met_cell(
        before.x_cells,before.y_cells,met.nx,met.ny);
    order.completed(VendorSubstepPhase::ResolveMetCell);
    // Original REAL4 vertical average followed by METFM=1 ADVWND.
    // Both are evaluated together by the source-verified native bridge.
    const auto sampled=v.sample(id,met,statics,
        before.x_cells,before.y_cells,before.z_m);
    require(sampled.cell.offset_0based==cell.offset_0based,
            "COMP GETCELL and vertical wind cells disagreed");
    order.completed(VendorSubstepPhase::EvaluatePuffDz);
    order.completed(VendorSubstepPhase::EvaluateAdvect);

    // Explicit supported subset: *no* stack rise, no RISEWIND correction,
    // no calm forcing. Never use this path for the non-neutral Hefei plume.
    const float u=f32(sampled.wind.u_ms),w=f32(sampled.wind.v_ms);
    const float ws=std::sqrt(f32(u*u+w*w));
    require(std::isfinite(ws) && ws>0.0F,
            "native neutral COMP refuses calm and zero-wind branches");
    require(!control.dispersion.calm,
            "native neutral COMP cannot approximate CALPUFF calm");
    order.completed(VendorSubstepPhase::ApplyRiseWindAndCalm);

    SubstepInputs sub;
    sub.sampling_step_s=cadence.sampling_step_s;
    sub.wind_speed_ms=ws;
    sub.puff=true;
    sub.free_slug=false;
    sub.subsampling=cadence.subsampling;
    sub.grid_spacing_m=control.met_spacing_m;
    sub.x_sam_len=control.max_sample_distance_cells;
    sub.substep_index=1;
    sub.sampling_steps=1;
    const auto next=substep_length(sub);
    require(!next.split && f32(next.step_s)==control.seconds,
            "native neutral COMP refuses split and partial sampling steps");
    order.completed(VendorSubstepPhase::ChooseSubstepLength);

    // Advance position with source-ported REAL4 COMP + SETPUF coordinate
    // path; no DGM state or externally supplied adjusted wind.
    const auto post=m.advance(id,{
        .uadv_ms=u,.vadv_ms=w,.ws_ms=ws,.tsamp_s=f32(next.step_s),
        .met_spacing_m=control.met_spacing_m,
        .end_height_m=before.z_m,.settling_velocity_ms=0.0F,.plume_tilt=0
    });
    order.completed(VendorSubstepPhase::AdvancePosition);

    // Native P7Q/SETPUF sigma, followed by the original VMASS and COMP
    // history assignment. Physical dispersion curves remain caller-provided
    // (not yet calculated from the full frozen CALPUFF local met).
    auto sigma=control.dispersion;
    sigma.step.sigma_y_start_m=existing.sigma_y_m;
    sigma.step.sigma_z_start_m=existing.sigma_z_m;
    sigma.step.puff_code=existing.puff_code;
    sigma.step.puff_height_m=before.z_m;
    sigma.step.mid_time_s=f32(before.travel_time_s+
                              f32(next.step_s)*0.5F);
    sigma.step.mid_distance_km=f32((before.total_distance_m+
                                post.distance_m*0.5F)*0.001F);
    const auto sigmas=v.commit_sigma(id,sigma,yc,zc);
    const auto vmass=v.commit_vmass_and_lid(
        id,sampled.mixing_height_m,before.z_m,post.state.z_m,
        f32(sigmas.adjust.sigma_z_end_m),sampled.stability_class,0);
    order.completed(VendorSubstepPhase::CommitSetpuf);
    order.finish();
    const auto updated=v.state(id);
    require(updated.completed_substeps==running.steps+1 &&
            updated.committed_lid_steps==running.steps+1,
            "native COMP did not commit exactly one complete vertical step");

    NeutralCompResult result{
        before,post.state,updated,cell,
        sampled.extent,sampled.wind,cadence,next,sigmas,vmass,ws
    };
    // No new-puff insertion or earlier state is ever lost on failure.
    motion_=std::move(m);
    vertical_=std::move(v);
    entry->second.steps++;
    entry->second.previous_ws_ms=ws;
    return result;
}
OriginalPuffMotionState OriginalCompNeutralPointDriver::motion(int id)const{
    return motion_.state(id);
}
NativeVerticalPuffState OriginalCompNeutralPointDriver::vertical(int id)const{
    return vertical_.state(id);
}
std::size_t OriginalCompNeutralPointDriver::steps(int id)const{
    const auto it=running_.find(id);
    if(it==running_.end())throw std::out_of_range("unknown native COMP puff");
    return it->second.steps;
}
void OriginalCompNeutralPointDriver::retire(int id) {
    if(running_.find(id)==running_.end())
        throw std::out_of_range("unknown native COMP retire");
    auto m=motion_,v=vertical_;
    m.retire(id);v.retire(id);
    motion_=std::move(m);vertical_=std::move(v);
    running_.erase(id);
}
std::size_t OriginalCompNeutralPointDriver::active_count()const noexcept{
    return running_.size();
}
} // namespace cteas::puff
