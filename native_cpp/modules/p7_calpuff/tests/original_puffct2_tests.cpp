#include "cteas/puff/original_puffct2.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace cteas::puff;
static void require(bool ok,const char* what){
    if(!ok)throw std::runtime_error(what);
}
int main() {
    try {
        OriginalPuffct2Step s;
        s.x_begin_m=100.0F;s.y_begin_m=200.0F;s.z_begin_m=50.0F;
        s.x_end_m=200.0F;s.y_end_m=300.0F;s.z_end_m=20.0F;
        s.terrain_fraction=0.4F;s.upslope=true;
        const auto a=original_puffct2(s,0.2F);
        const auto end=original_puffct2(s,0.8F);
        require(a.x_m==120.0F && a.y_m==220.0F &&
                a.z_m==35.0F && end.z_m==20.0F,
                "uphill fractional fall/plateau changed");
        s.upslope=false;
        const auto pre=original_puffct2(s,0.2F);
        const auto post=original_puffct2(s,0.7F);
        require(pre.z_m==50.0F &&
                std::abs(post.z_m-35.0F)<1.e-5F,
                "downhill delayed height transition changed");
        s.upslope=true;s.terrain_fraction=0.0F;
        require(original_puffct2(s,0.5F).z_m==50.0F,
                "upslope no terrain response must retain initial height");
        s.upslope=false;s.terrain_fraction=1.0F;
        require(original_puffct2(s,0.5F).z_m==50.0F,
                "downslope no terrain response must retain initial height");
        s.upslope=true;s.terrain_fraction=0.4F;
        const auto once=original_puffct2(s,-0.3F);
        const auto twice=original_puffct2(s,-0.3F);
        require(std::bit_cast<std::uint32_t>(once.z_m)==
                std::bit_cast<std::uint32_t>(twice.z_m),
                "FRAC extrapolation must preserve bitwise determinism");
        bool refused=false;
        s.terrain_fraction=std::numeric_limits<float>::quiet_NaN();
        try{(void)original_puffct2(s,0.5F);}
        catch(const std::invalid_argument&){refused=true;}
        require(refused,"PUFFCT2 silently accepted missing/NaN CTADJ2 fraction");
        std::cout<<"P7_ORIGINAL_PUFFCT2_NATIVE_UNIT=PASS"
                 <<" UPSLOPE_DOWNSLOPE=YES FRAC_EXTRAPOLATE=YES"
                 <<" FINITE_STATE_REQUIRED=YES NO_ORACLE_INPUT=YES\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"P7_ORIGINAL_PUFFCT2_NATIVE_UNIT=FAIL "<<e.what()<<"\n";
        return 1;
    }
}
