#pragma once

#include "cteas/puff/ctsg.hpp"

namespace cteas::puff {

// P7Q: SETPUF's sigma update for the non-PRIME branch, transcribed from the
// unmodified vendor source and cross-checked against it by CTest:
//   begin/mid/end sigma + virtuals   calpuff.for L48505-48536
//   sigma-y final adjustments        calpuff.for L48570-48604
//   sigma-z final adjustments        calpuff.for L48635-48668
//
// The vendor reads and writes the sigma state through /PUFF/ and /CSIGMA/; here
// the dispersion curves are explicit dependencies (SIGTY/SIGTZ carry the P7D
// parity evidence) and the stored sigmas are returned instead of being written
// into a common block. Everything is REAL*4 internally.
//
// NOT reconstructed, and refused by the callers of this module: the PRIME wake
// branch (`WAKE_TAB`, `IDW0` 3/4), the `MCTADJ=2` strain and valley constraints
// (`CTADJ2` L20542-20835, `WALLTRAP` L21745-21894) and the `MHFTSZ=1` Heffter
// branch (`HEFTRAN` L52621-52768).

using SigmaUpdateReal = float;

// The begin/mid/end sigma and virtual computation.
struct SigmaStepInputs {
    double sigma_y_start_m = 0.0;      // SIGYB(II)
    double sigma_z_start_m = 0.0;      // SIGZB(II)
    double puff_height_m = 0.0;        // ZPB(II)
    double mid_time_s = 0.0;           // TMID
    double mid_distance_km = 0.0;      // DMIDKM
    int puff_code = 1;                 // ICODE (odd => Gaussian in the vertical)
};

struct SigmaStep {
    double sigma_y_begin_m = 0.0;      // SYB1
    double sigma_y_mid_m = 0.0;        // SYM1
    double sigma_y_end_m = 0.0;        // SYE1
    double virtual_time_y_begin_s = 0.0;      // VTYB1
    double virtual_time_y_mid_s = 0.0;        // VTYM1
    double virtual_time_y_end_s = 0.0;        // VTYE1
    double virtual_distance_y_begin_km = 0.0; // VDYB1
    double virtual_distance_y_mid_km = 0.0;   // VDYM1
    double virtual_distance_y_end_km = 0.0;   // VDYE1
    double sigma_z_begin_m = 0.0;      // SZB1
    double sigma_z_mid_m = 0.0;        // SZM1
    double sigma_z_end_m = 0.0;        // SZE1
    double virtual_time_z_begin_s = 0.0;      // VTZB1
    double virtual_time_z_mid_s = 0.0;        // VTZM1
    double virtual_time_z_end_s = 0.0;        // VTZE1
    double virtual_distance_z_begin_km = 0.0; // VDZB1
    double virtual_distance_z_mid_km = 0.0;   // VDZM1
    double virtual_distance_z_end_km = 0.0;   // VDZE1
    bool gaussian_vertical = false;    // mod(ICODE,2) == 1
};

[[nodiscard]] SigmaStep sigma_step(const SigmaStepInputs& inputs,
                                   const SigmaYCurve& sigma_y_curve,
                                   const SigmaZCurve& sigma_z_curve);

// The final adjustments: the precision monotonicity fix, the stored sigma for the
// next step, the BID/sy0 quadrature and the calm minimum.
struct SigmaAdjustInputs {
    double sigma_y_begin_m = 0.0;
    double sigma_y_mid_m = 0.0;
    double sigma_y_end_m = 0.0;
    double sigma_z_begin_m = 0.0;
    double sigma_z_mid_m = 0.0;
    double sigma_z_end_m = 0.0;
    double final_rise_variance_m2 = 0.0;   // BIDFNL(II)
    double initial_sigma_y_squared_m2 = 0.0; // SY0SQ
    bool calm = false;                     // LCALM
    bool gaussian_vertical = false;        // mod(ICODE,2) == 1
};

struct SigmaAdjust {
    double sigma_y_begin_m = 0.0;
    double sigma_y_mid_m = 0.0;
    double sigma_y_end_m = 0.0;
    double sigma_z_begin_m = 0.0;
    double sigma_z_mid_m = 0.0;
    double sigma_z_end_m = 0.0;
    double sigma_y_quadrature_m2 = 0.0;    // SYSQ
    double stored_sigma_y_m = 0.0;         // SIGYB(II) for the next step
    double stored_sigma_z_m = 0.0;         // SIGZB(II) for the next step
    double minimum_sigma_y_m = 0.0;        // SY0(II) during calms
    double minimum_sigma_z_m = 0.0;        // SZ0(II) during calms
};

[[nodiscard]] SigmaAdjust adjust_sigmas(const SigmaAdjustInputs& inputs);

// The closed sub-chain: the begin/mid/end sigmas and virtuals followed by the
// adjustments and the stores.
struct SigmaUpdateInputs {
    SigmaStepInputs step;
    double final_rise_variance_m2 = 0.0;
    double initial_sigma_y_squared_m2 = 0.0;
    bool calm = false;
};

struct SigmaUpdateResult {
    SigmaStep step;
    SigmaAdjust adjust;
};

[[nodiscard]] SigmaUpdateResult sigma_update(const SigmaUpdateInputs& inputs,
                                             const SigmaYCurve& sigma_y_curve,
                                             const SigmaZCurve& sigma_z_curve);

} // namespace cteas::puff
