#pragma once

#include "cteas/puff/advection.hpp"
#include "cteas/puff/original_met_hours.hpp"

#include <cstddef>
#include <vector>

namespace cteas::puff {
// CALPUFF original XOLD/YOLD are REAL*4 outer-grid cell coordinates.
// Selected 2022 domain is [0,nx) x [0,ny); C++ index is 0-based,
// original GETCELL IXS/IYS are 1-based. Off-grid puffs fail closed.
struct OriginalMetCell {
    int ix_1based=0, iy_1based=0;
    std::size_t offset_0based=0;
};
[[nodiscard]] OriginalMetCell original_met_cell(
    float x_cells,float y_cells,int nx,int ny);

// Extracts METFM=1 vertical column in the original Fortran level order and
// calls previously source-verified ADVECT/ADVWND (REAL*4). This only proves
// the cell-profile path, NOT full GETCELL inter-cell interpolation or PUFFDZ.
[[nodiscard]] AdvectiveWind original_met_cell_advect(
    const OriginalMetHour& hour,const Calmet21Static& statics,
    OriginalMetCell cell,float zbot_m,float ztop_m);
} // namespace cteas::puff
