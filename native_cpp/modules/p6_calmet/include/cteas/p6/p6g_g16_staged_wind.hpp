#pragma once
// G16A: stage-scoped C++ CALMET WIND1 and INTER2, in PR #53's original
// DIAGNO stage positions. Never blend before the post-FRADJ/SLOPE boundary.
// This is a selected-kernel adapter, NOT a complete hourly CALMET solver.
// PREPDI, terrain/thermal, WINDBC, COMP and P6F remain fail-closed until
// independently integrated. Source/oracle fields are never accepted as output.
#include "cteas/p6/p6g_unified_hourly.hpp"
#include "cteas/p6/p6g_multi_wind1_firstguess.hpp"
#include "cteas/p6/inter2_selected.hpp"
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cteas::calmet::p6g {
struct G16ObservedProfiles {
    std::chrono::sys_seconds end{};
    // Station major order in each layer: SS1..SSn, US1..USn (met.inp).
    // Must be supplied by source-validated SIMILT/VERTAV/PREPDI upstream.
    std::vector<float> u_mps,v_mps;
    std::vector<unsigned char> present;
};
struct G16PostSlopeFields {
    std::chrono::sys_seconds end{};
    // Mandatory source-computed fields AFTER FRADJ, HEATFX, SLOPE and first
    // WINDBC, never a substitute built by copying the WIND1 first guess.
    std::vector<float> u_mps,v_mps,observation_weight;
};
struct G16WindStages {
    std::chrono::sys_seconds end{};
    Wind1MultiField first_guess;
    p6::Inter2SelectedOutput blended_u,blended_v;
    bool wind1_complete=false,inter2_complete=false;
};
class G16SelectedWindPhysics final : public MultiHourPhysics {
public:
    explicit G16SelectedWindPhysics(G16ObservedProfiles profiles)
        : profiles_(std::move(profiles)) {}

    const G16WindStages& selected_result() const noexcept {return stage_;}
    // Explicit post-SLOPE handoff is an independent physical requirement.
    // It cannot be silently synthesized from the WIND1 preliminary field.
    void provide_post_slope(G16PostSlopeFields input) {
        if(!stage_.wind1_complete||stage_.inter2_complete||corrected_)
            throw std::logic_error("G16 post-SLOPE handoff out of source order");
        if(input.end!=stage_.end)
            throw std::invalid_argument("G16 post-SLOPE wind has wrong UTC hour");
        corrected_=std::move(input);
    }

    p6::NativeHourStageResult run(
        p6::NativeHourStage stage,MultiHourFrame& hour) override {
        if(stage==p6::NativeHourStage::DiagnoWind1)
            return do_wind1(hour);
        if(stage==p6::NativeHourStage::DiagnoInter2)
            return do_inter2(hour);
        // Explicit NOT completed: no fake PREPDI, FRADJ, HEATFX, SLOPE,
        // WINDBC, SMOOTH/W, COMP, CGAMMA, TEMP3D or output stages.
        return {p6::NativeHourSource::Unknown,false};
    }
private:
    static void require_finite(const std::vector<float>& v,
                               std::size_t count,const char* label) {
        if(v.size()!=count)
            throw std::invalid_argument(std::string("G16 bad size for ")+label);
        for(float x:v)
            if(!std::isfinite(x))
                throw std::invalid_argument(std::string("G16 nonfinite ")+label);
    }
    const OriginalWindControls& source(const MultiHourFrame& hour) const {
        if(!hour.source_controls)
            throw std::invalid_argument("G16 missing authoritative met.inp controls");
        const auto& controls=*hour.source_controls;
        if(controls.iextrp!=-4||controls.noobs!=0||controls.iprog!=0||
           controls.lvary||controls.iobr!=0||
           (controls.icalm!=0&&controls.icalm!=1))
            throw std::invalid_argument("G16 unsupported original wind controls");
        return controls;
    }
    p6::NativeHourStageResult do_wind1(MultiHourFrame& hour) {
        if(stage_.wind1_complete||corrected_)
            throw std::logic_error("G16 WIND1 cannot be repeated or reordered");
        const auto& controls=source(hour);
        const auto ns=hour.stations.surface.size();
        const auto nu=hour.stations.upper.size();
        const auto total=ns+nu;
        const auto count=total*9;
        if(ns<1||nu<1||total>479||
           profiles_.end!=hour.input.end||hour.surface.end!=hour.input.end||
           hour.surface.by_station.size()!=ns||
           hour.input.upper.size()!=nu)
            throw std::invalid_argument("G16 WIND1 station/time mismatch");
        require_finite(profiles_.u_mps,count,"station U");
        require_finite(profiles_.v_mps,count,"station V");
        if(profiles_.present.size()!=count)
            throw std::invalid_argument("G16 WIND1 missing station flags");
        Wind1MultiInput input;
        input.nx=hour.grid.nx;input.ny=hour.grid.ny;input.nz=9;
        input.spacing_m=hour.grid.grid_km*1000.f;
        input.surface_count=static_cast<int>(ns);
        input.upper_count=static_cast<int>(nu);
        input.icalm=controls.icalm;
        input.vertical_bias.assign(controls.bias.begin(),controls.bias.end());
        input.station_u_mps=profiles_.u_mps;
        input.station_v_mps=profiles_.v_mps;
        input.present=profiles_.present;
        for(std::size_t i=0;i<ns;++i){
            const auto& obs=hour.surface.by_station[i];
            if(!obs.wind_valid||!std::isfinite(obs.wind_speed_mps))
                throw std::invalid_argument("G16 missing original SURF wind for WIND1");
            input.surface_speeds_mps.push_back(obs.wind_speed_mps);
            input.station_x_m.push_back(hour.stations.surface[i].local_x_m);
            input.station_y_m.push_back(hour.stations.surface[i].local_y_m);
        }
        for(const auto& us:hour.stations.upper){
            input.station_x_m.push_back(us.local_x_m);
            input.station_y_m.push_back(us.local_y_m);
        }
        const auto result=multi_station_wind1_firstguess(input);
        stage_.end=hour.input.end;
        stage_.first_guess=result;
        stage_.wind1_complete=true;
        return {p6::NativeHourSource::NativeCpp,true};
    }
    p6::NativeHourStageResult do_inter2(MultiHourFrame& hour) {
        if(!stage_.wind1_complete||stage_.inter2_complete||!corrected_)
            throw std::logic_error("G16 INTER2 before corrected post-SLOPE wind");
        const auto& controls=source(hour);
        const auto cells=static_cast<std::size_t>(hour.grid.nx)*
                         static_cast<std::size_t>(hour.grid.ny);
        const auto count=cells*9;
        if(hour.input.end!=stage_.end||corrected_->end!=stage_.end)
            throw std::invalid_argument("G16 INTER2 cross-hour corrected state");
        require_finite(corrected_->u_mps,count,"corrected U");
        require_finite(corrected_->v_mps,count,"corrected V");
        require_finite(corrected_->observation_weight,count,"physical WO");
        const auto ns=hour.stations.surface.size();
        const auto nu=hour.stations.upper.size();
        const auto total=ns+nu;
        p6::Inter2SelectedInput in;
        in.nx=hour.grid.nx;in.ny=hour.grid.ny;in.nz=9;
        in.nstations=static_cast<int>(total);
        in.dx_m=hour.grid.grid_km*1000.f;in.dy_m=in.dx_m;
        in.rmin_km=controls.rmin_km;in.rmax1_km=controls.rmax1_km;
        in.rmax2_km=controls.rmax2_km;in.rmax3_km=controls.rmax3_km;
        in.r1_km=controls.r1_km;in.r2_km=controls.r2_km;
        in.lvary=controls.lvary;
        in.landuse=hour.grid.landuse;
        in.water_first=hour.grid.iwat1;in.water_last=hour.grid.iwat2;
        for(const auto& s:hour.stations.surface){
            in.station_x_km.push_back(s.local_x_m*0.001f);
            in.station_y_km.push_back(s.local_y_m*0.001f);
        }
        for(const auto& s:hour.stations.upper){
            in.station_x_km.push_back(s.local_x_m*0.001f);
            in.station_y_km.push_back(s.local_y_m*0.001f);
        }
        for(std::size_t k=0;k<9;++k){
            if(controls.nintr2[k]<1)
                throw std::invalid_argument("G16 invalid original NINTR2");
            in.nintrp_per_layer.push_back(
                std::min(controls.nintr2[k],static_cast<int>(total)));
        }
        in.observation_weight=corrected_->observation_weight;
        const auto make_component=[&](const std::vector<float>& station,
                                      const std::vector<float>& corrected) {
            in.station_component_mps=station;
            // Explicit original EDIT/EDITL to distinguish missing observations.
            for(std::size_t k=0;k<9;++k)
                for(std::size_t l=0;l<total;++l)
                    if(!profiles_.present[k*total+l])
                        in.station_component_mps[k*total+l]=in.edit;
            in.step1_mps=corrected;
            return p6::original_inter2_selected(in);
        };
        const auto u=make_component(profiles_.u_mps,corrected_->u_mps);
        const auto v=make_component(profiles_.v_mps,corrected_->v_mps);
        stage_.blended_u=u;
        stage_.blended_v=v;
        stage_.inter2_complete=true;
        return {p6::NativeHourSource::NativeCpp,true};
    }
    G16ObservedProfiles profiles_;
    G16WindStages stage_{};
    std::optional<G16PostSlopeFields> corrected_;
};
} // namespace cteas::calmet::p6g
