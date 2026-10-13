#pragma once

#include "cteas/puff/original_comp_neutral.hpp"
#include "cteas/puff/original_rise_met.hpp"
#include <cstddef>
#include <unordered_map>

namespace cteas::puff {

// Restricted REAL*4 point-puff COMP integration WITH native Briggs GRISE and
// RISEWIND coupled to actual METFM1 winds and native PG SIGTY/SIGTZ, SETPUF
// position, VMASS and historical lids. No original DGM driving inputs.
// This covers one complete noncalm / one-sample / no-building puff substep,
// not original POINTS1/SWAP/MXSAM multi-substep scheduling or frozen Hefei
// numerical parity. PRFIN/PRFPP release parameters must be native inputs.
struct RiseCompStep {
    OriginalPuffMotionState before;
    OriginalPuffMotionState after;
    NativeVerticalPuffState vertical_after;
    NativeVerticalWind initial_met;
    OriginalRiseMetResult rise;
    SamplingCadence cadence;
    Substep substep;
    StepBookkeeping bookkeeping;
    SigmaUpdateResult sigma;
    VmassResult vmass;
    float final_height_m=0.0F;
};

class OriginalCompRisePointDriver {
public:
    void admit(int id,const NewPuffInputs& source,
               const InjectionInputs& release_mass);
    [[nodiscard]] RiseCompStep step_pg_rise(
        int id,const OriginalMetHour& met,const Calmet21Static& statics,
        const NeutralCompControls& controls,
        const DispersionSetup& pg,bool urban);
    [[nodiscard]] OriginalPuffMotionState motion(int id)const;
    [[nodiscard]] NativeVerticalPuffState vertical(int id)const;
    [[nodiscard]] std::size_t steps(int id)const;
    [[nodiscard]] std::size_t active_count()const noexcept;
    void retire(int id);
private:
    struct RiseSource {
        PlumeRiseState physics;
        float x_final_m=0.0F;
        float previous_wind_ms=0.0F;
        std::size_t completed=0U;
    };
    OriginalPuffMotionLifecycle motion_;
    OriginalVerticalLifecycle vertical_;
    std::unordered_map<int,RiseSource> rise_;
};

} // namespace cteas::puff
