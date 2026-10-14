// P6G G14: real multi-station input/clock binding, exact accepted #53
// 17-stage execution order, n-1 thermal-state ownership and strict rollback.
// This test's physical callback is explicitly SYNTHETIC. It does not qualify
// a multi-station CALMET physics implementation or original field parity.
#include "cteas/p6/p6g_unified_hourly.hpp"
#include "cteas/p6/p6g_g18d7_source_station_cell.hpp"
#include "cteas/p6/p6g_g18d2_station_clock_transaction.hpp"
#include "cteas/p6/p6g_g18e4_prepdi_stage_adapter.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std::chrono;
namespace p6=cteas::calmet::p6;
namespace p6g=cteas::calmet::p6g;
static void check(bool yes, const char* why) {
    if (!yes)throw std::runtime_error(why);
}
template<class F> static void must_reject(F&& f,const char* why) {
    bool caught=false;
    try{f();}catch(const std::exception&){caught=true;}
    check(caught,why);
}
static p6g::StationSet stations() {
    p6g::StationSet v;
    const auto add=[](int id, float x, float y) {
        p6::DeckStation a{};
        a.id=id;a.local_x_m=x;a.local_y_m=y;
        return a;
    };
    v.surface={add(58321,10000.f,10000.f),add(58322,30000.f,30000.f)};
    v.upper={add(58424,20000.f,20000.f),add(58425,40000.f,40000.f)};
    return v;
}
static p6g::GeoGrid geo() {
    p6g::GeoGrid g;
    g.nx=12;g.ny=12;g.zone=50;g.hemisphere='N';g.grid_km=10.f;
    g.landuse.assign(144,10);
    return g;
}

// G18D7 regression: literal CALMET DIAGI uses REAL*4 (X/DGRID+1)
// assigned to INTEGER, then STHEOR clamps the 1-based index. Truncating
// X/DGRID before +1 selects the WRONG cell for near-boundary REAL*4 input.
static void original_station_cell_real4_boundary() {
    auto grid=geo();
    grid.grid_km=1.f;
    p6::DeckStation site{};
    site.local_x_m=std::nextafter(1000.f,0.f);
    site.local_y_m=100.f;
    check(p6g::g18d7_source_diagi_station_cell(grid,site)==1,
          "DIAGI REAL4 near 1km boundary did not round before INTEGER");
    site.local_x_m=std::nextafter(2000.f,0.f);
    check(p6g::g18d7_source_diagi_station_cell(grid,site)==2,
          "DIAGI REAL4 near 2km boundary did not round before INTEGER");
    site.local_x_m=100.f;
    site.local_y_m=std::nextafter(1000.f,0.f);
    check(p6g::g18d7_source_diagi_station_cell(grid,site)==12,
          "DIAGI REAL4 JST boundary was not source-equivalent");
    site.local_x_m=std::nextafter(12000.f,0.f);
    site.local_y_m=100.f;
    check(p6g::g18d7_source_diagi_station_cell(grid,site)==11,
          "STHEOR must clamp DIAGI rounded index to NX");
    site.local_x_m=12000.f;
    must_reject([&]{(void)p6g::g18d7_source_diagi_station_cell(grid,site);},
                "Outside scoped GEO station was silently clamped");
    grid.grid_km=10.f;
    site.local_x_m=std::nextafter(10000.f,0.f);
    check(p6g::g18d7_source_diagi_station_cell(grid,site)==0,
          "10km source REAL4 rounding unexpectedly changed station cell");
}

static p6g::SurfaceSeries surface(sys_seconds begin,std::size_t hours) {
    p6g::SurfaceSeries s;
    s.begin=begin;s.end=begin+std::chrono::hours{static_cast<int>(hours)};
    s.station_ids={58321,58322};
    s.hourly.reserve(hours);
    for(std::size_t h=0;h<hours;++h){
        p6g::SurfaceHour row;
        row.start=begin+std::chrono::hours{static_cast<int>(h)};
        row.end=row.start+std::chrono::hours{1};
        row.by_station.resize(2);
        for(std::size_t st=0;st<2;++st){
            row.by_station[st].wind_valid=true;
            row.by_station[st].wind_speed_mps=static_cast<float>(st+3);
            row.by_station[st].wind_direction_deg=270.f;
        }
        s.hourly.push_back(std::move(row));
    }
    return s;
}
static std::vector<p6g::UpperSeries> upper(sys_seconds begin,
                                           std::size_t hours) {
    std::vector<p6g::UpperSeries> result(2);
    for(int st=0;st<2;++st){
        auto& v=result[static_cast<std::size_t>(st)];
        v.station_id=(st==0?58424:58425);v.pstop_hpa=500.f;
        v.begin=begin;v.end=begin+std::chrono::hours{static_cast<int>(hours)};
        // 12-hour bracketing throughout a 143h or 8784h model year;
        // midpoint model hour exactly at sounding uses NEXT 12h segment.
        for(int h=-12;h<=static_cast<int>(hours)+24;h+=12){
            p6::UpperSounding u;
            u.station_id=v.station_id;
            u.start=begin+std::chrono::hours{h};u.end=u.start;
            // Selected PREPDI/CGAMMA test needs true per-US thermal
            // soundings; these are explicitly SYNTHETIC, not vendor
            // multi-station meteorological acceptance inputs.
            for(int k=0;k<4;++k) {
                p6::UpperLevel level;
                level.height_valid=true;
                level.temperature_valid=true;
                level.height_lgl_m=static_cast<float>(k*500);
                level.temperature_k=285.f+static_cast<float>(st+k*2)+
                    static_cast<float>(h)*.001f;
                u.data.push_back(level);
            }
            v.soundings.push_back(std::move(u));
        }
    }
    return result;
}
struct SyntheticNativePhysics final : p6g::MultiHourPhysics {
    bool throw_at_mixht=false,inject_oracle=false,omit_w=false;
    bool expect_source_controls=false;
    p6g::G18E4SelectedPrepdiStageAdapter* selected_prepdi=nullptr;
    std::size_t runs=0;
    std::vector<p6::NativeHourStage> order;
    p6::NativeHourStageResult run(p6::NativeHourStage stage,
                                  p6g::MultiHourFrame& f) override {
        order.push_back(stage);
        const auto n=static_cast<std::size_t>(f.grid.nx)*f.grid.ny;
        if(expect_source_controls)
            check(f.source_controls!=nullptr&&
                  f.source_controls->bias[0]==-1.f&&
                  f.source_controls->rmax1_km==100.f,
                  "G15 did not deliver source-effective controls into CALMET stage");
        else
            check(f.source_controls==nullptr,
                  "in-memory G14 test unexpectedly received forged original met.inp");
        check(f.surface.by_station.size()==2 && f.input.upper.size()==2 &&
              f.stations.surface.size()==2 && f.stations.upper.size()==2,
              "2SS+2US were not bound into the hourly stage frame");
        check(f.surface.end==f.input.end,"SURF/UP timestamp mismatch");
        check(f.previous.ziconv_m.size()==n,"n-1 native thermal state absent");
        check(f.previous.ziconv_m[0]==static_cast<float>(f.hour_index),
              "previous hour ZICONV was not used");
        if(stage==p6::NativeHourStage::PrepdiPreviousZiconv&&selected_prepdi) {
            // G14, not the test caller, executes selected multi-US PREPDI.
            selected_prepdi->run_selected(stage,f,true);
            const auto& up=selected_prepdi->selected_upper_for(f);
            const auto& gamma=selected_prepdi->selected_gamma_for(f);
            check(up.station_ids==std::vector<int>({58424,58425})&&
                  gamma.station_ids==up.station_ids&&
                  gamma.lapse.size()==2&&up.justa.size()==2,
                  "G18E4 2US PREPDI AA/BB + CGAMMA not joined to G14");
            for(const auto& lapse:gamma.lapse)
                check(std::isfinite(lapse.kelvin_per_m),
                      "G18E4 source CGAMMA invalid lapse");
        }
        if(stage==p6::NativeHourStage::DiagnoStheorElustr2Mixhtst)
            f.next.dptt_k[0]=static_cast<float>(f.hour_index+7);
        if(stage==p6::NativeHourStage::CompGridMixht){
            check(f.next.dptt_k[0]==static_cast<float>(f.hour_index+7),
                  "station DPTT DIAGNO mutation did not reach COMP");
            if(throw_at_mixht) throw std::runtime_error("synthetic stage failure");
            std::fill(f.next.zi_m.begin(),f.next.zi_m.end(),
                      static_cast<float>(f.hour_index+2));
            std::fill(f.next.ziconv_m.begin(),f.next.ziconv_m.end(),
                      static_cast<float>(f.hour_index+1));
        }
        if(stage==p6::NativeHourStage::CommitOutput) {
            // Fabricated data only for testing the full-field gate, never a
            // claim of CALMET Fortran numerical equivalence.
            f.fields.u_mps.assign(n*9,3.f);
            f.fields.v_mps.assign(n*9,4.f);
            if(!omit_w) f.fields.w_face_mps.assign(n*10,0.f);
            f.fields.temperature_k.assign(n*9,281.f);
            f.fields.qh_wm2.assign(n,20.f);
            f.fields.ustar_mps.assign(n,0.2f);
            f.fields.el_m.assign(n,-50.f);
            f.fields.wstar_mps.assign(n,0.1f);
            ++runs;
            if(inject_oracle)
                return {p6::NativeHourSource::OriginalFortranOracle,true};
        }
        return {p6::NativeHourSource::NativeCpp,true};
    }
};
static void case_hours(sys_seconds begin,std::size_t count){
    // Synthetic callback values remain synthetic. But the stage receives a
    // typed original-source control object through the SAME G14 authority.
    p6g::OriginalWindControls config;
    config.bias[0]=-1.f;config.rmax1_km=100.f;
    p6g::UnifiedMultistationHourlyScheduler sched(
        geo(),stations(),surface(begin,count),upper(begin,count),config);
    check(sched.planned_hours()==count,"declared hour count was truncated");
    SyntheticNativePhysics synthetic;
    synthetic.expect_source_controls=true;
    for(std::size_t hour=0;hour<count;++hour){
        synthetic.order.clear();
        sched.advance(synthetic);
        check(synthetic.order.size()==p6::kHefeiOriginalStageOrder.size()&&
              std::equal(synthetic.order.begin(),synthetic.order.end(),
                         p6::kHefeiOriginalStageOrder.begin()),
              "multi station solver diverged from #53 original stage order");
        check(sched.committed_memory().ziconv_m[0]==static_cast<float>(hour+1),
              "cross-hour state failed to commit");
    }
    sched.require_complete();
    check(synthetic.runs==count,"missing complete hourly output attempts");
    must_reject([&]{sched.advance(synthetic);},
                "unbounded year-end hours accepted");
}
int main(){
    try{
        original_station_cell_real4_boundary();
        const auto jan1=sys_days{year{2024}/January/1};
        case_hours(jan1,1);
        case_hours(jan1,6);
        case_hours(jan1,143);
        case_hours(jan1,8784); // leap year 2024, 10km / 2SS / 2US
        p6g::UnifiedMultistationHourlyScheduler rollback(
            geo(),stations(),surface(jan1,6),upper(jan1,6));
        SyntheticNativePhysics mock;
        mock.throw_at_mixht=true;
        must_reject([&]{rollback.advance(mock);},"failed COMP/MIXHT was committed");
        check(rollback.completed_hours()==0&&
              rollback.committed_memory().ziconv_m[0]==0.f,
              "failed native hour changed the committed n-1 ZICONV");
        mock.throw_at_mixht=false;mock.inject_oracle=true;
        must_reject([&]{rollback.advance(mock);},"source-oracle injection was accepted");
        check(rollback.completed_hours()==0,"oracle attempt advanced clock");
        mock.inject_oracle=false;mock.omit_w=true;
        must_reject([&]{rollback.advance(mock);},"uncomputed WFACE was accepted");
        check(rollback.completed_hours()==0,"missing native WFACE advanced clock");
        mock.omit_w=false;
        // G18D1: the real G14 authority must offer a precommit barrier.
        // Even with all 17 stages marked native, failed output validation
        // cannot publish current ZICONV or advance source UTC time.
        std::size_t p6f_checked=0;
        must_reject([&]{
            rollback.advance_with_precommit(mock,
              [&](const p6g::MultiHourFrame& candidate) {
                ++p6f_checked;
                check(candidate.hour_index==0&&
                      candidate.previous.ziconv_m[0]==0.f&&
                      candidate.next.ziconv_m[0]==1.f&&
                      candidate.fields.temperature_k.size()==12*12*9,
                      "G18D1 G14 precommit candidate data incomplete");
                throw std::runtime_error("synthetic P6F readback failure");
              });
        },"G18D1 failed native P6F precommit gate advanced hour");
        check(p6f_checked==1&&rollback.completed_hours()==0&&
              rollback.committed_memory().ziconv_m[0]==0.f,
              "G18D1 P6F failure mutated accepted preceding-hour ZICONV");
        rollback.advance_with_precommit(mock,
          [&](const p6g::MultiHourFrame& candidate) {
            ++p6f_checked;
            check(candidate.hour_index==0&&
                  candidate.next.ziconv_m[0]==1.f,
                  "G18D1 P6F gate accepted stale stage-time candidate");
          });
        check(p6f_checked==2&&rollback.completed_hours()==1&&
              rollback.committed_memory().ziconv_m[0]==1.f,
              "G18D1 successful P6F precommit did not preserve atomic G14 hour");
        // The retry must not permit an arbitrary future-hour offset.
        rollback.advance_with_precommit(mock,
          [&](const p6g::MultiHourFrame& candidate) {
            check(candidate.hour_index==1&&
                  candidate.previous.ziconv_m[0]==1.f&&
                  candidate.next.ziconv_m[0]==2.f,
                  "G18D1 2nd hour re-read tentative rather than committed ZICONV");
          });
        check(rollback.completed_hours()==2&&
              rollback.committed_memory().ziconv_m[0]==2.f,
              "G18D1 successive 2SS/2US native hours lacked isolated histories");
        // G18D2: PREPDI AA/BB and per-US CGAMMA history are staged
        // from G14's *own* next-hour preview and cannot publish
        // on a failed precommit/oracle or to a foreign scheduler.
        p6g::OriginalWindControls deck_controls;
        deck_controls.bias[0]=-1.f;deck_controls.rmax1_km=100.f;
        p6g::UnifiedMultistationHourlyScheduler source(
            geo(),stations(),surface(jan1,6),upper(jan1,6),
            deck_controls);
        p6g::G18D2StationClockAuthority station_clocks;
        const std::string synthetic_deck=
            "! IUPT = -1 !\n! IDIOPT2 = 0 !\n! ZUPT = 200. !\n";
        must_reject([&]{
            (void)station_clocks.prepare_selected(source,synthetic_deck,false);
        },"G18D2 selected upper thermal clock misrepresented production acceptance");
        auto stage0=station_clocks.prepare_selected(source,synthetic_deck,true);
        check(stage0.hour_index==0&&stage0.observed.station_ids.size()==2&&
              stage0.cgamma.lapse.size()==2&&
              !station_clocks.has_published_hour(),
              "G18D2 source station transaction was not prepared off committed G14");
        must_reject([&]{
            station_clocks.publish_only_after_completed_g14(source,stage0);
        },"G18D2 source upper/CGAMMA published before G14 completed hour");
        SyntheticNativePhysics source_mock;
        source_mock.expect_source_controls=true;
        must_reject([&]{
            source.advance_with_precommit(source_mock,
              [](const p6g::MultiHourFrame&) {
                  throw std::runtime_error("G18D2 injected P6F write/read failure");
              });
        },"G18D2 stage accepted failed P6F original output");
        check(source.completed_hours()==0&&
              !station_clocks.has_published_hour()&&
              source.committed_memory().ziconv_m[0]==0.f,
              "G18D2 leaked upper/CGAMMA or previous ZICONV after P6F failure");
        must_reject([&]{
            station_clocks.publish_only_after_completed_g14(source,stage0);
        },"G18D2 accepted station history despite failed P6F barrier");
        source.advance_with_precommit(source_mock,
          [](const p6g::MultiHourFrame& accepted) {
              check(accepted.hour_index==0&&
                    accepted.previous.ziconv_m[0]==0.f,
                    "G18D2 P6F retry read tentative ZICONV");
          });
        station_clocks.publish_only_after_completed_g14(source,stage0);
        check(station_clocks.has_published_hour()&&
              station_clocks.latest_upper_hour()==jan1+hours{1}&&
              station_clocks.latest_gamma_hour()==jan1+hours{1}&&
              source.completed_hours()==1,
              "G18D2 did not publish both accepted upper/CGAMMA clocks");
        must_reject([&]{
            station_clocks.publish_only_after_completed_g14(source,stage0);
        },"G18D2 published source AA/BB/CGAMMA history twice");
        auto stage1=station_clocks.prepare_selected(source,synthetic_deck,true);
        source_mock.inject_oracle=true;
        must_reject([&]{source.advance(source_mock);},
                    "G18D2 source-oracle injection bypassed strict G14 native gate");
        must_reject([&]{
            station_clocks.publish_only_after_completed_g14(source,stage1);
        },"G18D2 staged second UP clock after rejected Fortran-oracle hour");
        check(source.completed_hours()==1&&
              station_clocks.latest_gamma_hour()==jan1+hours{1},
              "G18D2 failed second hour altered committed thermal history");
        source_mock.inject_oracle=false;
        source.advance_with_precommit(source_mock,
          [](const p6g::MultiHourFrame& accepted) {
              check(accepted.hour_index==1&&
                    accepted.previous.ziconv_m[0]==1.f,
                    "G18D2 second source CGAMMA hour did not read committed prior");
          });
        station_clocks.publish_only_after_completed_g14(source,stage1);
        check(source.completed_hours()==2&&
              station_clocks.latest_upper_hour()==jan1+hours{2}&&
              station_clocks.latest_gamma_hour()==jan1+hours{2},
              "G18D2 second accepted AA/BB/CGAMMA rollover lost source time");
        auto bad=surface(jan1,6);
        bad.station_ids={58322,58321};
        must_reject([&]{
            p6g::UnifiedMultistationHourlyScheduler impossible(
                geo(),stations(),bad,upper(jan1,6));
        },"reordered SURF station IDs were accepted");
        auto grid=geo();grid.landuse.pop_back();
        must_reject([&]{
            p6g::UnifiedMultistationHourlyScheduler impossible(
                grid,stations(),surface(jan1,6),upper(jan1,6));
        },"incorrect GEO field width was accepted");
        std::cout << "PASS_P6G_G14_UNIFIED_HOURLY_INPUT_AND_CLOCK "
                     "1h_6h_143h_8784h 2SS_2US dynamic_10km "
                     "17_accepted_stages prior_ZICONV DPTT rollback "
                     "oracle_rejected WFACE_required "
                     "G18D1_P6F_PRECOMMIT_ROLLBACK_AND_RETRY_PASS "
                     "G18D2_UPPER_CGAMMA_SAME_G14_COMMIT_PASS "
                     "FULL_MULTI_SITE_PHYSICS=NOT_YET_ACCEPTED\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr << "FAIL_P6G_G14 "<<e.what()<<"\n";
        return 1;
    }
}
