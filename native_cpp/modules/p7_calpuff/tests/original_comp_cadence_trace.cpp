// Original 143-hour REAL*4 COMP sampling-cadence audit.
// Original DGM supplies the independent IOLD/NSAM/IPNO/REFTSAMP comparator
// ONLY. The C++ production sampling_cadence and POINTS1 release_cadence
// calculate the new-puff schedule without reading expected NSAM as input.
// Old-puff apparent divergence from final (post-rise) WS is DIAGNOSTIC ONLY,
// since COMP calculates its old-puff cadence using pre-RISEWIND ADVECT WS.
#include "cteas/puff/release.hpp"
#include "cteas/puff/step_integration.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
using namespace cteas::puff;
struct OriginalRecord {
    std::array<std::int32_t,24> i{};
    std::array<float,64> r{};
};
static_assert(sizeof(OriginalRecord)==352);
static_assert(std::endian::native==std::endian::little);
void need(bool p,const char* reason) {
    if(!p) throw std::runtime_error(reason);
}
std::uint32_t bits(float x) {
    return std::bit_cast<std::uint32_t>(x);
}
int main_run(const std::string& file,const int expected_hours) {
    std::ifstream input(file,std::ios::binary);
    need(bool(input),"original COMP DGM is missing");
    std::set<std::pair<int,int>> unique_new,unique_old;
    std::set<int> hours;
    std::size_t records=0,correct_new=0,wrong_new=0;
    std::size_t old_measured=0,old_postwind_differences=0;
    int first_error_hour=0,first_error_puff=0,first_error_ns=0,first_error_pred=0;
    int first_old_hour=0,first_old_puff=0,first_old_ns=0,first_old_pred=0;
    OriginalRecord rec;
    while(input.read(reinterpret_cast<char*>(&rec),sizeof(rec))) {
        ++records;
        const int hour=rec.i[1], puff=rec.i[3], nsam=rec.i[10];
        const int nsstr=rec.i[11],iold=rec.i[12],ipno=rec.i[14],ksam=rec.i[16];
        const float wsvar=rec.r[18],postwind=rec.r[11];
        const float reftsamp=rec.r[17];
        need(rec.i[0]==1 && hour>202200000 && puff>0 &&
             nsam>=1 && nsam<=99 && nsstr>=1 && nsstr<=nsam &&
             ksam>=nsstr && ksam<=nsam && std::isfinite(reftsamp) &&
             reftsamp>0.0F,"malformed 143h original COMP DGM sampling row");
        hours.insert(hour);
        if(iold==0 && unique_new.emplace(hour,puff).second) {
            need(rec.i[13]==1,"only the selected point source is supported");
            ReleaseCadenceInputs q;
            q.wind_speed_ms=wsvar;
            q.emission_interval=36.0;  // DHR/(XSAMLEN*DGRIDM): 3600/100
            q.hour_seconds=3600;
            q.max_puffs=99;
            const ReleaseCadence source=release_cadence(q);
            const SamplingCadence plan=sampling_cadence({
                .new_puff=true,
                .source_type=1,
                .emission_period_s=source.emission_duration_s,
                .step_seconds=3600.0,
                .release_number=ipno,
                .wind_speed_ms=wsvar,
                .x_sam_len=1.0,
                .grid_spacing_m=100.0,
                .max_sampling_steps=99
            });
            const bool equal=plan.sampling_steps==nsam &&
                plan.start_substep==nsstr &&
                plan.old_flag==iold &&
                bits(static_cast<float>(plan.reference_step_s))==bits(reftsamp);
            if(equal) ++correct_new;
            else {
                ++wrong_new;
                if(!first_error_hour) {
                    first_error_hour=hour;first_error_puff=puff;
                    first_error_ns=nsam;first_error_pred=plan.sampling_steps;
                    std::cerr<<"P7_ORIGINAL_COMP_NEW_FIRST_DIFFERENCE"
                             <<" HOUR="<<hour<<" PUFF="<<puff
                             <<" REF_NSAM="<<nsam<<" CPP_NSAM="<<plan.sampling_steps
                             <<" REF_NSSTR="<<nsstr<<" CPP_NSSTR="<<plan.start_substep
                             <<" REF_REFTSAMP="<<reftsamp
                             <<" CPP_REFTSAMP="<<plan.reference_step_s
                             <<" WSVAR="<<wsvar<<"\n";
                }
            }
        } else if(iold==1 && unique_old.emplace(hour,puff).second) {
            // This is NOT a numerical parity claim for old puffs. The DGM
            // captures WS after RISEWIND, but source L10119-10143 uses
            // ADVECT WS before entering the old-puff NSAM expression.
            const SamplingCadence guess=sampling_cadence({
                .new_puff=false,.source_type=1,
                .emission_period_s=0.0,.step_seconds=3600.0,
                .release_number=1,.wind_speed_ms=postwind,
                .x_sam_len=1.0,.grid_spacing_m=100.0,
                .max_sampling_steps=99
            });
            ++old_measured;
            if(guess.sampling_steps!=nsam) {
                ++old_postwind_differences;
                if(!first_old_hour) {
                    first_old_hour=hour;first_old_puff=puff;
                    first_old_ns=nsam;first_old_pred=guess.sampling_steps;
                }
            }
        }
    }
    need(input.eof(),"original COMP DGM truncated at a non-record boundary");
    if(expected_hours==143) {
        need(hours.size()==143,"not all 143 vendor COMP hours were audited");
        need(records==254687U,"vendor 143h COMP row count changed");
        need(unique_new.size()==11985U,
             "original 143h POINTS1 release episodes changed");
    } else {
        need(hours.size()==1 && records==1275U &&
             unique_new.size()==54U,
             "original first-hour COMP reference changed");
    }
    std::cout<<"P7_COMP_NEW_NATIVE_CADENCE"
             <<" HOURS="<<hours.size()
             <<" RECORDS="<<records
             <<" NEW_PUFF_EPISODES="<<unique_new.size()
             <<" NEW_EXACT="<<correct_new
             <<" NEW_MISMATCH="<<wrong_new
             <<" OLD_PUFF_EPISODES="<<old_measured
             <<" OLD_POSTRISE_WS_PROXY_DIFFERENCES="<<old_postwind_differences
             <<" OLD_TRUE_PRE_CADENCE_WS=NOT_IN_DGM"
             <<" ORIGINAL_SOURCE_MUTATED=NO"
             <<" FULL_NATIVE_COMP_MXSAM99=NOT_ACCEPTED\n";
    if(first_old_hour)
        std::cout<<"P7_COMP_OLD_POSTRISE_PROXY_FIRST_DIFFERENCE"
                 <<" HOUR="<<first_old_hour
                 <<" PUFF="<<first_old_puff
                 <<" REF_NSAM="<<first_old_ns
                 <<" POSTRISE_PROXY_CPP_NSAM="<<first_old_pred<<"\n";
    need(wrong_new==0,"source-ported POINTS1/COMP NEW REAL4 cadence diverges");
    need(correct_new==unique_new.size(),"new-puff cadence coverage incomplete");
    return 0;
}
}
int main(int argc,char** argv) {
    try {
        if(argc!=3)throw std::invalid_argument("use DGM.bin [1|143]");
        const int hours=std::stoi(argv[2]);
        if(hours!=1 && hours!=143)throw std::invalid_argument("unsupported hour span");
        return main_run(argv[1],hours);
    } catch(const std::exception& e) {
        std::cerr<<"P7_COMP_NEW_NATIVE_CADENCE=FAIL "<<e.what()<<"\n";
        return 1;
    }
}
