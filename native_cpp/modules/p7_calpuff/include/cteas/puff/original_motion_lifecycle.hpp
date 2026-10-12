#pragma once

#include "cteas/puff/original_precision.hpp"
#include <cstddef>
#include <unordered_map>

namespace cteas::puff {

// Real*4 state stored between consecutive original COMP sampling substeps.
// Coordinates are MET-GRID CELLS, NOT absolute UTM kilometers.
// This covers ADVECT -> SETPUF position, distance, travel-time state only.
// The upstream PUFFDZ/ADVECT wind choice and downstream sigma/CTSG/CONC
// are separate unimplemented integrated-driver dependencies.
struct OriginalPuffMotionState {
    VendorPuffCommonReal x_cells = 0.0F;
    VendorPuffCommonReal y_cells = 0.0F;
    VendorPuffCommonReal z_m = 0.0F;
    VendorPuffCommonReal total_distance_m = 0.0F;
    VendorPuffCommonReal travel_time_s = 0.0F;
};

struct OriginalPostWindInputs {
    VendorPuffCommonReal uadv_ms = 0.0F;
    VendorPuffCommonReal vadv_ms = 0.0F;
    VendorPuffCommonReal ws_ms = 0.0F;
    VendorPuffCommonReal tsamp_s = 0.0F;
    VendorPuffCommonReal met_spacing_m = 0.0F;
    VendorPuffCommonReal end_height_m = 0.0F;  // SETPUF ZE1 from separately audited rise
    VendorPuffCommonReal settling_velocity_ms = 0.0F;
    int plume_tilt = 0;
};

struct OriginalPostWindResult {
    OriginalPuffMotionState state;
    VendorPuffCommonReal distance_m = 0.0F;
};

// State-preserving native C++ COMP post-wind slice. Both physical operations
// are the separately source-verified P7X ADVECT/position and P7P SETPUF
// implementations; it does not copy xnew, distance, age from a Fortran oracle.
class OriginalPuffMotionLifecycle {
public:
    void introduce(int puff_id, OriginalPuffMotionState initial);
    [[nodiscard]] bool contains(int puff_id) const noexcept;
    [[nodiscard]] OriginalPuffMotionState state(int puff_id) const;
    [[nodiscard]] OriginalPostWindResult advance(int puff_id, OriginalPostWindInputs inputs);
    void retire(int puff_id);
    [[nodiscard]] std::size_t active_count() const noexcept;
private:
    std::unordered_map<int, OriginalPuffMotionState> puffs_;
};

} // namespace cteas::puff
