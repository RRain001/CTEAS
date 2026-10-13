#pragma once

#include "cteas/puff/original_comp_rise.hpp"
#include "cteas/puff/original_release_wind.hpp"
#include "cteas/puff/original_rolldn.hpp"
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
// Original 99-slot (old/new) sampling plan + actual rising COMP physics.
struct OriginalCompMxsamSubstep {
    int puff_id=0;
    int release_number=0;
    int ksam=0;
    SamplingCadence original_plan;
    RiseCompStep step;
};
struct OriginalCompMxsamHour {
    std::size_t hour_index=0;
    ReleaseCadence release;
    std::vector<OriginalCompMxsamSubstep> steps;
    std::size_t active_puffs=0;
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
    // First native original POINTS1/WSNEW path. Source must contain
    // independently computed native PRFIN HEFF; only selected original
    // METFM1 ZFIND/WINDSET/RLSMET speed is re-evaluated here.
    // Source.cadence.wind_speed_ms is never used as the physical input.
    [[nodiscard]] RiseCadencedHour advance_hour_point_native_wsnew(
        const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,const DispersionSetup& pg,
        bool urban,const OriginalPointHourSource& source,
        float calm_threshold_ms);
    // Bounded source-ordered MXSAM=99 branch for constant POINTS1 puff:
    // real old pre-rise ADVECT WS -> NSAM, all old/new KSAM slots and
    // native physical PG/GRISE/VMASS, full-hour atomic commit. FSTEP>1,
    // CALM and off-grid stop FAIL CLOSED until original resplit/ROLLDN.
    // Original POINTS1 HTSTAK/HEFF WINDSET -> WSNEW -> MXSAM=99 COMP.
    // Unlike the lower-level unsplit API, ignores externally prefilled
    // source.cadence.wind_speed_ms. Native PRFIN HEFF remains caller-provided.
    // Still fails closed on vendor FSTEP split, calm old-puff, off-grid cases.
    [[nodiscard]] OriginalCompMxsamHour advance_hour_point_mxsam99_native_wsnew_unsplit(
        const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,const DispersionSetup& pg,
        bool urban,const OriginalPointHourSource& source,
        float calm_threshold_ms);
    // Atomic vendor slot compaction + native release wind + restricted COMP.
    // inactive_old_ids is an EXPLICIT, previously diagnosed list; this
    // overload does not claim autonomous off-domain/zero-mass detection.
    // If the subsequent COMP hour fails (including unsupported FSTEP),
    // ROLLDN retirement, physical states and the hour counter ALL roll back.
    [[nodiscard]] OriginalCompMxsamHour
    advance_hour_point_mxsam99_native_wsnew_unsplit_rolldn(
        const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,const DispersionSetup& pg,
        bool urban,const OriginalPointHourSource& source,
        float calm_threshold_ms,const std::vector<int>& inactive_old_ids);
    [[nodiscard]] OriginalCompMxsamHour advance_hour_point_mxsam99_unsplit(
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
    // Fortran ROLLDN/SWAP slot compaction with native puff state retained
    // under stable episode IDs. Caller supplies codes marked inactive by
    // native off-grid, zero-mass or retirement checks; this does NOT yet
    // perform original autonomous 143h exit detection.
    [[nodiscard]] std::size_t compact_original_inactive(
        const std::vector<int>& inactive_state_ids);
private:
    OriginalCompRisePointDriver puffs_;
    std::vector<int> active_ids_;
    std::size_t hours_=0;
    int last_id_=0;
};
} // namespace cteas::puff
