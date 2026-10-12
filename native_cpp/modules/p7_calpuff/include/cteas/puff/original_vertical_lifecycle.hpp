#pragma once

#include "cteas/puff/original_met_cell.hpp"
#include "cteas/puff/release.hpp"
#include "cteas/puff/original_vmass.hpp"
#include "cteas/puff/sigma_update.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace cteas::puff {

// One authoritative native copy of the CALPUFF point-puff vertical COMMON
// variables at the boundary between successive COMP substeps. All values
// are REAL*4, including those returned by source-verified P7O/P7Q routines.
// Original DGM data is never accepted by any production method.
struct NativeVerticalPuffState {
    float sigma_y_m = 0.0F;
    float sigma_z_m = 0.0F;
    float ziold_m = 0.0F;
    float zimax_m = 0.0F;
    int puff_code = 0;
    std::vector<float> mass_mixed;
    std::vector<float> mass_above;
    std::size_t completed_substeps = 0;
    std::size_t committed_lid_steps = 0;
};

struct NativeVerticalWind {
    OriginalMetCell cell;
    PuffExtent extent;
    AdvectiveWind wind;
    int stability_class = 0;
    float mixing_height_m = 0.0F;
};

// This advances the native physical *state connection*, not the complete
// COMP or SETPUF scheduler. After point-source admission, PUFFDZ reads
// persistent native SIGZB/ZIOLD/ZIMAX and ADVECT reads original METFM=1
// input via the already validated CALMET hourly reader. The caller must
// supply independently calculated HTMET (not a DGM/oracle value).
// commit_sigma() uses native SIGTY/SIGTZ via source-ported P7Q; until native
// ZIOLD/ZIMAX historical evolution is ported, changing mixing-height
// histories is intentionally NOT approximated.
class OriginalVerticalLifecycle {
public:
    void admit_point(int puff_id, const NewPuffInputs& source,
                     const InjectionInputs& injection);
    [[nodiscard]] NativeVerticalPuffState state(int puff_id) const;
    [[nodiscard]] NativeVerticalWind sample(
        int puff_id, const OriginalMetHour& met,
        const Calmet21Static& statics, float x_cells, float y_cells,
        float height_m) const;
    // Source-ported full P7Q sigma-step plus adjustment; commits only if
    // the step's initial sigma matches the persisted native state exactly.
    // Failures leave the old state unchanged.
    [[nodiscard]] SigmaUpdateResult commit_sigma(
        int puff_id, const SigmaUpdateInputs& inputs,
        const SigmaYCurve& sigma_y, const SigmaZCurve& sigma_z);
    // COMP calpuff.for 11391-11398, AFTER SETPUF and any VMASS operation:
    //   IPUFCD(II)=ICODE; ZIMAX=AMAX1(ZIMAX,HLID); ZIOLD=HLID.
    // HLID / adjusted puff code MUST be calculated by the native upstream
    // VMASS/COMP branch; this method cannot invent the 2-layer mass transfer.
    // One lid commit for each completed native sigma substep.
    void commit_lid(int puff_id, float native_hlid_m, int native_puff_code);
    // Production VMASS + COMP transition. Uses native state QM/QU and
    // ZIOLD/ZIMAX; never accepts an original DGM state. The supplied
    // height/PBL must come from the independently computed COMP/MET state.
    // Enforces exactly one transition per native sigma update.
    [[nodiscard]] VmassResult commit_vmass_and_lid(
        int puff_id, float native_pbl_m,
        float native_height_begin_m, float native_height_end_m,
        float native_sigma_z_end_m, int native_stability_class,
        int native_plume_tilt);
    void retire(int puff_id);
    [[nodiscard]] std::size_t active_count() const noexcept;
private:
    std::unordered_map<int, NativeVerticalPuffState> puffs_;
};

} // namespace cteas::puff
