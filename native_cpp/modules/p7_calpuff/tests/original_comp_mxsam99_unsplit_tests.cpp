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
    // Missing original meteorology must fail before any hour is committed.
    auto invalid_met=native_wind_met;
    invalid_met.u.clear();
    bool rejected=false;
    try {
        (void)native_wind_driver.advance_hour_point_mxsam99_native_wsnew_unsplit(
            invalid_met,geo,ctl,pg,false,native_wind_source,0.5F);
    } catch(const std::exception&) { rejected=true; }
    need(rejected && native_wind_driver.completed_hours()==1 &&
         native_wind_driver.active_count()==55,
         "native WSNEW failure partially committed a COMP hour");

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
