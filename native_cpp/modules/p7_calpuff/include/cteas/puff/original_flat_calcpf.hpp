#pragma once

#include "cteas/puff/original_comp_rise.hpp"
#include "cteas/puff/receptors.hpp"

namespace cteas::puff {

// Restricted, deliberately non-oracle flat-grid CALCPF integration.
// This connects native COMP's per-substep hook to the independently
// source-ported PFSCRN -> PFSAMP -> VCOUP -> CALCPF horizontal kernel.
// It is valid ONLY for gridded ground-level receptors, MESHDN=1,
// MCTADJ=0, no plume PDF, no receptor-specific PUFRECS adjustment,
// no settling/PRIME and M(CHEM,WET,DRY)=0. Unsupported modes must
// not be labeled full 2022 CALPUFF parity.
//
// The vendor uses sampling-grid XY indices for PFSAMP, but METRES for
// 1/(2*pi*sigma_y^2). The returned callback is pure: results are
// staged by COMP and committed only at the successful hour boundary.
[[nodiscard]] RiseCompSamplingKernel make_original_flat_calcpf_sampler(
    const ReceptorSet& receptors, float met_spacing_m,
    float averaging_period_s, float impact_sigma_multiple = 3.0F);

} // namespace cteas::puff
