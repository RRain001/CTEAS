#pragma once

namespace cteas::puff {

// The REAL*4 variables in original CALPUFF /CURRENT/ consumed by
// PUFFCT2 (calpuff.for L21035-21096). Positions are METRES relative to
// the CALMET computational origin, NOT sampling-grid indices.
// FRACZ1/LUP1 must come from native CTADJ2/SETPUF; inventing them from
// receptor terrain would break the original step history.
struct OriginalPuffct2Step {
    float x_begin_m=0.0F, y_begin_m=0.0F, z_begin_m=0.0F;
    float x_end_m=0.0F, y_end_m=0.0F, z_end_m=0.0F;
    float terrain_fraction=0.0F;   // /CURRENT/ FRACZ1
    bool upslope=false;            // /CURRENT/ LUP1
};
struct OriginalPuffct2Result {
    float x_m=0.0F, y_m=0.0F, z_m=0.0F;
};

// Exact source-ordered standalone PUFFCT2 port. FRAC can extend beyond
// [0,1] because PUFRECS performs FCLIP/extrapolation; do not clamp it.
// This solves receptor-specific puff HEIGHT only, not PUFRECS MCTADJ=2
// pole-height/relief adjustments or CTADJ2 terrain state production.
[[nodiscard]] OriginalPuffct2Result original_puffct2(
    const OriginalPuffct2Step& step, float fraction);

} // namespace cteas::puff
