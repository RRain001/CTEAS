#include "cteas/puff/vendor_order.hpp"
#include <iostream>
#include <stdexcept>

namespace {
template <typename F> void throws(F f, const char* reason) {
    try { f(); } catch (const std::logic_error&) { return; }
    throw std::runtime_error(reason);
}
}
int main() {
    try {
        using namespace cteas::puff;
        static_assert(kVendorSubstepOrder.size() == 9U);
        VendorSubstepOrder order;
        throws([&]{ order.completed(VendorSubstepPhase::EvaluateAdvect); },
               "advection accepted before cadence");
        throws([&]{ order.finish(); }, "incomplete puff falsely accepted");
        for (auto phase : kVendorSubstepOrder) order.completed(phase);
        order.finish();
        throws([&]{ order.completed(VendorSubstepPhase::CommitSetpuf); },
               "repeated SETPUF accepted");
        order.begin_next_substep();
        order.completed(VendorSubstepPhase::SamplingCadence);
        throws([&]{ order.completed(VendorSubstepPhase::SamplingCadence); },
               "duplicated cadence accepted");
        throws([&]{ order.completed(VendorSubstepPhase::CommitSetpuf); },
               "SETPUF jumped ahead of wind and position");
        for (std::size_t i=1; i<kVendorSubstepOrder.size(); ++i)
            order.completed(kVendorSubstepOrder[i]);
        order.finish();
        std::cout << "P7_VENDOR_COMP_ORDER_CONTRACT=PASS"
                  << " PHASES=9 OUT_OF_ORDER=REJECTED"
                  << " PHYSICS_DRIVER_PARITY=NOT_YET_TESTED\n";
    } catch (const std::exception& e) {
        std::cerr << "P7_VENDOR_COMP_ORDER_CONTRACT=FAIL " << e.what() << '\n';
        return 1;
    }
}
