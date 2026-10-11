#include "cteas/puff/ctsg.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace cteas::puff {
namespace {

using R = float;   // the vendor QATR/TFAC state is REAL*4

constexpr R kHalf = 0.5F;
constexpr R kOne = 1.0F;
constexpr int kJjLimit = 16000;   // QATR integer*2 guard

void require_true(bool condition, const std::string& message) {
    if (!condition) throw std::invalid_argument("P7I CTSG support: " + message);
}

void require_finite(double value, const std::string& what) {
    require_true(std::isfinite(value), what + " must be finite");
}

} // namespace

QuadratureResult integrate_romberg(double lower, double upper, double eps, int dimension,
                                   const std::function<double(double)>& integrand) {
    require_finite(lower, "lower limit");
    require_finite(upper, "upper limit");
    require_finite(eps, "fractional tolerance");
    require_true(eps > 0.0, "fractional tolerance must be positive");
    require_true(dimension >= 1, "dimension must be at least 1");
    require_true(static_cast<bool>(integrand), "integrand must be provided");

    // QATR (calpuff.for L25406-25520), REAL*4 with a Romberg extrapolation table.
    // The vendor arguments are REAL*4, so the limits are rounded to single
    // precision before they reach the integrand (this is observable for
    // integrands like sin() near pi and is required for exact parity).
    const R lower_r = static_cast<R>(lower);
    const R upper_r = static_cast<R>(upper);
    std::vector<R> aux(static_cast<std::size_t>(dimension), 0.0F);
    QuadratureResult result;
    aux[0] = kHalf * static_cast<R>(integrand(static_cast<double>(lower_r)) +
                                   integrand(static_cast<double>(upper_r)));
    const R width = upper_r - lower_r;
    R value = width * aux[0];
    if (dimension <= 1) {
        result.value = value;
        result.status = QuadratureStatus::no_convergence;
        return result;
    }
    if (width == 0.0F) {
        result.value = value;
        result.status = QuadratureStatus::converged;
        return result;
    }

    R hh = width;
    R delt2 = 0.0F;
    R p = 1.0F;
    int jj = 1;
    bool stopped = false;
    for (int i = 2; i <= dimension; ++i) {
        value = aux[0];
        const R delt1 = delt2;
        const R hd = hh;
        hh = kHalf * hh;
        p = kHalf * p;
        R x = lower_r + hh;
        R sum = 0.0F;
        if (stopped) {
            throw std::invalid_argument(
                "P7I CTSG support: QATR interval count exceeded 32000 (vendor halts)");
        }
        if (jj > kJjLimit) stopped = true;
        for (int j = 1; j <= jj; ++j) {
            sum += static_cast<R>(integrand(static_cast<double>(x)));
            x += hd;
        }
        aux[static_cast<std::size_t>(i - 1)] = kHalf * aux[static_cast<std::size_t>(i - 2)] + p * sum;

        // Romberg extrapolation.
        R q = 1.0F;
        for (int j = 1; j <= i - 1; ++j) {
            const int index = i - j;
            q = q + q;
            q = q + q;
            aux[static_cast<std::size_t>(index - 1)] =
                aux[static_cast<std::size_t>(index)] +
                (aux[static_cast<std::size_t>(index)] - aux[static_cast<std::size_t>(index - 1)]) /
                    (q - 1.0F);
        }

        delt2 = std::abs(value - aux[0]);
        if (i >= 5) {
            if (value == 0.0F) {
                result.value = static_cast<double>(value);
                result.status = QuadratureStatus::converged;
                return result;
            }
            if ((delt2 / value) <= static_cast<R>(eps)) {
                result.value = static_cast<double>(width * aux[0]);
                result.status = QuadratureStatus::converged;
                return result;
            }
            if (delt2 >= delt1) {
                result.value = static_cast<double>(width * value);
                result.status = QuadratureStatus::diverging;
                return result;
            }
        }
        jj = jj + jj;
    }
    result.value = static_cast<double>(width * aux[0]);
    result.status = QuadratureStatus::no_convergence;
    return result;
}

TerrainFactors terrain_factors(double x_m, double begin_m, double end_m, double spacing_m,
                               const std::vector<double>& top_factors,
                               const std::vector<double>& lateral_factors) {
    require_finite(x_m, "position");
    require_finite(begin_m, "begin position");
    require_finite(end_m, "end position");
    require_finite(spacing_m, "spacing");
    require_true(spacing_m > 0.0, "spacing must be positive");
    require_true(end_m > begin_m, "end position must exceed the begin position");
    require_true(!top_factors.empty() && top_factors.size() == lateral_factors.size(),
                 "terrain factor arrays must be non-empty and of equal length");

    // TFAC (calpuff.for L25765-25813), REAL*4.
    TerrainFactors factors;
    const R x = static_cast<R>(x_m);
    const R begin = static_cast<R>(begin_m);
    const R end = static_cast<R>(end_m);
    const R spacing = static_cast<R>(spacing_m);
    if (!(x > begin && x < end)) return factors;
    const int previous = static_cast<int>((x - begin) / spacing);
    const int position = previous + 1;
    const int next = position + 1;
    require_true(position >= 1 && static_cast<std::size_t>(next) <= top_factors.size(),
                 "terrain factor arrays are too short for this position");
    const R grid_x = begin + spacing * static_cast<R>(previous);
    const R fraction = (x - grid_x) / spacing;
    const auto index = [](int one_based) { return static_cast<std::size_t>(one_based - 1); };
    const R top = fraction * static_cast<R>(top_factors[index(next)] -
                                            top_factors[index(position)]) +
                  static_cast<R>(top_factors[index(position)]);
    const R lateral = fraction * static_cast<R>(lateral_factors[index(next)] -
                                               lateral_factors[index(position)]) +
                      static_cast<R>(lateral_factors[index(position)]);
    require_true(top != 0.0 && lateral != 0.0, "terrain factors must not be zero");
    factors.top = static_cast<double>(top);
    factors.lateral = static_cast<double>(lateral);
    factors.total = static_cast<double>(kOne / (top * lateral));
    return factors;
}

} // namespace cteas::puff
