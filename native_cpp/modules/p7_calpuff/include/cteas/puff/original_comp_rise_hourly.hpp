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
class OriginalCompRiseMultiPuffHours {
public:
    [[nodiscard]] RiseCompHour advance_hour(
        const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,
        const DispersionSetup& pg,bool urban,
        const std::vector<RisePointAdmission>& admissions);
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
