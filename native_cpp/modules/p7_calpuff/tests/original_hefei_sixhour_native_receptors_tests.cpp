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
#include <cstdlib>
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
struct OriginalDgmStep {
    std::array<std::int32_t,24> integers{};
    std::array<float,64> reals{};
};
static_assert(sizeof(OriginalDgmStep)==352U);
static_assert(std::endian::native==std::endian::little);
std::vector<OriginalDgmStep> read_original_dgm(const std::string& path, bool full_143h) {
    std::ifstream input(path,std::ios::binary);
    require(bool(input),"original pinned sixhour DGM oracle missing");
    std::vector<OriginalDgmStep> rows;
    OriginalDgmStep row;
    while(input.read(reinterpret_cast<char*>(&row),sizeof(row)))
        rows.push_back(row);
    require(input.eof() && input.gcount()==0 &&
                rows.size()==(full_143h?254687U:11458U),
            "original frozen DGM row count or record length drifted");
    return rows;
}
bool equal_real4(float a,float b) {
    return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);
}
} // namespace

int main(int argc,char** argv) {
  try {
    require(argc>=4 && argc<=8,
            "usage: sixhour_real_h3 MET.DAT puff.inp puff.con [original-DGM.bin] [hour2-25-28.tsv] [hour4-47-13.tsv] [native-puff200-mass.tsv]");
    require(argc<6 || std::string(argv[4]).size()>0U,
            "hour2 CALCPF trace requires an original DGM provenance input");
    // The accepted six-hour strict gate remains the DEFAULT. Longer
    // source-first probes are explicit opt-ins, never silent acceptance.
    const char* extended=std::getenv("CTEAS_P7_PROBE_HOURS");
    std::size_t probe_hours=6U;
    if(extended){
        char* end=nullptr;
        const long requested=std::strtol(extended,&end,10);
        require(end && end!=extended && *end=='\0' &&
                requested>=7L && requested<=143L,
                "CTEAS_P7_PROBE_HOURS must be a whole number in 7..143");
        probe_hours=static_cast<std::size_t>(requested);
    }
    // Optional native stage trace (argv[5]) must never disable the
    // independent original DGM step oracle carried in argv[4].
    require(probe_hours==6U || argc>=5,
            "extended source probe requires the FULL original 143h DGM file");
    const auto dgm=argc>=5?read_original_dgm(argv[4],probe_hours>6U):
                           std::vector<OriginalDgmStep>{};
    require(argc!=6 || !dgm.empty(),
            "hour2 source trace must retain pinned original sixhour DGM gate");
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
    std::size_t matched_step_keys=0U,matched_new_xy=0U;
    std::size_t matched_stored_sigy=0U,matched_stored_sigz=0U;
    std::size_t first_dgm_mismatch_record=0U;
    std::string first_dgm_mismatch_field;
    struct NativeH3ReceptorRow {
        int episode=0;
        float time_before=0.0F,time_after=0.0F;
        OriginalCalcpfReceiverIntermediate values;
    };
    std::size_t original_hour2_trace_rows=0U;
    bool every_hour_real4=true;
    std::size_t first_mismatched_hour=0U,first_mismatched_cell=0U;
    const auto summary=OriginalMetHourlyReader::read(argv[1],
       [&](const Calmet21Header& header,const Calmet21Static& static_met,
           const OriginalMetHour& met) {
        if(completed>=probe_hours)return;
        const std::size_t hour_index=completed;
        require(met.begin.year==2022 && met.begin.julian_day==1 &&
                met.begin.hour==static_cast<int>(hour_index%24U) &&
                met.begin.julian_day==1+static_cast<int>(hour_index/24U) &&
                header.nx==50&&header.ny==50&&header.nz==9&&
                header.dx_m==100.0F,
                "Hefei consecutive original MET time/grid drift");
        if(hour_index>=6U) {
            require(matching_words==2500U*hour_index &&
                    first_mismatched_hour==0U &&
                    matched_step_keys==physical_steps &&
                    matched_new_xy==physical_steps &&
                    matched_stored_sigy==physical_steps &&
                    matched_stored_sigz==physical_steps,
                    "extended probe cannot bypass preceding contiguous REAL4 gate");
            std::cout<<"P7_REAL_EXTENDED_PROBE_ENTER_HOUR="<<hour_index+1U
                     <<" ACTIVE_BEFORE="<<driver.active_count()
                     <<" ORIGINAL_FULL_DGM_ROWS="<<dgm.size()<<"\n";
        }
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
        // Read-only first unsupported POINTS1 source admission at real H9.
        // Report the native original-scoped PRFIN branch's exact physics
        // rather than relaxing it or seeding it from the vendor DGM.
        if(hour_index>=8U) {
            const auto src_cell=original_met_cell(
                request.x_cells,request.y_cells,met.nx,met.ny);
            std::cout<<std::setprecision(10)
                     <<"P7_REAL_EXTENDED_PRFIN_INPUT"
                     <<" HOUR="<<hour_index+1U
                     <<" SRC_CELL0="<<src_cell.offset_0based
                     <<" PGT="<<met.ipgt.at(src_cell.offset_0based)
                     <<" TEMP="<<prfin.ambient_temperature_k
                     <<" PBL="<<prfin.mixing_height_m
                     <<" STACK_WIND="<<prfin.stack_wind_ms
                     <<" HEFF_WIND="<<prfin.effective_wind_ms
                     <<" RELEASE_WIND="<<prfin.release_wind_ms
                     <<" HEFF="<<prfin.effective_height_m
                     <<" STACK_HEIGHT="<<request.stack_height_m
                     <<" XFINAL="<<prfin.x_final_m
                     <<" RISE="<<prfin.final_rise_m
                     <<" BUOYANCY="<<prfin.buoyancy_flux
                     <<" MOMENTUM="<<prfin.momentum_flux
                     <<" BIDSQ="<<prfin.buoyancy_variance_m2
                     <<" NEWPUFFS="<<prfin.release.puff_count
                     <<"\n";
        }
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

        const bool trace_this_hour=
            (argc>=6 && hour_index==1U) ||
            (argc>=7 && hour_index==3U);
        const std::size_t trace_cell=hour_index==1U?1374U:646U;
        std::vector<NativeH3ReceptorRow> trace_rows;
        OriginalCalcpfIntermediateHook stage_hook;
        if(trace_this_hour) {
            stage_hook=[&trace_rows,trace_cell](
                const RiseCompSamplingState& state,
                const OriginalCalcpfReceiverIntermediate& row){
                if(row.receptor_index!=trace_cell)return;
                trace_rows.push_back({state.puff_id,
                    state.before.travel_time_s,state.after.travel_time_s,row});
            };
        }
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
            },true,stage_hook,0.0F);
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
        if(hour_index<6U)
            require(hour.steps.size()==original_hour_steps[hour_index] &&
                    hour.steps.front().puff_id==original_first_puff[hour_index] &&
                    hour.steps.back().puff_id==original_last_puff[hour_index],
                    "real COMP old/new puff chronology differs from pinned original DGM");
        if(!dgm.empty()) {
            for(std::size_t n=0;n<hour.steps.size();++n){
                const std::size_t global=physical_steps+n;
                const auto& ref=dgm.at(global);
                const auto& stage=hour.steps[n];
                // Read-only source-aligned native VDCOMP inputs at the first
                // original puff200 foliage resistance disagreement (DGM #5823).
                if(global+1U==5823U){
                    require(stage.puff_id==200 && stage.ksam==2 &&
                            stage.step.dry_input_resistance.has_value() &&
                            controls.gaussian_dry.has_value(),
                            "native #5823 source key or DRY provenance missing");
                    const auto cell=stage.step.initial_met.cell.offset_0based;
                    const auto midpoint=original_met_cell_clamped(
                        0.5F*(stage.step.before.x_cells+
                              stage.step.after.x_cells),
                        0.5F*(stage.step.before.y_cells+
                              stage.step.after.y_cells),met.nx,met.ny);
                    const auto endpoint=original_met_cell_clamped(
                        stage.step.after.x_cells,stage.step.after.y_cells,
                        met.nx,met.ny);
                    const float t=met.tempk.at(cell);
                    const float sw=met.qsw.at(cell);
                    const float pd=static_cast<float>(
                        controls.gaussian_dry->setup.diffusivity_m2_s.at(0));
                    const float fr=std::max(0.0F,std::min(1.0F,sw/600.0F));
                    const float opening=t>308.0F?2.5e-6F:
                        t<283.0F?0.1e-6F:
                        2.5e-6F*fr+0.1e-6F*(1.0F-fr);
                    const float rs=2.3e-8F/(opening*pd);
                    const float native_rf=static_cast<float>(
                        stage.step.dry_input_resistance->
                            foliage_resistance_s_m.at(0));
                    std::cout<<std::setprecision(10)
                        <<"P7_NATIVE_VDCOMP5823"
                        <<" CELL0="<<cell
                        <<" MID_CELL0="<<midpoint.offset_0based
                        <<" MID_TEMPK="<<met.tempk.at(midpoint.offset_0based)
                        <<" MID_QSW="<<met.qsw.at(midpoint.offset_0based)
                        <<" END_CELL0="<<endpoint.offset_0based
                        <<" END_TEMPK="<<met.tempk.at(endpoint.offset_0based)
                        <<" END_QSW="<<met.qsw.at(endpoint.offset_0based)
                        <<" TEMPK="<<t<<" QSW="<<sw
                        <<" PDIFF="<<pd<<" B="<<opening
                        <<" RS="<<rs<<" RF="<<native_rf
                        <<" BIT_T=0x"<<std::hex<<std::bit_cast<std::uint32_t>(t)
                        <<" BIT_QSW=0x"<<std::bit_cast<std::uint32_t>(sw)
                        <<" BIT_PDIFF=0x"<<std::bit_cast<std::uint32_t>(pd)
                        <<" BIT_B=0x"<<std::bit_cast<std::uint32_t>(opening)
                        <<" BIT_RS=0x"<<std::bit_cast<std::uint32_t>(rs)
                        <<std::dec<<" ORIGINAL_VDCOMP_INPUT_PARITY=TO_BE_COMPARED"
                        <<"\n";
                }
                // Pinned original SETPUF #3874: record the native CTADJ2
                // end calculation even after full SIGZ parity is achieved.
                if(global+1U==3874U){
                    require(stage.step.sigma.terrain.has_value() &&
                            stage.puff_id==138 && stage.ksam==23,
                            "original CTADJ2 DGM3874 physical key missing");
                    const auto& t=*stage.step.sigma.terrain;
                    std::cout<<std::setprecision(10)
                        <<"P7_HEFEI_6H_NATIVE_SETPUF_END_DGM3874"
                        <<" RECORD=3874"
                        <<" R="<<t.end_ratio
                        <<" SIGTZ_END_INPUT="<<t.end_sigma_before_strain_m
                        <<" CTADJ2_STRAIN="<<t.end_strain
                        <<" SZADJ="<<t.end_sigma_adjusted_m
                        <<" SIGTZ_END_OUTPUT="<<t.end_sigma_after_sigtz_m
                        <<" BIT_R=0x"<<std::hex<<std::bit_cast<std::uint32_t>(t.end_ratio)
                        <<" BIT_STRAIN=0x"<<std::bit_cast<std::uint32_t>(t.end_strain)
                        <<" BIT_SZADJ=0x"<<std::bit_cast<std::uint32_t>(t.end_sigma_adjusted_m)
                        <<" BIT_SZE1=0x"<<std::bit_cast<std::uint32_t>(t.end_sigma_after_sigtz_m)
                        <<std::dec<<"\n";
                }
                const bool keys=stage.puff_id==ref.integers[3] &&
                    stage.ksam==ref.integers[16] &&
                    ref.integers[1]==202200100+static_cast<int>(hour_index);
                const bool xy=equal_real4(stage.step.after.x_cells,ref.reals[38]) &&
                    equal_real4(stage.step.after.y_cells,ref.reals[39]);
                const bool sy=equal_real4(stage.step.vertical_after.sigma_y_m,
                                          ref.reals[42]);
                const bool sz=equal_real4(stage.step.vertical_after.sigma_z_m,
                                          ref.reals[43]);
                matched_step_keys+=keys?1U:0U;
                matched_new_xy+=xy?1U:0U;
                matched_stored_sigy+=sy?1U:0U;
                matched_stored_sigz+=sz?1U:0U;
                if(first_dgm_mismatch_record==0U &&
                   (!keys||!xy||!sy||!sz)) {
                    first_dgm_mismatch_record=global+1U;
                    first_dgm_mismatch_field=!keys?"PUFF_ID_KSAM":
                        !xy?"ADVECT_XY":!sy?"STORED_SIGY":"STORED_SIGZ";
                    std::cout<<std::setprecision(9)
                        <<"P7_HEFEI_6H_FIRST_ORIGINAL_DGM_DIFFERENCE"
                        <<" RECORD="<<global+1U
                        <<" HOUR="<<hour_index+1U
                        <<" STAGE="<<first_dgm_mismatch_field
                        <<" PUFF_NATIVE="<<stage.puff_id
                        <<" PUFF_VENDOR="<<ref.integers[3]
                        <<" KSAM_NATIVE="<<stage.ksam
                        <<" KSAM_VENDOR="<<ref.integers[16]
                        <<" XY_NATIVE="<<stage.step.after.x_cells<<","
                                         <<stage.step.after.y_cells
                        <<" XY_VENDOR="<<ref.reals[38]<<","<<ref.reals[39]
                        <<" SIGY_NATIVE="<<stage.step.vertical_after.sigma_y_m
                        <<" SIGY_VENDOR="<<ref.reals[42]
                        <<" SIGZ_NATIVE="<<stage.step.vertical_after.sigma_z_m
                        <<" SIGZ_VENDOR="<<ref.reals[43]
                        <<" SIGZ_IN_VENDOR="<<ref.reals[22]
                        <<" SIGZ_IN_NATIVE="<<stage.step.sigma.step.sigma_z_begin_m
                        <<" SIGZ_BME_VENDOR="<<ref.reals[56]<<","
                                               <<ref.reals[57]<<","<<ref.reals[58]
                        <<" SIGZ_BME_NATIVE="<<stage.step.sigma.adjust.sigma_z_begin_m<<","
                                               <<stage.step.sigma.adjust.sigma_z_mid_m<<","
                                               <<stage.step.sigma.adjust.sigma_z_end_m
                        <<" FIRST_BME_SOURCE_STAGE_DIAGNOSTIC=YES"<<"\n";
                    if(stage.step.sigma.terrain.has_value()){
                        const auto& t=*stage.step.sigma.terrain;
                        std::cout<<std::setprecision(10)
                            <<"P7_HEFEI_6H_NATIVE_SETPUF_END_FIRST_DIFFERENCE"
                            <<" RECORD="<<global+1U
                            <<" R="<<t.end_ratio
                            <<" SIGTZ_END_INPUT="<<t.end_sigma_before_strain_m
                            <<" CTADJ2_STRAIN="<<t.end_strain
                            <<" SZADJ="<<t.end_sigma_adjusted_m
                            <<" SIGTZ_END_OUTPUT="<<t.end_sigma_after_sigtz_m
                            <<" VDZE1="<<t.end_virtual_distance_km
                            <<" BIT_R=0x"<<std::hex<<std::bit_cast<std::uint32_t>(t.end_ratio)
                            <<" BIT_STRAIN=0x"<<std::bit_cast<std::uint32_t>(t.end_strain)
                            <<" BIT_SZADJ=0x"<<std::bit_cast<std::uint32_t>(t.end_sigma_adjusted_m)
                            <<" BIT_SZE1=0x"<<std::bit_cast<std::uint32_t>(t.end_sigma_after_sigtz_m)
                            <<std::dec<<"\n";
                    }
                }
            }
        }
        if(trace_this_hour){
            require(!trace_rows.empty(),
                    "native original hour2/4 H3 receptor had no physical trace");
            std::ofstream receiver_trace(
                hour_index==1U?argv[5]:argv[6],std::ios::binary);
            require(bool(receiver_trace),
                    "cannot open original hour2/4 native stage trace");
            receiver_trace<<std::setprecision(9);
            std::size_t prev=0U;bool seen_prev=false;
            for(const auto& record:trace_rows){
                std::size_t found=hour.steps.size();
                for(std::size_t j=0;j<hour.steps.size();++j){
                    const auto& stage=hour.steps[j];
                    if(stage.puff_id==record.episode &&
                       stage.step.before.travel_time_s==record.time_before &&
                       stage.step.after.travel_time_s==record.time_after){
                        require(found==hour.steps.size(),
                                "ambiguous native original COMP hour2 receptor step");
                        found=j;
                    }
                }
                require(found<hour.steps.size() && (!seen_prev || prev<found),
                        "native hour2 CALCPF step keys not in canonical COMP order");
                seen_prev=true;prev=found;
                const auto& d=record.values;
                const auto& st=hour.steps[found];
                receiver_trace<<physical_steps+found+1U<<" "<<record.episode
                    <<" "<<st.ksam<<" "<<d.sigma_y_m<<" "<<d.sigma_z_m
                    <<" "<<d.puff_height_m<<" "<<d.receptor_pole_m
                    <<" "<<d.vertical_factor<<" "<<d.xi1<<" "<<d.xi2
                    <<" "<<d.t1<<" "<<d.t2<<" "<<d.qold<<" "<<d.qnew
                    <<" "<<d.xtemp<<" "<<d.concentration<<"\n";
            }
            receiver_trace.flush();
            require(bool(receiver_trace),
                    "native original hour2 CALCPF stage trace flush failed");
            original_hour2_trace_rows=trace_rows.size();
            std::cout<<"P7_HEFEI_NATIVE_H3_CALCPF_TRACE=PASS"
                     <<" HOUR="<<hour_index+1U
                     <<" RECEPTOR_I="<<(hour_index==1U?25:47)
                     <<" RECEPTOR_J="<<(hour_index==1U?28:13)
                     <<" RECORDS="<<original_hour2_trace_rows
                     <<" ORIGINAL_VENDOR_PARITY=NOT_YET_COMPARED\\n";
        }
        if(argc==8){
            // Independent, read-only physical mass history of original puff 200.
            // Emit EVERY substep, not just ones that reach any receptor.
            std::ofstream mass_trace(argv[7],std::ios::app);
            require(bool(mass_trace),"cannot open native original puff200 mass trace");
            mass_trace<<std::setprecision(9);
            for(std::size_t n=0;n<hour.steps.size();++n){
                const auto& stage=hour.steps[n];
                if(stage.puff_id!=200)continue;
                const auto& step=stage.step;
                require(step.pre_dry_qold_mixed.size()>=1 &&
                        step.vmass.mass_mixed.size()>=1 && step.dry.has_value() &&
                        step.dry->fraction_remaining.size()>=1 &&
                        step.dry_input_resistance.has_value() &&
                        !step.dry_input_resistance->velocity_ms.empty(),
                        "puff200 original dry mass/VD source provenance missing");
                mass_trace<<physical_steps+n+1U<<" "
                    <<stage.puff_id<<" "<<stage.ksam<<" "
                    <<step.pre_dry_qold_mixed[0]<<" "
                    <<step.vmass.mass_mixed[0]<<" "
                    <<step.dry->fraction_remaining[0]<<" "
                    <<step.dry_input_resistance->velocity_ms[0]<<" "
                    <<1.0F<<" "
                    <<step.substep.step_s<<" "
                    <<step.dry->vcbar_m_inverse<<" "
                    <<static_cast<float>(step.substep.step_s*
                                        step.dry->vcbar_m_inverse)<<" "
                    <<step.dry_input_resistance->atmospheric_resistance_s_m<<" "
                    <<step.dry_input_resistance->deposition_layer_resistance_s_m[0]<<" "
                    <<step.dry_input_resistance->canopy_resistance_s_m[0]<<" "
                    <<step.dry_input_resistance->ground_resistance_s_m[0]<<" "
                    <<step.dry_input_resistance->foliage_resistance_s_m[0]<<" "
                    <<step.dry_input_resistance->cuticle_resistance_s_m[0]<<" "
                    <<step.dry_input_resistance->source_leaf_area_index<<"\n";
            }
            mass_trace.flush();
            require(bool(mass_trace),"native original puff200 mass trace failed");
        }
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
        // Both first two source-real hours are now scientifically accepted.
        // This gate must not loosen when investigating the hour3 SIGZ.
        require(hour_index>=3U || matches==2500U,
                "first three real H3 concentration fields lost REAL4 parity");
        if(hour_index==1U && !dgm.empty())
            require(first_dgm_mismatch_record==0U,
                    "original/native first two hours of puff history diverged");
        ++completed;
       },true);
    require(summary.intervals==143U&&completed==probe_hours&&
            driver.completed_hours()==probe_hours&&
            physical_steps>=11458U&&
            (probe_hours>6U||physical_steps==11458U),
            "frozen MET and requested contiguous native COMP hours required");
    if(!dgm.empty()) {
        require(matched_step_keys==physical_steps &&
                matched_new_xy==physical_steps &&
                matched_stored_sigy==physical_steps &&
                matched_stored_sigz==physical_steps,
                "requested-hour original REAL4 COMP puff identity/XY/SIGY/SIGZ parity regressed");
        std::cout<<"P7_HEFEI_6H_ORIGINAL_DGM_REAL4_DIAGNOSTIC=PASS"
                 <<" KEYS="<<matched_step_keys<<"/11458"
                 <<" ADVECT_XY="<<matched_new_xy<<"/11458"
                 <<" STORED_SIGY="<<matched_stored_sigy<<"/11458"
                 <<" STORED_SIGZ="<<matched_stored_sigz<<"/11458"
                 <<" FIRST_DIFFERENCE_RECORD="<<first_dgm_mismatch_record
                 <<" FIRST_DIFFERENCE_STAGE="<<first_dgm_mismatch_field
                 <<" 6H_STAGE_BITWISE="
                 <<(first_dgm_mismatch_record==0U?"YES":"OPEN")<<"\n";
    }
    // The frozen native/original six-hour numerical equivalence is now
    // an acceptance gate, not a diagnostic trend. Do not silently reduce
    // it to first-three-hour parity or a floating-point tolerance.
    require(every_hour_real4 && matching_words==2500U*probe_hours &&
            first_mismatched_hour==0U && first_mismatched_cell==0U,
            "real original/native H-3 must match every requested hour in REAL4 bits");
    if(probe_hours>6U)
        std::cout<<"P7_HEFEI_REAL_EXTENDED_SOURCE_PROBE=PASS"
                 <<" HOURS="<<probe_hours
                 <<" ORIGINAL_DGM_ROWS="<<physical_steps
                 <<" H3_REAL4_WORDS="<<matching_words<<"/"<<2500U*probe_hours
                 <<" FIRST_DIFFERENCE=NONE 143H_ACCEPTED=NO\n";
    if(probe_hours==6U)std::cout<<"P7_HEFEI_6H_COMP_ORIGINAL_SUBSTEP_SCHEDULE=PASS"
             <<" ORIGINAL_DGM_ROWS=11458/11458"
             <<" HOURLY_ROWS=1275,2242,1906,2132,1742,2161"
             <<" FIRST_AND_LAST_EPISODE_IDS=SOURCE_EQUIVALENT"
             <<" DGM_EXPORTED_XY_SIGY_SIGZ=BITWISE_11458_OF_11458"
             <<" OTHER_INTERNAL_STAGES=NOT_YET_ALL_BITWISE"<<"\n";
    if(probe_hours==6U)std::cout<<"P7_HEFEI_REAL_6H_COMPUTATION_AND_PRECISION_DIAGNOSTIC=PASS"
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
    return 0; // Strict requested-hour REAL4 concentration and DGM gate.
  }catch(const std::exception& e){
    std::cerr<<"P7_HEFEI_REAL_6H_COMPUTATION_AND_PRECISION_DIAGNOSTIC=FAIL "
             <<e.what()<<"\n";
    return 1;
  }
}
