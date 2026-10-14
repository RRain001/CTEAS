#pragma once
// G17A: source-ordered selected terrain stages, reusing the immutable PR #53
// physical FRADJ/SLOPE kernels and G16 native WIND1/INTER2 stages.
// This explicitly controlled-fixture adapter NEVER claims original P6E
// HEATFX, ELUSTR, AIRDEN, COMP, or complete CALMET native hour acceptance.
// Real multi-site thermal P6E state remains mandatory for production.
#include "cteas/p6/p6g_g16_staged_wind.hpp"
#include "cteas/p6/p6g_terrain_hour.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cteas::calmet::p6g {

struct G17FradjSource {
    std::chrono::sys_seconds end{};
    TerrainHourProvenance provenance=TerrainHourProvenance::Missing;
    int isurft=-1;
    float domain_temperature_k=0.f,critfn=0.f;
    std::vector<float> lapse_k_per_m,temperature_k;
};
struct G17SlopeHeatSource {
    std::chrono::sys_seconds end{};
    TerrainHourProvenance provenance=TerrainHourProvenance::Missing;
    std::vector<float> sensible_heat_w_m2,temperature_k;
};

class G17SelectedTerrainFixturePhysics final : public MultiHourPhysics {
public:
    G17SelectedTerrainFixturePhysics(G16ObservedProfiles profiles,
                                    TerrainStaticG11 terrain,
                                    G17FradjSource fradj)
        : wind_(std::move(profiles)),terrain_(std::move(terrain)),
          fradj_(std::move(fradj)) {}

    [[nodiscard]] const G16WindStages& wind_stages() const {
        return wind_.selected_result();
    }
    [[nodiscard]] const p6::WindUV& after_fradj() const {
        if(!after_fradj_)throw std::logic_error("G17 FRADJ not yet computed");
        return *after_fradj_;
    }
    [[nodiscard]] const p6::WindUV& after_slope() const {
        if(!after_slope_)throw std::logic_error("G17 SLOPE not yet computed");
        return *after_slope_;
    }

    // G17 consumes QH only after FRADJ. A fixture is deliberately supplied
    // outside the still-unimplemented HEATFX/AIRDEN/ELUSTR callbacks. An
    // invalid/missing source fails closed; the full hourly scheduler cannot
    // accept the partial adapter because those stages return Unknown/false.
    void provide_slope_heat(G17SlopeHeatSource source) {
        if(!after_fradj_||after_slope_||heat_)
            throw std::logic_error("G17 heat supplied outside FRADJ→SLOPE window");
        if(source.end!=current_hour_ ||
           source.provenance!=TerrainHourProvenance::SyntheticReferenceFixture)
            throw std::invalid_argument("G17 unvalidated or wrong-hour heat input");
        const auto n=static_cast<std::size_t>(terrain_.nx)*
                     static_cast<std::size_t>(terrain_.ny);
        require_array(source.sensible_heat_w_m2,n);
        require_array(source.temperature_k,n);
        for(float t:source.temperature_k)
            if(t<=0.f)throw std::invalid_argument("G17 invalid surface T");
        heat_=std::move(source);
    }

    p6::NativeHourStageResult run(p6::NativeHourStage stage,
                                  MultiHourFrame& frame) override {
        if(stage==p6::NativeHourStage::DiagnoWind1){
            if(wind_.selected_result().wind1_complete)
                throw std::logic_error("G17 WIND1 reused");
            if(terrain_.nx!=frame.grid.nx||terrain_.ny!=frame.grid.ny ||
               terrain_.spacing_m!=frame.grid.grid_km*1000.f ||
               terrain_.landuse!=frame.grid.landuse)
                throw std::invalid_argument("G17 TERSET not from current GEO");
            auto result=wind_.run(stage,frame);
            if(result.completed)current_hour_=frame.input.end;
            return result;
        }
        if(stage==p6::NativeHourStage::DiagnoFradj)
            return do_fradj(frame);
        if(stage==p6::NativeHourStage::DiagnoSlope)
            return do_slope(frame);
        if(stage==p6::NativeHourStage::DiagnoWindbcFirstGuess)
            return first_windbc(frame);
        if(stage==p6::NativeHourStage::DiagnoInter2){
            if(!first_windbc_)throw std::logic_error(
                "G17 INTER2 before first source WINDBC");
            return wind_.run(stage,frame);
        }
        // HEATFX/AIRDEN/ELUSTR and DIAGI stability-dependent SIMILT are
        // NOT implemented; neither are COMP, P6F, final mass-consistency.
        return {p6::NativeHourSource::Unknown,false};
    }
private:
    static void require_array(const std::vector<float>& a,std::size_t n) {
        if(a.size()!=n)throw std::invalid_argument("G17 missing source field");
        for(float v:a)
            if(!std::isfinite(v))
                throw std::invalid_argument("G17 nonfinite source field");
    }
    void verify_hour(const MultiHourFrame& frame) const {
        if(frame.input.end!=current_hour_ || frame.surface.end!=current_hour_ ||
           fradj_.end!=current_hour_)
            throw std::invalid_argument("G17 terrain forcing has wrong model hour");
        if(!frame.source_controls)
            throw std::invalid_argument("G17 missing original met.inp ZFACE");
    }
    p6::NativeHourStageResult do_fradj(const MultiHourFrame& frame) {
        if(!wind_.selected_result().wind1_complete||after_fradj_||heat_||
           first_windbc_)
            throw std::logic_error("G17 FRADJ invalid source stage ordering");
        verify_hour(frame);
        if(fradj_.provenance!=TerrainHourProvenance::SyntheticReferenceFixture||
           fradj_.isurft!=-1)
            throw std::invalid_argument("G17 FRADJ forcing is not accepted fixture");
        const auto n=static_cast<std::size_t>(terrain_.nx)*
                     static_cast<std::size_t>(terrain_.ny);
        require_array(fradj_.lapse_k_per_m,n);
        require_array(fradj_.temperature_k,n);
        const auto& first=wind_.selected_result().first_guess;
        const std::vector<float> zface(frame.source_controls->zface_m.begin(),
                                       frame.source_controls->zface_m.end());
        p6::WindUV initial{first.u_mps,first.v_mps};
        // Original CALMET source kernel, same as G11; no reimplementation.
        auto calculated=p6::froude_adjust(
            terrain_.nx,terrain_.ny,9,terrain_.spacing_m,terrain_.spacing_m,
            terrain_.terrain_m,terrain_.hmax_m,fradj_.lapse_k_per_m,
            fradj_.temperature_k,fradj_.isurft,
            fradj_.domain_temperature_k,fradj_.critfn,
            zface,std::move(initial));
        after_fradj_=std::move(calculated);
        return {p6::NativeHourSource::NativeCpp,true};
    }
    p6::NativeHourStageResult do_slope(const MultiHourFrame& frame) {
        if(!after_fradj_||!heat_||after_slope_||first_windbc_)
            throw std::logic_error("G17 SLOPE without FRADJ and explicit QH");
        verify_hour(frame);
        const auto n=static_cast<std::size_t>(terrain_.nx)*
                     static_cast<std::size_t>(terrain_.ny);
        require_array(heat_->sensible_heat_w_m2,n);
        require_array(heat_->temperature_k,n);
        p6::SlopeInput slope;
        slope.nx=terrain_.nx;slope.ny=terrain_.ny;slope.nz=9;
        slope.dx_m=terrain_.spacing_m;slope.dy_m=terrain_.spacing_m;
        slope.water_first=terrain_.water_first;
        slope.water_last=terrain_.water_last;
        slope.isurft=fradj_.isurft;
        slope.domain_temperature_k=fradj_.domain_temperature_k;
        slope.zface_m.assign(frame.source_controls->zface_m.begin(),
                             frame.source_controls->zface_m.end());
        slope.terrain_m=terrain_.terrain_m;
        slope.hmax_m=terrain_.hmax_m;slope.hmin_m=terrain_.hmin_m;
        slope.disthmax_m=terrain_.dist_hmax_m;
        slope.disthmin_m=terrain_.dist_hmin_m;
        slope.sensible_heat_w_m2=heat_->sensible_heat_w_m2;
        slope.surface_temperature_k=heat_->temperature_k;
        slope.landuse=terrain_.landuse;
        const auto addition=p6::compute_original_slope_scoped(slope);
        auto corrected=*after_fradj_;
        if(corrected.u.size()!=addition.u_mps.size()||
           corrected.v.size()!=addition.v_mps.size())
            throw std::invalid_argument("G17 FRADJ/SLOPE array geometry mismatch");
        for(std::size_t index=0;index<corrected.u.size();++index){
            corrected.u[index]+=addition.u_mps[index];
            corrected.v[index]+=addition.v_mps[index];
            if(!std::isfinite(corrected.u[index])||
               !std::isfinite(corrected.v[index]))
                throw std::invalid_argument("G17 SLOPE produced nonfinite winds");
        }
        after_slope_=std::move(corrected);
        return {p6::NativeHourSource::NativeCpp,true};
    }
    p6::NativeHourStageResult first_windbc(const MultiHourFrame& frame) {
        if(!after_slope_||first_windbc_)
            throw std::logic_error("G17 first WINDBC before source SLOPE");
        verify_hour(frame);
        const auto nx=static_cast<std::size_t>(terrain_.nx);
        const auto ny=static_cast<std::size_t>(terrain_.ny);
        const auto n=nx*ny;
        G16PostSlopeFields handoff;
        handoff.end=current_hour_;
        handoff.u_mps=after_slope_->u;
        handoff.v_mps=after_slope_->v;
        // For the currently accepted IPROG=0 route RDWT supplies WO=1;
        // other spatial WO cases cannot be silently emulated.
        if(frame.source_controls->iprog!=0)
            throw std::invalid_argument("G17 first WINDBC needs source WO");
        handoff.observation_weight.assign(n*9,1.f);
        handoff.ub_west_east_mps.resize(18*ny);
        handoff.vb_south_north_mps.resize(18*nx);
        for(std::size_t k=0;k<9;++k){
            for(std::size_t j=0;j<ny;++j){
                const auto row=k*n+j*nx;
                handoff.ub_west_east_mps[k*2*ny+j]=handoff.u_mps[row];
                handoff.ub_west_east_mps[k*2*ny+ny+j]=handoff.u_mps[row+nx-1];
            }
            for(std::size_t i=0;i<nx;++i){
                const auto row=k*n+i;
                handoff.vb_south_north_mps[k*2*nx+i]=handoff.v_mps[row];
                handoff.vb_south_north_mps[k*2*nx+nx+i]=
                    handoff.v_mps[row+(ny-1)*nx];
            }
        }
        wind_.provide_post_slope(std::move(handoff));
        first_windbc_=true;
        return {p6::NativeHourSource::NativeCpp,true};
    }
    G16SelectedWindPhysics wind_;
    TerrainStaticG11 terrain_;
    G17FradjSource fradj_;
    std::chrono::sys_seconds current_hour_{};
    std::optional<p6::WindUV> after_fradj_,after_slope_;
    std::optional<G17SlopeHeatSource> heat_;
    bool first_windbc_=false;
};
} // namespace cteas::calmet::p6g
