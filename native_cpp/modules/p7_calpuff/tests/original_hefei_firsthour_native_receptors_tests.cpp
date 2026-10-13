// Frozen Hefei 2022 first-hour native H-3 concentration diagnostic.
// Original puff.con is a post-run comparator ONLY: it is never fed to
// POINTS1, COMP, DRY, PUFRECS or CALCPF.
#include "cteas/puff/conc_file.hpp"
#include "cteas/puff/control.hpp"
#include "cteas/puff/dry_deposition.hpp"
#include "cteas/puff/original_comp_rise_hourly.hpp"
#include "cteas/puff/original_flat_calcpf.hpp"
#include "cteas/puff/original_met_hours.hpp"
#include "cteas/puff/original_native_terrain.hpp"
#include "cteas/puff/receptors.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cteas::puff;
namespace {
void need(bool condition,const char* why) {
    if(!condition)throw std::runtime_error(why);
}
std::string control_bytes(const std::string& path) {
    std::ifstream file(path,std::ios::binary);
    need(bool(file),"frozen 2022 puff.inp unavailable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    need(!text.empty(),"empty frozen puff.inp");
    return text;
}
}

int main(int argc,char** argv) {
  try {
    need(argc==4,"usage: p7_hefei_real_h3 MET.DAT puff.inp puff.con");
    const auto control=control_bytes(argv[2]);
    const auto deck=ControlFile::read_file(argv[2]);
    need(deck.required_int("METFM")==1 &&
         deck.required_int("NPT1")==1 &&
         deck.required_int("NSPEC")==2 &&
         deck.required_int("MCTADJ")==2 &&
         deck.required_int("MDRY")==1 &&
         deck.required_int("MTRANS")==1 &&
         deck.required_int("MXSAM")==99,
         "unexpected frozen source/meteorology/terrain/dry scenario");
    // Read immutable reference once for DIAGNOSTIC comparison, never to
    // supply source parameters, winds, mass, positions or gradients.
    const auto original=read_conc_species_fields(argv[3],50,50);
    need(original.size()>=143 && original.front().species_index==1 &&
         original.front().label.find("H-3")!=std::string::npos &&
         original.front().values.size()==2500,
         "frozen original puff.con first H-3 field unavailable");
    const auto stack=deck.required_float_list("X");
    const auto factors=deck.required_float_list("FMFAC");
    const auto ymin=deck.required_float_list("SYMIN");
    const auto zmin=deck.required_float_list("SZMIN");
    const auto lengths=deck.required_float_list("XSAMLEN");
    const auto cdiv=deck.required_float_list("CDIV");
    need(stack.size()==10 && factors.size()==1 &&
         ymin.size()==1 && zmin.size()==1 &&
         lengths.size()==1 && cdiv.size()==2,
         "frozen original point and COMP controls missing");

    bool evaluated=false;
    const auto summary=OriginalMetHourlyReader::read(argv[1],
      [&](const Calmet21Header& header,const Calmet21Static& static_met,
          const OriginalMetHour& met) {
        if(evaluated)return;
        evaluated=true;
        need(met.begin.year==2022 && met.begin.julian_day==1 &&
             met.begin.hour==0 &&
             header.nx==50 && header.ny==50 &&
             header.nz==9 && header.dx_m==100.0F,
             "not immutable first-hour 50x50x9 original MET");
        OriginalPointPrfinRequest request;
        request.x_cells=static_cast<float>(
            (static_cast<float>(stack[0])*1000.0F-header.origin_x_m)/
             header.dx_m);
        request.y_cells=static_cast<float>(
            (static_cast<float>(stack[1])*1000.0F-header.origin_y_m)/
             header.dx_m);
        request.stack_height_m=static_cast<float>(stack[2]);
        request.diameter_m=static_cast<float>(stack[4]);
        request.exit_velocity_ms=static_cast<float>(stack[5]);
        request.stack_temperature_k=static_cast<float>(stack[6]);
        request.momentum_flux_factor=static_cast<float>(factors[0]);
        request.calm_threshold_ms=0.5F;
        request.max_sample_distance_cells=static_cast<float>(lengths[0]);
        request.max_new_puffs=deck.required_int("MXNEW");
        const auto prfin=original_point_prfin_metfm1(
            met,static_met,header,request);
        OriginalPointNativeSourceParameters native_source;
        native_source.base_elevation_m=static_cast<float>(stack[3]);
        native_source.met_spacing_m=header.dx_m;
        native_source.initial_sigma_y_m=static_cast<float>(ymin[0]);
        native_source.initial_sigma_z_m=static_cast<float>(zmin[0]);
        native_source.emission_rates={stack[8],stack[9]};
        const auto source=original_point_hour_from_prfin(
            request,prfin,native_source);
        need(source.mass.species_count==2 &&
             source.mass.emission_rate.at(0)==834000 &&
             source.mass.emission_rate.at(1)==0 &&
             prfin.release.puff_count==54,
             "first-hour actual source emission/release changed");

        NeutralCompControls controls;
        controls.met_spacing_m=header.dx_m;
        controls.max_sample_distance_cells=
            static_cast<float>(lengths[0]);
        controls.calm_threshold_ms=0.5F;
        controls.max_sampling_steps=99;
        controls.original_wdiv=NativeOriginalWdivControls{
            static_cast<float>(cdiv[0]),static_cast<float>(cdiv[1])};
        const auto terrain=OriginalNativeTerrain::from_calmet(
            static_met,met.nx,met.ny,header.dx_m);
        NativeMctadj2Controls terrain_control;
        terrain_control.terrain=terrain.constraint_lookup();
        terrain_control.derive_bvf_from_metfm1=true;
        terrain_control.minimum_pt_gradient_k_per_m=0.005F;
        controls.mctadj2=terrain_control;
        controls.gaussian_dry=NativeGaussianDryControls{
            dry_deposition_setup_from_control(control),1.0F};
        DispersionSetup pg;
        pg.mdisp=3;
        pg.averaging_factor=1;
        pg.sigma_y_heffter_m=550;
        pg.heffter_sigma_z=false;
        pg.roughness_adjustment=false;

        const ReceptorSet receptors=receptors_from_original_met_grid(
            control,{header.origin_x_m,header.origin_y_m,header.dx_m});
        need(receptors.grid.size()==2500 &&
             receptors.sampling_nx==50 && receptors.sampling_ny==50 &&
             receptors.ibsamp==1 && receptors.jbsamp==1 &&
             receptors.mesh_down==1 && receptors.discrete.empty(),
             "original selected 2500-grid receptor geometry drifted");
        const auto receptor_sampler=make_original_flat_calcpf_sampler(
            receptors,header.dx_m,3600.0F,3.0F,
            OriginalFlatVerticalMethod::receptor_pg_vcoup_mctadj2,
            [&terrain,&receptors](const RiseCompSamplingState& state,
                std::size_t receiver,const OriginalPuffct2Result& position){
                need(state.original_ctadj2_step.has_value() &&
                     state.receptor_effective_wind_ms>0.0F &&
                     std::isfinite(state.receptor_brunt_vaisala_s),
                     "receptor METFM1 BVF/WS not sourced from live COMP");
                return terrain.receptor(
                    receiver,receptors.ibsamp,receptors.jbsamp,
                    receptors.sampling_nx,receptors.sampling_ny,position,
                    state.receptor_brunt_vaisala_s,
                    state.receptor_effective_wind_ms);
            },true);
        OriginalCompRiseMultiPuffHours driver;
        const OriginalCompPuffBounds bounds{0.0F,50.0F,0.0F,50.0F};
        // Capacity is deliberately nonbinding for the first hour. The
        // original MXPUFF/ROLLDN pressure chronology is a later gate.
        const auto hour=
            driver.advance_hour_point_mxsam99_adaptive_bounded_capacity_sampled(
                met,static_met,controls,pg,false,source,bounds,
                4096U,2500U,receptor_sampler);
        need(hour.steps.size()==1275U && hour.release.puff_count==54 &&
             hour.receptor_concentrations.size()==2500U &&
             driver.completed_hours()==1U,
             "real first-hour native 1275-step/2500-receptor integration failed");
        std::size_t dried=0,receiver_nonzero=0;
        double dry_mass=0.0,max_abs_diff=0.0,sum_abs_diff=0.0;
        double native_total=0.0,reference_total=0.0;
        std::size_t max_diff_cell=0,bitwise_equal=0,reference_nonzero=0;
        struct ReceptorDifference {std::size_t cell;float native,reference;double error;};
        std::vector<ReceptorDifference> mismatches;
        for(const auto& step:hour.steps) {
            need(step.step.dry.has_value() &&
                 step.step.dry->removed_mass.size()==2 &&
                 step.step.dry->removed_mass[1]==0.0F,
                 "H-3/HE-3 physical original DRY species handling failed");
            const float r=step.step.dry->removed_mass[0];
            need(std::isfinite(r)&&r>=0.0F,
                 "original H-3 dry deposition became invalid");
            if(r>0)++dried;
            dry_mass+=r;
        }
        const auto& expected=original.front().values;
        for(std::size_t i=0;i<2500U;++i){
            const float observed=hour.receptor_concentrations[i];
            const float reference=expected[i];
            need(std::isfinite(observed)&&observed>=0.0F &&
                 std::isfinite(reference)&&reference>=0.0F,
                 "original/native H-3 concentration contains invalid value");
            receiver_nonzero+=observed>0.0F?1U:0U;
            reference_nonzero+=reference>0.0F?1U:0U;
            native_total+=observed;
            reference_total+=reference;
            if(std::bit_cast<std::uint32_t>(observed)==
               std::bit_cast<std::uint32_t>(reference))++bitwise_equal;
            else mismatches.push_back({i,observed,reference,
                static_cast<double>(observed)-static_cast<double>(reference)});
            const double diff=std::abs(
                static_cast<double>(observed)-static_cast<double>(reference));
            sum_abs_diff+=diff;
            if(diff>max_abs_diff){max_abs_diff=diff;max_diff_cell=i;}
        }
        std::sort(mismatches.begin(),mismatches.end(),
            [](const auto& left,const auto& right){
                return std::abs(left.error)>std::abs(right.error);
            });
        const auto top=std::min<std::size_t>(mismatches.size(),12U);
        for(std::size_t k=0;k<top;++k){
            const auto& d=mismatches[k];
            std::cout<<std::setprecision(9)
                     <<"P7_HEFEI_H3_FIRSTHOUR_RECEPTOR_DIFF"
                     <<" RANK="<<k+1<<" INDEX0="<<d.cell
                     <<" GRID_I="<<d.cell%50U+1U
                     <<" GRID_J="<<d.cell/50U+1U
                     <<" NATIVE_BQ_M3="<<d.native
                     <<" ORIGINAL_BQ_M3="<<d.reference
                     <<" SIGNED_DIFFERENCE="<<d.error
                     <<" RELATIVE_TO_ORIGINAL="<<
                     (d.reference!=0.0F ?
                       d.error/static_cast<double>(d.reference):0.0)
                     <<" ORIGINAL_ZERO="<<(d.reference==0.0F?"YES":"NO")
                     <<"\n";
        }
        need(dried>0 && dry_mass>0,
             "real first-hour source-ordered MDRY=1 never removed H-3 mass");
        need(receiver_nonzero>0,
             "real first-hour PUFRECS/CALCPF produced all-zero H-3");
        std::cout<<std::setprecision(9)
                 <<"P7_HEFEI_REAL_FIRSTHOUR_NATIVE_H3_RECEPTORS=PASS"
                 <<" COMP_SUBSTEPS="<<hour.steps.size()
                 <<" GRID_CELLS="<<hour.receptor_concentrations.size()
                 <<" NONZERO_NATIVE="<<receiver_nonzero
                 <<" NONZERO_REFERENCE="<<reference_nonzero
                 <<" POSITIVE_DRY_SUBSTEPS="<<dried
                 <<" DRY_REMOVED_BQ="<<dry_mass
                 <<" REFERENCE_BITWISE_MATCH="<<bitwise_equal<<"/2500"
                 <<" REFERENCE_MEAN_ABS_ERROR="<<sum_abs_diff/2500.0
                 <<" REFERENCE_MAX_ABS_ERROR="<<max_abs_diff
                 <<" REFERENCE_MAX_ERROR_CELL="<<max_diff_cell
                 <<" NATIVE_FIELD_SUM_BQ_M3="<<native_total
                 <<" REFERENCE_FIELD_SUM_BQ_M3="<<reference_total
                 <<" CONCENTRATION_MISMATCH_CELLS="<<mismatches.size()
                 <<" REFERENCE_SCIENTIFIC_PARITY=OPEN"
                 <<" 6H_143H=NOT_YET_ACCEPTED\n";
      },true);
    need(evaluated && summary.intervals==143,
         "full frozen MET provenance 143 hours is required");
    return 0;
  }catch(const std::exception& e){
    std::cerr<<"P7_HEFEI_REAL_FIRSTHOUR_NATIVE_H3_RECEPTORS=FAIL "
             <<e.what()<<"\n";
    return 1;
  }
}
