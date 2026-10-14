#pragma once
// P6G: dynamic-domain hourly authority using the exact accepted #53 CALMET
// DIAGNO/COMP stage order, native physical-field contract and previous-hour
// state semantics. The input/clock authority is real; the multi-station
// P6D/P6E physical adapter is NOT yet full source-parity accepted.
// No edits to #53 P6SinglePassHourlyScheduler or its native physics drivers.
#include "cteas/p6/hourly_comp_scheduler.hpp"
#include "cteas/p6/observation_input_contract.hpp"
#include "cteas/p6/p6g_original_wind_controls.hpp"
#include "cteas/p6/read_geo_dynamic.hpp"
#include <chrono>
#include <cmath>
#include <optional>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cteas::calmet::p6g {
namespace p6 = cteas::calmet::p6;

struct MultiHourFrame {
    std::size_t hour_index;
    const GeoGrid& grid;
    const StationSet& stations;
    const SurfaceHour& surface;
    const InputHour& input;
    const std::vector<UpperSeries>& upper;
    // Null only for explicit synthetic in-memory scheduler tests.
    // File-backed G15 stages receive the actual parsed met.inp controls.
    const OriginalWindControls* source_controls;
    const p6::NativeGridMemory& previous;
    p6::NativeGridMemory next;
    p6::NativeHourFields fields;
};

// Only a complete C++ implementation of each CALMET stage may return
// {NativeCpp, true}. Source oracles are validation-only and always rejected.
class MultiHourPhysics {
public:
    virtual ~MultiHourPhysics() = default;
    virtual p6::NativeHourStageResult run(
        p6::NativeHourStage stage, MultiHourFrame& frame) = 0;
};

class UnifiedMultistationHourlyScheduler {
public:
    UnifiedMultistationHourlyScheduler(GeoGrid grid, StationSet stations,
                                       SurfaceSeries surface,
                                       std::vector<UpperSeries> upper,
                                       std::optional<OriginalWindControls> source_controls=std::nullopt)
        : grid_(std::move(grid)), stations_(std::move(stations)),
          surface_(std::move(surface)), upper_(std::move(upper)),
          source_controls_(std::move(source_controls)),
          input_(bind_original_calmet_station_inputs(
              stations_, surface_, upper_)) {
        // Only 9-layer CALMET physics is source-checked at present.
        if (grid_.nx < 2 || grid_.ny < 2 ||
            grid_.nx > 400 || grid_.ny > 400 ||
            !std::isfinite(grid_.grid_km) || grid_.grid_km <= 0.f ||
            grid_.zone < 1 || grid_.zone > 60 ||
            (grid_.hemisphere != 'N' && grid_.hemisphere != 'S') ||
            grid_.landuse.size() != cells())
            throw std::invalid_argument("P6G dynamic GEO dimensions, CRS or landuse invalid");
        if (input_.empty() || input_.size() != surface_.hourly.size() ||
            input_.size() > 200000)
            throw std::invalid_argument("P6G hour count exceeds scoped input contract");
        for (std::size_t i = 0; i < stations_.surface.size(); ++i)
            if (surface_.station_ids[i] != stations_.surface[i].id)
                throw std::invalid_argument("P6G SURF station identity mismatch");
        for (std::size_t i = 0; i < stations_.upper.size(); ++i)
            if (upper_[i].station_id != stations_.upper[i].id)
                throw std::invalid_argument("P6G UP station identity mismatch");
        memory_.zi_m.assign(cells(), 0.f);
        memory_.ziconv_m.assign(cells(), 0.f);
        memory_.dptt_k.assign(cells(), 0.f);
    }

    [[nodiscard]] std::size_t planned_hours() const noexcept {return input_.size();}
    [[nodiscard]] std::size_t completed_hours() const noexcept {return completed_;}
    [[nodiscard]] const p6::NativeGridMemory& committed_memory() const noexcept {
        return memory_;
    }
    [[nodiscard]] const GeoGrid& grid() const noexcept {return grid_;}
    [[nodiscard]] const StationSet& stations() const noexcept {return stations_;}
    [[nodiscard]] bool has_original_controls() const noexcept {
        return source_controls_.has_value();
    }
    [[nodiscard]] const OriginalWindControls& original_controls() const {
        if(!source_controls_)
            throw std::runtime_error(
                "P6G synthetic in-memory scheduler has no source-controlled met.inp");
        return *source_controls_;
    }

    // G18D1: precommit is invoked only AFTER every original G14 stage
    // and ALL eight physical field + historical-memory checks succeed.
    // A P6F source writer can serialize and independently validate the
    // full hour here; its failure leaves this scheduler at hour n-1.
    // The hook must NOT publish externally observable final output or
    // mutate other persistent station clocks: only candidate validation
    // belongs before the one final state commit.
    // Existing synthetic G14 tests retain their historical API.
    void advance(MultiHourPhysics& physics) {
        advance_with_precommit(physics, [](const MultiHourFrame&){});
    }

    template<class PrecommitGate>
    void advance_with_precommit(MultiHourPhysics& physics,
                                PrecommitGate&& verify_output) {
        if (completed_ >= input_.size())
            throw std::invalid_argument("P6G cannot advance beyond declared SURF hour window");
        const auto i = completed_;
        const auto& obs = surface_.hourly[i];
        const auto& slot = input_[i];
        if (obs.end != slot.end || obs.end - obs.start != std::chrono::hours{1})
            throw std::runtime_error("P6G SURF and upper bound hour no longer coincide");
        MultiHourFrame frame{i, grid_, stations_, obs, slot, upper_,
                             source_controls_ ? &*source_controls_ : nullptr,
                             memory_, memory_, {}};
        // Reuse the accepted #53 stage list byte-for-byte: no second physics
        // scheduler authority or altered DIAGNO/COMP execution order.
        for (const auto stage : p6::kHefeiOriginalStageOrder) {
            const auto result = physics.run(stage, frame);
            if (!result.completed || result.source != p6::NativeHourSource::NativeCpp)
                throw std::runtime_error(
                    "P6G full CALMET native stage absent or Fortran/reference backfill");
        }
        validate_memory(frame.next);
        validate_fields(frame.fields);
        // Any output writer, read-back, source provenance or missing-field
        // failure happens BEFORE committing previous-hour physics memory.
        // No reference/oracle source can bypass the 17-stage gate above.
        std::forward<PrecommitGate>(verify_output)(
            static_cast<const MultiHourFrame&>(frame));
        memory_ = std::move(frame.next);
        ++completed_;
    }

    void require_complete() const {
        if (completed_ != input_.size())
            throw std::runtime_error("P6G not all declared hourly CALMET steps committed");
    }

private:
    [[nodiscard]] std::size_t cells() const noexcept {
        return static_cast<std::size_t>(grid_.nx) *
               static_cast<std::size_t>(grid_.ny);
    }
    static void check_field(const std::vector<float>& data,
                            std::size_t count, const char* label) {
        if (data.size() != count)
            throw std::runtime_error(std::string("P6G native field absent: ") + label);
        for (float v : data)
            if (!std::isfinite(v) || v == 9999.f)
                throw std::runtime_error(std::string("P6G invalid native field: ") + label);
    }
    void validate_memory(const p6::NativeGridMemory& m) const {
        check_field(m.zi_m, cells(), "ZI");
        check_field(m.ziconv_m, cells(), "ZICONV");
        check_field(m.dptt_k, cells(), "DPTT");
        for (std::size_t i = 0; i < cells(); ++i)
            if (m.zi_m[i] < 0.f || m.ziconv_m[i] < 0.f)
                throw std::runtime_error("P6G invalid negative mixing height");
    }
    void validate_fields(const p6::NativeHourFields& f) const {
        const auto n = cells();
        check_field(f.u_mps, n * 9, "U");
        check_field(f.v_mps, n * 9, "V");
        check_field(f.w_face_mps, n * 10, "WFACE");
        check_field(f.temperature_k, n * 9, "TEMP3D");
        check_field(f.qh_wm2, n, "QH");
        check_field(f.ustar_mps, n, "USTAR");
        check_field(f.el_m, n, "EL");
        check_field(f.wstar_mps, n, "WSTAR");
    }

    GeoGrid grid_;
    StationSet stations_;
    SurfaceSeries surface_;
    std::vector<UpperSeries> upper_;
    std::optional<OriginalWindControls> source_controls_;
    std::vector<InputHour> input_;
    std::size_t completed_ = 0;
    p6::NativeGridMemory memory_;
};

} // namespace cteas::calmet::p6g
