// G20: inspect REAL source-file-backed 2SS+2US native WIND1 at the
// first actual G14 model hour. This outputs one reproducible REAL*4
// per-gridcell trace for future independent original Fortran comparison.
// No native FRADJ/P6E/COMP/P6F output, no committed G14 hour, no oracle.
#include "cteas/p6/p6g_g19_file_backed_case.hpp"
#include "cteas/p6/p6g_g20_file_backed_wind1_profiles.hpp"
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main(int argc,char** argv) {
    if(argc==2 && std::string(argv[1])=="--help") {
        std::cout<<"Usage: cteas_p6g_g20_source_wind1_probe "
                    "<met.inp> <GEO.DAT> <SURF.DAT> "
                    "<US1_UP.DAT> <US2_UP.DAT> [US3_UP.DAT ...] "
                    "--output <original-WIND1-candidate.csv>\n"
                    "Output: first-hour native WIND1 U/V for every (k,j,i), "
                    "source-ordered original 2-6 SS/US and actual "
                    "READCF/READGE/SURF/RDUPN files. "
                    "TEST-ONLY native DIAGNO subset: no original Fortran "
                    "whole-hour parity, COMP, P6F or accepted G14 commit.\n";
        return 0;
    }
    try {
        if(argc<8||std::string(argv[argc-2])!="--output"||
           std::string(argv[argc-1]).empty())
            throw std::invalid_argument(
                "G20 needs 2+ ordered UP.DAT files and --output CSV");
        cteas::calmet::p6g::G19SourceFilePaths paths;
        paths.met_inp=argv[1];
        paths.geo_dat=argv[2];
        paths.surf_dat=argv[3];
        for(int i=4;i<argc-2;++i)paths.ordered_up_dat.emplace_back(argv[i]);
        const auto source=cteas::calmet::p6g::g19_load_original_source_case(paths);
        auto scheduler=source.make_scheduler();
        auto frame=scheduler.preview_next_hour();
        auto profiles=cteas::calmet::p6g::g20_file_backed_source_wind1_profiles(
            frame,true);
        cteas::calmet::p6g::G16SelectedWindPhysics native(std::move(profiles));
        const auto stage=native.run(
            cteas::calmet::p6::NativeHourStage::DiagnoWind1,frame);
        if(!stage.completed||
           stage.source!=cteas::calmet::p6::NativeHourSource::NativeCpp||
           scheduler.completed_hours()!=0)
            throw std::runtime_error("G20 native WIND1 was not an uncommitted original hour");
        const auto& wind=native.selected_result().first_guess;
        const auto nx=static_cast<std::size_t>(source.grid.nx);
        const auto ny=static_cast<std::size_t>(source.grid.ny);
        const auto cells=nx*ny;
        if(wind.u_mps.size()!=cells*9||wind.v_mps.size()!=cells*9)
            throw std::runtime_error("G20 original U/V output dimensions invalid");
        // Parse/physics complete first; never leave an output from a
        // partially validated original file-backed WIND1 source hour.
        std::ofstream csv(argv[argc-1],std::ios::trunc);
        if(!csv)throw std::runtime_error("G20 WIND1 output file unavailable");
        csv<<"hour_end_utc_seconds,k_1based,j_1based,i_1based,u_mps_real4,v_mps_real4\n";
        csv<<std::setprecision(std::numeric_limits<float>::max_digits10);
        std::size_t rows=0;
        for(std::size_t k=0;k<9;++k)
            for(std::size_t j=0;j<ny;++j)
                for(std::size_t i=0;i<nx;++i) {
                    const auto idx=k*cells+j*nx+i;
                    if(!std::isfinite(wind.u_mps[idx])||
                       !std::isfinite(wind.v_mps[idx]))
                        throw std::runtime_error("G20 non-finite source WIND1 output");
                    csv<<frame.input.end.time_since_epoch().count()<<","
                       <<(k+1)<<","<<(j+1)<<","<<(i+1)<<","
                       <<wind.u_mps[idx]<<","<<wind.v_mps[idx]<<"\n";
                    ++rows;
                }
        csv.flush();
        if(!csv)throw std::runtime_error("G20 WIND1 CSV write failed");
        std::cout<<"PASS_P6G_G20_FIRST_HOUR_FILE_BACKED_NATIVE_WIND1\n"
                 <<"HOUR_END_UTC_SECONDS="
                 <<frame.input.end.time_since_epoch().count()
                 <<" CELLS="<<cells<<" VERTICAL_LAYERS=9"
                 <<" ROWS="<<rows
                 <<" NATIVE_ORIGINAL_FORTRAN_WHOLE_HOUR_PARITY=NOT_ACCEPTED\n"
                 <<"P6D_P6E_COMP_P6F_PRODUCTION=NOT_ACCEPTED\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL_P6G_G20_SOURCE_WIND1 "<<e.what()<<"\n";
        return 1;
    }
}
