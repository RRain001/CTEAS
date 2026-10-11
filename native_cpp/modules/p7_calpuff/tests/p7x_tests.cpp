// P7X contract test: the ported step integration against hand-computed vendor
// semantics, plus every fail-closed refusal.
#include "cteas/puff/step_integration.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const std::string& what) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << what << '\n';
    }
}

void check_close(double got, double want, double tolerance, const std::string& what) {
    ++checks;
    if (!(std::abs(got - want) <= tolerance)) {
        ++failures;
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << '\n';
    }
}

template <typename Callable>
void check_refuses(Callable&& callable, const std::string& what) {
    ++checks;
    try {
        callable();
        ++failures;
        std::cerr << "FAIL: " << what << " did not refuse\n";
    } catch (const std::invalid_argument&) {
        // expected
    }
}

} // namespace

int main() {
    using namespace cteas::puff;

    // New point-source puff: NSAM = NINT(SECDT/TEMIT0), TSAMP = TEMIT0, NSSTR = IPNO.
    SamplingCadenceInputs cadence;
    cadence.new_puff = true;
    cadence.source_type = 1;
    cadence.emission_period_s = 3600.0 / 54.0;
    cadence.step_seconds = 3600.0;
    cadence.release_number = 1;
    cadence.grid_spacing_m = 100.0;
    auto computed = sampling_cadence(cadence);
    check(computed.sampling_steps == 54, "new puff NSAM = NINT(SECDT/TEMIT0)");
    check_close(computed.sampling_step_s, 3600.0 / 54.0, 1.0e-3, "new puff TSAMP = TEMIT0");
    check_close(computed.reference_step_s, computed.sampling_step_s, 1.0e-9, "REFTSAMP = TSAMP");
    check(computed.start_substep == 1, "NSSTR = IPNO");
    check(computed.old_flag == 0, "IOLD = 0 for a new puff");
    check(computed.subsampling, "LSUBSAM holds while NSAM < MXSAM");
    cadence.release_number = 7;
    check(sampling_cadence(cadence).start_substep == 7, "NSSTR follows IPNO");

    // Old puff: NSAM = NINT(WS*SECDT/(XSAMLEN*XGRIDM) + 1), clamped by MXSAM.
    SamplingCadenceInputs old;
    old.new_puff = false;
    old.step_seconds = 3600.0;
    old.wind_speed_ms = 1.5;
    old.grid_spacing_m = 100.0;
    old.x_sam_len = 1.0;
    computed = sampling_cadence(old);
    // SMXI2 = SECDT/(XSAMLEN*XGRIDM) = 36, so NSAM = NINT(1.5*36+1) = 55.
    check(computed.sampling_steps == 55, "old puff NSAM = NINT(WS*SMXI2 + 1)");
    check_close(computed.sampling_step_inverse, 1.0 / 55.0, 1.0e-6, "old puff XSAMI = 1/NSAM");
    check_close(computed.sampling_step_s, 3600.0 / 55.0, 1.0e-2, "old puff TSAMP = SECDT*XSAMI");
    check(computed.start_substep == 1, "old puffs start at substep 1");
    check(computed.subsampling, "LSUBSAM holds while NSAM < MXSAM");
    old.wind_speed_ms = 0.01;   // NSAM = NINT(0.01*36+1) = 1
    computed = sampling_cadence(old);
    check(computed.sampling_steps == 1, "old puff NSAM floors at 1");
    check_close(computed.sampling_step_s, 3600.0, 1.0e-2, "old puff TSAMP = SECDT/NSAM");
    old.wind_speed_ms = 100.0;  // NSAM = NINT(100*36+1) -> clamped by MXSAM
    computed = sampling_cadence(old);
    check(computed.sampling_steps == 99, "old puff NSAM is clamped by MXSAM");
    check(!computed.subsampling, "LSUBSAM is off once NSAM reaches MXSAM");

    // Substep length: FSTEP = WS*TSAMP/(XSAMLEN*XGRIDM).
    SubstepInputs substep;
    substep.sampling_step_s = 3600.0 / 54.0;
    substep.wind_speed_ms = 1.5;
    substep.puff = true;
    substep.subsampling = true;
    substep.grid_spacing_m = 100.0;
    substep.x_sam_len = 1.0;
    substep.substep_index = 1;
    substep.sampling_steps = 54;
    auto step = substep_length(substep);
    check_close(step.step_distance, 1.0, 1.0e-5, "FSTEP for the deck's first substep");
    check(!step.split, "no split when FSTEP <= 1");
    check_close(step.step_s, 3600.0 / 54.0, 1.0e-3, "the substep keeps TSAMP");

    substep.wind_speed_ms = 3.0;      // FSTEP = 2.0 -> a full XSAMLEN substep
    step = substep_length(substep);
    check(step.split, "FSTEP >= 2 splits the substep");
    check_close(step.step_s, (3600.0 / 54.0) / 2.0, 1.0e-2, "TSAMP is divided by FSTEP");

    substep.wind_speed_ms = 2.25;     // FSTEP = 1.5, last substep -> two halves
    substep.substep_index = 54;
    substep.sampling_steps = 54;
    step = substep_length(substep);
    check(step.split, "1 < FSTEP < 2 on the last substep splits");
    check_close(step.step_s, 0.5 * (3600.0 / 54.0), 1.0e-2, "the last substep halves TSAMP");

    substep.substep_index = 10;       // not the last substep -> a full XSAMLEN
    step = substep_length(substep);
    check_close(step.step_s, (3600.0 / 54.0) / 1.5, 1.0e-2, "a non-final substep divides by FSTEP");

    // An emitted-now slug is not altered; a non-subsampled step is not either.
    substep.free_slug = true;
    substep.puff = false;
    substep.wind_speed_ms = 3.0;
    check(substep_length(substep).split, "a free slug still takes substeps");
    substep.free_slug = false;
    check(!substep_length(substep).split, "a slug attached to the source is not altered");
    substep.free_slug = true;
    substep.subsampling = false;
    check(!substep_length(substep).split, "LSUBSAM off means no substeps");

    // Position update: DX = U*TSAMP in meters, then in outermost grid units.
    AdvanceInputs advance;
    advance.x_cells = 25.0;
    advance.y_cells = 25.0;
    advance.u_ms = -0.5793;
    advance.v_ms = 1.3557;
    advance.step_s = 66.666666;
    advance.grid_spacing_m = 100.0;
    advance.wind_speed_ms = std::hypot(-0.5793, 1.3557);
    const auto moved = advance_puff(advance);
    check_close(moved.x_cells, 25.0 + (-0.5793 * 66.666666) / 100.0, 1.0e-5, "XNEW");
    check_close(moved.y_cells, 25.0 + (1.3557 * 66.666666) / 100.0, 1.0e-5, "YNEW");
    check_close(moved.distance_m, advance.wind_speed_ms * 66.666666, 1.0e-4, "DISTM = WS*TSAMP");
    check_close(moved.dx_cells, (-0.5793 * 66.666666) / 100.0, 1.0e-6, "DXOLD in grid units");

    // Fail-closed refusals.
    SamplingCadenceInputs bad = cadence;
    bad.source_type = 2;
    check_refuses([&] { (void)sampling_cadence(bad); }, "refuses ISTYPE=2 (VARSAM)");
    bad = cadence;
    bad.emission_period_s = 0.0;
    check_refuses([&] { (void)sampling_cadence(bad); }, "refuses a zero emission period");
    bad = cadence;
    bad.step_seconds = 0.0;
    check_refuses([&] { (void)sampling_cadence(bad); }, "refuses a zero time step");
    bad = cadence;
    bad.grid_spacing_m = 0.0;
    check_refuses([&] { (void)sampling_cadence(bad); }, "refuses a zero grid spacing");
    SubstepInputs bad_step = substep;
    bad_step.sampling_step_s = 0.0;
    check_refuses([&] { (void)substep_length(bad_step); }, "refuses a zero TSAMP");
    bad_step = substep;
    bad_step.sampling_steps = 0;
    check_refuses([&] { (void)substep_length(bad_step); }, "refuses NSAM=0");
    AdvanceInputs bad_advance = advance;
    bad_advance.grid_spacing_m = 0.0;
    check_refuses([&] { (void)advance_puff(bad_advance); }, "refuses a zero grid spacing");
    bad_advance = advance;
    bad_advance.step_s = std::nan("");
    check_refuses([&] { (void)advance_puff(bad_advance); }, "refuses a NaN step");

    std::cout << "P7X_STEP_INTEGRATION_CONTRACT=" << (failures == 0 ? "PASS" : "FAIL")
              << " CHECKS=" << checks << " FAILURES=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
