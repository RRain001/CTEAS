// Real 2022 Hefei SOURCE admission, then diagnostic original COMP first-hour.
// The frozen DGM is an immutable comparator, never an input to the solver.
#include "cteas/puff/control.hpp"
#include "cteas/puff/original_comp_rise_hourly.hpp"
#include "cteas/puff/original_met_hours.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cteas::puff;
namespace {
struct Dgm {
    std::array<std::int32_t,24> i;
    std::array<float,64> f;
};
static_assert(sizeof(Dgm)==352U);
static_assert(std::endian::native==std::endian::little);
void demand(bool ok,const char* message){
    if(!ok)throw std::runtime_error(message);
}
std::vector<Dgm> read_reference(const std::string& path){
    std::ifstream file(path,std::ios::binary);
    demand(static_cast<bool>(file),"original DGM missing");
    std::vector<Dgm> out;
    Dgm row{};
    while(file.read(reinterpret_cast<char*>(&row),sizeof(row)))out.push_back(row);
    demand(file.eof()&&file.gcount()==0&&out.size()==1275,
           "not the immutable first-hour 1275-record DGM");
    return out;
}
}

int main(int argc,char** argv){
 try{
    demand(argc==4,"usage: firsthour_point_comp MET.DAT puff.inp DGM.bin");
    const auto deck=ControlFile::read_file(argv[2]);
    demand(deck.required_int("METFM")==1 && deck.required_int("MGAUSS")==1&&
           deck.required_int("NPT1")==1 && deck.required_int("MRISE")==1 &&
           deck.required_int("MTIP")==1 && deck.required_int("MSHEAR")==0&&
           deck.required_int("MXNEW")==99 && deck.required_int("MXSAM")==99,
           "source deck is not original selected Gaussian point-puff mode");
    const auto original=read_reference(argv[3]);
    const auto stack=deck.required_float_list("X");
    const auto factor=deck.required_float_list("FMFAC");
    const auto sy=deck.required_float_list("SYMIN");
    const auto sz=deck.required_float_list("SZMIN");
    const auto sample=deck.required_float_list("XSAMLEN");
    demand(stack.size()==10&&factor.size()==1&&factor[0]==1.0&&
           sy.size()==1&&sy[0]==1.0&&sz.size()==1&&sz[0]==1.0&&
           sample.size()==1&&sample[0]==1.0,
           "frozen source and dispersion-minimum controls missing");
    bool examined=false;
    const auto summary=OriginalMetHourlyReader::read(argv[1],
      [&](const Calmet21Header& header,const Calmet21Static& geo,
          const OriginalMetHour& met){
        if(examined)return;
        examined=true;
        demand(met.begin.year==2022 && met.begin.julian_day==1 &&
               met.begin.hour==0 && header.nx==50 && header.ny==50 &&
               header.nz==9 && header.dx_m==100.0F,
               "first original hour or grid unexpectedly changed");
        OriginalPointPrfinRequest req;
        req.x_cells=static_cast<float>(
            (static_cast<float>(stack[0])*1000.0F-header.origin_x_m)/header.dx_m);
        req.y_cells=static_cast<float>(
            (static_cast<float>(stack[1])*1000.0F-header.origin_y_m)/header.dx_m);
        req.stack_height_m=static_cast<float>(stack[2]);
        req.diameter_m=static_cast<float>(stack[4]);
        req.exit_velocity_ms=static_cast<float>(stack[5]);
        req.stack_temperature_k=static_cast<float>(stack[6]);
        req.momentum_flux_factor=static_cast<float>(factor[0]);
        req.calm_threshold_ms=0.5F;
        req.max_sample_distance_cells=static_cast<float>(sample[0]);
        req.max_new_puffs=deck.required_int("MXNEW");
        const auto prfin=original_point_prfin_metfm1(met,geo,header,req);
        demand(req.x_cells==25.0F && req.y_cells==25.0F &&
               prfin.release.puff_count==54 &&
               std::bit_cast<std::uint32_t>(prfin.effective_height_m)==
               std::bit_cast<std::uint32_t>(136.40403747558594F) &&
               std::bit_cast<std::uint32_t>(prfin.release_wind_ms)==
               std::bit_cast<std::uint32_t>(1.493919849395752F),
               "independently computed real PRFIN source drifted");
        const auto cell=original_met_cell(
            req.x_cells,req.y_cells,met.nx,met.ny).offset_0based;
        demand(cell==1275U && met.ipgt.at(cell)==4 &&
               std::abs(met.tempk.at(cell)-279.15F)<0.01F,
               "original source-cell 2D meteorology changed");
        OriginalPointNativeSourceParameters args;
        args.base_elevation_m=static_cast<float>(stack[3]);
        args.met_spacing_m=header.dx_m;
        args.initial_sigma_y_m=static_cast<float>(sy[0]);
        args.initial_sigma_z_m=static_cast<float>(sz[0]);
        args.emission_rates={stack[8],stack[9]};
        auto src=original_point_hour_from_prfin(req,prfin,args);
        demand(src.mass.species_count==2 &&
               src.mass.emission_rate.at(0)==834000 &&
               src.mass.emission_rate.at(1)==0 &&
               src.mass.mass_fraction_mixed==1.0 &&
               src.point.final_rise_variance_m2>0 &&
               src.point.stability_class==4 &&
               src.point.source_x_km==25 &&
               src.point.source_y_km==25,
               "native POINTS1 release data did not come from original source");
        OriginalCompRisePointDriver admitted;
        const float emitted=static_cast<float>(
            stack[8]*prfin.release.emission_duration_s);
        std::size_t checked_rows=0;
        std::vector<bool> seen(55,false);
        for(const auto& d:original){
            const int id=d.i[3],release_no=d.i[14],ksam=d.i[16];
            demand(d.i[1]==202200100 && d.i[12]==0 &&
                   id>=1&&id<=54&&release_no>=1&&release_no<=54 &&
                   d.i[10]==54 && d.i[11]==release_no &&
                   ksam>=release_no&&ksam<=54,
                   "DGM 54-puff first-hour identity/cadence not genuine");
            if(seen[static_cast<std::size_t>(id)])continue;
            demand(!seen[static_cast<std::size_t>(id)] &&
                   id==release_no && d.f[0]==req.x_cells &&
                   d.f[1]==req.y_cells && d.f[2]==prfin.effective_height_m &&
                   d.f[19]==0.0F && d.f[20]==0.0F,
                   "original first-seen puff not born at native point source");
            seen[static_cast<std::size_t>(id)]=true;
            ++checked_rows;
            auto point=src.point;
            auto mass=src.mass;
            point.release_index=id;
            point.emission_duration_s=prfin.release.emission_duration_s;
            mass.emission_duration_s=prfin.release.emission_duration_s;
            admitted.admit(id,point,mass);
            const auto pos=admitted.motion(id);
            const auto vertical=admitted.vertical(id);
            demand(pos.x_cells==d.f[0] && pos.y_cells==d.f[1] &&
                   pos.z_m==d.f[2] && pos.travel_time_s==0 &&
                   pos.total_distance_m==0 &&
                   vertical.sigma_y_m==static_cast<float>(sy[0]) &&
                   vertical.sigma_z_m==static_cast<float>(sz[0]) &&
                   vertical.puff_code==1 &&
                   vertical.mass_mixed.size()==2 &&
                   vertical.mass_above.size()==2 &&
                   vertical.mass_mixed[0]==emitted &&
                   vertical.mass_mixed[1]==0 &&
                   vertical.mass_above[0]==0,
                   "genuine native SOURCE/SWAP/INJECT puff admission diverged");
        }
        demand(checked_rows==54 && admitted.active_count()==54 &&
               std::all_of(seen.begin()+1,seen.end(),[](bool v){return v;}),
               "native POINTS1 did not independently admit all 54 original puffs");
        std::cout<<"P7_HEFEI_54_NATIVE_ADMISSIONS=PASS"
                 <<" HEFF=REAL4_ORIGINAL BIDSQ=POINTS1_SOURCE"
                 <<" PUFFS="<<checked_rows<<" FIRST_HOUR_DGM_ROWS="<<original.size()
                 <<" RELEASE_MASS_BQ="<<emitted
                 <<" DGM_INITIAL_STATES_USED_AS_INPUT=NO"
                 <<" OTHER_142H=NOT_YET_ACCEPTED\\n";
        // Expose the first physical barrier without treating diagnostic
        // incomplete COMP as an accepted full first-hour physics run.
        NeutralCompControls ctrl;
        ctrl.met_spacing_m=header.dx_m;
        ctrl.max_sample_distance_cells=static_cast<float>(sample[0]);
        ctrl.calm_threshold_ms=0.5F;
        ctrl.max_sampling_steps=99;
        const auto cdiv=deck.required_float_list("CDIV");
        demand(cdiv.size()==2 && cdiv[0]==0.0 && cdiv[1]==0.0,
               "frozen 2022 CDIV thresholds were silently substituted");
        ctrl.original_wdiv=NativeOriginalWdivControls{
            static_cast<float>(cdiv[0]),static_cast<float>(cdiv[1])};

        DispersionSetup pg;
        pg.mdisp=3;
        pg.averaging_factor=1;
        pg.sigma_y_heffter_m=550;
        pg.heffter_sigma_z=false;
        pg.roughness_adjustment=false;
        OriginalCompRiseMultiPuffHours driver;
        const OriginalCompPuffBounds bounds{0.0F,50.0F,0.0F,50.0F};
        try {
            const auto hour=driver.advance_hour_point_mxsam99_adaptive_bounded(
                 met,geo,ctrl,pg,false,src,bounds);
            if(!hour.steps.empty()){
                std::cout<<"P7_HEFEI_FIRSTHOUR_NATIVE_WDIV"
                         <<" ORIG_FIRST_SIGZ_IN="<<original.front().f[22]
                         <<" NAT_FIRST_SIGZ_IN="<<hour.steps.front().step.sigma.step.sigma_z_begin_m
                         <<" ORIG_FIRST_SIGZ_OUT="<<original.front().f[43]
                         <<" NAT_FIRST_SIGZ_OUT="<<hour.steps.front().step.vertical_after.sigma_z_m
                         <<" VERTICAL_DIVERGENCE=SOURCE_PORTED"
                         <<" REAL4_NUMERICAL_PARITY=UNDER_TEST\\n";
            }
            std::size_t compare=std::min(hour.steps.size(),original.size());
            std::size_t aligned=0,first=0;
            float dx=0,dy=0;
            for(std::size_t i=0;i<compare;++i){
                const auto& a=hour.steps[i];
                const auto& b=original[i];
                // First original divergence at row 8 needs an upstream
                // layer-by-layer audit, not a fitted receptor tolerance.
                if(i<12){
                    const auto& n=a.step;
                    std::cout<<"P7_HEFEI_FIRSTHOUR_SUBSTEP_TRACE"
                             <<" ROW="<<i+1<<" ID="<<a.puff_id
                             <<" KSAM="<<a.ksam
                             <<" ORIG_OLD_XY="<<b.f[0]<<","<<b.f[1]
                             <<" NAT_OLD_XY="<<n.before.x_cells<<","<<n.before.y_cells
                             <<" ORIG_SIGZ_IN="<<b.f[22]
                             <<" NAT_SIGZ_IN="<<n.sigma.step.sigma_z_begin_m
                             <<" ORIG_ZBOT_ZTOP="<<b.f[8]<<","<<b.f[7]
                             <<" NAT_ZBOT_ZTOP="<<n.initial_met.extent.z_bottom_m
                                                    <<","<<n.initial_met.extent.z_top_m
                             <<" ORIG_PRE_UV="<<b.f[9]<<","<<b.f[10]
                             <<" NAT_PRE_UV="<<n.initial_met.wind.u_ms<<","
                                             <<n.initial_met.wind.v_ms
                             <<" ORIG_ADV_UV="<<b.f[12]<<","<<b.f[13]
                             <<" NAT_ADV_UV="<<n.rise.adjusted.u_ms<<","
                                             <<n.rise.adjusted.v_ms
                             <<" ORIG_END_XY="<<b.f[38]<<","<<b.f[39]
                             <<" NAT_END_XY="<<n.after.x_cells<<","<<n.after.y_cells
                             <<" ORIG_XTOT="<<b.f[40]
                             <<" NAT_XTOT="<<n.after.total_distance_m
                             <<" ORIG_SIGZ_OUT="<<b.f[43]
                             <<" NAT_SIGZ_OUT="<<n.vertical_after.sigma_z_m
                             <<"\n";
                }

                if(a.puff_id!=b.i[3]||a.ksam!=b.i[16]||
                   a.step.after.x_cells!=b.f[38]||
                   a.step.after.y_cells!=b.f[39]){
                    if(first==0){
                        first=i+1;
                        dx=a.step.after.x_cells-b.f[38];
                        dy=a.step.after.y_cells-b.f[39];
                    }
                }else ++aligned;
            }
            std::cout<<"P7_HEFEI_FIRST_HOUR_NATIVE_COMP_DIAGNOSTIC"
                     <<" NATIVE_SUBSTEPS="<<hour.steps.size()
                     <<" REFERENCE_SUBSTEPS="<<original.size()
                     <<" MATCHED_EXACT_XY_STEPS="<<aligned
                     <<" FIRST_DIFFERENCE_RECORD="<<first
                     <<" FIRST_DX_CELLS="<<dx<<" FIRST_DY_CELLS="<<dy
                     <<" RETIRED="<<hour.retired.size()
                     <<" COMP_NUMERICAL_PARITY=OPEN\\n";
        }catch(const std::exception& e){
            std::cout<<"P7_HEFEI_FIRST_HOUR_NATIVE_COMP_BLOCKER="
                     <<e.what()
                     <<" SOURCE_ADMISSION_GATE=PASS"
                     <<" FULL_COMP=NOT_YET_ACCEPTED\\n";
        }
     },true);
    demand(examined && summary.intervals==143,
           "real source test must read complete frozen MET file");
    return 0;
 }catch(const std::exception& e){
    std::cerr<<"P7_HEFEI_NATIVE_POINT_ADMISSION=FAIL "<<e.what()<<'\n';
    return 1;
 }
}
