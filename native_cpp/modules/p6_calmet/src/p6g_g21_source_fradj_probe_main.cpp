// G21: source-file-driven first-hour PREPDI/CGAMMA/WIND1/SURFVAR/FRADJ
// stage data for independent ORIGINAL Fortran TERSET+FRADJ numeric parity.
// This is a scoped scientific diagnostic, never a production 17-stage hour.
#include "cteas/p6/p6g_g19_file_backed_case.hpp"
#include "cteas/p6/p6g_g20_file_backed_wind1_profiles.hpp"
#include "cteas/p6/p6g_g21_file_backed_fradj.hpp"
#include "cteas/p6/p6g_g22_file_backed_thermal_slope.hpp"
#include "cteas/p6/p6g_g23_file_backed_stheor_inter2.hpp"
#include "cteas/p6/p6g_g18d2_station_clock_transaction.hpp"
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
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
        std::string output_file,stage_file,slope_output_file,slope_stage_file;
        std::string inter2_output_file,inter2_original_stage_prefix;
        for(int i=4;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--output"||arg=="--source-stage"||
               arg=="--slope-output"||arg=="--slope-stage"||
               arg=="--inter2-output"||arg=="--inter2-original-stage-prefix") {
                if(i+1>=argc||std::string(argv[i+1]).empty())
                    throw std::invalid_argument("G21/G22 source output missing path");
                auto& selected=arg=="--output"?output_file:
                               arg=="--source-stage"?stage_file:
                               arg=="--slope-output"?slope_output_file:
                               arg=="--slope-stage"?slope_stage_file:
                               arg=="--inter2-output"?inter2_output_file:
                               inter2_original_stage_prefix;
                if(!selected.empty())
                    throw std::invalid_argument("G21/G22 duplicate output argument");
                selected=argv[++i];
            } else {
                if(arg.starts_with("--"))throw std::invalid_argument("G21 unknown source option");
                paths.ordered_up_dat.push_back(arg);
            }
        }
        if(output_file.empty()||stage_file.empty()||
           stage_file==output_file||paths.ordered_up_dat.size()<2||
           slope_output_file.empty()!=slope_stage_file.empty()||
           (!inter2_output_file.empty()&&slope_output_file.empty())||
           (!inter2_original_stage_prefix.empty()&&inter2_output_file.empty()))
            throw std::invalid_argument("G21/G22 requires independent stage/output file pairs");
        const std::vector<std::string> outputs={
            output_file,stage_file,slope_output_file,slope_stage_file,
            inter2_output_file,
            inter2_original_stage_prefix.empty()?std::string{}:inter2_original_stage_prefix+".u.input",
            inter2_original_stage_prefix.empty()?std::string{}:inter2_original_stage_prefix+".v.input"};
        for(std::size_t i=0;i<outputs.size();++i)
            if(!outputs[i].empty())
                for(std::size_t j=0;j<i;++j)
                    if(outputs[i]==outputs[j])
                        throw std::invalid_argument("G21/G22 output filename collision");
        const auto source=cteas::calmet::p6g::g19_load_original_source_case(paths);
        auto scheduler=source.make_scheduler();
        auto frame=scheduler.preview_next_hour();
        cteas::calmet::p6g::G18D2StationClockAuthority clocks;
        const auto prepared=clocks.prepare_selected(
            scheduler,source.original_metinp,true);
        const auto source_profiles=
            cteas::calmet::p6g::g20_file_backed_source_wind1_profiles(frame,true);
        cteas::calmet::p6g::G16SelectedWindPhysics wind(source_profiles);
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
        if(!slope_output_file.empty()) {
            const auto joined=cteas::calmet::p6g::g22_selected_source_thermal_slope(
                frame,source,{first.u_mps,first.v_mps},selected,true);
            if(scheduler.completed_hours()!=0||clocks.has_published_hour())
                throw std::logic_error("G22 incomplete source hour was committed");
            std::ofstream stage22(slope_stage_file,std::ios::trunc);
            if(!stage22)throw std::runtime_error("G22 cannot export original SLOPE input");
            stage22<<std::setprecision(std::numeric_limits<float>::max_digits10);
            stage22<<nx<<" "<<ny<<" 9 "<<source.grid.grid_km*1000.f<<" "
                   <<source.grid.grid_km*1000.f<<" "
                   <<selected.source_terrad_km<<" "<<joined.heat.temperature_k[0]
                   <<" "<<source.grid.iwat1<<" "<<source.grid.iwat2<<" -1\n";
            for(const auto z:source.controls.zface_m)stage22<<z<<" ";
            stage22<<"\n";
            for(std::size_t j=0;j<ny;++j)
                for(std::size_t i=0;i<nx;++i) {
                    const auto p=j*nx+i;
                    stage22<<source.grid.terrain_m[p]<<" "
                           <<joined.heat.temperature_k[p]<<" "
                           <<joined.elustr.qh_wm2[p]<<" "
                           <<source.grid.landuse[p]<<"\n";
                }
            for(std::size_t k=0;k<9;++k)
                for(std::size_t j=0;j<ny;++j)
                    for(std::size_t i=0;i<nx;++i) {
                        const auto p=k*cells+j*nx+i;
                        stage22<<selected.corrected_wind.u[p]<<" "
                               <<selected.corrected_wind.v[p]<<"\n";
                    }
            stage22.flush();
            if(!stage22)throw std::runtime_error("G22 SLOPE input export failed");
            std::ofstream csv22(slope_output_file,std::ios::trunc);
            if(!csv22)throw std::runtime_error("G22 cannot export native slope winds");
            csv22<<"hour_end_utc_seconds,k_1based,j_1based,i_1based,"
                    "u_mps_real4,v_mps_real4\n";
            csv22<<std::setprecision(std::numeric_limits<float>::max_digits10);
            for(std::size_t k=0;k<9;++k)
                for(std::size_t j=0;j<ny;++j)
                    for(std::size_t i=0;i<nx;++i) {
                        const auto p=k*cells+j*nx+i;
                        csv22<<frame.input.end.time_since_epoch().count()<<","
                             <<(k+1)<<","<<(j+1)<<","<<(i+1)<<","
                             <<joined.post_slope_wind.u[p]<<","
                             <<joined.post_slope_wind.v[p]<<"\n";
                    }
            csv22.flush();
            if(!csv22)throw std::runtime_error("G22 native SLOPE CSV failed");
            if(!inter2_output_file.empty()) {
                const auto station=cteas::calmet::p6g::g23_source_stheor_diagi_inter2(
                    frame,source,source_profiles,selected,joined,true);
                if(scheduler.completed_hours()!=0||clocks.has_published_hour())
                    throw std::logic_error("G23 incomplete G14 hour was published");
                std::ofstream i2(inter2_output_file,std::ios::trunc);
                if(!i2)throw std::runtime_error("G23 source INTER2 output unavailable");
                i2<<"hour_end_utc_seconds,k_1based,j_1based,i_1based,"
                     "u_mps_real4,v_mps_real4\n";
                i2<<std::setprecision(std::numeric_limits<float>::max_digits10);
                if(station.inter2_u.component_mps.size()!=cells*9||
                   station.inter2_v.component_mps.size()!=cells*9)
                    throw std::logic_error("G23 source INTER2 dimensions invalid");
                for(std::size_t k=0;k<9;++k)
                    for(std::size_t j=0;j<ny;++j)
                        for(std::size_t i=0;i<nx;++i) {
                            const auto p=k*cells+j*nx+i;
                            const auto u=station.inter2_u.component_mps[p];
                            const auto v=station.inter2_v.component_mps[p];
                            if(!std::isfinite(u)||!std::isfinite(v))
                                throw std::runtime_error("G23 source INTER2 nonfinite U/V");
                            i2<<frame.input.end.time_since_epoch().count()<<","
                              <<(k+1)<<","<<(j+1)<<","<<(i+1)<<","
                              <<u<<","<<v<<"\n";
                        }
                i2.flush();
                if(!i2)throw std::runtime_error("G23 source INTER2 CSV write failed");
                if(!inter2_original_stage_prefix.empty()) {
                    // Feed untouched original CALMET INTER2 its EXACT
                    // source-file selected DIAGI(SS)/PREPDI(US), post-SLOPE
                    // first-guess grid and original READCF radii.  This
                    // captures intermediate *inputs*, NOT reference results.
                    const auto& c=source.controls;
                    const auto ns=source.stations.surface.size();
                    const auto nu=source.stations.upper.size();
                    const auto nst=ns+nu;
                    const auto& profiles=station.observed_after_diagi;
                    if(profiles.inter2_u_mps.size()!=9*nst||
                       profiles.inter2_v_mps.size()!=9*nst||
                       profiles.inter2_present.size()!=9*nst||
                       c.iprog!=0||nst!=4u)
                        throw std::logic_error("G23 original INTER2 reference input not sourced");
                    for(const auto& [tag,observations,firstguess]:
                        {std::tuple<const char*,const std::vector<float>*,
                                    const std::vector<float>*>{
                            "u",&profiles.inter2_u_mps,&joined.post_slope_wind.u},
                         std::tuple<const char*,const std::vector<float>*,
                                    const std::vector<float>*>{
                            "v",&profiles.inter2_v_mps,&joined.post_slope_wind.v}}) {
                        std::ofstream original(inter2_original_stage_prefix+
                                              "."+tag+".input",std::ios::trunc);
                        if(!original)
                            throw std::runtime_error("G23 original INTER2 input not writable");
                        original<<std::setprecision(std::numeric_limits<float>::max_digits10);
                        original<<nst<<" "<<static_cast<int>(c.lvary)<<" "
                                <<c.rmin_km<<" "<<c.rmax1_km<<" "
                                <<c.rmax2_km<<" "<<c.rmax3_km<<" "
                                <<c.r1_km<<" "<<c.r2_km<<" 999999 999998\n";
                        for(std::size_t k=0;k<9;++k)
                            original<<std::min(c.nintr2[k],static_cast<int>(nst))<<" ";
                        original<<"\n";
                        for(const auto& st:source.stations.surface)
                            original<<st.local_x_m*0.001f<<" ";
                        for(const auto& st:source.stations.upper)
                            original<<st.local_x_m*0.001f<<" ";
                        original<<"\n";
                        for(const auto& st:source.stations.surface)
                            original<<st.local_y_m*0.001f<<" ";
                        for(const auto& st:source.stations.upper)
                            original<<st.local_y_m*0.001f<<" ";
                        original<<"\n";
                        for(std::size_t j=0;j<ny;++j) {
                            for(std::size_t i=0;i<nx;++i)
                                original<<source.grid.landuse[j*nx+i]<<" ";
                            original<<"\n";
                        }
                        for(std::size_t k=0;k<9;++k) {
                            for(std::size_t l=0;l<nst;++l) {
                                const auto p=k*nst+l;
                                original<<(profiles.inter2_present[p]?
                                    (*observations)[p]:999999.f)<<" ";
                            }
                            original<<"\n";
                        }
                        for(std::size_t k=0;k<9;++k)
                            for(std::size_t j=0;j<ny;++j) {
                                for(std::size_t i=0;i<nx;++i)
                                    original<<(*firstguess)[k*cells+j*nx+i]<<" ";
                                original<<"\n";
                            }
                        // Same READCF IPROG=0 and original RDWT WO=1
                        // physically consumed by frozen G16::do_inter2.
                        for(std::size_t k=0;k<9;++k)
                            for(std::size_t j=0;j<ny;++j) {
                                for(std::size_t i=0;i<nx;++i)original<<"1 ";
                                original<<"\n";
                            }
                        original.flush();
                        if(!original)
                            throw std::runtime_error("G23 source original INTER2 file not complete");
                    }
                    std::cout<<"PASS_P6G_G23_ORIGINAL_INTER2_INPUT_STAGES "
                             <<"STATIONS="<<nst<<" LAYERS=9 "
                             <<"WIND_UV_VALUES="<<cells*9*2
                             <<" REFERENCE_CALCULATION=NOT_YET_PERFORMED\n";
                }
                std::cout<<"PASS_P6G_G23_SOURCE_STHEOR_DIAGI_INTER2 "
                         <<"STATIONS="<<station.station_diagi.size()
                         <<" CELLS="<<cells
                         <<" UV_VALUES="<<cells*9*2
                         <<" CHANGED_DIAGI_SS_COMPONENTS="
                         <<station.replaced_surface_profile_values
                         <<" SOURCE_CMECH="<<station.source_cmech
                         <<" PREVIOUS_STATE_MUTATED=0 G14_HOURS_COMMITTED=0 "
                           "FULL_VENDOR_45_FIELD=NOT_ACCEPTED\n";
            }
            std::cout<<"PASS_P6G_G22_SOURCE_HEATFX_AIRDEN_ELUSTR_SLOPE "
                     <<"CELLS="<<cells<<" WIND_UV_VALUES="<<cells*9*2
                     <<" CHANGED_SLOPE_COMPONENTS="<<joined.changed_slope_components
                     <<" STABLE_CELLS="<<joined.elustr.stable_cells
                     <<" DAYTIME_CELLS="<<joined.elustr.daytime_cells
                     <<" G14_HOURS_COMMITTED=0 FULL_45_FIELD=NOT_ACCEPTED\n";
        }
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
