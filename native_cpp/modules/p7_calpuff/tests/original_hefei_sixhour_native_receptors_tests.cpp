// Original-CALPUFF six consecutive real Hefei MET hours: live native COMP,
// capacity inventory, and CALCPF at original REAL4 precision. No native state
// is seeded from vendor output or original DGM. Original puff.con is read
// exclusively AFTER each native hour has been independently advanced.
#include "cteas/puff/conc_file.hpp"
#include "cteas/puff/control.hpp"
#include "cteas/puff/dry_deposition.hpp"
#include "cteas/puff/original_comp_rise_hourly.hpp"
#include "cteas/puff/original_flat_calcpf.hpp"
#include "cteas/puff/original_met_hours.hpp"
#include "cteas/puff/original_met_cell.hpp"
#include "cteas/puff/original_native_terrain.hpp"
#include "cteas/puff/receptors.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cteas::puff;
namespace {
void require(bool good,const char* message){
    if(!good)throw std::runtime_error(message);
}
std::string bytes(const char* path) {
    std::ifstream file(path,std::ios::binary);
    require(bool(file),"frozen source file missing");
    return {std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()};
}
bool equal_real4(float a,float b) {
    return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);
}
} // namespace

int main(int argc,char** argv) {
  try {
    require(argc==4,"usage: sixhour_real_h3 MET.DAT puff.inp puff.con");
    // Source order: strict GETCELL for pre-step puff centre, original
    // GRIDIJ clamping for the mid/end meteorological cell after movement.
    // A boundary crossing is sampled and retired by COMP, not aborted by
    // an inappropriately strict midpoint lookup.
    const auto mid=original_met_cell_clamped(18.251936F,50.00001F,50,50);
    require(mid.ix_1based==19 && mid.iy_1based==50 &&
            mid.offset_0based==2468U,
            "CALPUFF GETCELL outer-edge midpoint mapped to wrong REAL4 cell");
    bool strict_rejects=false;
    try {
        (void)original_met_cell(18.251936F,50.00001F,50,50);
    } catch(const std::out_of_range&) {strict_rejects=true;}
    require(strict_rejects,
            "original COMP start cell unexpectedly accepts out-of-grid puffs");
    const auto control=bytes(argv[2]);
    const auto deck=ControlFile::read_file(argv[2]);
    require(deck.required_int("METFM")==1 &&
            deck.required_int("NPT1")==1 &&
            deck.required_int("NSPEC")==2 &&
            deck.required_int("MCTADJ")==2 &&
            deck.required_int("MDRY")==1 &&
            deck.required_int("MTRANS")==1 &&
            deck.required_int("MXSAM")==99,
            "unapproved frozen 2022 CALPUFF controls");
    const auto original=read_conc_species_fields(argv[3],50,50);
    // Original WROUT1 emits 143 H-3 concentration fields. Its 286
    // period/grid descriptors are NOT 286 species fields; HE-3 is a
    // modeled species in NSPEC=2 but has no concentration OUTPUT record.
    require(original.size()==143U &&
            original.front().species_index==1 &&
            original.front().label.find("H-3")!=std::string::npos,
            "original 143-hour H3/HE3 concentration oracle changed");
    const auto stack=deck.required_float_list("X");
    const auto factors=deck.required_float_list("FMFAC");
    const auto ymin=deck.required_float_list("SYMIN");
    const auto zmin=deck.required_float_list("SZMIN");
    const auto lengths=deck.required_float_list("XSAMLEN");
    const auto cdiv=deck.required_float_list("CDIV");
    require(stack.size()==10U&&factors.size()==1U&&ymin.size()==1U&&
            zmin.size()==1U&&lengths.size()==1U&&cdiv.size()==2U,
            "original point-source or dispersion source fields missing");

    // ONE instance owns all six meteorological hours and all puff state.
    OriginalCompRiseMultiPuffHours driver;
    // Pinned write-only original Fortran DGM metadata, verified against
    // dgm_puffstep.json SHA cbbdaa29c6222e6ca6aefe9f605ad13e7dd6d258f6ad243afcac07cb578108c6.
    // These values validate scheduling AFTER native execution; they never
    // supply puff states, winds, positions, mass or source parameters.
    constexpr std::array<std::size_t,6> original_hour_steps{
        1275U,2242U,1906U,2132U,1742U,2161U};
    constexpr std::array<int,6> original_first_puff{
        1,22,113,183,247,314};
    constexpr std::array<int,6> original_last_puff{
        54,137,208,272,337,417};
    std::size_t completed=0U,matching_words=0U,physical_steps=0U;
    bool every_hour_real4=true;
    std::size_t first_mismatched_hour=0U,first_mismatched_cell=0U;
    const auto summary=OriginalMetHourlyReader::read(argv[1],
       [&](const Calmet21Header& header,const Calmet21Static& static_met,
           const OriginalMetHour& met) {
        if(completed>=6U)return;
        const std::size_t hour_index=completed;
        require(met.begin.year==2022 && met.begin.julian_day==1 &&
                met.begin.hour==static_cast<int>(hour_index) &&
                header.nx==50&&header.ny==50&&header.nz==9&&
                header.dx_m==100.0F,
                "Hefei consecutive original MET time/grid drift");
        const auto active_before=driver.canonical_ids();
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
        require(source.mass.species_count==2 &&
                source.mass.emission_rate[0]==834000.0F &&
                source.mass.emission_rate[1]==0.0F,
                "Hefei native source emission parameters changed");

        NeutralCompControls controls;
        controls.met_spacing_m=header.dx_m;
        controls.max_sample_distance_cells=static_cast<float>(lengths[0]);
        controls.calm_threshold_ms=0.5F;
        controls.max_sampling_steps=99;
        controls.original_wdiv=NativeOriginalWdivControls{
            static_cast<float>(cdiv[0]),static_cast<float>(cdiv[1])};
        const auto terrain=OriginalNativeTerrain::from_calmet(
            static_met,met.nx,met.ny,header.dx_m);
        NativeMctadj2Controls mctadj2;
        mctadj2.terrain=terrain.constraint_lookup();
        mctadj2.derive_bvf_from_metfm1=true;
        mctadj2.minimum_pt_gradient_k_per_m=0.005F;
        controls.mctadj2=mctadj2;
        controls.gaussian_dry=NativeGaussianDryControls{
            dry_deposition_setup_from_control(control),1.0F};
        DispersionSetup pg;
        pg.mdisp=3;
        pg.averaging_factor=1;
        pg.sigma_y_heffter_m=550;
        pg.heffter_sigma_z=false;
        pg.roughness_adjustment=false;
        const auto receptors=receptors_from_original_met_grid(
            control,{header.origin_x_m,header.origin_y_m,header.dx_m});
        require(receptors.grid.size()==2500U&&receptors.sampling_nx==50&&
                receptors.sampling_ny==50&&receptors.mesh_down==1&&
                receptors.discrete.empty(),"sixhour native receptor frame drift");

        const auto sampling=make_original_flat_calcpf_sampler(
            receptors,header.dx_m,3600.0F,3.0F,
            OriginalFlatVerticalMethod::receptor_pg_vcoup_mctadj2,
            [&terrain,&receptors](const RiseCompSamplingState& state,
                std::size_t receiver,const OriginalPuffct2Result& pos){
                require(state.original_ctadj2_step.has_value() &&
                        state.receptor_effective_wind_ms>0.0F &&
                        std::isfinite(state.receptor_brunt_vaisala_s),
                        "Hefei receptor terrain state is not native");
                return terrain.receptor(
                    receiver,receptors.ibsamp,receptors.jbsamp,
                    receptors.sampling_nx,receptors.sampling_ny,pos,
                    state.receptor_brunt_vaisala_s,
                    state.receptor_effective_wind_ms);
            },true,{},0.0F);
        // 4096 capacity is intentionally non-binding in this SIX-HOUR
        // instrumented gate. MXPUFF-pressure acceptance is a distinct test.
        const OriginalCompPuffBounds bounds{0.0F,50.0F,0.0F,50.0F};
        const auto hour=driver.advance_hour_point_mxsam99_adaptive_bounded_capacity_sampled(
            met,static_met,controls,pg,false,source,bounds,
            4096U,2500U,sampling);
        require(driver.completed_hours()==hour_index+1U &&
                hour.hour_index==hour_index &&
                hour.receptor_concentrations.size()==2500U &&
                hour.steps.size()>0U,
                "real sixhour COMP hour transaction/cadence invalid");
        require(hour.steps.size()==original_hour_steps[hour_index] &&
                hour.steps.front().puff_id==original_first_puff[hour_index] &&
                hour.steps.back().puff_id==original_last_puff[hour_index],
                "real COMP old/new puff chronology differs from pinned original DGM");
        physical_steps+=hour.steps.size();
        if(hour_index==0U)
            require(hour.steps.size()==1275U &&
                    hour.release.puff_count==54 &&
                    active_before.empty(),
                    "first real original COMP hour was not preserved");

        // Read-only oracle: exactly one gridded H-3 field per hour;
        // HE-3 is carried in native species mass, not emitted as puff.con.
        const auto& reference=original.at(hour_index);
        require(reference.species_index==1 &&
                reference.label.find("H-3")!=std::string::npos &&
                reference.values.size()==2500U,
                "original H3/HE3 hourly record order or field size changed");
        std::size_t matches=0U,nonzero=0U,first_cell=2500U;
        double max_abs=0.0,sum_abs=0.0,native_sum=0.0,original_sum=0.0;
        for(std::size_t cell=0;cell<2500U;++cell) {
            const float native=hour.receptor_concentrations[cell];
            const float expected=reference.values[cell];
            require(std::isfinite(native)&&std::isfinite(expected)&&
                    native>=0.0F&&expected>=0.0F,
                    "invalid hour concentration/mass, source precision drift");
            native_sum+=native;
            original_sum+=expected;
            nonzero+=native>0.0F ? 1U:0U;
            if(equal_real4(native,expected))++matches;
            else if(first_cell==2500U)first_cell=cell;
            const double delta=std::abs(
                static_cast<double>(native)-static_cast<double>(expected));
            sum_abs+=delta;
            max_abs=std::max(max_abs,delta);
        }
        matching_words+=matches;
        if(matches!=2500U) {
            every_hour_real4=false;
            if(first_mismatched_hour==0U) {
                first_mismatched_hour=hour_index+1U;
                first_mismatched_cell=first_cell;
            }
        }
        // Verify native historical episode count and canonical stable IDs.
        require(driver.capacity_inventory().active_ids()==
                driver.canonical_ids() &&
                driver.active_count()==driver.canonical_ids().size(),
                "historical COMP active IDs diverged from capacity slots");
        require(std::is_sorted(driver.canonical_ids().begin(),
                               driver.canonical_ids().end()),
                "historical COMP puff chronology no longer canonical");
        std::cout<<std::setprecision(10)
                 <<"P7_HEFEI_6H_NATIVE_ORIGINAL_HOURLY"
                 <<" HOUR="<<hour_index+1U
                 <<" OLD_PUFFS="<<active_before.size()
                 <<" NEW_PUFFS="<<hour.release.puff_count
                 <<" ACTIVE_AFTER="<<driver.active_count()
                 <<" RETIRED="<<hour.retired.size()
                 <<" OCCUPIED_SLOTS="<<hour.occupied_slots_after
                 <<" FSTEP_SUBSTEPS="<<hour.steps.size()
                 <<" FSTEP_REENTRIES="<<hour.fstep_reentries
                 <<" REAL4_WORDS="<<matches<<"/2500"
                 <<" FIRST_DIFFERENT_CELL0="<<first_cell
                 <<" NONZERO_NATIVE="<<nonzero
                 <<" MAX_ABS_BQ_M3="<<max_abs
                 <<" MAE_BQ_M3="<<sum_abs/2500.0
                 <<" NATIVE_SUM_BQ_M3="<<native_sum
                 <<" ORIGINAL_SUM_BQ_M3="<<original_sum
                 <<" SCIENTIFIC_6H_PARITY="
                 <<(matches==2500U?"PASS_THIS_HOUR":"OPEN_THIS_HOUR")
                 <<"\n";
        require(hour_index>0U || matches==2500U,
                "first-hour bitwise source parity regressed inside sixhour COMP");
        ++completed;
       },true);
    require(summary.intervals==143U&&completed==6U&&
            driver.completed_hours()==6U&&physical_steps==11458U,
            "full 143-hour frozen MET and 6 native consecutive hours required");
    std::cout<<"P7_HEFEI_6H_COMP_ORIGINAL_SUBSTEP_SCHEDULE=PASS"
             <<" ORIGINAL_DGM_ROWS=11458/11458"
             <<" HOURLY_ROWS=1275,2242,1906,2132,1742,2161"
             <<" FIRST_AND_LAST_EPISODE_IDS=SOURCE_EQUIVALENT"
             <<" NATIVE_EPISODE_PHYSICS_VALUES=NOT_YET_ALL_BITWISE"<<"\\n";
    std::cout<<"P7_HEFEI_REAL_6H_COMPUTATION_AND_PRECISION_DIAGNOSTIC=PASS"
             <<" HOURS=6 METFM1=ORIGINAL"
             <<" COMP_AUTHORITY=SINGLE_STATEFUL_DRIVER"
             <<" REAL4_WORDS_MATCHED="<<matching_words<<"/15000"
             <<" FIRST_DIFFERENT_HOUR="<<first_mismatched_hour
             <<" FIRST_DIFFERENT_CELL0="<<first_mismatched_cell
             <<" EVERY_HOUR_BITWISE="<<(every_hour_real4?"YES":"NO")
             <<" REAL_6H_SCIENTIFIC_PARITY="
             <<(every_hour_real4?"PASS":"OPEN")
             <<" ORIGINAL_143H=NOT_ACCEPTED"
             <<" WROUT1_FULL_OUTPUT=NOT_ACCEPTED\n";
    return 0; // Diagnostic can PASS only as a diagnostic, not 6h scientific acceptance.
  }catch(const std::exception& e){
    std::cerr<<"P7_HEFEI_REAL_6H_COMPUTATION_AND_PRECISION_DIAGNOSTIC=FAIL "
             <<e.what()<<"\n";
    return 1;
  }
}
