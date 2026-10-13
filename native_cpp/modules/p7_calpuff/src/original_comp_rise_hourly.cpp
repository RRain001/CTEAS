#include "cteas/puff/original_comp_rise_hourly.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cteas::puff {
RiseCompHour OriginalCompRiseMultiPuffHours::advance_hour(
    const OriginalMetHour& met,const Calmet21Static& statics,
    const NeutralCompControls& controls,const DispersionSetup& pg,
    bool urban,const std::vector<RisePointAdmission>& admissions) {
    int last=last_id_;
    for(const auto& admission:admissions){
        if(admission.puff_id<=last)
            throw std::invalid_argument("noncanonical rising point release IDs");
        last=admission.puff_id;
    }
    // Copy *all* native rise, motion and mass state at the hour boundary.
    // No puff (old or newly admitted) is committed if ANY puff fails.
    auto trial=puffs_;
    auto ids=active_ids_;
    for(const auto& a:admissions){
        trial.admit(a.puff_id,a.point,a.mass);
        ids.push_back(a.puff_id);
    }
    RiseCompHour result;
    result.hour_index=hours_;
    result.puffs.reserve(ids.size());
    for(const int id:ids){
        result.puffs.push_back({id,
            trial.step_pg_rise(id,met,statics,controls,pg,urban)});
    }
    result.active_after=ids.size();
    puffs_=std::move(trial);
    active_ids_=std::move(ids);
    last_id_=last;
    ++hours_;
    return result;
}

RiseCadencedHour OriginalCompRiseMultiPuffHours::advance_hour_point_native_wsnew(
    const OriginalMetHour& met,const Calmet21Static& statics,
    const NeutralCompControls& controls,const DispersionSetup& pg,
    bool urban,const OriginalPointHourSource& source,
    float calm_threshold_ms) {
    // Exact original POINTS1 L15448 and L15965-15970: first RLSMET at
    // HTSTAK, then RLSMET at PRFIN effective height HEFF. WSNEW must use
    // AMAX1(stack WS, final-rise WS), never only the final-rise wind.
    OriginalReleaseWindRequest position{
        .x_cells=static_cast<float>(source.point.source_x_km),
        .y_cells=static_cast<float>(source.point.source_y_km),
        .release_height_m=static_cast<float>(source.point.effective_height_m),
        .calm_threshold_ms=calm_threshold_ms
    };
    const auto final=original_release_wind_metfm1(met,statics,position);
    position.release_height_m=static_cast<float>(source.point.stack_height_m);
    const auto stack=original_release_wind_metfm1(met,statics,position);
    auto native=source;
    native.cadence.wind_speed_ms=std::max(
        stack.wind_speed_ms,final.wind_speed_ms);
    return advance_hour_point_cadence(
        met,statics,controls,pg,urban,native);
}

RiseCadencedHour OriginalCompRiseMultiPuffHours::advance_hour_point_cadence(
    const OriginalMetHour& met,const Calmet21Static& statics,
    const NeutralCompControls& controls,const DispersionSetup& pg,
    bool urban,const OriginalPointHourSource& source) {
    if(!std::isfinite(controls.seconds) || controls.seconds<=0.0F ||
       controls.max_sampling_steps!=1 || controls.release_number!=1 ||
       !std::isfinite(source.cadence.hour_seconds) ||
       static_cast<float>(source.cadence.hour_seconds)!=controls.seconds ||
       source.point.source_type!=1 ||
       source.mass.step_index!=0 || source.mass.species_count<1 ||
       last_id_>std::numeric_limits<int>::max()-source.cadence.max_puffs)
       throw std::invalid_argument("POINTS1 cadence requires one finite full hour / point-puff path");

    // POINTS1 L15975-16002, itself independently compared to ORIGINAL.
    const ReleaseCadence cadence=release_cadence(source.cadence);
    const float dt=static_cast<float>(cadence.emission_duration_s);
    if(!std::isfinite(dt) || dt<=0.0F ||
       cadence.puff_count<1 || cadence.new_puffs!=cadence.puff_count)
       throw std::invalid_argument("invalid native REAL4 POINTS1 emission duration");

    // Copy all active puffs; the entire hour is one atomic transaction.
    auto trial=puffs_;
    auto ids=active_ids_;
    int next_id=last_id_;
    RiseCadencedHour result;
    result.hour_index=hours_;
    result.cadence=cadence;
    result.new_puffs=static_cast<std::size_t>(cadence.puff_count);
    const std::size_t old_count=ids.size();
    result.steps.reserve(old_count+
                         static_cast<std::size_t>(cadence.puff_count)*
                         static_cast<std::size_t>(cadence.puff_count+1)/2U);

    // Existing puffs get their full hour. The original may divide old
    // puff trajectories further for MXSAM>1; explicitly refuse that here.
    for(const int id:ids){
        const auto sub=trial.step_pg_rise(
            id,met,statics,controls,pg,urban);
        result.steps.push_back({id,0,1,sub});
    }

    for(int j=1;j<=cadence.puff_count;++j) {
        const int id=++next_id;
        auto point=source.point;
        auto mass=source.mass;
        point.release_index=j;
        point.emission_duration_s=dt;   // TEMIT0 / DT, original REAL*4
        mass.emission_duration_s=dt;    // INJECT: QM+QU = Q*DT
        trial.admit(id,point,mass);
        ids.push_back(id);
        auto partial=controls;
        partial.seconds=dt;
        partial.source_period_s=dt;
        // CALPUFF new puff releases start on slot IPNO (not the beginning
        // of the hour). The initial remaining age after this hour is
        // (NPUFFS-J+1)*DT; every substep is native GRISE+PG+VMASS.
        for(int k=j;k<=cadence.puff_count;++k) {
            const auto sub=trial.step_pg_rise(
                id,met,statics,partial,pg,urban);
            result.steps.push_back({id,j,k,sub});
        }
    }
    result.active_after=ids.size();
    puffs_=std::move(trial);
    active_ids_=std::move(ids);
    last_id_=next_id;
    ++hours_;
    return result;
}

OriginalCompMxsamHour
OriginalCompRiseMultiPuffHours::advance_hour_point_mxsam99_unsplit(
    const OriginalMetHour& met,const Calmet21Static& statics,
    const NeutralCompControls& controls,const DispersionSetup& pg,
    bool urban,const OriginalPointHourSource& source) {
    if(controls.max_sampling_steps!=99 || controls.release_number!=1 ||
       controls.met_spacing_m<=0.0F ||
       controls.max_sample_distance_cells<=0.0F ||
       controls.seconds!=3600.0F ||
       source.cadence.hour_seconds!=3600.0 ||
       source.point.source_type!=1 ||
       source.mass.step_index!=0 ||
       source.cadence.max_puffs>99 ||
       last_id_>std::numeric_limits<int>::max()-99)
        throw std::invalid_argument("original COMP MXSAM99 unsplit point-puff scope invalid");
    const ReleaseCadence release=release_cadence(source.cadence);
    const float dt=static_cast<float>(release.emission_duration_s);
    if(!std::isfinite(dt)||dt<=0 || release.puff_count<1)
        throw std::invalid_argument("invalid source-ported NPUFFS/DT");
    auto trial=puffs_;
    auto ids=active_ids_;
    int next_id=last_id_;
    OriginalCompMxsamHour result;
    result.hour_index=hours_;
    result.release=release;
    const auto execute=[&](int id,int rel,int k,
                           const SamplingCadence& plan){
        auto ctrl=controls;
        ctrl.seconds=static_cast<float>(plan.sampling_step_s);
        ctrl.source_period_s=ctrl.seconds;
        ctrl.release_number=1;
        ctrl.max_sampling_steps=1; // already selected outer original KSAM
        const auto physics=trial.step_pg_rise(
            id,met,statics,ctrl,pg,urban);
        // We do not silently approximate the vendor L10556-10578
        // FSTEP shortening and a renewed RISEWIND at changed TSAMP.
        const auto constrained=substep_length({
            .sampling_step_s=plan.sampling_step_s,
            .wind_speed_ms=physics.rise.adjusted.wind_speed_ms,
            .puff=true,.free_slug=false,.subsampling=plan.subsampling,
            .grid_spacing_m=controls.met_spacing_m,
            .x_sam_len=controls.max_sample_distance_cells,
            .substep_index=k,.sampling_steps=plan.sampling_steps
        });
        if(constrained.split ||
           static_cast<float>(constrained.step_s)!=ctrl.seconds)
            throw std::invalid_argument(
                "COMP source requires FSTEP split and RISEWIND resampling");
        result.steps.push_back({id,rel,k,plan,physics});
    };
    for(const int id:ids){
        const auto native=trial.pre_cadence_wind(id,met,statics);
        const float u=static_cast<float>(native.wind.u_ms);
        const float v=static_cast<float>(native.wind.v_ms);
        const float ws=std::sqrt(u*u+v*v);
        if(!std::isfinite(ws)||ws<0.5F)
            throw std::invalid_argument(
                "original COMP old-puff calm treatment not yet integrated");
        const auto plan=sampling_cadence({
            .new_puff=false,.step_seconds=controls.seconds,
            .wind_speed_ms=ws,
            .x_sam_len=controls.max_sample_distance_cells,
            .grid_spacing_m=controls.met_spacing_m,
            .max_sampling_steps=99
        });
        for(int k=plan.start_substep;k<=plan.sampling_steps;++k)
            execute(id,0,k,plan);
    }
    for(int j=1;j<=release.puff_count;++j){
        auto point=source.point;
        auto mass=source.mass;
        point.release_index=j;
        point.emission_duration_s=dt;
        mass.emission_duration_s=dt;
        const int id=++next_id;
        trial.admit(id,point,mass);
        ids.push_back(id);
        const auto plan=sampling_cadence({
            .new_puff=true,.source_type=1,
            .emission_period_s=dt,.step_seconds=controls.seconds,
            .release_number=j,
            .wind_speed_ms=source.cadence.wind_speed_ms,
            .x_sam_len=controls.max_sample_distance_cells,
            .grid_spacing_m=controls.met_spacing_m,
            .max_sampling_steps=99
        });
        if(plan.sampling_steps!=release.puff_count)
            throw std::invalid_argument("native POINTS1/COMP NEW NSAM drift");
        for(int k=plan.start_substep;k<=plan.sampling_steps;++k)
            execute(id,j,k,plan);
    }
    result.active_puffs=ids.size();
    puffs_=std::move(trial);
    active_ids_=std::move(ids);
    last_id_=next_id;
    ++hours_;
    return result;
}
OriginalPuffMotionState OriginalCompRiseMultiPuffHours::motion(int id)const {
    return puffs_.motion(id);
}
NativeVerticalPuffState OriginalCompRiseMultiPuffHours::vertical(int id)const{
    return puffs_.vertical(id);
}
std::size_t OriginalCompRiseMultiPuffHours::puff_steps(int id)const{
    return puffs_.steps(id);
}
std::size_t OriginalCompRiseMultiPuffHours::completed_hours()const noexcept{
    return hours_;
}
std::size_t OriginalCompRiseMultiPuffHours::active_count()const noexcept{
    return active_ids_.size();
}
const std::vector<int>& OriginalCompRiseMultiPuffHours::canonical_ids()const noexcept{
    return active_ids_;
}
void OriginalCompRiseMultiPuffHours::retire(int id){
    if(std::find(active_ids_.begin(),active_ids_.end(),id)==active_ids_.end())
        throw std::out_of_range("retire unknown rising puff");
    auto next=puffs_;
    next.retire(id);
    puffs_=std::move(next);
    active_ids_.erase(
        std::remove(active_ids_.begin(),active_ids_.end(),id),
        active_ids_.end());
}
} // namespace cteas::puff
