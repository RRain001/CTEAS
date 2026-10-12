#include "cteas/puff/original_motion_lifecycle.hpp"
#include "cteas/puff/control.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
struct Record {
    std::array<std::int32_t,24> i;
    std::array<float,64> f;
};
static_assert(sizeof(Record)==352U);
static_assert(std::endian::native==std::endian::little);
void require(bool test, const char* what) {
    if (!test) throw std::runtime_error(what);
}
void negative_tests() {
    using namespace cteas::puff;
    OriginalPuffMotionLifecycle lifecycle;
    bool rejected=false;
    try {
        (void)lifecycle.advance(7,{.uadv_ms=1.0F,.tsamp_s=1.0F,.met_spacing_m=100.0F});
    } catch(const std::out_of_range&) { rejected=true; }
    require(rejected,"advance without puff must fail");
    lifecycle.introduce(7,{25.0F,25.0F,70.0F,0.0F,0.0F});
    rejected=false;
    try { lifecycle.introduce(7,{25.0F,25.0F,70.0F,0.0F,0.0F}); }
    catch(const std::invalid_argument&) { rejected=true; }
    require(rejected,"duplicate admission must fail");
    const auto original=lifecycle.state(7);
    rejected=false;
    try { (void)lifecycle.advance(7,{.uadv_ms=1.0F,.tsamp_s=0.0F,.met_spacing_m=100.0F}); }
    catch(const std::invalid_argument&) { rejected=true; }
    require(rejected,"bad timestep must fail");
    require(lifecycle.state(7).x_cells==original.x_cells &&
            lifecycle.state(7).travel_time_s==original.travel_time_s,
            "failed step changed persisted puff state");
    lifecycle.retire(7);
    require(!lifecycle.contains(7) && lifecycle.active_count()==0U,
            "retired puff is still reachable");
}
}
int main(int argc,char** argv) {
    try {
        require(argc==3 || argc==4,"usage: lifecycle_real4 PUFF_INP ORIGINAL_DGM [6]");
        const bool six_hours=argc==4 && std::string(argv[3])=="6";
        require(argc==3 || six_hours,"only 6-hour extension is supported");
        using namespace cteas::puff;
        negative_tests();
        const auto deck=ControlFile::read_file(argv[1]);
        require(deck.required_int("NPT1")==1 && deck.required_int("METFM")==1,
                "unsupported source/meteorology mode");
        const auto grid=deck.required_float_list("DGRIDKM");
        require(grid.size()==1U && grid.front()==0.1,"not original 100m grid");
        const float spacing=100.0F;
        std::ifstream input(argv[2],std::ios::binary);
        require(static_cast<bool>(input),"cannot open locked original DGM");
        OriginalPuffMotionLifecycle life;
        std::size_t rows=0,episodes=0,linked_steps=0,hour_transitions=0;
        int current_id=0, previous_hour=0;
        Record original{};
        while(input.read(reinterpret_cast<char*>(&original),sizeof(original))) {
            ++rows;
            const int current_hour=original.i[1];
            require(original.i[0]==1 && current_hour>=202200100 &&
                    current_hour <= (six_hours?202200105:202200100),
                    "unexpected original step type/time");
            if(previous_hour!=0 && current_hour!=previous_hour) {
                require(current_hour==previous_hour+1,
                        "nonconsecutive original hourly puff-state stream");
                if(current_id!=0) life.retire(current_id);
                current_id=0;
                ++hour_transitions;
            }
            previous_hour=current_hour;
            const int id=original.i[3];
            const auto& f=original.f;
            // A puff can be admitted only when its first reference row is
            // encountered. After admission, subsequent rows for the SAME
            // puff must advance from our OWN persisted native state, not
            // from the DGM's next XOLD or YOLD.
            if(id!=current_id) {
                if(current_id!=0) life.retire(current_id);
                current_id=id;
                ++episodes;
                life.introduce(id,{f[0],f[1],f[2],f[19],f[20]});
            } else {
                const auto state=life.state(id);
                if(state.x_cells!=f[0] || state.y_cells!=f[1] ||
                   state.total_distance_m!=f[19] || state.travel_time_s!=f[20]) {
                    std::cerr<<"P7_LIFECYCLE_FIRST_DIVERGENCE RECORD="<<rows
                             <<" PUFF="<<id<<" NEXT_KSAM="<<original.i[16]
                             <<" NATIVE_X="<<state.x_cells<<" FORTRAN_XOLD="<<f[0]
                             <<" NATIVE_Y="<<state.y_cells<<" FORTRAN_YOLD="<<f[1]
                             <<" NATIVE_AGE="<<state.travel_time_s<<" FORTRAN_AGE="<<f[20]
                             <<'\n';
                    return 1;
                }
                ++linked_steps;
            }
            // Upstream RISEWIND-adapted wind and ZE1 are still supplied by
            // original, rather than claiming independently coupled PUFFDZ
            // and plume-rise physics in the first lifecycle slice.
            const auto step=life.advance(id,{
                .uadv_ms=f[12],.vadv_ms=f[13],.ws_ms=f[11],
                .tsamp_s=f[62],.met_spacing_m=spacing,
                .end_height_m=f[52],.settling_velocity_ms=0.0F,.plume_tilt=0
            });
            if(step.state.x_cells!=f[38] || step.state.y_cells!=f[39] ||
               step.distance_m!=f[59] ||
               step.state.total_distance_m!=f[40] ||
               step.state.travel_time_s!=f[41] ||
               step.state.x_cells!=f[44] || step.state.y_cells!=f[45] ||
               step.state.z_m!=f[46]) {
                std::cerr<<"P7_LIFECYCLE_STEP_DIVERGENCE RECORD="<<rows<<" PUFF="<<id
                         <<" NATIVE_X="<<step.state.x_cells<<" ORIGINAL_X="<<f[38]
                         <<" NATIVE_XTOT="<<step.state.total_distance_m
                         <<" ORIGINAL_XTOT="<<f[40]
                         <<" NATIVE_AGE="<<step.state.travel_time_s
                         <<" ORIGINAL_AGE="<<f[41]<<'\n';
                return 1;
            }
        }
        require(input.eof() && input.gcount()==0,"truncated DGM record");
        require(rows==(six_hours?11458U:1275U) && episodes>0U && linked_steps>0U &&
                hour_transitions==(six_hours?5U:0U),
                "original lifecycle record count / hour sequence not established");
        std::cout<<(six_hours?"P7_FIRST_6H_PERSISTED_LIFECYCLE=PASS":"P7_FIRST_HOUR_PERSISTED_LIFECYCLE=PASS")
                 <<" ROWS="<<rows<<" HOUR_TRANSITIONS="<<hour_transitions
                 <<" PUFF_EPISODES="<<episodes<<" SELF_PROPAGATED_STEPS="<<linked_steps
                 <<" INITIAL_WIND_ORACLE=true VERTICAL_STATE_ORACLE=true"
                 <<" FULL_AUTONOMOUS_COMP=NOT_YET_AVAILABLE\n";
    }catch(const std::exception& err) {
        std::cerr<<"P7_FIRST_HOUR_PERSISTED_LIFECYCLE=FAIL "<<err.what()<<'\n';
        return 1;
    }
    return 0;
}
