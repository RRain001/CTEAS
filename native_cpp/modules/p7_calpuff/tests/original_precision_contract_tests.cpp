#include "cteas/puff/original_precision.hpp"
#include "cteas/puff/original_grid_coords.hpp"
#include "cteas/puff/advection.hpp"
#include "cteas/puff/release.hpp"
#include "cteas/puff/step.hpp"
#include "cteas/puff/step_integration.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

using namespace cteas::puff;
static_assert(std::is_same_v<ReleaseReal, VendorPuffCommonReal>);
static_assert(std::is_same_v<AdvectionReal, VendorPuffCommonReal>);
static_assert(std::is_same_v<StepReal, VendorPuffCommonReal>);
static_assert(std::is_same_v<VendorPointCommonReal, float>);
static_assert(std::is_same_v<VendorPuffCommonReal, float>);
static_assert(std::is_same_v<VendorExplicitDouble, double>);
static_assert(std::is_same_v<VendorDefaultInteger, std::int32_t>);

int main() {
    try {
        if (sizeof(ReleaseReal) != 4U || sizeof(VendorExplicitDouble) != 8U ||
            sizeof(VendorDefaultInteger) != 4U)
            throw std::runtime_error("type ABI does not match original Fortran");
        const auto rounded_source = original_real_from_text(3533.769);
        const OriginalMetGridFrame frame{510885.0F, 3531269.0F, 100.0F};
        const auto qa = original_point_qa_xy(
            frame, original_real_from_text(513.385),
            rounded_source, original_real_from_text(510.885),
            original_real_from_text(3531.269));
        // Verify the original two-step precision boundary, not merely that
        // a float type exists. This difference is visible in vendor QA.
        if (rounded_source != 3533.76904296875F ||
            qa.y_km != 3533.769287109375F ||
            qa.x_km != 513.385009765625F ||
            qa.y_km == rounded_source)
            throw std::runtime_error("original REAL4 source/origin rounding has changed");
        std::cout << "P7_ORIGINAL_CPP_PRECISION=PASS"
                     " COMMON_REAL_BYTES=4 EXPLICIT_DOUBLE_BYTES=8"
                     " COMMON_INTEGER_BYTES=4 REAL4_SOURCE_VS_MET_ORIGIN=DIFFERENT"
                     " WHOLE_PUFF_PARITY=NOT_TESTED\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "P7_ORIGINAL_CPP_PRECISION=FAIL " << e.what() << '\n';
        return 1;
    }
}
