#pragma once

#include "cteas/puff/original_motion_lifecycle.hpp"
#include "cteas/puff/original_vertical_lifecycle.hpp"
#include "cteas/puff/step_integration.hpp"
#include "cteas/puff/step.hpp"
#include "cteas/puff/dispersion.hpp"
#include "cteas/puff/vendor_order.hpp"
#include "cteas/puff/dry_deposition.hpp"
#include "cteas/puff/original_turbset_mdisp3.hpp"
#include <optional>

namespace cteas::puff {

// A physically restricted but genuinely autonomous, original-ordered COMP
// *point-puff* integration path. It reads actual CALMET hourly U/V and native
// vertical state, performs PUFFDZ/ADVECT -> position -> SETPUF sigma -> VMASS
// -> COMP lid history and commits both state stores atomically.
// IMPORTANT: Rise-neutral, non-calm, one sampling substep only; no split,
// settling, buildings, terrain/PRIME, deposition or receptors. The frozen
// Hefei source is NOT rise-neutral; this is an integration stepping stone,
// never the original 143h case acceptance.
// Production MCTADJ2 requires the genuine TRELIEF/GETELEV native grid
// functions and a source-computed BVF. Synthetic callbacks are permitted
// only in labelled regression cases, never silently substituted on Hefei.
struct NativeMctadj2Controls {
    TerrainLookup terrain;
    float brunt_vaisala_s=0.0F;  // from independently resolved CALPUFF met
};
// Selected original CALPUFF MDRY=1 gas deposition at COMP
// L11076-11097. Explicitly opt-in until frozen 2022 setup/cell
// selection and uniform-code transition reach final case parity.
struct NativeGaussianDryControls {
    DryDepositionSetup setup;          // validated from original puff.inp
    float plume_path_coefficient=1.0F; // PPCOEF of selected source
};
// Opt-in original MDISP3 turbulence, with water category boundaries from
// authoritative MAKEGEO control rather than guessed land-use categories.
struct NativeCalmMdisp3Controls {
    OriginalTurbsetMdisp3 original_turbulence;
    int iwat1=0, iwat2=0;
};
struct NeutralCompControls {
    float seconds=3600.0F;
    float source_period_s=3600.0F;
    float met_spacing_m=100.0F;
    float max_sample_distance_cells=1.0F;
    // Original CALPUFF COMP WSCALM, distinct from CALMET's ICALM.
    // Frozen Hefei puff.inp explicitly selects 0.5 m/s.
    float calm_threshold_ms=0.5F;
    int max_sampling_steps=1;
    int release_number=1;
    SigmaUpdateInputs dispersion;
    std::optional<NativeMctadj2Controls> mctadj2;
    std::optional<NativeGaussianDryControls> gaussian_dry;
    // Only post-rise unsampled calm point steps until receptor/DRY/MCTADJ2
    // source parity is established. Missing data is a hard error.
    std::optional<NativeCalmMdisp3Controls> calm_mdisp3;
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
    // Same native ordered COMP transaction, now sourcing the SIGTY/SIGTZ
    // callbacks from the REAL4 source-ported MDISP=3 PG/MP curves using
    // the puff's current native METFM=1 wind/PG class, NOT test doubles.
    // The urban flag and MDISP options remain explicit until native
    // SETCSIG cell classification is integrated. Rise/calm limitations
    // of the single-substep controller remain in force.
    [[nodiscard]] NeutralCompResult step_pg(
        int id,const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,
        const DispersionSetup& setup,bool urban);
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
