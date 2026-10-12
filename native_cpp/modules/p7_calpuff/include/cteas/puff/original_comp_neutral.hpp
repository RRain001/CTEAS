#pragma once

#include "cteas/puff/original_motion_lifecycle.hpp"
#include "cteas/puff/original_vertical_lifecycle.hpp"
#include "cteas/puff/step_integration.hpp"
#include "cteas/puff/step.hpp"
#include "cteas/puff/vendor_order.hpp"

namespace cteas::puff {

// A physically restricted but genuinely autonomous, original-ordered COMP
// *point-puff* integration path. It reads actual CALMET hourly U/V and native
// vertical state, performs PUFFDZ/ADVECT -> position -> SETPUF sigma -> VMASS
// -> COMP lid history and commits both state stores atomically.
// IMPORTANT: Rise-neutral, non-calm, one sampling substep only; no split,
// settling, buildings, terrain/PRIME, deposition or receptors. The frozen
// Hefei source is NOT rise-neutral; this is an integration stepping stone,
// never the original 143h case acceptance.
struct NeutralCompControls {
    float seconds=3600.0F;
    float source_period_s=3600.0F;
    float met_spacing_m=100.0F;
    float max_sample_distance_cells=1.0F;
    int max_sampling_steps=1;
    int release_number=1;
    SigmaUpdateInputs dispersion;
};
struct NeutralCompResult {
    OriginalPuffMotionState before;
    OriginalPuffMotionState after;
    NativeVerticalPuffState vertical_after;
    OriginalMetCell cell;
    PuffExtent vertical_extent;
    AdvectiveWind wind;
    SamplingCadence cadence;
    Substep substep;
    StepBookkeeping bookkeeping;
    SigmaUpdateResult sigmas;
    VmassResult mass;
    float native_wind_speed_ms=0.0F;
};
class OriginalCompNeutralPointDriver {
public:
    void admit(int id,const NewPuffInputs& point,const InjectionInputs& injection);
    [[nodiscard]] NeutralCompResult step(
        int id,const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,
        const SigmaYCurve& curve_y,const SigmaZCurve& curve_z);
    [[nodiscard]] OriginalPuffMotionState motion(int id)const;
    [[nodiscard]] NativeVerticalPuffState vertical(int id)const;
    [[nodiscard]] std::size_t steps(int id)const;
    void retire(int id);
    [[nodiscard]] std::size_t active_count() const noexcept;
private:
    struct RunningPuff {
        std::size_t steps=0;
        float previous_ws_ms=0.0F;
    };
    OriginalPuffMotionLifecycle motion_;
    OriginalVerticalLifecycle vertical_;
    std::unordered_map<int,RunningPuff> running_;
};
} // namespace cteas::puff
