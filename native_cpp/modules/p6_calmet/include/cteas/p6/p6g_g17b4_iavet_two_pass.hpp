#pragma once
// G17B4: CALMET 6.5.0 DIAGNO L5607-5611/L5667-5670 performs
// SURFVAR(IALL=0) TWICE when ISURFT<0 and ISFCMET=0.
// IAVET=1 calls the immutable P6E original-source AVETMP each time,
// using WIND1 U/V before FRADJ and corrected U/V AFTER FRADJ.
// This is a stage-ordered selected source adapter, not complete CALMET.
#include "cteas/p6/p6g_original_surfvar_controls.hpp"
#include "cteas/p6/calmet_station_set.hpp"
#include "cteas/p6/read_geo_dynamic.hpp"
#include "cteas/p6/read_surface_dynamic.hpp"
#include "cteas/p6e/surfvar_multistation.hpp"
#include "cteas/p6e/avetmp.hpp"
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cteas::calmet::p6g {
struct G17B4SurfaceControls {
    OriginalSurfvarControls source{};
    int mnmdav=0;
    float hafang_deg=0.f;
};
inline G17B4SurfaceControls parse_g17b4_surface_controls(
    const std::string& original_met_inp) {
    G17B4SurfaceControls c;
    c.source=parse_original_surfvar_controls(original_met_inp);
    const auto integer=[&](const std::string& key) {
        const auto str=source_wind_value(original_met_inp,key);
        std::size_t used=0;int value=0;
        try{value=std::stoi(str,&used);}
        catch(const std::exception&) {
            throw std::invalid_argument("G17B4 invalid original "+key);
        }
        if(used!=str.size())
            throw std::invalid_argument("G17B4 original integer trailing text "+key);
        return value;
    };
    const auto real=[&](const std::string& key) {
        const auto str=source_wind_normalize(source_wind_value(original_met_inp,key));
        std::size_t used=0;float value=0.f;
        try{value=std::stof(str,&used);}
        catch(const std::exception&) {
            throw std::invalid_argument("G17B4 invalid original "+key);
        }
        if(used!=str.size()||!std::isfinite(value))
            throw std::invalid_argument("G17B4 original real invalid "+key);
        return value;
    };
    c.mnmdav=integer("MNMDAV");
    c.hafang_deg=real("HAFANG");
    if(c.source.iavet!=1||c.source.itprog!=0||
       c.source.irhprog!=0||c.source.npsta!=0||
       c.mnmdav<0||c.mnmdav>1000||
       c.hafang_deg<=0.f||c.hafang_deg>180.f)
        throw std::invalid_argument(
            "G17B4 source path requires IAVET=1, observed multi-SS SURFVAR");
    return c;
}
struct G17B4TwoPassResult {
    std::chrono::sys_seconds end{};
    std::vector<float> temperature_before_fradj_k;
    std::vector<float> temperature_after_fradj_k;
    bool complete=false;
};
class G17B4TwoPassSurfvar {
public:
    G17B4TwoPassSurfvar(const GeoGrid& grid,
                       const StationSet& station_set,
                       const SurfaceSeries& series,
                       const SurfaceHour& hour,
                       G17B4SurfaceControls controls)
        : controls_(std::move(controls)), end_(hour.end),
          nx_(static_cast<std::size_t>(grid.nx)),
          ny_(static_cast<std::size_t>(grid.ny)),
          dgrid_m_(grid.grid_km*1000.f) {
        if(nx_<2||ny_<2||nx_>400||ny_>400||
           !std::isfinite(dgrid_m_)||dgrid_m_<=0.f||
           hour.end-hour.start!=std::chrono::hours{1}||
           hour.start<series.begin||hour.end>series.end||
           station_set.surface.size()<2||station_set.surface.size()>6||
           series.station_ids.size()!=station_set.surface.size()||
           hour.by_station.size()!=station_set.surface.size()||
           controls_.source.iavet!=1||controls_.source.itprog!=0||
           controls_.source.irhprog!=0||controls_.source.npsta!=0)
            throw std::invalid_argument("G17B4 source two-pass SURFVAR scope invalid");
        for(int land: grid.landuse) {
            if(land>=grid.iwat1&&land<=grid.iwat2)
                throw std::invalid_argument(
                    "G17B4 IAVET=1 selected all-land kernel: water branch not yet accepted");
        }
        if(grid.landuse.size()!=nx_*ny_)
            throw std::invalid_argument("G17B4 missing LANDUSE source extent");
        p6e::SurfvarMultistationInput obs;
        obs.nx=nx_;obs.ny=ny_;obs.dgrid_m=dgrid_m_;
        obs.irad=controls_.source.irad;
        obs.numts=controls_.source.numts;
        // Source TRADKM converted into grid units via REAL*4 arithmetic.
        obs.trad_grid=controls_.source.trad_km/grid.grid_km;
        for(std::size_t i=0;i<station_set.surface.size();++i) {
            if(series.station_ids[i]!=station_set.surface[i].id)
                throw std::invalid_argument("G17B4 source SS ordinal mismatch");
            const auto& st=station_set.surface[i];
            const auto& rec=hour.by_station[i];
            obs.stations.push_back({
                st.local_x_m,st.local_y_m,
                rec.temperature_valid?rec.temperature_k:9999.f,
                rec.humidity_valid?rec.humidity_pct:9999});
        }
        const auto raw=p6e::surfvar_multistation(obs);
        raw_observed_temperature_=raw.temp2d_k;
        for(float t:raw_observed_temperature_)
            if(!std::isfinite(t)||t<180.f||t>350.f)
                throw std::invalid_argument("G17B4 invalid observed TEMP2D");
    }

    [[nodiscard]] const std::vector<float>& raw_observed_temperature() const {
        return raw_observed_temperature_;
    }
    [[nodiscard]] std::chrono::sys_seconds end() const noexcept {return end_;}
    [[nodiscard]] bool first_ready() const noexcept {return first_.has_value();}
    [[nodiscard]] bool second_ready() const noexcept {return final_.has_value();}
    [[nodiscard]] const std::vector<float>& first_temperature() const {
        if(!first_)throw std::logic_error("G17B4 first SURFVAR not performed");
        return *first_;
    }
    [[nodiscard]] const std::vector<float>& final_temperature() const {
        if(!final_)throw std::logic_error("G17B4 second SURFVAR not performed");
        return *final_;
    }

    void after_wind1_before_fradj(
        std::chrono::sys_seconds source_hour_end,
        const std::vector<float>& original_wind1_u3d,
        const std::vector<float>& original_wind1_v3d) {
        if(first_||final_)
            throw std::logic_error("G17B4 duplicate/late pre-FRADJ SURFVAR");
        first_=make_temperature(source_hour_end,original_wind1_u3d,
                                original_wind1_v3d);
    }
    void after_fradj_before_heatfx(
        std::chrono::sys_seconds source_hour_end,
        const std::vector<float>& corrected_fradj_u3d,
        const std::vector<float>& corrected_fradj_v3d) {
        if(!first_||final_)
            throw std::logic_error("G17B4 post-FRADJ SURFVAR out of source order");
        final_=make_temperature(source_hour_end,corrected_fradj_u3d,
                                corrected_fradj_v3d);
    }
    [[nodiscard]] G17B4TwoPassResult result() const {
        if(!first_||!final_)
            throw std::logic_error("G17B4 missing original two SURFVAR stages");
        return {end_,*first_,*final_,true};
    }
private:
    [[nodiscard]] std::vector<float> make_temperature(
        std::chrono::sys_seconds source_end,
        const std::vector<float>& u3d,
        const std::vector<float>& v3d) const {
        const auto n=nx_*ny_;
        if(source_end!=end_||u3d.size()!=n*9||v3d.size()!=n*9)
            throw std::invalid_argument("G17B4 source hour/3D wind geometry mismatch");
        p6e::AvetmpInput av;
        av.nx=nx_;av.ny=ny_;
        av.mnmdav=controls_.mnmdav;
        av.hafang_deg=controls_.hafang_deg;
        av.dgrid_m=dgrid_m_;
        av.temperature_k=raw_observed_temperature_;
        // Original SURFVAR passes UG(1,1,1), VG(1,1,1) only, never an
        // altitude-average or interpolated 2D guess.
        av.u_ms.assign(u3d.begin(),u3d.begin()+static_cast<std::ptrdiff_t>(n));
        av.v_ms.assign(v3d.begin(),v3d.begin()+static_cast<std::ptrdiff_t>(n));
        return p6e::avetmp(av);
    }
    G17B4SurfaceControls controls_;
    std::chrono::sys_seconds end_{};
    std::size_t nx_=0,ny_=0;
    float dgrid_m_=0.f;
    std::vector<float> raw_observed_temperature_;
    std::optional<std::vector<float>> first_,final_;
};
} // namespace cteas::calmet::p6g
