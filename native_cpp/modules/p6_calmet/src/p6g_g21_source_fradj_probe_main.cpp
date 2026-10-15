// G21: source-file-driven first-hour PREPDI/CGAMMA/WIND1/SURFVAR/FRADJ
// stage data for independent ORIGINAL Fortran TERSET+FRADJ numeric parity.
// This is a scoped scientific diagnostic, never a production 17-stage hour.
#include "cteas/p6/p6g_g19_file_backed_case.hpp"
#include "cteas/p6/p6g_g20_file_backed_wind1_profiles.hpp"
#include "cteas/p6/p6g_g21_file_backed_fradj.hpp"
#include "cteas/p6/p6g_g18d2_station_clock_transaction.hpp"
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc,char** argv) {
    if(argc==2&&std::string(argv[1])=="--help") {
        std::cout<<"Usage: cteas_p6g_g21_source_fradj_probe "
                    "<met.inp> <GEO.DAT> <SURF.DAT> <US1_UP.DAT> "
                    "<US2_UP.DAT> [USn_UP.DAT ...] "
                    "--output <fradj-native.csv> --source-stage <fradj-input.dat>\n"
                    "Source-only 1h native PREPDI CGAMMA GAMMA, original "
                    "SURFVAR(TEMP2D), TERSET and FRADJ. TEST ONLY; "
                    "45-field CALMET/COMP/P6F NOT ACCEPTED.\n";
        return 0;
    }
    try {
        cteas::calmet::p6g::G19SourceFilePaths paths;
        if(argc<10)throw std::invalid_argument("G21 requires real original source file set");
        paths.met_inp=argv[1];
        paths.geo_dat=argv[2];
        paths.surf_dat=argv[3];
        std::string output_file,stage_file;
        for(int i=4;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--output"||arg=="--source-stage") {
                if(i+1>=argc||std::string(argv[i+1]).empty())
                    throw std::invalid_argument("G21 source output missing path");
                auto& selected=arg=="--output"?output_file:stage_file;
                if(!selected.empty())throw std::invalid_argument("G21 duplicate output argument");
                selected=argv[++i];
            } else {
                if(arg.starts_with("--"))throw std::invalid_argument("G21 unknown source option");
                paths.ordered_up_dat.push_back(arg);
            }
        }
        if(output_file.empty()||stage_file.empty()||
           stage_file==output_file||paths.ordered_up_dat.size()<2)
            throw std::invalid_argument("G21 requires independent output and source-stage files");
        const auto source=cteas::calmet::p6g::g19_load_original_source_case(paths);
        auto scheduler=source.make_scheduler();
        auto frame=scheduler.preview_next_hour();
        cteas::calmet::p6g::G18D2StationClockAuthority clocks;
        const auto prepared=clocks.prepare_selected(
            scheduler,source.original_metinp,true);
        cteas::calmet::p6g::G16SelectedWindPhysics wind(
            cteas::calmet::p6g::g20_file_backed_source_wind1_profiles(frame,true));
        const auto result=wind.run(
            cteas::calmet::p6::NativeHourStage::DiagnoWind1,frame);
        if(!result.completed||
           result.source!=cteas::calmet::p6::NativeHourSource::NativeCpp)
            throw std::logic_error("G21 native source WIND1 unavailable");
        const auto& first=wind.selected_result().first_guess;
        const auto selected=cteas::calmet::p6g::g21_selected_original_file_fradj(
            frame,source,prepared.cgamma,first,true);
        if(scheduler.completed_hours()!=0||clocks.has_published_hour()||
           selected.end!=frame.input.end)
            throw std::logic_error("G21 incorrectly published incomplete source hour");
        const auto nx=static_cast<std::size_t>(source.grid.nx);
        const auto ny=static_cast<std::size_t>(source.grid.ny);
        const auto cells=nx*ny;
        if(first.u_mps.size()!=cells*9||
           selected.corrected_wind.u.size()!=cells*9||
           selected.gamma_k_per_m.size()!=cells||
           selected.temperature_before_k.size()!=cells)
            throw std::logic_error("G21 stage source geometry mismatch");

        std::ofstream stage(stage_file,std::ios::trunc);
        if(!stage)throw std::runtime_error("G21 cannot write original FRADJ stage input");
        stage<<std::setprecision(std::numeric_limits<float>::max_digits10);
        stage<<nx<<" "<<ny<<" 9 "<<source.grid.grid_km*1000.f<<" "
             <<source.grid.grid_km*1000.f<<" "<<selected.source_terrad_km<<" "
             <<selected.source_critfn<<" "<<selected.temperature_before_k[0]
             <<" -1\n";
        for(const auto z:source.controls.zface_m)stage<<z<<" ";
        stage<<"\n";
        for(std::size_t j=0;j<ny;++j)
            for(std::size_t i=0;i<nx;++i) {
                const auto p=j*nx+i;
                stage<<source.grid.terrain_m[p]<<" "
                     <<selected.gamma_k_per_m[p]<<" "
                     <<selected.temperature_before_k[p]<<"\n";
            }
        for(std::size_t k=0;k<9;++k)
            for(std::size_t j=0;j<ny;++j)
                for(std::size_t i=0;i<nx;++i) {
                    const auto p=k*cells+j*nx+i;
                    stage<<first.u_mps[p]<<" "<<first.v_mps[p]<<"\n";
                }
        stage.flush();
        if(!stage)throw std::runtime_error("G21 source FRADJ input write failed");

        std::ofstream csv(output_file,std::ios::trunc);
        if(!csv)throw std::runtime_error("G21 cannot write native FRADJ CSV");
        csv<<"hour_end_utc_seconds,k_1based,j_1based,i_1based,u_mps_real4,v_mps_real4\n";
        csv<<std::setprecision(std::numeric_limits<float>::max_digits10);
        for(std::size_t k=0;k<9;++k)
            for(std::size_t j=0;j<ny;++j)
                for(std::size_t i=0;i<nx;++i) {
                    const auto p=k*cells+j*nx+i;
                    if(!std::isfinite(selected.corrected_wind.u[p])||
                       !std::isfinite(selected.corrected_wind.v[p]))
                        throw std::runtime_error("G21 source FRADJ U/V nonfinite");
                    csv<<frame.input.end.time_since_epoch().count()<<","
                       <<(k+1)<<","<<(j+1)<<","<<(i+1)<<","
                       <<selected.corrected_wind.u[p]<<","
                       <<selected.corrected_wind.v[p]<<"\n";
                }
        csv.flush();
        if(!csv)throw std::runtime_error("G21 native FRADJ output write failed");
        std::cout<<"PASS_P6G_G21_FILE_SOURCE_SELECTED_FRADJ "
                 <<"GRID_CELLS="<<cells<<" UV_VALUES="<<cells*9*2
                 <<" FRADJ_CHANGED_COMPONENTS="<<selected.corrected_uv_values
                 <<" TWO_PASS_IAVET=1 HOURLY_COMMITTED=0 "
                 <<"FULL_CALMET_VENDOR_PARITY=NOT_ACCEPTED\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<"FAIL_P6G_G21_SELECTED_FRADJ "<<e.what()<<"\n";
        return 1;
    }
}
