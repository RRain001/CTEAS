// P6G G14: real multi-station input/clock binding, exact accepted #53
// 17-stage execution order, n-1 thermal-state ownership and strict rollback.
// This test's physical callback is explicitly SYNTHETIC. It does not qualify
// a multi-station CALMET physics implementation or original field parity.
#include "cteas/p6/p6g_unified_hourly.hpp"
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
            v.soundings.push_back(std::move(u));
        }
    }
    return result;
}
struct SyntheticNativePhysics final : p6g::MultiHourPhysics {
    bool throw_at_mixht=false,inject_oracle=false,omit_w=false;
    std::size_t runs=0;
    std::vector<p6::NativeHourStage> order;
    p6::NativeHourStageResult run(p6::NativeHourStage stage,
                                  p6g::MultiHourFrame& f) override {
        order.push_back(stage);
        const auto n=static_cast<std::size_t>(f.grid.nx)*f.grid.ny;
        check(f.surface.by_station.size()==2 && f.input.upper.size()==2 &&
              f.stations.surface.size()==2 && f.stations.upper.size()==2,
              "2SS+2US were not bound into the hourly stage frame");
        check(f.surface.end==f.input.end,"SURF/UP timestamp mismatch");
        check(f.previous.ziconv_m.size()==n,"n-1 native thermal state absent");
        check(f.previous.ziconv_m[0]==static_cast<float>(f.hour_index),
              "previous hour ZICONV was not used");
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
    p6g::UnifiedMultistationHourlyScheduler sched(
        geo(),stations(),surface(begin,count),upper(begin,count));
    check(sched.planned_hours()==count,"declared hour count was truncated");
    SyntheticNativePhysics synthetic;
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
        rollback.advance(mock);
        check(rollback.completed_hours()==1&&
              rollback.committed_memory().ziconv_m[0]==1.f,
              "valid synthetic retry failed to commit");
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
                     "FULL_MULTI_SITE_PHYSICS=NOT_YET_ACCEPTED\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr << "FAIL_P6G_G14 "<<e.what()<<"\n";
        return 1;
    }
}
