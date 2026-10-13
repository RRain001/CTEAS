#pragma once
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace cteas::puff {

// One puff substep within original CALPUFF COMP, frozen 2022 METFM=1 path.
// Original source anchors are independently checked in the Python DGM oracle.
// This is a sequencing guard, NOT a replacement for the missing full driver.
// CHEM/WET/DRY and CALCPF between SETPUF and late lid are not yet integrated.
enum class VendorSubstepPhase {
    SamplingCadence,         // COMP L10347
    LoadPreviousPuffState,   // COMP L10473
    ResolveMetCell,          // COMP L10487
    EvaluatePuffDz,         // COMP L10495
    EvaluateAdvect,         // COMP L10499
    ApplyRiseWindAndCalm,   // COMP L10528
    ChooseSubstepLength,    // COMP L10548-10582
    AdvancePosition,        // COMP L10607-10619
    ApplyVmass,             // COMP L10859 (pre-SETPUF)
    CommitSetpuf,           // COMP L10940-10942
    CommitNextStepHistory   // COMP L11391-11398, post-CALCPF
};

inline constexpr std::array<VendorSubstepPhase, 11> kVendorSubstepOrder{{
    VendorSubstepPhase::SamplingCadence,
    VendorSubstepPhase::LoadPreviousPuffState,
    VendorSubstepPhase::ResolveMetCell,
    VendorSubstepPhase::EvaluatePuffDz,
    VendorSubstepPhase::EvaluateAdvect,
    VendorSubstepPhase::ApplyRiseWindAndCalm,
    VendorSubstepPhase::ChooseSubstepLength,
    VendorSubstepPhase::AdvancePosition,
    VendorSubstepPhase::ApplyVmass,
    VendorSubstepPhase::CommitSetpuf,
    VendorSubstepPhase::CommitNextStepHistory,
}};

// A future original-equivalent C++ orchestrator MUST advance this guard
// *only after* the matching physics call completes, using the SAME puff state.
// Do not connect the non-vendor P7C absolute-UTM Runge-Kutta prototype here.
class VendorSubstepOrder {
public:
    void completed(VendorSubstepPhase phase) {
        if (next_ >= kVendorSubstepOrder.size() || phase != kVendorSubstepOrder[next_])
            throw std::logic_error("P7 original COMP phase order violation");
        ++next_;
    }
    [[nodiscard]] bool complete() const noexcept { return next_ == kVendorSubstepOrder.size(); }
    [[nodiscard]] std::size_t next_index() const noexcept { return next_; }
    void finish() const {
        if (!complete()) throw std::logic_error("P7 original COMP substep incomplete");
    }
    void begin_next_substep() {
        finish();
        next_ = 0U;
    }
private:
    std::size_t next_ = 0U;
};
} // namespace cteas::puff
