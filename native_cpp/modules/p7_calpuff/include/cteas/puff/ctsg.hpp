#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace cteas::puff {

// P7I: CTSG support blocks of CALPUFF 7.2.1, transcribed from the unmodified
// vendor source and cross-checked against it by CTest:
//   QATR  calpuff.for L25406-25520  (Romberg integrator used by UPPER/LOWER)
//   TFAC  calpuff.for L25765-25813  (terrain flow-distortion interpolation)
//
// These are the parts of the CTSG dependency closure that are self-contained
// and can be closed with original-source parity. The remaining CTSG closure
// (CTSG/CTREC/CTPAR/MUNU/LOWER/UPPER plus BI/BJIL/FLOPUFF and the /CTPASS/
// state) is listed precisely in the reconstruction ledger.

// QATR status flags, as documented in the vendor routine.
enum class QuadratureStatus : int {
    converged = 0,   // IER=0
    diverging = 1,   // IER=1
    no_convergence = 2, // IER=2
};

struct QuadratureResult {
    double value = 0.0;
    QuadratureStatus status = QuadratureStatus::no_convergence;
};

// QATR equivalent: Romberg integration of `integrand` over [lower, upper] with
// fractional tolerance `eps` and at most `dimension` extrapolation levels.
// `dimension` is bounded like the vendor's integer*2 guard (jj <= 32000).
[[nodiscard]] QuadratureResult integrate_romberg(double lower, double upper, double eps,
                                                 int dimension,
                                                 const std::function<double(double)>& integrand);

// TFAC equivalent: interpolate the terrain flow-distortion factors at position
// `x` along the trajectory. Outside [begin, end] the factors are unity, exactly
// as the vendor routine does.
struct TerrainFactors {
    double top = 1.0;     // TH
    double lateral = 1.0; // TL
    double total = 1.0;   // TU = 1/(TH*TL)
};

// ROTATE equivalent (calpuff.for L25522-25549): rotate (x, y) through `angle`
// radians, the coordinate transform CTREC/CTPAR use.
struct RotatedPoint {
    double x = 0.0;
    double y = 0.0;
};

[[nodiscard]] RotatedPoint rotate_xy(double x, double y, double angle_rad);

[[nodiscard]] TerrainFactors terrain_factors(double x_m, double begin_m, double end_m,
                                             double spacing_m,
                                             const std::vector<double>& top_factors,
                                             const std::vector<double>& lateral_factors);

} // namespace cteas::puff
