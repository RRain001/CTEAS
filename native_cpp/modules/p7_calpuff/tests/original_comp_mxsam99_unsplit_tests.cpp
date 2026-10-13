#include "cteas/puff/original_comp_rise_hourly.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace cteas::puff;
namespace {
void need(bool v,const char* s){if(!v)throw std::runtime_error(s);}
void test(){
    OriginalCompRiseMultiPuffHours driver;
    OriginalMetHour met;
    met.nx=120;met.ny=120;met.nz=3;
    const auto n=120U*120U;
    met.zi.assign(n,250.0F);
    met.ipgt.assign(n,4);
    met.u.assign(n*3,1.2F);
    met.v.assign(n*3,0.0F);
    Calmet21Static geo;
    geo.cell_face_m={0,100,200,300};
    geo.roughness_m.assign(n,0.25F);
    NeutralCompControls ctl;
    ctl.max_sampling_steps=99;
    ctl.met_spacing_m=100.0F;
    ctl.max_sample_distance_cells=1.0F;
    ctl.seconds=3600.0F;
    ctl.source_period_s=3600.0F;
    DispersionSetup pg;
    pg.mdisp=3;pg.averaging_factor=1;
    pg.sigma_y_heffter_m=550;
    pg.heffter_sigma_z=false;pg.roughness_adjustment=false;
    OriginalPointHourSource source;
    source.point.source_x_km=5;
    source.point.source_y_km=5;
    source.point.stack_height_m=50;
    source.point.effective_height_m=160;
    source.point.x_final_m=230;
    source.point.buoyancy_flux=49;
    source.point.x_final_buoyancy_m=230;
    source.point.z_final_buoyancy_m=110;
    source.point.stack_wind_speed_ms=1.2;
    source.point.sigma_y_source_m=1;
    source.point.sigma_z_source_m=1;
    source.point.stability_class=4;
    source.mass.species_count=2;
    source.mass.emission_duration_s=3600;
    source.mass.effective_height_m=160;
    source.mass.mixing_height_m=250;
    source.mass.max_layer_height_m=300;
    source.mass.mass_fraction_mixed=1;
    source.mass.stability_class=4;
    source.mass.gaussian_distribution=1;
    source.mass.emission_rate={2,3};
    source.cadence.wind_speed_ms=1.2;
    source.cadence.emission_interval=36.0;
    source.cadence.max_puffs=99;
    source.cadence.hour_seconds=3600;
    const auto a=driver.advance_hour_point_mxsam99_unsplit(
        met,geo,ctl,pg,false,source);
    need(a.release.puff_count==44 &&
         a.active_puffs==44 &&
         a.steps.size()==44U*45U/2U &&
         driver.completed_hours()==1,
         "MXSAM99 new release slots absent");
    for(const auto& s:a.steps){
        need(s.release_number>0 &&
             s.original_plan.sampling_steps==44 &&
             s.original_plan.start_substep==s.release_number &&
             s.ksam>=s.release_number &&
             s.step.after.x_cells>s.step.before.x_cells,
             "native original new MXSAM99 rise/PG step drift");
    }
    const auto b=driver.advance_hour_point_mxsam99_unsplit(
        met,geo,ctl,pg,false,source);
    need(b.active_puffs==88 &&
         driver.completed_hours()==2,
         "second rising MXSAM99 hour not committed");
    std::size_t old_steps=0;
    for(const auto& s:b.steps){
        if(s.release_number==0){
            ++old_steps;
            need(s.original_plan.sampling_steps==44 &&
                 s.original_plan.start_substep==1 &&
                 s.original_plan.old_flag==1,
                 "old puff NSAM99 selected wrong vendor cadence");
        }
        need(s.step.vertical_after.committed_lid_steps>0 &&
             s.step.after.x_cells>s.step.before.x_cells,
             "native PG/VMASS substep did not move puff");
    }
    need(old_steps==44U*44U,
         "old puff multi-step COMP execution omitted slots");
    // Integrated POINTS1 -> original RLSMET WINDSET at stack AND HEFF ->
    // MXSAM=99 COMP. The caller's deliberately incorrect WSNEW must not be
    // used. Stack level 1.5 m/s dominates final-rise level 1.2 m/s;
    // original POINTS1 uses INT(1.5*36)+1 = 55 releases, not 44 or 1.
    OriginalCompRiseMultiPuffHours native_wind_driver;
    auto native_wind_met=met;
    for(std::size_t cell=0;cell<n;++cell)
        native_wind_met.u[cell]=1.5F;  // HTSTAK=50 m, layer 1
    auto native_wind_source=source;
    native_wind_source.cadence.wind_speed_ms=0.001;
    const auto native_wind_hour=
        native_wind_driver.advance_hour_point_mxsam99_native_wsnew_unsplit(
            native_wind_met,geo,ctl,pg,false,native_wind_source,0.5F);
    need(native_wind_hour.release.puff_count==55 &&
         native_wind_hour.active_puffs==55 &&
         native_wind_hour.steps.size()==55U*56U/2U &&
         native_wind_driver.completed_hours()==1,
         "native RLSMET stack/HEFF max not connected to MXSAM99 release");
    for(const auto& step:native_wind_hour.steps)
        need(step.original_plan.sampling_steps==55 &&
             step.release_number>=1 &&
             step.ksam>=step.release_number,
             "MXSAM99 native WSNEW did not reach original NSAM selection");
    // The second hour selects a fresh release speed from its OWN MET
    // profile (stack 1.0 m/s, HEFF 1.2 m/s), while independently
    // advancing the 55 persisted old native puffs from hour one.
    auto second_native_met=native_wind_met;
    for(std::size_t cell=0;cell<n;++cell)
        second_native_met.u[cell]=1.0F;
    const auto second_native_hour=
        native_wind_driver.advance_hour_point_mxsam99_native_wsnew_unsplit(
            second_native_met,geo,ctl,pg,false,native_wind_source,0.5F);
    need(second_native_hour.release.puff_count==44 &&
         second_native_hour.active_puffs==99 &&
         native_wind_driver.completed_hours()==2,
         "hour-two fresh native WSNEW or cross-hour puff state diverged");
    std::size_t carried_old_steps=0;
    for(const auto& s:second_native_hour.steps)
        if(s.release_number==0)++carried_old_steps;
    need(carried_old_steps>=55U,
         "hour-two native WSNEW path dropped persisted old puff substeps");

    // Missing original meteorology must fail before any hour is committed.
    auto invalid_met=native_wind_met;
    invalid_met.u.clear();
    bool rejected=false;
    try {
        (void)native_wind_driver.advance_hour_point_mxsam99_native_wsnew_unsplit(
            invalid_met,geo,ctl,pg,false,native_wind_source,0.5F);
    } catch(const std::exception&) { rejected=true; }
    need(rejected && native_wind_driver.completed_hours()==2 &&
         native_wind_driver.active_count()==99,
         "native WSNEW failure partially committed a COMP hour");

    // One original-style POINTS1 transaction: compact two inactive old
    // slot episodes by vendor ROLLDN COPY order, then source-derived WSNEW
    // and MXSAM99 execute. Previously separated compaction would commit
    // retirement even if the subsequent hour later aborted.
    OriginalCompRiseMultiPuffHours packed_driver;
    const auto initial_packed=
        packed_driver.advance_hour_point_mxsam99_native_wsnew_unsplit(
            native_wind_met,geo,ctl,pg,false,native_wind_source,0.5F);
    need(initial_packed.active_puffs==55,
         "atomic ROLLDN setup lacks 55 old puff slots");
    const auto packed=
        packed_driver.advance_hour_point_mxsam99_native_wsnew_unsplit_rolldn(
            second_native_met,geo,ctl,pg,false,native_wind_source,
            0.5F,{2,4});
    const auto& order=packed_driver.canonical_ids();
    need(packed.release.puff_count==44 && packed.active_puffs==97 &&
         packed_driver.completed_hours()==2 && order.size()==97 &&
         order.at(0)==1 && order.at(1)==55 && order.at(2)==3 &&
         order.at(3)==54 && order.at(52)==53 && order.at(53)==56 &&
         order.back()==99,
         "vendor ROLLDN COPY slot order was not preserved into native COMP");
    bool retired2=false,retired4=false;
    try{(void)packed_driver.motion(2);}catch(const std::out_of_range&){retired2=true;}
    try{(void)packed_driver.vertical(4);}catch(const std::out_of_range&){retired4=true;}
    need(retired2 && retired4 && packed_driver.puff_steps(55)>0,
         "retired slots remain alive or ROLLDN copied state lost history");
    const auto prior=packed_driver.motion(1);
    const auto active_before=packed_driver.canonical_ids();
    auto broken_met=second_native_met;
    broken_met.u.clear();
    bool rollback=false;
    try {
        (void)packed_driver.advance_hour_point_mxsam99_native_wsnew_unsplit_rolldn(
            broken_met,geo,ctl,pg,false,native_wind_source,0.5F,{1});
    }catch(const std::exception&){rollback=true;}
    need(rollback && packed_driver.completed_hours()==2 &&
         packed_driver.active_count()==97 &&
         packed_driver.canonical_ids()==active_before &&
         packed_driver.motion(1).travel_time_s==prior.travel_time_s,
         "failing COMP hour retained a partial ROLLDN retirement");
    std::cout<<"P7_ROLLDN_NATIVE_MXSAM99_ATOMIC=PASS"
             <<" OLD_COPIES=2 NATIVE_NEW=44 ACTIVE=97"
             <<" VENDOR_SLOT_ORDER=YES ABORT_ROLLBACK=YES"
             <<" AUTONOMOUS_OFFDOMAIN=NOT_YET_ACCEPTED\\n";

    // Force a physically meaningful FSTEP > 1: only one newly released
    // puff (externally controlled release speed 0.001 m/s), but its actual
    // 3-D METFM1 wind is 1.2 m/s. Unlike the existing unsplit branch, the
    // new scheduler must shorten TSAMP, re-evaluate rising RISEWIND, and
    // use the original TLEFT/goto-500 loop without discarding puff state.
    auto fast=source;
    fast.cadence.wind_speed_ms=0.001;
    fast.cadence.emission_interval=36.0;
    OriginalCompRiseMultiPuffHours refused_split;
    bool fstep_refusal=false;
    try{(void)refused_split.advance_hour_point_mxsam99_unsplit(
        met,geo,ctl,pg,false,fast);}catch(const std::invalid_argument&){
        fstep_refusal=true;
    }
    need(fstep_refusal && refused_split.completed_hours()==0 &&
         refused_split.active_count()==0,
         "legacy unsplit COMP silently accepted forced FSTEP");
    OriginalCompRiseMultiPuffHours adaptive;
    const auto split_hour=adaptive.advance_hour_point_mxsam99_adaptive(
        met,geo,ctl,pg,false,fast);
    need(split_hour.release.puff_count==1 &&
         split_hour.active_puffs==1 &&
         split_hour.fstep_shortened_steps>0 &&
         split_hour.fstep_reentries>0 &&
         split_hour.steps.size()>1 &&
         adaptive.completed_hours()==1,
         "native FSTEP did not execute source TLEFT/substep reentries");
    double used_time=0.0;
    bool rising_resample=false;
    for(const auto& rec:split_hour.steps){
        need(rec.puff_id==1 && rec.ksam==1 &&
             rec.step.after.travel_time_s>rec.step.before.travel_time_s &&
             rec.step.vertical_after.committed_lid_steps>0 &&
             rec.step.substep.step_s>0.0,
             "native FSTEP resampling corrupted puff clock or lid");
        used_time+=rec.step.substep.step_s;
        if(rec.step.before.total_distance_m<230.0F &&
           rec.step.substep.split && rec.step.rise.adjusted.passes==2)
            rising_resample=true;
    }
    need(rising_resample &&
         std::abs(adaptive.motion(1).travel_time_s-3600.0F)<1.0F &&
         std::abs(used_time-3600.0)<1.0 &&
         adaptive.puff_steps(1)==split_hour.steps.size(),
         "native FSTEP lost full-hour time or rising puff state");
    const auto mass_state=adaptive.vertical(1);
    need(std::abs(mass_state.mass_mixed.at(0)+
                  mass_state.mass_above.at(0)-7200.0F)<0.1F &&
         std::abs(mass_state.mass_mixed.at(1)+
                  mass_state.mass_above.at(1)-10800.0F)<0.1F,
         "native FSTEP introduced source-mass drift");
    const auto saved=adaptive.motion(1);
    auto invalid_admission=fast;
    invalid_admission.point.source_x_km=130;
    bool adaptive_rollback=false;
    try{(void)adaptive.advance_hour_point_mxsam99_adaptive(
        met,geo,ctl,pg,false,invalid_admission);
    }catch(const std::exception&){adaptive_rollback=true;}
    need(adaptive_rollback && adaptive.completed_hours()==1 &&
         adaptive.active_count()==1 &&
         adaptive.motion(1).travel_time_s==saved.travel_time_s &&
         adaptive.motion(1).x_cells==saved.x_cells,
         "late FSTEP+POINTS1 error corrupted an earlier physical hour");
    std::cout<<"P7_NATIVE_FSTEP_TLEFT_REENTRY=PASS"
             <<" SOURCE_KSAM=1 RESAMPLE_RISEWIND=YES"
             <<" FULL_HOUR_MOTION=YES MASS_CONSERVED=YES"
             <<" ATOMIC_ROLLBACK=YES"
             <<" ORIGINAL_FROZEN_143H_NOT_YET_ACCEPTED=YES\\n";

    const auto state=driver.motion(1);
    const auto count=driver.completed_hours();
    auto bad=source;bad.point.source_x_km=130;
    bool fail=false;try{(void)driver.advance_hour_point_mxsam99_unsplit(
        met,geo,ctl,pg,false,bad);}catch(const std::exception&){fail=true;}
    need(fail && driver.completed_hours()==count &&
         driver.motion(1).travel_time_s==state.travel_time_s &&
         driver.active_count()==88,
         "late source failure partially committed the MXSAM99 hour");
    std::cout<<"P7_NATIVE_COMP_MXSAM99_UNSPLIT=PASS"
             <<" NEW_FIRST_HOUR_STEPS="<<a.steps.size()
             <<" OLD_SECOND_HOUR_STEPS="<<old_steps
             <<" SOURCE_VMASS=YES"
             <<" NATIVE_RLSMET_WSNEW_MXSAM99=YES"
             <<" FSTEP_SPLIT=NOT_YET_IMPLEMENTED"
             <<" ORIGINAL_HEFEI_143H=NOT_YET_ACCEPTED\n";
}
}
int main(){try{test();return 0;}catch(const std::exception& e){
    std::cerr<<"P7_NATIVE_COMP_MXSAM99_UNSPLIT=FAIL "<<e.what()<<"\n";return 1;}}
