// P6D real-case wind-field producer (partial, explicitly labelled).
//
// Reads a CALMET 6.5.0 case tree read-only:
//   run/met/met.inp                        (controls, ZFACE, station ids)
//   geo/**/geo.dat                         (GEO.DAT 2.1 terrain + land use)
//   met/surface/final/surf.dat             (SURF.DAT 2.1 surface observations)
//   met/upperair/final/up_<id>.dat         (UP.DAT 2.1 upper-air soundings)
// and writes, per hourly label, chain-internal fields plus a JSON metadata
// block that names exactly which DIAGNO stages were applied and which inputs
// would still be required for the remaining ones.
//
// This is a PARTIAL product: it delivers the original WIND1 step-one UG/VG
// grid (IEXTRP=-4 neutral SIMILT first guess, the field DIAGNO starts from)
// and the in-chain TERSET envelope. It deliberately does not invent values
// for stages whose real-case inputs are not available in this branch
// (SURFVAR TEMP2D, ISFCMET QH), so those stages are reported as not applied
// rather than silently substituted.
#include "cteas/p6/read_geo.hpp"
#include "cteas/p6/read_surface.hpp"
#include "cteas/p6/read_upper.hpp"
#include "cteas/p6/similt_neutral.hpp"
#include "cteas/p6/terrain_envelope.hpp"
#include "cteas/p6/upper_hourly_wind.hpp"
#include "cteas/p6/wind1_selected_firstguess.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace cteas::calmet::p6;
namespace fs = std::filesystem;

namespace {
constexpr float MISSING=9999.f;

std::string read_text(const fs::path& path){
    std::ifstream in(path,std::ios::binary);
    if(!in)throw std::runtime_error("cannot read "+path.string());
    std::ostringstream buffer;buffer<<in.rdbuf();
    return buffer.str();
}

// CALMET control decks annotate each value as "! KEY = value !"; take the
// annotated value when present, otherwise the first number on the key's line.
std::map<std::string,std::vector<float>> deck_values(const std::string& text){
    std::map<std::string,std::vector<float>> values;
    static const std::regex annotated(R"(!\s*([A-Z0-9]+)\s*=\s*([^!]*)!)");
    for(auto it=std::sregex_iterator(text.begin(),text.end(),annotated);
        it!=std::sregex_iterator();++it){
        const std::string key=(*it)[1].str();
        std::vector<float> numbers;
        std::istringstream stream((*it)[2].str());
        std::string token;
        while(stream>>token){
            if(token==",")continue;
            try{numbers.push_back(std::stof(token));}
            catch(const std::exception&){numbers.clear();break;}
        }
        if(!numbers.empty()&&!values.count(key))values[key]=numbers;
    }
    return values;
}

float deck_scalar(const std::map<std::string,std::vector<float>>& v,
                  const std::string& key,float fallback){
    const auto it=v.find(key);
    return it==v.end()?fallback:it->second.front();
}

int deck_int(const std::map<std::string,std::vector<float>>& v,
             const std::string& key,int fallback){
    return static_cast<int>(deck_scalar(v,key,static_cast<float>(fallback)));
}

void write_f32(const fs::path& path,const std::vector<float>& data){
    std::ofstream out(path,std::ios::binary);
    if(!out)throw std::runtime_error("cannot write "+path.string());
    for(float value:data){
        const std::uint32_t raw=*reinterpret_cast<const std::uint32_t*>(&value);
        out.write(reinterpret_cast<const char*>(&raw),4);
    }
}

std::string sha256_short(const fs::path& path){
    // Content identity without pulling a hash library into the module: FNV-1a
    // over the bytes, plus the byte count, is enough to label inputs here.
    std::ifstream in(path,std::ios::binary);
    if(!in)throw std::runtime_error("cannot hash "+path.string());
    std::uint64_t hash=1469598103934665603ull;
    std::size_t bytes=0;
    char buffer[65536];
    while(in){
        in.read(buffer,sizeof(buffer));
        const auto got=static_cast<std::size_t>(in.gcount());
        for(std::size_t n=0;n<got;++n){
            hash^=static_cast<unsigned char>(buffer[n]);
            hash*=1099511628211ull;
        }
        bytes+=got;
    }
    std::ostringstream text;
    text<<"fnv1a64:"<<std::hex<<hash<<":bytes:"<<std::dec<<bytes;
    return text.str();
}

std::string hour_label(std::chrono::sys_seconds clock){
    const auto day=std::chrono::floor<std::chrono::days>(clock);
    const std::chrono::year_month_day ymd{day};
    const auto hour=std::chrono::duration_cast<std::chrono::hours>(clock-day).count();
    std::ostringstream text;
    text<<int(ymd.year())
        <<'-'<<(unsigned(ymd.month())<10?"0":"")<<unsigned(ymd.month())
        <<'-'<<(unsigned(ymd.day())<10?"0":"")<<unsigned(ymd.day())
        <<'_'<<(hour<10?"0":"")<<hour;
    return text.str();
}

struct Arguments {
    fs::path case_dir,out_dir,geo,deck,surface,upper;
    int hours=143;
    fs::path out()const{return out_dir;}
};
} // namespace

int main(int argc,char** argv){
 try{
    Arguments args;
    for(int n=1;n<argc;++n){
        const std::string flag=argv[n];
        auto next=[&]()->std::string{
            if(n+1>=argc)throw std::invalid_argument("missing value for "+flag);
            return argv[++n];
        };
        if(flag=="--case")args.case_dir=next();
        else if(flag=="--out")args.out_dir=next();
        else if(flag=="--geo")args.geo=next();
        else if(flag=="--deck")args.deck=next();
        else if(flag=="--surface")args.surface=next();
        else if(flag=="--upper")args.upper=next();
        else if(flag=="--hours")args.hours=std::stoi(next());
        else throw std::invalid_argument("unknown argument "+flag);
    }
    if(args.out_dir.empty())throw std::invalid_argument("--out is required");
    if(args.case_dir.empty()&&(args.geo.empty()||args.deck.empty()||
                               args.surface.empty()||args.upper.empty()))
        throw std::invalid_argument(
            "realcase-wind needs --case or all of --geo/--deck/--surface/--upper");
    if(!args.case_dir.empty()){
        if(args.deck.empty())args.deck=args.case_dir/"run/met/met.inp";
        if(args.surface.empty())args.surface=args.case_dir/"met/surface/final/surf.dat";
        if(args.upper.empty()){
            std::vector<fs::path> found;
            for(const auto& entry:fs::directory_iterator(args.case_dir/"met/upperair/final"))
                if(entry.path().extension()==".dat")found.push_back(entry.path());
            if(found.size()!=1)
                throw std::runtime_error("ambiguous upper-air file; pass --upper");
            args.upper=found.front();
        }
        if(args.geo.empty()){
            for(const auto& entry:fs::recursive_directory_iterator(args.case_dir/"geo"))
                if(entry.path().filename()=="geo.dat")args.geo=entry.path();
            if(args.geo.empty())throw std::runtime_error("geo.dat not found; pass --geo");
        }
    }
    fs::create_directories(args.out_dir);

    const std::string deck=read_text(args.deck);
    const auto controls=deck_values(deck);
    const int nx=deck_int(controls,"NX",50);
    const int ny=deck_int(controls,"NY",50);
    const int nz=deck_int(controls,"NZ",9);
    const float grid_m=deck_scalar(controls,"DGRIDKM",0.1f)*1000.f;
    const float terrad_km=deck_scalar(controls,"TERRAD",15.f);
    const int iextrp=deck_int(controls,"IEXTRP",-4);
    const int icalm=deck_int(controls,"ICALM",0);
    if(nx!=50||ny!=50||nz!=9)
        throw std::runtime_error(
            "this producer is scoped to the frozen 50x50x9 case grid");
    if(std::abs(grid_m-100.f)>1e-3f)
        throw std::runtime_error("this producer is scoped to the 100 m grid");
    std::array<float,10> zface{};
    {
        const auto it=controls.find("ZFACE");
        if(it==controls.end()||it->second.size()!=10)
            throw std::runtime_error("the deck must declare ten ZFACE values");
        for(std::size_t k=0;k<10;++k)zface[k]=it->second[k];
    }
    std::vector<float> thickness_m(9),center_m(9);
    for(int k=0;k<9;++k){
        thickness_m[std::size_t(k)]=zface[std::size_t(k)+1]-zface[std::size_t(k)];
        center_m[std::size_t(k)]=
            0.5f*(zface[std::size_t(k)]+zface[std::size_t(k)+1]);
    }
    std::array<float,9> bias{};
    {
        const auto it=controls.find("BIAS");
        if(it!=controls.end()&&it->second.size()>=9)
            for(std::size_t k=0;k<9;++k)bias[k]=it->second[k];
    }

    const auto geo=read_geo_50(args.geo.string(),nx,ny);
    const auto surface=read_surface_2022(args.surface.string());
    const auto upper=read_upper_2022(args.upper.string(),-1);

    const auto start=std::chrono::sys_days{std::chrono::year{2022}/1/1};
    std::vector<std::chrono::sys_seconds> labels;
    for(int n=1;n<=args.hours;++n)
        labels.push_back(start+std::chrono::hours{n});

    // Original PREPDI hourly upper-air preparation (VERTAV layer averages plus
    // the PREPDI time interpolation), exactly the 143 case hours.
    const std::vector<float> zface_vector(zface.begin(),zface.end());
    const auto hourly_upper=calmet_prepdi_2022_hours(upper,zface_vector);

    const auto envelope=terset(geo.terrain_m,nx,ny,grid_m,grid_m,terrad_km);
    write_f32(args.out()/"terset_hmax.f32",envelope.maximum_m);
    write_f32(args.out()/"terset_hmin.f32",envelope.minimum_m);
    write_f32(args.out()/"terset_disthmax.f32",envelope.distance_to_maximum_m);
    write_f32(args.out()/"terset_disthmin.f32",envelope.distance_to_minimum_m);

    // Surface station roughness: the GEO roughness at the station cell is the
    // scoped original choice when the deck selects no explicit SURFVAR value.
    const std::size_t stations_written=1;
    std::ofstream index(args.out()/"produced_hours.txt");
    if(!index)throw std::runtime_error("cannot write produced_hours.txt");
    int written=0;
    float speed_min=1.e9f,speed_max=-1.e9f;
    for(std::size_t h=0;h<labels.size()&&written<args.hours;++h){
        const auto clock=labels[h];
        const auto& hour=hourly_upper[h];
        // Surface observation at this hour; fail closed on missing winds.
        const SurfaceObserved* obs=nullptr;
        for(const auto& interval:surface.hourly)
            if(interval.end==clock&&interval.obs.wind_valid){
                obs=&interval.obs;
                break;
            }
        if(obs==nullptr)
            throw std::runtime_error(
                "surface wind observation missing for "+hour_label(clock));
        const float speed=obs->wind_speed_mps;
        const float radians=obs->wind_direction_deg*3.14159265358979f/180.f;
        const float surface_u=-speed*std::sin(radians);
        const float surface_v=-speed*std::cos(radians);
        speed_min=std::min(speed_min,speed);
        speed_max=std::max(speed_max,speed);

        Wind1SelectedStationHour station;
        station.hour_end=clock;
        station.surface_station_id=surface.station_id;
        station.upper_station_id=upper.station_id;
        station.surface_observed_u_mps=surface_u;
        station.surface_observed_v_mps=surface_v;
        const float roughness=geo.z0_m.empty()?0.1f:geo.z0_m[0];
        if(std::hypot(surface_u,surface_v)>0.0001f){
            const SimiltNeutralInput neutral{
                31.939774f,10.f,roughness,surface_u,surface_v,iextrp,0};
            const auto profile=original_similt_wind1_neutral(neutral);
            for(std::size_t k=0;k<9;++k)
                station.surface_neutral_profile[k]=
                    LayerWind{profile.u_mps[k],profile.v_mps[k]};
        }else if(icalm!=0){
            for(auto& wind:station.surface_neutral_profile)wind=LayerWind{0.f,0.f};
        }else{
            throw std::runtime_error(
                "calm surface wind with ICALM=0 at "+hour_label(clock));
        }
        if(hour.winds.size()!=9)
            throw std::runtime_error(
                "hourly upper profile is not nine layers at "+hour_label(clock));
        for(std::size_t k=0;k<9;++k)station.upper_observed_profile[k]=hour.winds[k];

        Wind1FirstGuessSelectedControls settings;
        settings.nx=nx;settings.ny=ny;settings.nz=nz;
        settings.grid_spacing_m=grid_m;
        settings.vertical_bias=bias;
        settings.icalm=icalm;settings.iextrp=iextrp;
        for(std::size_t k=0;k<9;++k)
            settings.upper_layer_valid[k]=true;
        const auto first=wind1_firstguess_selected(station,settings);
        write_f32(args.out()/("wind1_u_"+hour_label(clock)+".f32"),first.ug_mps);
        write_f32(args.out()/("wind1_v_"+hour_label(clock)+".f32"),first.vg_mps);
        index<<hour_label(clock)<<"\n";
        ++written;
    }
    index.close();

    std::ostringstream meta;
    meta<<"{\n";
    meta<<"  \"product\": \"p6d_realcase_wind_fields\",\n";
    meta<<"  \"partial\": true,\n";
    meta<<"  \"status\": \"PARTIAL_WIND1_STEP_ONE_AND_TERSET_ENVELOPE\",\n";
    meta<<"  \"case_dir\": \""<<args.case_dir.string()<<"\",\n";
    meta<<"  \"grid\": {\"nx\": "<<nx<<", \"ny\": "<<ny<<", \"nz\": "<<nz
        <<", \"grid_spacing_m\": "<<grid_m
        <<", \"layout\": \"k-major, j-major, i-fastest\", \"dtype\": \"float32 little-endian\"},\n";
    meta<<"  \"levels\": {\"zface_m\": [";
    for(std::size_t k=0;k<10;++k)meta<<(k?", ":"")<<zface[k];
    meta<<"], \"thickness_m\": [";
    for(std::size_t k=0;k<9;++k)meta<<(k?", ":"")<<thickness_m[k];
    meta<<"], \"center_m\": [";
    for(std::size_t k=0;k<9;++k)meta<<(k?", ":"")<<center_m[k];
    meta<<"]},\n";
    meta<<"  \"units\": {\"wind1_u_*.f32\": \"m/s\", \"wind1_v_*.f32\": \"m/s\","
          " \"terset_hmax.f32\": \"m\", \"terset_hmin.f32\": \"m\","
          " \"terset_disthmax.f32\": \"m\", \"terset_disthmin.f32\": \"m\"},\n";
    meta<<"  \"missing_sentinel\": "<<MISSING<<",\n";
    meta<<"  \"hourly_labels\": "<<written<<",\n";
    meta<<"  \"hour_label\": \"YYYY-MM-DD_HH (UTC), the label is the hour END\",\n";
    meta<<"  \"surface_wind_speed_range_mps\": ["<<speed_min<<", "<<speed_max<<"],\n";
    meta<<"  \"stages_applied_in_chain\": [";
    meta<<"\"WIND1 selected first guess (IEXTRP=-4 neutral SIMILT)\"";
    meta<<", \"TERSET envelope (DIAGI L5228, TERRAD="<<terrad_km<<" km)\"],\n";
    meta<<"  \"stages_not_applied\": [\n";
    meta<<"    {\"stage\": \"SURFVAR -> TEMP2D (DIAGNO L5609-L5610)\", \"status\": \"BLOCKED\","
          " \"missing\": [\"TPROG 3-D prognostic temperature\", \"IRH2D\", \"IPCODE2D\"]},\n";
    meta<<"    {\"stage\": \"HEATFX/ELUSTR -> QH (DIAGNO L5676-L5715)\", \"status\": \"BLOCKED\","
          " \"missing\": [\"SINAPC\", \"NEARS\", \"TSFNSP\", \"CCGRID\", \"MCLOUD\", \"IPSIFCN\", \"Z0\", \"RHO\", \"USTAR\", \"EL\"]},\n";
    meta<<"    {\"stage\": \"FRADJ (L5643-L5644)\", \"status\": \"NEEDS_CALLER_PLANES\","
          " \"missing\": [\"GAMMA\", \"TEMP2D\", \"HMAX (provided here as terset_hmax.f32)\"]},\n";
    meta<<"    {\"stage\": \"SLOPE (L5722-L5723)\", \"status\": \"NEEDS_CALLER_PLANES\","
          " \"missing\": [\"QH\", \"TEMP2D\", \"HMIN/DISTHMAX/DISTHMIN (provided here as terset_*.f32)\"]},\n";
    meta<<"    {\"stage\": \"INTER2 blend + SMOOTH/DIVCEL/W tail\", \"status\": \"NOT_APPLIED_IN_THIS_PARTIAL\","
          " \"note\": \"the D9A bridge models the INTER2 observation weighting as a 22500-cell array, while the original reads per-station UDAT/VDAT and WO; the real-case mapping is not yet established, so no blend is emitted\"},\n";
    meta<<"    {\"stage\": \"TOPOF2/IKINE route and O'Brien/IOBR=1 tail\", \"status\": \"NON_CASE_PATH\","
          " \"note\": \"the case deck sets IKINE=0 and IOBR=0\"}\n";
    meta<<"  ],\n";
    meta<<"  \"control_values_read\": {\"TERRAD\": "<<terrad_km
        <<", \"IEXTRP\": "<<iextrp<<", \"ICALM\": "<<icalm
        <<", \"DGRIDKM\": "<<(grid_m/1000.f)<<"},\n";
    meta<<"  \"inputs\": {\"deck\": \""<<args.deck.string()<<"\", \"deck_fingerprint\": \""
        <<sha256_short(args.deck)<<"\", \"geo\": \""<<args.geo.string()
        <<"\", \"geo_fingerprint\": \""<<sha256_short(args.geo)
        <<"\", \"surface\": \""<<args.surface.string()<<"\", \"surface_fingerprint\": \""
        <<sha256_short(args.surface)<<"\", \"upper\": \""<<args.upper.string()
        <<"\", \"upper_fingerprint\": \""<<sha256_short(args.upper)<<"\"},\n";
    meta<<"  \"stations\": {\"surface_station_id\": "<<surface.station_id
        <<", \"upper_station_id\": "<<upper.station_id
        <<", \"written\": "<<stations_written<<"},\n";
    meta<<"  \"upper_soundings\": "<<upper.soundings.size()<<"\n";
    meta<<"}\n";
    std::ofstream metadata(args.out()/"metadata.json");
    if(!metadata)throw std::runtime_error("cannot write metadata.json");
    metadata<<meta.str();
    std::cout<<"P6D_REALCASE_WIND_PARTIAL hours="<<written
             <<" out="<<args.out().string()<<"\n";
    return 0;
 }catch(const std::exception& e){
    std::cerr<<"realcase-wind failed: "<<e.what()<<'\n';
    return 1;
 }
}
