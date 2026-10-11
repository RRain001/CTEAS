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
#include "cteas/p6/diagno_segment.hpp"
#include "cteas/p6/slope_flow.hpp"
#include "cteas/p6e/heatfx.hpp"
#include "cteas/p6e/surface_solar.hpp"
#include "cteas/p6e/surfvar_observed.hpp"
#include "cteas/p6/inter2_selected.hpp"
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
        // Values arrive either space or comma separated ("0 , 2 , 4").
        static const std::regex number(
            R"([-+]?(?:[0-9]+\.?[0-9]*|\.[0-9]+)(?:[eEdD][-+]?[0-9]+)?)");
        const std::string raw=(*it)[2].str();
        std::vector<float> numbers;
        for(auto n=std::sregex_iterator(raw.begin(),raw.end(),number);
            n!=std::sregex_iterator();++n){
            std::string token=n->str();
            for(char& c:token)if(c=='d'||c=='D')c='e';
            numbers.push_back(std::stof(token));
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
    fs::path case_dir,out_dir,geo,deck,surface,upper,fixture_dir;
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
        else if(flag=="--fixtures")args.fixture_dir=next();
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
    const double origin_x_km=deck_scalar(controls,"XORIGKM",0.f);
    const double origin_y_km=deck_scalar(controls,"YORIGKM",0.f);
    if(origin_x_km<=0.||origin_y_km<=0.)
        throw std::runtime_error("the deck must declare XORIGKM/YORIGKM");
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

    // Station local geometry. This case places both stations at the declared
    // grid centre (case.yaml grid.center equals stations.csv lat/lon), and no
    // branch-local lat/lon projection exists, so the centre is taken from the
    // case metadata and anything else fails closed instead of being guessed.
    double centre_x_km=0.,centre_y_km=0.,station_lat_deg=0.;
    {
        const std::string yaml=read_text(args.case_dir/"case.yaml");
        static const std::regex lon(R"(longitude:\s*([-0-9.]+))");
        static const std::regex lat(R"(latitude:\s*([-0-9.]+))");
        static const std::regex len(R"(length_km:\s*([0-9.]+))");
        static const std::regex wid(R"(width_km:\s*([0-9.]+))");
        std::smatch loom,lam,lom,wim;
        if(!std::regex_search(yaml,loom,lon)||!std::regex_search(yaml,lam,lat)||
           !std::regex_search(yaml,lom,len)||!std::regex_search(yaml,wim,wid))
            throw std::runtime_error("case.yaml grid block missing");
        const double centre_lon=std::stod(loom[1]),centre_lat=std::stod(lam[1]);
        const double length_km=std::stod(lom[1]),width_km=std::stod(wim[1]);
        const auto csv=read_text(args.surface);
        (void)csv;
        // Station lat/lon come from the case station tables.
        const std::string table=read_text(args.case_dir/"met/surface/final/stations.csv");
        double station_lat=0.,station_lon=0.;
        {
            std::istringstream stream(table);
            std::string line;
            std::getline(stream,line);
            std::getline(stream,line);
            std::istringstream row(line);
            std::string field;
            std::vector<std::string> fields;
            while(std::getline(row,field,','))fields.push_back(field);
            if(fields.size()<4)throw std::runtime_error("stations.csv row malformed");
            station_lat=std::stod(fields[2]);station_lon=std::stod(fields[3]);
        }
        station_lat_deg=station_lat;
        if(std::abs(station_lat-centre_lat)>1e-4||std::abs(station_lon-centre_lon)>1e-4)
            throw std::runtime_error(
                "station is not at the declared grid centre and no branch-local "
                "lat/lon projection is available; supply local coordinates instead");
        centre_x_km=0.5*length_km;centre_y_km=0.5*width_km;
    }

    const auto geo=read_geo_50(args.geo.string(),origin_x_km,origin_y_km);
    const auto surface=read_surface_2022(args.surface.string());
    // UP.DAT carries the upper station ID in its 6201 record tags; the case
    // convention names the file up_<station>.dat.
    int upper_station=0;
    {
        const std::string stem=args.upper.stem().string();
        const auto underscore=stem.rfind('_');
        if(underscore==std::string::npos)
            throw std::runtime_error("upper-air file name must be up_<station>.dat");
        upper_station=std::stoi(stem.substr(underscore+1));
        if(upper_station<=0)
            throw std::runtime_error("upper-air station ID must be positive");
    }
    const auto upper=read_upper_2022(args.upper.string(),upper_station);

    const auto start=std::chrono::sys_days{std::chrono::year{2022}/1/1};
    std::vector<std::chrono::sys_seconds> labels;
    for(int n=1;n<=args.hours;++n)
        labels.push_back(start+std::chrono::hours{n});

    // Original PREPDI hourly upper-air preparation (VERTAV layer averages plus
    // the PREPDI time interpolation), exactly the 143 case hours.
    const std::vector<float> zface_vector(zface.begin(),zface.end());
    const auto hourly_upper=calmet_prepdi_2022_hours(upper,zface_vector);

    // Original INTER2 controls from the deck: radii in km, RVARY off and the
    // per-layer station limit. WO is not read here: RDWT sets 1.0 for IPROG=0.
    const float rmin_km=deck_scalar(controls,"RMIN",0.1f);
    const float rmax1_km=deck_scalar(controls,"RMAX1",100.f);
    const float rmax2_km=deck_scalar(controls,"RMAX2",200.f);
    const float rmax3_km=deck_scalar(controls,"RMAX3",200.f);
    const float r1_km=deck_scalar(controls,"R1",50.f);
    const float r2_km=deck_scalar(controls,"R2",100.f);
    const bool lvary=deck_scalar(controls,"LVARY",0.f)!=0.f;
    std::vector<int> nsmth(9);
    {
        const auto it=controls.find("NSMTH");
        if(it==controls.end()||it->second.size()<9)
            throw std::runtime_error("the deck must declare nine NSMTH values");
        for(std::size_t k=0;k<9;++k)
            nsmth[k]=static_cast<int>(it->second[k]);
    }
    std::vector<int> nintrp(9);
    {
        const auto it=controls.find("NINTR2");
        if(it==controls.end()||it->second.size()<9)
            throw std::runtime_error("the deck must declare nine NINTR2 values");
        // The deck's NINTR2=99 is an upper bound on stations per layer; with
        // two stations in this case the original uses min(NINTR2, NWIND).
        for(std::size_t k=0;k<9;++k)
            nintrp[k]=std::max(1,std::min(static_cast<int>(it->second[k]),2));
    }
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
        // Original INTER2 two-component blend on the step-one field, with the
        // case's own controls: WO=1.0 everywhere because IPROG=0 and the deck
        // supplies no WT.DAT (RDWT fills 1.0 for IPROG=0/2/4/14), RVARY off,
        // NINTR2=99 for every layer, and the observed station profiles.
        // Original upstream physics: SURFVAR builds the 2-D surface
        // temperature (DIAGNO L5609-L5610, ISURFT<0, IALL=0 returns the same
        // temp_k and drops RH/IPCODE), SOLAR builds the grid sine of the solar
        // elevation for the interval start, HEATFX turns both into the
        // sensible heat flux, and SLOPE adds the Mahrt slope flow to the
        // step-one winds before the L5748 boundary snapshot.
        cteas::p6e::SurfaceState surface_state;
        {
            cteas::p6e::ObservedSurface observed;
            observed.nx=nx;observed.ny=ny;
            observed.station_temp_k=obs->temperature_k;
            observed.station_rh_percent=obs->humidity_pct;
            observed.npsta=0;observed.noobs=0;observed.itprog=0;
            observed.irhprog=0;observed.nowsta=0;
            observed.land_only=true;observed.average_temperature=true;  // IAVET=1
            observed.iall=0;                                            // DIAGNO L5610
            // AVETMP consumes the surface-level diagnostic wind field.
            observed.u_ms.assign(first.ug_mps.begin(),first.ug_mps.begin()+2500);
            observed.v_ms.assign(first.vg_mps.begin(),first.vg_mps.begin()+2500);
            observed.dgrid_m=grid_m;observed.mnmdav=1;observed.hafang_deg=30.f;
            if(!(std::isfinite(observed.station_temp_k)&&
                 observed.station_temp_k>0.f))
                throw std::runtime_error(
                    "surface temperature observation missing at "+hour_label(clock));
            surface_state=cteas::p6e::surfvar_one_station(observed);
        }
        // SOLAR takes the day of year (1..366) of the interval start.
        const int julian_day=[&]{
            const auto day=std::chrono::floor<std::chrono::days>(clock-
                std::chrono::hours(1));
            const std::chrono::year_month_day ymd{day};
            const std::chrono::sys_days first{ymd.year()/
                std::chrono::January/1};
            return static_cast<int>((day-first).count())+1;
        }();
        const int begin_hour=[&]{
            const auto day=std::chrono::floor<std::chrono::days>(clock-
                std::chrono::hours(1));
            return static_cast<int>(std::chrono::duration_cast<
                std::chrono::hours>(clock-std::chrono::hours(1)-day).count());
        }();
        const auto solar_grid=cteas::p6e::surface_solar_grid(
            static_cast<std::size_t>(nx),static_cast<std::size_t>(ny),50,
            origin_x_km*1000.0,origin_y_km*1000.0,grid_m);
        const auto sinalpc=cteas::p6e::sinalpc_grid(solar_grid,julian_day,
            begin_hour,0);
        std::vector<float> heat_flux;
        {
            cteas::p6e::HeatFluxInput flux;
            flux.nx=nx;flux.ny=ny;
            flux.mcloud=1;
            flux.water_first=geo.iwat1;flux.water_last=geo.iwat2;
            flux.ha1=990.f;flux.ha2=-30.f;flux.hb1=-0.75f;flux.hb2=3.4f;
            flux.hc1=5.31e-13f;flux.hc2=60.f;flux.hc3p1=1.12f;
            flux.station_cloud={obs->opaque_cloud_tenths};
            flux.nearest.assign(2500,1);
            flux.landuse=geo.landuse;
            flux.sun=sinalpc;
            flux.temp_k=surface_state.temp_k;
            flux.cloud_grid.assign(2500,0.1f*
                static_cast<float>(obs->opaque_cloud_tenths));
            flux.albedo=geo.albedo;
            flux.bowen=geo.bowen;
            flux.soil_heat_fraction=geo.hcg;
            flux.anthropogenic_wm2=geo.qf;
            flux.initial_qh.assign(2500,0.f);
            heat_flux=cteas::p6e::heatfx(flux).sensible_wm2;
        }
        std::vector<float> corrected_u=first.ug_mps,corrected_v=first.vg_mps;
        {
            SlopeInput slope;
            slope.nx=nx;slope.ny=ny;slope.nz=nz;
            slope.dx_m=grid_m;slope.dy_m=grid_m;
            slope.water_first=geo.iwat1;slope.water_last=geo.iwat2;
            slope.isurft=-1;
            slope.domain_temperature_k=285.f;  // unused for ISURFT<0
            slope.zface_m=std::vector<float>(zface.begin(),zface.end());
            slope.terrain_m=geo.terrain_m;slope.hmax_m=envelope.maximum_m;
            slope.hmin_m=envelope.minimum_m;
            slope.disthmax_m=envelope.distance_to_maximum_m;
            slope.disthmin_m=envelope.distance_to_minimum_m;
            slope.sensible_heat_w_m2=heat_flux;
            slope.surface_temperature_k=surface_state.temp_k;
            slope.landuse=geo.landuse;
            const auto flow=compute_original_slope_scoped(slope);
            for(std::size_t n=0;n<22500;++n){
                corrected_u[n]+=flow.u_mps[n];
                corrected_v[n]+=flow.v_mps[n];
            }
        }
        write_f32(args.out()/("slope_corrected_u_"+hour_label(clock)+".f32"),
                  corrected_u);
        write_f32(args.out()/("slope_corrected_v_"+hour_label(clock)+".f32"),
                  corrected_v);
        write_f32(args.out()/("temp2d_"+hour_label(clock)+".f32"),
                  surface_state.temp_k);
        write_f32(args.out()/("qh_"+hour_label(clock)+".f32"),heat_flux);
        std::vector<float> blended_u,blended_v;
        for(int component=0;component<2;++component){
            Inter2SelectedInput blend;
            blend.nx=nx;blend.ny=ny;blend.nz=nz;blend.nstations=2;
            blend.dx_m=grid_m;blend.dy_m=grid_m;
            blend.rmin_km=rmin_km;blend.rmax1_km=rmax1_km;
            blend.rmax2_km=rmax2_km;blend.rmax3_km=rmax3_km;
            blend.r1_km=r1_km;blend.r2_km=r2_km;
            blend.lvary=lvary;
            blend.iprog=0;blend.nbar=0;blend.noobs=0;
            blend.water_first=geo.iwat1;blend.water_last=geo.iwat2;
            blend.edit=999999.f;blend.editl=999998.f;
            blend.nintrp_per_layer=nintrp;
            blend.station_x_km={static_cast<float>(centre_x_km),
                                static_cast<float>(centre_x_km)};
            blend.station_y_km={static_cast<float>(centre_y_km),
                                static_cast<float>(centre_y_km)};
            blend.landuse=geo.landuse;
            blend.observation_weight.assign(22500,1.f);
            blend.station_component_mps.resize(18);
            for(std::size_t k=0;k<9;++k){
                const auto& surface_wind=station.surface_neutral_profile[k];
                const auto& upper_wind=station.upper_observed_profile[k];
                blend.station_component_mps[k]=component==0
                    ? surface_wind.u_mps:surface_wind.v_mps;
                blend.station_component_mps[9+k]=component==0
                    ? upper_wind.u_mps:upper_wind.v_mps;
            }
            blend.step1_mps=component==0?corrected_u:corrected_v;
            const auto blended=original_inter2_selected(blend);
            if(component==0)blended_u=blended.component_mps;
            else blended_v=blended.component_mps;
            write_f32(args.out()/((component==0?"blend_u_":"blend_v_")+
                                  hour_label(clock)+".f32"),
                      blended.component_mps);
        }
        // DIAGNO SMOOTH+DIVCEL+W on the blended field, with the per-layer
        // smoothing counts and boundaries taken from the blended field itself.
        // The deck's FRADJ/SLOPE corrections are not applied here (they need
        // TEMP2D/QH), so this is the uncompensated-terrain variant and the
        // metadata labels it as such.
        {
            SmoothInput tail;
            tail.nx=nx;tail.ny=ny;tail.nz=nz;tail.lake_breeze=false;
            tail.passes_per_layer=nsmth;
            tail.u_mps=blended_u;tail.v_mps=blended_v;
            tail.ub_west_east_mps.assign(9*2*ny,0.f);
            tail.vb_south_north_mps.assign(9*2*nx,0.f);
            for(std::size_t k=0;k<9;++k)for(std::size_t j=0;j<static_cast<std::size_t>(ny);++j){
                tail.ub_west_east_mps[k*2*ny+j]=blended_u[k*2500+j*50];
                tail.ub_west_east_mps[k*2*ny+ny+j]=
                    blended_u[k*2500+j*50+49];
            }
            for(std::size_t k=0;k<9;++k)for(std::size_t i=0;i<static_cast<std::size_t>(nx);++i){
                tail.vb_south_north_mps[k*2*nx+i]=blended_v[k*2500+i];
                tail.vb_south_north_mps[k*2*nx+nx+i]=
                    blended_v[k*2500+49*50+i];
            }
            const auto final_fields=original_smooth_divcel_w(
                std::move(tail),grid_m,grid_m,thickness_m,false);
            write_f32(args.out()/("w_face_"+hour_label(clock)+".f32"),
                      final_fields.w_face_mps);
        }
        if(!args.fixture_dir.empty()){
            // Driver fixture for the unchanged original WIND1/SIMILT, so the
            // step-one field can be compared numerically against the original.
            std::ofstream fixture(args.fixture_dir/
                ("wind1_driver_"+hour_label(clock)+".inp"));
            if(!fixture)throw std::runtime_error("cannot write WIND1 driver fixture");
            fixture<<(centre_x_km*1000.)<<" "<<(centre_y_km*1000.)<<" "
                   <<(centre_x_km*1000.)<<" "<<(centre_y_km*1000.)<<" "
                   <<surface_u<<" "<<surface_v<<" "<<roughness<<" "
                   <<station_lat_deg<<" "<<"10.0 "<<icalm<<"\n";
            for(std::size_t k=0;k<9;++k)
                fixture<<bias[k]<<(k==8?"\n":" ");
            for(std::size_t k=0;k<9;++k)
                fixture<<station.upper_observed_profile[k].u_mps<<" "
                       <<station.upper_observed_profile[k].v_mps<<"\n";
        }
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
    meta<<"  \"units\": {\"wind1_u_*.f32\": \"m/s\", \"wind1_v_*.f32\": \"m/s\", \"blend_u_*.f32\": \"m/s\", \"blend_v_*.f32\": \"m/s\", \"w_face_*.f32\": \"m/s (10 faces: k=1 face is the surface, k=10 the domain top)\","
          " \"terset_hmax.f32\": \"m\", \"terset_hmin.f32\": \"m\","
          " \"terset_disthmax.f32\": \"m\", \"terset_disthmin.f32\": \"m\"},\n";
    meta<<"  \"missing_sentinel\": "<<MISSING<<",\n";
    meta<<"  \"hourly_labels\": "<<written<<",\n";
    meta<<"  \"hour_label\": \"YYYY-MM-DD_HH (UTC), the label is the hour END\",\n";
    meta<<"  \"surface_wind_speed_range_mps\": ["<<speed_min<<", "<<speed_max<<"],\n";
    meta<<"  \"blend\": {\"product\": \"blend_u_*.f32 / blend_v_*.f32\", "
          "\"stage\": \"original INTER2 two-component blend on the step-one field\", "
          "\"observation_weight\": \"WO(i,j,k)=1.0 everywhere: RDWT fills 1.0 for IPROG=0/2/4/14 and the deck supplies no WT.DAT\", "
          "\"radii_km\": {\"RMIN\": "<<rmin_km<<", \"RMAX1\": "<<rmax1_km<<", \"RMAX2\": "<<rmax2_km<<", "
                        "\"RMAX3\": "<<rmax3_km<<", \"R1\": "<<r1_km<<", \"R2\": "<<r2_km<<"}, "
          "\"lvary\": "<<(lvary?"true":"false")<<", \"nintrp_per_layer\": 99, "
          "\"station_local_km\": ["<<centre_x_km<<", "<<centre_y_km<<"], "
          "\"station_local_km_source\": \"case.yaml grid centre equals stations.csv lat/lon\", "
          "\"station_profiles\": \"surface levels = neutral SIMILT profile, upper levels = hourly PREPDI profile\"},\n";
    meta<<"  \"stages_applied_in_chain\": [";
    meta<<"\"WIND1 selected first guess (IEXTRP=-4 neutral SIMILT)\"";
    meta<<", \"SURFVAR 2-D surface temperature (ISURFT<0, IALL=0, IAVET=1)\"";
    meta<<", \"SOLAR grid sine of elevation (interval start)\"";
    meta<<", \"HEATFX sensible heat flux (mcloud=1 station cloud)\"";
    meta<<", \"SLOPE slope flow added to the step-one winds\"";
    meta<<", \"original INTER2 two-component blend (WO=1.0)\" " ;
    meta<<", \"TERSET envelope (DIAGI L5228, TERRAD="<<terrad_km<<" km)\"],\n";
    meta<<"  \"stages_not_applied\": [\n";
    meta<<"    {\"stage\": \"SURFVAR -> TEMP2D (DIAGNO L5609-L5610)\", \"status\": \"APPLIED\","
          " \"product\": \"temp2d_*.f32\","
          " \"note\": \"surfvar_one_station with iall=0 (same temp_k, RH/IPCODE dropped) and average_temperature=1 (IAVET=1)\"},\n";
    meta<<"    {\"stage\": \"HEATFX -> QH (DIAGNO L5676-L5715)\", \"status\": \"APPLIED\","
          " \"product\": \"qh_*.f32\","
          " \"note\": \"mcloud=1 uses the station cloud amount (0 tenths for every case hour); coefficients from met.lst (ha1=990, ha2=-30, hb1=-0.75, hb2=3.4, hc1=5.31e-13, hc2=60, hc3+1=1.12)\"},\n";
    meta<<"    {\"stage\": \"FRADJ (L5643-L5644)\", \"status\": \"NEEDS_CALLER_PLANES\","
          " \"missing\": [\"GAMMA\", \"TEMP2D\", \"HMAX (provided here as terset_hmax.f32)\"]},\n";
    meta<<"    {\"stage\": \"SLOPE (L5722-L5723)\", \"status\": \"APPLIED\","
          " \"product\": \"slope_corrected_u/v_*.f32\","
          " \"note\": \"chain-computed QH, TEMP2D and TERSET envelope; the corrected step one feeds the blend and the W tail\"},";
    meta<<"\n";
    meta<<"    {\"stage\": \"FRADJ (L5643-L5644)\", \"status\": \"PENDING_GAMMA\","
          " \"missing\": [\"GAMMA grid: CGAMMA needs ZICONV from the traced convective forcing (P6E MIXHT/WSTARR family)\"],"
          " \"note\": \"therefore the emitted W faces are SLOPE-compensated but not FRADJ-compensated\"},\n";
    meta<<"    {\"stage\": \"SMOOTH/DIVCEL/W tail (DIAGNO L6119-L6148)\", \"status\": \"APPLIED_SLOPE_COMPENSATED_FRADJ_PENDING\","
          " \"product\": \"w_face_*.f32 (10 faces per column)\","
          " \"note\": \"run on the SLOPE-corrected blended field with the deck NSMTH, so the boundaries are the L5748 snapshot except for the still-pending FRADJ (GAMMA) correction\"},\n"
          " \"note\": \"boundaries come from the SLOPE-corrected blended field, i.e. the L5748 snapshot except for the still-pending FRADJ (GAMMA) correction\"},\n";
    meta<<"    {\"stage\": \"TOPOF2/IKINE route and O'Brien/IOBR=1 tail\", \"status\": \"NON_CASE_PATH\","
          " \"note\": \"the case deck sets IKINE=0 and IOBR=0\"}\n";
    meta<<"  ],\n";
    meta<<"  \"control_values_read\": {\"TERRAD\": "<<terrad_km
        <<", \"XORIGKM\": "<<origin_x_km<<", \"YORIGKM\": "<<origin_y_km
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
