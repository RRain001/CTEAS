#pragma once

#include "cteas/puff/original_comp_rise.hpp"
#include <cstddef>
#include <vector>

namespace cteas::puff {

// Deterministic 1/6/143h native MULTI-POINT COMP transaction with the
// genuine source-ported GRISE/RISEWIND + METFM1 + SIGTY/SIGTZ + VMASS chain.
// Input point admissions are EXPLICIT; native POINTS1/SWAP/ROLLDN original
// source release cadence is still independent pending integration.
struct RisePointAdmission {
    int puff_id=0;
    NewPuffInputs point;
    InjectionInputs mass;
};
struct RiseCompHourRecord {
    int puff_id=0;
    RiseCompStep step;
};
struct RiseCompHour {
    std::size_t hour_index=0;
    std::vector<RiseCompHourRecord> puffs;
    std::size_t active_after=0;
};

// One original POINTS1 *constant* point source with externally resolved
// WSNEW. Native RLSMET/PRFIN wind-at-final-rise selection must supply WSNEW
// upstream; this API NEVER infers WSNEW from a synthetic stack wind.
struct OriginalPointHourSource {
    NewPuffInputs point;
    InjectionInputs mass;
    ReleaseCadenceInputs cadence;
};
struct RiseCadencedSubstep {
    int puff_id=0;
    int original_release_number=0; // J=1..NPUFFS, 0 for old puff
    int sampling_slot=0;            // KSAM 1..NPUFFS
    RiseCompStep step;
};
struct RiseCadencedHour {
    std::size_t hour_index=0;
    ReleaseCadence cadence;
    std::vector<RiseCadencedSubstep> steps;
    std::size_t active_after=0;
    std::size_t new_puffs=0;
};
class OriginalCompRiseMultiPuffHours {
public:
    [[nodiscard]] RiseCompHour advance_hour(
        const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,
        const DispersionSetup& pg,bool urban,
        const std::vector<RisePointAdmission>& admissions);

    // Adds the original POINTS1 cadence NPUFFS and emits NPUFFS equal-mass
    // puffs per hour. Fresh release J executes KSAM=J..NPUFFS with TSAMP=DT,
    // old puffs advance one full hour in this explicitly restricted branch.
    // Until true COMP MXSAM/XSAMLEN and POINTS1/SWAP order are accepted,
    // this method REQUIRES max_sampling_steps=1, no split, and constant
    // point source (no building, variable emissions or MTAULY).
    [[nodiscard]] RiseCadencedHour advance_hour_point_cadence(
        const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,const DispersionSetup& pg,
        bool urban,const OriginalPointHourSource& source);
    [[nodiscard]] OriginalPuffMotionState motion(int id)const;
    [[nodiscard]] NativeVerticalPuffState vertical(int id)const;
    [[nodiscard]] std::size_t puff_steps(int id)const;
    [[nodiscard]] std::size_t completed_hours()const noexcept;
    [[nodiscard]] std::size_t active_count()const noexcept;
    [[nodiscard]] const std::vector<int>& canonical_ids()const noexcept;
    void retire(int id);
private:
    OriginalCompRisePointDriver puffs_;
    std::vector<int> active_ids_;
    std::size_t hours_=0;
    int last_id_=0;
};
} // namespace cteas::puff
