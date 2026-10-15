// G19: original files, not injected in-memory MET.DAT or fabricated G14
// physical fields. 2SS+2US source input binding and fail-closed tests.
// This is an INPUT CONTRACT ONLY, never native multistation physics parity.
#include "cteas/p6/p6g_g19_file_backed_case.hpp"
#include "cteas/p6/p6g_g18e4_prepdi_stage_adapter.hpp"
#include "cteas/p6/p6g_g20_file_backed_wind1_profiles.hpp"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std::chrono;
namespace p6g=cteas::calmet::p6g;

static void check(bool ok,const char* reason){
    if(!ok)throw std::runtime_error(reason);
}
template<class T>static void rejects(T&& f,const char* reason){
    bool bad=false;
    try{f();}catch(const std::exception&){bad=true;}
    check(bad,reason);
}
static std::string read(const std::string& path) {
    std::ifstream file(path);
    check(bool(file),"G19 original reference met.inp absent");
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
static void save(const std::string& name,const std::string& data) {
    std::ofstream out(name);
    check(bool(out),"G19 cannot write scoped test input");
    out<<data;
}
static std::string replace_one(std::string input,
                               const std::string& control,
                               const std::string& value) {
    const std::regex token("(!\\s*"+control+"\\s*=\\s*)[^!]*(!)",
                           std::regex_constants::icase);
    auto begin=std::sregex_iterator(input.begin(),input.end(),token);
    check(begin!=std::sregex_iterator(),"G19 original READCF control missing");
    auto next=begin;++next;
    check(next==std::sregex_iterator(),"G19 ambiguous source control");
    // Do NOT concatenate "$1" with a numeric value: for e.g. NX=12
    // "$112" is interpreted by std::regex_replace as capture group 112,
    // silently removing the original control instead of replacing it.
    const auto match=*begin;
    input.replace(static_cast<std::size_t>(match.position(0)),
                  static_cast<std::size_t>(match.length(0)),
                  match.str(1)+value+" "+match.str(2));
    return input;
}
static int day_of_year(sys_seconds t){
    const auto d=floor<days>(t);
    const auto y=year_month_day{d}.year();
    return int((d-sys_days{y/January/1}).count())+1;
}
static void stamp(std::ostream& out,sys_seconds t,bool ending=false) {
    bool at24=ending&&t==floor<days>(t);
    if(at24)t-=hours{1};
    const auto ymd=year_month_day{floor<days>(t)};
    int hour=at24?24:int(duration_cast<hours>(t-floor<days>(t)).count());
    out<<int(ymd.year())<<" "<<day_of_year(t)<<" "<<hour<<" 0";
}
static void geo_file(const std::string& path,bool water_cell=false) {
    std::ofstream out(path);
    check(bool(out),"G19 GEO test file");
    out<<"GEO.DAT         2.1             Header structure with coordinate parameters\n";
    out<<"1\nG19 original-format test GEO\nUTM\n 50N\n";
    out<<"WGS-84  02-21-2003\n12 12 510.885 3531.269 10 10\nKM  M\n";
    out<<"1 - land use\n14 50 55 - NLU IWAT1 IWAT2\n";
    out<<"10 -20 20 30 40 51 54 55 60 61 62 70 80 90 - categories\n";
    for(int j=11;j>=0;--j)
        for(int i=0;i<12;++i)out<<(water_cell&&i==0&&j==0?51:20)<<(i==11?"\n":",");
    out<<"1.0000 - HTFAC\n";
    for(int j=11;j>=0;--j)
        for(int i=0;i<12;++i)out<<100+i+2*j<<(i==11?"\n":",");
    for(int field=0;field<6;++field) {
        out<<"2 - gridded\n";
        for(int j=11;j>=0;--j)
            for(int i=0;i<12;++i)
                out<<(field==0?0.1:0.2+0.02*field)<<(i==11?"\n":",");
    }
}
static void surface_file(const std::string& path,sys_seconds begin,
                         int count,bool reverse_ids=false) {
    std::ofstream out(path);
    check(bool(out),"G19 SURF test file");
    out<<"SURF.DAT        2.1             Hour Start and End Times with Seconds\n";
    out<<"1\nG19 original-format source SURF\nNONE    \nUTC+0000\n";
    stamp(out,begin);out<<" ";stamp(out,begin+hours{count},true);
    out<<" 2\n";
    out<<(reverse_ids?583220:583210)<<"\n"
       <<(reverse_ids?583210:583220)<<"\n";
    for(int k=0;k<count;++k) {
        stamp(out,begin+hours{k});out<<" ";
        stamp(out,begin+hours{k+1},true);out<<"\n";
        out<<"4.0 170.0 722 5 285 70 1008.1 0\n";
        out<<"5.0 180.0 722 5 282 60 1002.2 0\n";
    }
}
static void upper_sounding(std::ostream& out,int station,sys_seconds at){
    const auto y=year_month_day{floor<days>(at)};
    const auto h=int(duration_cast<hours>(at-floor<days>(at)).count());
    out<<" 6201 "<<station<<" "<<int(y.year())<<" "
       <<unsigned(y.month())<<" "<<unsigned(y.day())<<" "<<h<<" 0 "
       <<int(y.year())<<" "<<unsigned(y.month())<<" "
       <<unsigned(y.day())<<" "<<h<<" 0 2 2\n";
    const auto row=[&](float p,int height,float temp,int wd,int ws) {
        out<<"   "<<std::fixed<<std::setprecision(1)
           <<std::setw(6)<<p<<"/"<<std::setw(5)<<height
           <<"/"<<std::setw(5)<<temp<<"/"<<std::setw(3)<<wd
           <<"/"<<std::setw(3)<<ws;
    };
    row(1000,120,280.f,90,50);
    row(500,5500,259.8f,180,45);
    out<<"\n";
}
static void upper_file(const std::string& path,int site,sys_seconds begin){
    std::ofstream out(path);
    check(bool(out),"G19 UP test file");
    const auto day0=floor<days>(begin);
    const auto start=year_month_day{day0};
    const auto day1=day0+days{2};
    const auto end=year_month_day{day1};
    out<<"UP.DAT          2.1             Hour Start and End Times with Seconds\n";
    out<<"1\nG19 original-format source UP\nNONE\nUTC+0000\n";
    out<<int(start.year())<<" "<<day_of_year(day0)<<" 0 0 "
       <<int(end.year())<<" "<<day_of_year(day1)<<" 0 0 500.0 2 1\nF F F F\n";
    for(const auto at:{day0+hours{12},day0+hours{24},day0+hours{36}})
        upper_sounding(out,site,at);
}
static std::string make_two_station_crossyear_deck(const std::string& original) {
    std::string two=original;
    for(const auto& [key,value]:std::vector<std::pair<std::string,std::string>>{
            {"NSSTA","2"},{"NUSTA","2"},{"NX","12"},
            {"NY","12"},{"DGRIDKM","10"}})
        two=replace_one(std::move(two),key,value);
    two+="\n! SS2 = 'S002' 583220 543.385 3563.769 0 10.0 !\n";
    two+="! US2 = 'U002' 58425 553.384 3573.772 0 !\n";
    return two;
}
int main(int argc,char** argv){
    if(argc==4&&std::string(argv[1])=="--export-case") {
        try {
            const auto original=read(argv[2]);
            const auto two=make_two_station_crossyear_deck(original);
            const auto root=std::filesystem::path(argv[3]);
            std::filesystem::create_directories(root);
            const p6g::G19SourceFilePaths paths{
                (root/"met.inp").string(),(root/"GEO.DAT").string(),
                (root/"SURF.DAT").string(),
                {(root/"UP1.DAT").string(),(root/"UP2.DAT").string()}};
            save(paths.met_inp,two);
            geo_file(paths.geo_dat);
            const auto begin=sys_days{year{2023}/December/31}+hours{22};
            surface_file(paths.surf_dat,begin,6);
            const auto day=sys_days{year{2023}/December/31};
            upper_file(paths.ordered_up_dat[0],58424,day);
            upper_file(paths.ordered_up_dat[1],58425,day);
            const auto g19=p6g::g19_load_original_source_case(paths);
            check(g19.make_scheduler().planned_hours()==6,
                  "G20 export source case does not match tested 6-hour input");
            std::cout<<"PASS_G20_EXPORTED_ORIGINAL_FORMAT_SYNTHETIC_2SS_2US "
                        "FULL_ORIGINAL_VENDOR_PARITY=NOT_ACCEPTED\n";
            return 0;
        } catch(const std::exception& error) {
            std::cerr<<"FAIL_G20_EXPORT_CASE "<<error.what()<<"\n";
            return 1;
        }
    }
    std::vector<std::string> cleanup={
        "g19_met.inp","g19_geo.dat","g19_surf.dat","g19_up1.dat","g19_up2.dat"};
    try{
        check(argc==2,"G19 requires pinned original CALMET met.inp path");
        const auto original=read(argv[1]);
        // The real single-station Hefei deck must not be promoted to a
        // two-surface/two-upper station scientific run.
        const auto source=p6g::g19_source_grid_spec(original);
        check(source.nx==50&&source.ny==50&&source.zone==50&&
              source.hemisphere=='N'&&source.spacing_km==0.1&&
              source.xorigin_km==510.885&&source.yorigin_km==3531.269,
              "G19 original Hefei source deck grid parse differs from READCF");
        p6g::G19SourceFilePaths paths{cleanup[0],cleanup[1],cleanup[2],
                                      {cleanup[3],cleanup[4]}};
        save(paths.met_inp,original);
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19 promoted original single-site deck to multisite production");
        const auto two=make_two_station_crossyear_deck(original);
        save(paths.met_inp,two);
        geo_file(paths.geo_dat);
        const auto begin=sys_days{year{2023}/December/31}+hours{22};
        surface_file(paths.surf_dat,begin,6);
        upper_file(paths.ordered_up_dat[0],58424,sys_days{year{2023}/December/31});
        upper_file(paths.ordered_up_dat[1],58425,sys_days{year{2023}/December/31});
        const auto source_case=p6g::g19_load_original_source_case(paths);
        auto scheduler=source_case.make_scheduler();
        // G19/G14B must agree on exactly the same on-disk source data.
        // Two independent input authorities with divergent READCF/UTM/UP
        // semantics would make a later genuine physical solver unsafe.
        const p6g::CaseInputPaths previous_paths{
            paths.geo_dat,paths.surf_dat,paths.ordered_up_dat,
            paths.met_inp,p6g::g19_source_grid_spec(two),2,2};
        auto prior_loader=p6g::load_unified_multistation_case(previous_paths);
        check(prior_loader.planned_hours()==scheduler.planned_hours()&&
              prior_loader.grid().nx==scheduler.grid().nx&&
              prior_loader.grid().ny==scheduler.grid().ny&&
              prior_loader.grid().landuse==scheduler.grid().landuse&&
              prior_loader.original_controls().zface_m==
                  scheduler.original_controls().zface_m&&
              prior_loader.original_controls().bias==
                  scheduler.original_controls().bias,
              "G19/G14B double entrypoint returned different original source model");
        for(std::size_t idx=0;idx<scheduler.planned_hours();++idx) {
            // Neither source-only scheduler may advance without physics;
            // compare all disk-observed forcing through independent source
            // readers instead of accidentally using one copied frame.
            check(source_case.surface.hourly[idx].end==
                      prior_loader.preview_next_hour().surface.end+hours{static_cast<int>(idx)},
                  "G19/G14B source file UTC hour count unexpectedly differs");
        }
        const auto a=scheduler.preview_next_hour();
        const auto b=prior_loader.preview_next_hour();
        check(a.input.end==b.input.end&&a.surface.start==b.surface.start&&
              a.stations.surface[0].local_x_m==b.stations.surface[0].local_x_m&&
              a.stations.surface[1].local_y_m==b.stations.surface[1].local_y_m&&
              a.stations.upper[0].local_x_m==b.stations.upper[0].local_x_m&&
              a.surface.by_station[0].wind_speed_mps==
                  b.surface.by_station[0].wind_speed_mps&&
              a.surface.by_station[1].pressure_mb==
                  b.surface.by_station[1].pressure_mb&&
              a.input.upper.size()==b.input.upper.size(),
              "G19/G14B first-hour forcing differed from identical GEO/SURF/UP");
        for(std::size_t site=0;site<a.input.upper.size();++site) {
            const auto& u=a.input.upper[site];
            const auto& v=b.input.upper[site];
            check(u.earlier==v.earlier&&u.later==v.later&&
                  u.span_seconds==v.span_seconds&&
                  u.past_seconds==v.past_seconds&&
                  u.future_seconds==v.future_seconds&&
                  a.upper[site].station_id==b.upper[site].station_id&&
                  a.upper[site].soundings[u.earlier].data[0].temperature_k==
                      b.upper[site].soundings[v.earlier].data[0].temperature_k,
                  "G19/G14B per-station original PREPDI AA/BB differs");
        }
        check(source_case.grid.nx==12&&source_case.grid.ny==12&&
              source_case.grid.grid_km==10.f&&
              source_case.stations.surface.size()==2&&
              source_case.stations.upper.size()==2&&
              source_case.surface.station_ids==
                  std::vector<int>({583210,583220})&&
              source_case.upper[0].station_id==58424&&
              source_case.upper[1].station_id==58425&&
              scheduler.planned_hours()==6&&scheduler.completed_hours()==0&&
              scheduler.preview_next_hour().input.end==begin+hours{1}&&
              scheduler.has_original_controls()&&
              source_case.stheor_upper_ordinal_for_surface==
                  std::vector<std::size_t>({0,1})&&
              source_case.surface_controls.source.iavet==1&&
              source_case.surface_controls.source.itprog==0&&
              source_case.surface_controls.source.irhprog==0&&
              source_case.heat_inputs.coefficients.hc1==5.31e-13f&&
              source_case.heat_inputs.coefficients.hc2==60.f,
              "G19 file-backed 2SS/2US UTC G14 source binding failed");
        // The assembly provides NO physics. A full hour must not commit
        // until a real source-authentic G14 executor exists.
        struct MissingNativePhysics final:p6g::MultiHourPhysics {
            cteas::calmet::p6::NativeHourStageResult run(
                cteas::calmet::p6::NativeHourStage,
                p6g::MultiHourFrame&) override {
                return {cteas::calmet::p6::NativeHourSource::Unknown,false};
            }
        } no_physics;
        rejects([&]{scheduler.advance(no_physics);},
                "G19 source-only assembly bypassed native physics admission");
        check(scheduler.completed_hours()==0&&
              scheduler.committed_memory().ziconv_m[0]==0.f,
              "G19 failed admission mutated committed prior-hour memory");
        // G19C: native original-file UP data now reaches REAL G14 stage
        // index zero, not a caller-built preview or independent time loop.
        // The G18E4 source PREPDI/CGAMMA remains selected-only and returns
        // Unknown,false; this must NOT commit an incomplete model hour.
        p6g::G18D2StationClockAuthority source_clocks;
        std::size_t observed_stage_zero=0;
        struct SourceBackedPrepdiProbe final : p6g::MultiHourPhysics {
            p6g::G18E4SelectedPrepdiStageAdapter prepdi;
            const p6g::MultiHourFrame* live;
            std::size_t& observed;
            SourceBackedPrepdiProbe(
                const p6g::UnifiedMultistationHourlyScheduler& owner,
                p6g::G18D2StationClockAuthority& clocks,
                const std::string& metinp,
                const p6g::MultiHourFrame& frame,std::size_t& counter)
                :prepdi(owner,clocks,metinp),live(&frame),observed(counter) {}
            cteas::calmet::p6::NativeHourStageResult run(
                cteas::calmet::p6::NativeHourStage stage,
                p6g::MultiHourFrame& frame) override {
                check(&frame==live,
                      "G19C source-backed PREPDI used detached G14 frame");
                if(stage==cteas::calmet::p6::NativeHourStage::PrepdiPreviousZiconv) {
                    prepdi.run_selected(stage,frame,true);
                    const auto& up=prepdi.selected_upper_for(frame);
                    const auto& gamma=prepdi.selected_gamma_for(frame);
                    check(up.station_ids==std::vector<int>({58424,58425})&&
                          gamma.station_ids==up.station_ids&&
                          up.end==frame.input.end&&
                          up.physical_sounding[0].size()==2&&
                          up.physical_sounding[0][0].temp_k[0]==280.f&&
                          up.physical_sounding[0][1].temp_k[0]==280.f&&
                          !prepdi.try_publish_after_completed_g14_hour_noexcept(),
                          "G19C REAL UP.DAT PREPDI AA/BB or CGAMMA source mismatch");
                    ++observed;
                }
                return {cteas::calmet::p6::NativeHourSource::Unknown,false};
            }
        };
        rejects([&]{
            scheduler.advance_with_fresh_physics(
                [&](const p6g::MultiHourFrame& frame)
                    ->std::unique_ptr<p6g::MultiHourPhysics> {
                    return std::make_unique<SourceBackedPrepdiProbe>(
                        scheduler,source_clocks,source_case.original_metinp,
                        frame,observed_stage_zero);
                });
        },"G19C partial source-file PREPDI was promoted into full native G14");
        check(observed_stage_zero==1&&scheduler.completed_hours()==0&&
              !source_clocks.has_published_hour()&&
              scheduler.committed_memory().ziconv_m[0]==0.f,
              "G19C actual GEO/SURF/UP candidate changed physical/UP clocks");

        // G20: source-backed file inputs now reach the existing native
        // WIND1 C++ kernel on the SAME fresh G14 transaction frame.
        // Unlike the earlier G18F6 hand-made station columns, G20 reads
        // actual met.inp ZFACE, GEO landuse/z0, SURF and UP VERTAV/PREPDI.
        // PREPDI stage admission is an explicit test-only proxy; FRADJ
        // is deliberately not accepted here, so zero G14 hours commit.
        struct SourceBackedWind1Probe final:p6g::MultiHourPhysics {
            p6g::G18E4SelectedPrepdiStageAdapter prepdi;
            std::optional<p6g::G16SelectedWindPhysics> native_wind;
            const p6g::MultiHourFrame* live;
            std::size_t& stages;
            std::vector<float>& u;
            std::vector<float>& v;
            SourceBackedWind1Probe(
                const p6g::UnifiedMultistationHourlyScheduler& owner,
                p6g::G18D2StationClockAuthority& clocks,
                const std::string& metinp,
                const p6g::MultiHourFrame& frame,std::size_t& count,
                std::vector<float>& out_u,std::vector<float>& out_v)
                :prepdi(owner,clocks,metinp),
                 live(&frame),stages(count),u(out_u),v(out_v){}
            cteas::calmet::p6::NativeHourStageResult run(
                cteas::calmet::p6::NativeHourStage stage,
                p6g::MultiHourFrame& hour) override {
                check(&hour==live,
                      "G20 native WIND1 source did not retain live G14 frame");
                if(stage==cteas::calmet::p6::NativeHourStage::PrepdiPreviousZiconv) {
                    prepdi.run_selected(stage,hour,true);
                    ++stages;
                    // TEST-ONLY PREPDI proxy for the next real C++ WIND1.
                    return {cteas::calmet::p6::NativeHourSource::NativeCpp,true};
                }
                if(stage==cteas::calmet::p6::NativeHourStage::DiagnoWind1) {
                    native_wind.emplace(
                        p6g::g20_file_backed_source_wind1_profiles(hour,true));
                    const auto result=native_wind->run(stage,hour);
                    check(result.completed&&
                          result.source==cteas::calmet::p6::NativeHourSource::NativeCpp,
                          "G20 source-file native WIND1 kernel not executed");
                    u=native_wind->selected_result().first_guess.u_mps;
                    v=native_wind->selected_result().first_guess.v_mps;
                    ++stages;
                    return result;
                }
                return {cteas::calmet::p6::NativeHourSource::Unknown,false};
            }
        };
        std::size_t live_stages=0;
        std::vector<float> actual_u,actual_v;
        rejects([&]{
            scheduler.advance_with_fresh_physics(
                [&](const p6g::MultiHourFrame& frame)
                    ->std::unique_ptr<p6g::MultiHourPhysics>{
                    return std::make_unique<SourceBackedWind1Probe>(
                        scheduler,source_clocks,source_case.original_metinp,
                        frame,live_stages,actual_u,actual_v);
                });
        },"G20 partial file-backed WIND1 was accepted as a full G14 CALMET hour");
        check(live_stages==2&&actual_u.size()==12u*12u*9u&&
              actual_v.size()==actual_u.size()&&
              scheduler.completed_hours()==0&&
              !source_clocks.has_published_hour(),
              "G20 original source SURF+UP native WIND1 missing or wrongly committed");

        // For the exact same on-disk original input, independently form
        // and execute G16 WIND1 on a preview frame, with no G14 mutation.
        auto standalone=scheduler.preview_next_hour();
        const auto selected_profiles=
            p6g::g20_file_backed_source_wind1_profiles(standalone,true);
        check(selected_profiles.station_ids==
                  std::vector<int>({583210,583220,58424,58425})&&
              selected_profiles.wind1_u_mps.size()==36u,
              "G20 source W1 station/vertical ordinal layout invalid");
        p6g::G16SelectedWindPhysics independent(selected_profiles);
        const auto direct=independent.run(
            cteas::calmet::p6::NativeHourStage::DiagnoWind1,standalone);
        check(direct.completed&&
              direct.source==cteas::calmet::p6::NativeHourSource::NativeCpp&&
              actual_u==independent.selected_result().first_guess.u_mps&&
              actual_v==independent.selected_result().first_guess.v_mps,
              "G20 native live W1 differs from direct source-file G16 kernel");
        rejects([&]{
            (void)p6g::g20_file_backed_source_wind1_profiles(
                standalone,false);
        },"G20 source selected WIND1 could be promoted without test boundary");
        auto missing_wind=source_case;
        missing_wind.surface.hourly.front().by_station[0].wind_valid=false;
        auto negative_scheduler=missing_wind.make_scheduler();
        auto negative_frame=negative_scheduler.preview_next_hour();
        rejects([&]{
            (void)p6g::g20_file_backed_source_wind1_profiles(
                negative_frame,true);
        },"G20 invented surface wind absent in real source SURF");

        // Wrong case-file mapping must be rejected before a single step.
        paths.ordered_up_dat={cleanup[4],cleanup[3]};
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19 reordered upper files accepted against US1/US2");
        paths.ordered_up_dat={cleanup[3],cleanup[4]};
        surface_file(paths.surf_dat,begin,6,true);
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19 reordered original SURF IDs accepted against SS1/SS2");
        surface_file(paths.surf_dat,begin,6);
        auto wrong_geo=two;
        wrong_geo=replace_one(wrong_geo,"NX","13");
        save(paths.met_inp,wrong_geo);
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19 GEO / met.inp original NX mismatch was accepted");
        save(paths.met_inp,two);

        // Both old and new readers must fail identically for source
        // coordinate system, datum, counts and grid geometry.
        for(const auto& [key,value]:std::vector<std::pair<std::string,std::string>>{
                {"PMAP","LCC"},{"DATUM","NAD83"},{"UTMHEM","S"}}) {
            const auto bad_source=replace_one(two,key,value);
            save(paths.met_inp,bad_source);
            rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                    "G19 accepted a met.inp source CRS variant outside G14B");
            rejects([&]{(void)p6g::load_unified_multistation_case(previous_paths);},
                    "G14B accepted a met.inp source CRS variant outside G19");
        }
        save(paths.met_inp,two);

        // G19B: original SETCOM IUPT, source IAVET/HFLUX, site-grid
        // position and all-land branch are declared input requirements,
        // not convenient fallbacks to other unverified CALMET physics.
        auto unsupported_iupt=replace_one(two,"IUPT","1");
        save(paths.met_inp,unsupported_iupt);
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19B silently accepted fixed US instead of original nearest-US branch");
        auto unsupported_iavet=replace_one(two,"IAVET","0");
        save(paths.met_inp,unsupported_iavet);
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19B silently substituted IAVET=0 for original source two-pass path");
        save(paths.met_inp,replace_one(two,"NZ","8"));
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19B unsupported 8-layer deck silently fed G14 nine-layer physics");
        save(paths.met_inp,replace_one(two,"ISURFT","1"));
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19B representative-station SURFVAR substituted for two-pass observed path");
        save(paths.met_inp,two+"\n! HC1 = 1. !\n");
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19B allowed synthetic heat defaults into original HFLUX branch");

        const std::regex ss1(R"(!\s*SS1\s*=\s*'[^']*'[^!]*!)");
        const auto displaced_site=std::regex_replace(
            two,ss1,"! SS1 = 'OUT' 583210 700.000 3533.769 0 10.0 !");
        check(displaced_site!=two,"G19B unable to modify original SSn source");
        save(paths.met_inp,displaced_site);
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19B SS outside GEO wrongly accepted for STHEOR");

        save(paths.met_inp,two);
        geo_file(paths.geo_dat,true);
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19B unsupported water landuse silently passed original HFLUX");
        geo_file(paths.geo_dat);
        check(p6g::g19_load_original_source_case(paths)
                  .stheor_upper_ordinal_for_surface==
                  std::vector<std::size_t>({0,1}),
              "G19B original station assignment was changed by rejected source probes");

        paths.ordered_up_dat.pop_back();
        rejects([&]{(void)p6g::g19_load_original_source_case(paths);},
                "G19 missing second upper station input was accepted");
        for(const auto& path:cleanup)std::remove(path.c_str());
        std::cout<<"PASS_P6G_G19_FILE_BACKED_2SS_2US_CROSS_YEAR_SOURCE_INPUT "
                    "GEO_SURF_UP_METINP G14_17_STAGE_FAIL_CLOSED "
                    "PRODUCTION_PHYSICS_NOT_ACCEPTED\n";
        return 0;
    }catch(const std::exception& e) {
        for(const auto& path:cleanup)std::remove(path.c_str());
        std::cerr<<"FAIL_P6G_G19 "<<e.what()<<"\n";
        return 1;
    }
}
