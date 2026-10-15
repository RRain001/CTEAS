// G19: original files, not injected in-memory MET.DAT or fabricated G14
// physical fields. 2SS+2US source input binding and fail-closed tests.
// This is an INPUT CONTRACT ONLY, never native multistation physics parity.
#include "cteas/p6/p6g_g19_file_backed_case.hpp"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
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
int main(int argc,char** argv){
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
        std::string two=original;
        for(const auto& [key,value]:std::vector<std::pair<std::string,std::string>>{
                {"NSSTA","2"},{"NUSTA","2"},{"NX","12"},
                {"NY","12"},{"DGRIDKM","10"}})
            two=replace_one(std::move(two),key,value);
        two+="\n! SS2 = 'S002' 583220 543.385 3563.769 0 10.0 !\n";
        two+="! US2 = 'U002' 58425 553.384 3573.772 0 !\n";
        save(paths.met_inp,two);
        geo_file(paths.geo_dat);
        const auto begin=sys_days{year{2023}/December/31}+hours{22};
        surface_file(paths.surf_dat,begin,6);
        upper_file(paths.ordered_up_dat[0],58424,sys_days{year{2023}/December/31});
        upper_file(paths.ordered_up_dat[1],58425,sys_days{year{2023}/December/31});
        const auto source_case=p6g::g19_load_original_source_case(paths);
        auto scheduler=source_case.make_scheduler();
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
