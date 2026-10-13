// CALMET Hefei 2022 native C++ hourly process coordinator (transition stage).
// This preserves the source-accepted P6D/P6E kernels without altering physics.
// File-backed process adapters must later become in-memory native step calls.
// A successful invocation does NOT imply 45-field original scientific parity.
#include "cteas/p6f/sha256.hpp"
#include <cerrno>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs=std::filesystem;
namespace {
constexpr const char* kMetHash="b1823cda92280888d70b00961ab39486036a3086e18637fff21efb4f4b237d30";
constexpr std::uintmax_t kStateBytes=50U*50U*sizeof(float);
struct Options {
    fs::path source, p6d, p6e, cgamma, bridge, work;
    int hours=6;
    bool emit_source_stage_trace=false;
};
std::string str(const fs::path& p){return p.string();}
void require(const fs::path& p) {
    if(!fs::is_regular_file(p))throw std::runtime_error("missing file: "+str(p));
}
std::string read(const fs::path& p){
    require(p);std::ifstream in(p,std::ios::binary);
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
void write(const fs::path& p,const std::string& body){
    std::ofstream out(p,std::ios::binary|std::ios::trunc);
    if(!out||!(out<<body))throw std::runtime_error("write failed: "+str(p));
}
Options parse(int argc,char** argv) {
    Options o;
    for(int i=1;i<argc;++i) {
        const std::string flag=argv[i];
        if(flag=="--emit-source-stage-trace"){o.emit_source_stage_trace=true;continue;}
        if(++i==argc)throw std::invalid_argument("missing argument "+flag);
        const std::string v=argv[i];
        if(flag=="--case")o.source=v;
        else if(flag=="--p6d")o.p6d=v;
        else if(flag=="--p6e")o.p6e=v;
        else if(flag=="--cgamma")o.cgamma=v;
        else if(flag=="--bridge")o.bridge=v;
        else if(flag=="--work")o.work=v;
        else if(flag=="--hours")o.hours=std::stoi(v);
        else throw std::invalid_argument("unknown argument "+flag);
    }
    if(o.source.empty()||o.p6d.empty()||o.p6e.empty()||
       o.cgamma.empty()||o.bridge.empty()||o.work.empty())
        throw std::invalid_argument("--case --p6d --p6e --cgamma --bridge --work required");
    if(o.hours<1||o.hours>143)throw std::invalid_argument("hours must be 1..143");
    if(o.emit_source_stage_trace&&o.hours!=143)
        throw std::invalid_argument("stage trace requires all 143 hours");
    o.source=fs::absolute(o.source).lexically_normal();
    o.p6d=fs::absolute(o.p6d).lexically_normal();
    o.p6e=fs::absolute(o.p6e).lexically_normal();
    o.cgamma=fs::absolute(o.cgamma).lexically_normal();
    o.bridge=fs::absolute(o.bridge).lexically_normal();
    o.work=fs::absolute(o.work).lexically_normal();
    if(fs::exists(o.work)&&!fs::is_empty(o.work))
        throw std::invalid_argument("work directory not empty: "+str(o.work));
    return o;
}
void run(const std::vector<std::string>& args,const fs::path& log){
#if defined(__unix__) || defined(__APPLE__)
    const pid_t pid=fork();
    if(pid<0)throw std::runtime_error("fork failed");
    if(pid==0){
        const int fd=open(log.c_str(),O_CREAT|O_WRONLY|O_TRUNC,0644);
        if(fd<0||dup2(fd,STDOUT_FILENO)<0||dup2(fd,STDERR_FILENO)<0)_exit(126);
        close(fd);
        std::vector<char*> a;
        for(const auto& s:args)a.push_back(const_cast<char*>(s.c_str()));
        a.push_back(nullptr);
        execv(a[0],a.data());
        _exit(127);
    }
    int code=0;pid_t waited=-1;
    do{waited=waitpid(pid,&code,0);}while(waited<0&&errno==EINTR);
    if(waited!=pid||!WIFEXITED(code)||WEXITSTATUS(code)!=0)
        throw std::runtime_error("child failed: "+str(log));
#else
    (void)args;(void)log;
    throw std::runtime_error("POSIX process adapter unavailable");
#endif
}
std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> values;std::string current;
    for(char c:text){
        if(c!='\n'){current+=c;continue;}
        if(!current.empty()&&current.back()=='\r')current.pop_back();
        values.push_back(current);current.clear();
    }
    if(!current.empty()){
        if(current.back()=='\r')current.pop_back();
        values.push_back(current);
    }
    return values;
}
std::string join(const std::vector<std::string>& list){
    std::string out;for(const auto& s:list)out+=s+"\n";return out;
}
void append_state(const fs::path& in,const fs::path& out,int hour) {
    require(in);
    if(fs::file_size(in)!=kStateBytes)throw std::runtime_error("wrong native state float32 size: "+str(in));
    std::ifstream input(in,std::ios::binary);
    std::ofstream target(out,std::ios::binary|std::ios::app);
    target<<input.rdbuf();target.flush();
    if(!target||fs::file_size(out)!=static_cast<std::uintmax_t>(hour+1)*kStateBytes)
        throw std::runtime_error("hourly state append failed: "+str(out));
}
void check_state_metadata(const fs::path& path,int hour) {
    const auto data=read(path);
    const std::regex hour_re("\"first_hour_zero_based\"\\s*:\\s*"+
        std::to_string(hour)+"\\s*[,}]");
    const std::regex shape_re("\"shape\"\\s*:\\s*\\[\\s*1\\s*,\\s*50\\s*,\\s*50\\s*\\]");
    if(!std::regex_search(data,hour_re)||!std::regex_search(data,shape_re)||
       data.find("p6e_calmet_dat21 emit-realcase")==std::string::npos||
       data.find("\"dptt\"")==std::string::npos)
        throw std::runtime_error("missing native COMP provenance/hour: "+str(path));
}
std::string hour_dir(int hour){
    return "hour_"+(hour<10?std::string("00"):hour<100?std::string("0"):std::string(""))+
        std::to_string(hour);
}
} // namespace
int main(int argc,char** argv){
    try{
        const auto o=parse(argc,argv);
        const auto met=o.source/"run/met/met.dat";
        const auto surf=o.source/"met/surface/final/surf.dat";
        const auto upper=o.source/"met/upperair/final/up_58424.dat";
        const auto geo=o.source/"geo/makegeo/geo.dat";
        for(const auto& f:{met,surf,upper,geo,o.p6d,o.p6e,o.bridge,o.cgamma})require(f);
        if(cteas::p6f::sha256_hex_file(str(met))!=kMetHash)
            throw std::runtime_error("locked original CALMET.DAT SHA256 mismatch");
        fs::create_directories(o.work);
        const auto observations=o.work/"observations.csv";
        run({str(o.bridge),str(surf),str(upper),"58424",str(observations)},o.work/"bridge.log");
        require(observations);
        const auto history=o.work/"history/comp";
        fs::create_directories(history.parent_path());
        std::vector<std::string> hourly_files,station_lines,stage_rows;
        const fs::path stage_root=o.work/"native_source_stage_143h";
        if(o.emit_source_stage_trace)fs::create_directories(stage_root);
        for(int hour=0;hour<o.hours;++hour){
            const auto dir=o.work/hour_dir(hour),diagno=dir/"diagno",gamma=dir/"gamma";
            fs::create_directories(diagno);fs::create_directories(gamma);
            run({str(o.cgamma),"--up",str(upper),"--p6e-state-prefix",str(history),
                 "--out",str(gamma),"--station-id","58424","--zupt","200",
                 "--upper-i","25","--upper-j","26","--causal-hours",
                 std::to_string(hour+1)},dir/"cgamma.log");
            std::size_t count=0;
            for(const auto& item:fs::directory_iterator(gamma))
                if(item.is_regular_file()&&item.path().extension()==".f32"&&
                   item.path().filename().string().starts_with("gamma_"))++count;
            if(count!=1)throw std::runtime_error("CGAMMA must emit one current-hour plane");
            std::vector<std::string> d={str(o.p6d),"--case",str(o.source),
                "--first-hour",std::to_string(hour),"--hours","1",
                "--out",str(diagno),"--gamma",str(gamma)};
            if(hour==0)d.push_back("--native-setup-hour0");
            else{d.push_back("--comp-state-prefix");d.push_back(str(history));}
            run(d,dir/"p6d.log");
            const auto flux=lines(read(diagno/"station_elustr2.csv"));
            if(flux.size()!=2)throw std::runtime_error("station flux not one-hour");
            if(station_lines.empty())station_lines.push_back(flux[0]);
            else if(station_lines[0]!=flux[0])
                throw std::runtime_error("station flux header changed");
            station_lines.push_back(flux[1]);
            const auto comp=dir/"comp",hour_met=dir/"met_native_1h.dat";
            std::vector<std::string> e={str(o.p6e),"emit-realcase",
                "--reference",str(met),"--observations",str(observations),
                "--geo-file",str(geo),"--native-hour-comp","1",
                "--native-p6d-dir",str(diagno),"--station-flux-csv",
                str(diagno/"station_elustr2.csv"),"--first-hour",std::to_string(hour),
                "--hours","1","--state-prefix",str(comp),"--out",str(hour_met),
                "--report",str(dir/"comp_report.json")};
            if(hour>0){
                e.push_back("--previous-comp-prefix");
                e.push_back(str(o.work/hour_dir(hour-1)/"comp"));
            }
            run(e,dir/"p6e.log");
            const fs::path meta=str(comp)+".state.json";
            check_state_metadata(meta,hour);
            for(const char* state:{"zi","ziconv","dptt"})
                append_state(str(comp)+"."+state+".f32",str(history)+"."+state+".f32",hour);
            fs::copy_file(meta,str(history)+".state.json",
                          fs::copy_options::overwrite_existing);
            require(hour_met);
            hourly_files.push_back(str(hour_met));
            if(o.emit_source_stage_trace){
                auto label=read(diagno/"produced_hours.txt");
                label=label.substr(0,label.find_first_of("\r\n\t "));
                if(label.empty())throw std::runtime_error("missing stage-trace hour label");
                for(const char* prefix:{"fradj_corrected_u","fradj_corrected_v",
                                       "wind1_u","wind1_v","slope_corrected_u",
                                       "slope_corrected_v","inter2_u","inter2_v"}){
                    const auto source=diagno/(std::string(prefix)+"_"+label+".f32");
                    require(source);
                    if(fs::file_size(source)!=9U*50U*50U*sizeof(float))
                        throw std::runtime_error("wrong stage-trace float32 shape: "+str(source));
                    fs::copy_file(source,stage_root/source.filename());
                }
                const auto row=lines(read(diagno/"station_inter2_layer1.csv"));
                if(row.size()!=2)throw std::runtime_error("station INTER2 row malformed");
                if(stage_rows.empty())stage_rows.push_back(row[0]);
                else if(stage_rows[0]!=row[0])
                    throw std::runtime_error("station INTER2 header changed");
                stage_rows.push_back(row[1]);
            }
        }
        const auto station=o.work/"native_station_flux.csv";
        write(station,join(station_lines));
        if(o.emit_source_stage_trace)
            write(stage_root/"station_inter2_layer1.csv",join(stage_rows));
        const auto manifest=o.work/"native_hourly_files.txt";
        write(manifest,join(hourly_files));
        const auto output=o.work/"native_causal_calmet.dat";
        run({str(o.p6e),"merge-native-hours","--manifest",str(manifest),
             "--out",str(output)},o.work/"writer.log");
        require(output);
        if(fs::file_size(output)<1000000U)
            throw std::runtime_error("merged native 45-field file incomplete");
        const auto digest=cteas::p6f::sha256_hex_file(str(output));
        std::string report="{\n  \"status\": \"PASS_CPP_PROCESS_COORDINATOR_EXECUTION\",\n"
            "  \"hours_completed\": "+std::to_string(o.hours)+",\n"
            "  \"native_wind\": true,\n  \"native_previous_hour_comp_state\": true,\n"
            "  \"original_met_role\": \"CASE_CONTROL_HEADER_ONLY\",\n"
            "  \"in_process_kernel_adapter\": false,\n"
            "  \"scientific_45_field_pass\": false,\n"
            "  \"native_calmet_dat_sha256\": \""+digest+"\"\n}\n";
        write(o.work/"native_cpp_hourly_manifest.json",report);
        std::cout<<report;
        return 0;
    }catch(const std::exception& error){
        std::cerr<<"FAIL_CPP_HOURLY_COORDINATOR: "<<error.what()<<'\n';
        return 3;
    }
}
