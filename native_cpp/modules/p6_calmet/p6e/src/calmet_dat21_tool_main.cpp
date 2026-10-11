// P6E CALMET.DAT 2.1 output-contract tool.
//
//   emit-synthetic --out FILE [--hours N] [--nx N --ny N --nz N]
//        Writes a layout-exact CALMET.DAT 2.1 whose micrometeorological
//        fields come from the real P6E chain: E13 MIXHT produces ZI, E14
//        TEMP3D produces T-LEV1..NZ from that ZI and from the 2-D TEMPK
//        field. USTAR/EL/WSTAR/RHO/QSW/IRH/IPGT and the P6D-owned wind
//        fields are deterministic synthetic case inputs, not physics claims.
//
//   check --file FILE [--out JSON] [--rewrite FILE]
//        --rewrite re-serialises the parsed dataset, which must be
//        byte-identical to the input for a faithful round trip.
//        Parses and validates a CALMET.DAT 2.1 file, reports the per-field
//        inventory (denominator, missing counts, min/max/mean), which fields
//        the P6E kernels own, which are not implemented in P6E, and the
//        source-derived invariants.
//
//   compare --reference FILE --candidate FILE [--out JSON]
//        Validates both layouts and reports per-field per-cell error
//        statistics with the frozen tolerances of the corresponding source
//        gates. Layout or unit mismatches fail closed.
#include "cteas/p6e/calmet_dat21.hpp"
#include "cteas/p6e/avetmp.hpp"
#include "cteas/p6e/elustr_daytime.hpp"
#include "cteas/p6e/elustr_stable.hpp"
#include "cteas/p6e/geo_landuse.hpp"
#include "cteas/p6e/heatfx.hpp"
#include "cteas/p6e/mixht_observed_mc.hpp"
#include "cteas/p6e/wstarr.hpp"
#include "cteas/p6e/pgtstb.hpp"
#include "cteas/p6e/solar.hpp"
#include "cteas/p6e/surface_solar.hpp"
#include "cteas/p6e/surfvar_multistation.hpp"
#include "cteas/p6e/temp3d_observed_land.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace cteas::p6e;

struct Tolerance { double abs_limit; double rel_limit; };

// Frozen limits inherited from the accepted independent source gates.
Tolerance tolerance_for(const std::string& label) {
    if(label=="IPGT"||label=="IRH")return {0.0,0.0};
    if(label=="USTAR")return {3e-5,5e-5};
    if(label=="ZI")return {0.07,8e-5};
    if(label=="EL")return {0.04,5e-5};
    if(label=="WSTAR")return {2e-5,3e-5};
    if(label=="TEMPK")return {3e-4,1e-5};
    if(label=="RHO")return {1e-5,1e-5};
    if(label=="QSW")return {0.02,3e-5};
    if(label.rfind("T-LEV",0)==0)return {5e-4,1e-5};
    if(label.rfind("U-LEV",0)==0||label.rfind("V-LEV",0)==0||
       label.rfind("WFACE",0)==0)return {1e-4,1e-5};
    if(label=="RMM")return {0.02,3e-5};
    if(label=="IPCODE")return {0.0,0.0};
    return {1e-4,1e-5};
}
const char* unit_name(CalmetDat21Unit unit) {
    switch(unit) {
    case CalmetDat21Unit::metre:return "m";
    case CalmetDat21Unit::metre_per_second:return "m/s";
    case CalmetDat21Unit::kelvin:return "K";
    case CalmetDat21Unit::kilogram_per_cubic_metre:return "kg/m3";
    case CalmetDat21Unit::watt_per_square_metre:return "W/m2";
    case CalmetDat21Unit::percent:return "percent";
    case CalmetDat21Unit::stability_code:return "code";
    case CalmetDat21Unit::precipitation_code:return "code";
    }
    return "unknown";
}
const char* owner_name(CalmetDat21Owner owner) {
    switch(owner) {
    case CalmetDat21Owner::p6e:return "p6e";
    case CalmetDat21Owner::p6d:return "p6d";
    case CalmetDat21Owner::case_input:return "case_input";
    }
    return "unknown";
}
std::string json_string(const std::string& s) {
    std::string out="\"";
    for(const char c:s) {
        if(c=='"'||c=='\\')out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}
std::string json_number(double v) {
    if(!std::isfinite(v))return "null";
    char buffer[64];
    std::snprintf(buffer,sizeof(buffer),"%.17g",v);
    return buffer;
}

CalmetDat21Time time_at(const CalmetDat21Time& start,std::size_t hour_offset) {
    CalmetDat21Time t=start;
    for(std::size_t k=0;k<hour_offset;++k)t=calmet21_next_hour(t);
    return t;
}

// -------------------------------------------------------------- synthetic
void emit_synthetic(const std::string& path,std::size_t hours,int nx,int ny,int nz) {
    CalmetDat21Header header;
    header.nx=nx;header.ny=ny;header.nz=nz;
    header.dgrid_m=100.f;header.xorigin_m=510885.f;header.yorigin_m=3531269.f;
    header.timezone="UTC+0000";
    header.nssta=1;header.nusta=1;header.npsta=0;header.nowsta=0;
    header.nlu=14;header.iwat1=50;header.iwat2=55;header.lcalgrd=true;
    header.run_hours=static_cast<int>(hours);
    header.begin_year=2022;header.begin_month=1;header.begin_day=1;
    header.begin_hour=0;header.begin_second=0;
    header.end_year=2022;header.end_month=1;header.end_day=1;
    header.end_hour=static_cast<int>(hours)-1;header.end_second=0;
    header.comments={"P6E CALMET.DAT 2.1 output-contract synthetic fixture",
                     "Micrometeorology from E13 MIXHT and E14 TEMP3D;",
                     "U/V/W and all remaining 2-D fields are synthetic inputs"};
    const auto cells=static_cast<std::size_t>(nx)*static_cast<std::size_t>(ny);

    MixhtObservedMcInput mix;
    mix.nx=static_cast<std::size_t>(nx);mix.ny=static_cast<std::size_t>(ny);
    mix.water_first=50;mix.water_last=55;mix.iupt=-1;mix.imixh=1;
    mix.izicrlx=1;mix.timestep_s=3600;
    mix.constn=2400.f;mix.zimin=20.f;mix.zimax=2500.f;
    mix.threshl=0.f;mix.dptmin=0.001f;mix.dzzi=200.f;
    mix.onedte=4140.f;mix.twodte=1080.f;mix.tzicrlx=800.f;
    mix.justa={1,-1};
    for(auto& buffer:mix.sounding)buffer.resize(2);
    const std::vector<float> levels={0.f,100.f,250.f,400.f,600.f,850.f,
                                     1150.f,1500.f,1900.f,2400.f};
    for(std::size_t station=0;station<2;++station)
        for(std::size_t buffer=0;buffer<2;++buffer) {
            MixdtSounding s;
            const float lapse=(station==0)?0.0075f:0.0085f;
            for(const float z:levels) {
                s.height_m.push_back(z);
                s.temp_k.push_back(300.f-lapse*z);
            }
            mix.sounding[buffer][station]=s;
        }
    mix.nearu.resize(cells);mix.landuse.resize(cells);
    mix.qh.resize(cells);mix.ustar.resize(cells);mix.el.resize(cells);
    mix.rho.resize(cells);mix.cmech.resize(cells);mix.fcori.resize(cells);
    mix.initial_zi.assign(cells,-7777.f);
    mix.initial_ziconv.resize(cells);mix.initial_dptt.resize(cells);
    for(int i=0;i<nx;++i)for(int j=0;j<ny;++j) {
        const auto k=static_cast<std::size_t>(j)*static_cast<std::size_t>(nx)+
                     static_cast<std::size_t>(i);
        const bool water=(i+2*j)%23==0;
        const bool night=((i+3*j)%7)==0;
        mix.landuse[k]=water?55:12;
        mix.nearu[k]=1+((i+j)%2);
        mix.qh[k]=(water||night)?-0.1f:80.f;
        mix.el[k]=mix.qh[k]<0.f?(20.f+5.f*static_cast<float>((i+j)%4))
                                :-(40.f+20.f*static_cast<float>((i+j)%5));
        mix.ustar[k]=0.15f+0.02f*static_cast<float>((i+j)%4);
        mix.rho[k]=1.15f;
        mix.cmech[k]=110.f;
        mix.fcori[k]=0.00007f;
        mix.initial_ziconv[k]=water?100.f:(60.f+10.f*static_cast<float>((i+j)%5));
        mix.initial_dptt[k]=0.3f;
    }
    const auto mixing=mixht_observed_mc(mix);

    CalmetDat21Static statics;
    for(int face=0;face<=nz;++face)
        statics.zface_m.push_back(120.f*static_cast<float>(face));
    statics.surface_x_m={2500.f};statics.surface_y_m={2500.f};
    statics.upper_x_m={2500.f};statics.upper_y_m={2500.f};
    statics.z0_m.resize(cells);statics.elev_m.resize(cells);
    statics.xlai.resize(cells);statics.ilandu.resize(cells);statics.nears.resize(cells);
    for(std::size_t k=0;k<cells;++k) {
        statics.z0_m[k]=mix.landuse[k]==55?0.0002f:0.5f;
        statics.elev_m[k]=20.f+5.f*static_cast<float>(k%17);
        statics.xlai[k]=mix.landuse[k]==55?0.f:1.5f;
        statics.ilandu[k]=mix.landuse[k];
        statics.nears[k]=1;
    }

    Temp3dObservedInput temp3d;
    temp3d.nx=static_cast<std::size_t>(nx);temp3d.ny=static_cast<std::size_t>(ny);
    temp3d.nz=static_cast<std::size_t>(nz);
    temp3d.dgrid_m=header.dgrid_m;temp3d.irad=1;temp3d.current_hour_utc=6;
    temp3d.face_z_m=statics.zface_m;
    temp3d.station_x_m={25.f,25.f};temp3d.station_y_m={25.f,25.f};
    temp3d.justa={1,-1};
    temp3d.aa_face_temp_k.assign(2U*(static_cast<std::size_t>(nz)+1U),0.f);
    temp3d.bb_face_temp_k.assign(2U*(static_cast<std::size_t>(nz)+1U),0.f);
    for(std::size_t station=0;station<2;++station) {
        const float lapse=(station==0)?0.0075f:0.0085f;
        for(int face=1;face<=nz;++face) {
            const float z=statics.zface_m[static_cast<std::size_t>(face)];
            temp3d.aa_face_temp_k[station*(static_cast<std::size_t>(nz)+1U)+
                                   static_cast<std::size_t>(face)]=300.f-lapse*z;
            temp3d.bb_face_temp_k[station*(static_cast<std::size_t>(nz)+1U)+
                                   static_cast<std::size_t>(face)]=299.f-lapse*z;
        }
    }
    temp3d.surface_temp_k.resize(cells);
    temp3d.convective_zi_m.resize(cells);
    for(std::size_t k=0;k<cells;++k) {
        temp3d.surface_temp_k[k]=268.f+2.f*static_cast<float>((k*3U+k/7U)%7U);
        temp3d.convective_zi_m[k]=mixing.ziconv[k];
    }
    const auto columns=temp3d_observed_land(temp3d);

    const auto labels=calmet21_hourly_labels(nz,header.irtype,header.lcalgrd,
                                             header.npsta);
    const CalmetDat21Time start{2022,1,0,0};
    std::vector<CalmetDat21Field> hourly;
    for(std::size_t hour=0;hour<hours;++hour) {
        const CalmetDat21Time begin=time_at(start,hour);
        CalmetDat21Time end=begin;
        end.seconds=begin.seconds+3600;
        for(const std::string& label:labels) {
            CalmetDat21Field field;
            field.label=label;field.begin=begin;field.end=end;
            field.integer=label=="IPGT"||label=="IRH";
            if(field.integer) {
                field.integer_values.resize(cells);
                for(std::size_t k=0;k<cells;++k) {
                    if(label=="IPGT")field.integer_values[k]=2+static_cast<std::int32_t>((k%4U));
                    else field.integer_values[k]=static_cast<std::int32_t>(40U+(k*7U)%60U);
                }
            } else if(label.rfind("T-LEV",0)==0) {
                const auto level=static_cast<std::size_t>(
                    std::stoi(label.substr(5U)));
                field.real_values.resize(cells);
                for(std::size_t k=0;k<cells;++k)
                    field.real_values[k]=columns[level*cells+k];
            } else {
                field.real_values.resize(cells);
                for(std::size_t k=0;k<cells;++k) {
                    const float wave=static_cast<float>((k+hour)%7U);
                    if(label=="ZI")field.real_values[k]=mixing.zi[k];
                    else if(label=="USTAR")field.real_values[k]=0.05f+0.02f*wave;
                    else if(label=="EL")field.real_values[k]=-120.f-20.f*wave;
                    else if(label=="WSTAR")field.real_values[k]=0.4f+0.05f*wave;
                    else if(label=="TEMPK")field.real_values[k]=temp3d.surface_temp_k[k];
                    else if(label=="RHO")field.real_values[k]=1.28f+0.01f*wave;
                    else if(label=="QSW")field.real_values[k]=140.f+10.f*wave;
                    else if(label.rfind("U-LEV",0)==0)field.real_values[k]=-1.5f+0.1f*wave;
                    else if(label.rfind("V-LEV",0)==0)field.real_values[k]=-2.0f+0.1f*wave;
                    else field.real_values[k]=(std::stoi(label.substr(5U))%2==0)?0.05f:-0.05f;
                }
            }
            hourly.push_back(std::move(field));
        }
    }
    write_calmet_dat21(path,header,statics,hourly);
}

// -------------------------------------------------------------- real case
// Real 2022 case options resolved from run/met/met.lst:
//   IMIXH=1 THRESHL=0 IZICRLX=1 ISURFT=-1 IUPT=-1 ITPROG=0 NOOBS=0 NOWSTA=0
//   IWFCOD=1 LCALGRD=T NSSTA=1 NUSTA=1 NPSTA=0 IAVET=1 IFRADJ=1 IAVEZI=1
//   IRAD=1 NUMTS=5 TRADKM=500 ICOARE=10 IRHPROG=0 NZ=9 DGRID=100
// Only the surface fields that the original computes WITHOUT the diagnostic
// wind field can be produced here: SURFVAR's temperature and relative
// humidity interpolation. `temp2d(i,j)=ztemp(i,j,1)` and TEMP3D make layer 1
// of T-LEV equal to that same temperature field. Everything else is written
// as the original missing sentinel so it is reported as "not produced"
// instead of being filled with invented values.
struct BridgeObservation {
    double air_temperature_k=9999.;
    double humidity_pct=9999.;
    double opaque_cloud_tenths=9999.;
    double pressure_mb=9999.;
    int ceiling_hundreds_ft=9999;
    int justa=1;
    MixdtSounding aa,bb;
    std::vector<float> aa_face,bb_face;
};
std::vector<BridgeObservation> read_bridge_csv(const std::string& path) {
    std::ifstream file(path);
    if(!file)throw std::invalid_argument("cannot open bridge CSV "+path);
    std::string line;
    if(!std::getline(file,line))throw std::invalid_argument("empty bridge CSV");
    std::vector<std::string> header;
    {
        std::string token;
        std::istringstream stream(line);
        while(std::getline(stream,token,','))header.push_back(token);
    }
    const auto column=[&](const char* name) {
        for(std::size_t k=0;k<header.size();++k)
            if(header[k]==name)return k;
        throw std::invalid_argument(std::string("bridge CSV lacks column ")+name);
    };
    const auto temperature=column("air_temperature_k");
    const auto humidity=column("humidity_pct");
    const auto cloud=column("opaque_cloud_tenths");
    const auto pressure=column("pressure_mb");
    const auto ceiling=column("ceiling_hundreds_ft");
    const auto justa=column("justa");
    const auto aa_profile=column("aa_profile");
    const auto bb_profile=column("bb_profile");
    const auto aa_face=column("aa_face_t");
    const auto bb_face=column("bb_face_t");
    const auto unpack_profile=[](const std::string& text,MixdtSounding& sounding) {
        std::size_t position=0;
        while(position<text.size()) {
            const auto separator=text.find(';',position);
            const auto entry=text.substr(position,separator==std::string::npos
                                         ?std::string::npos:separator-position);
            const auto colon=entry.find(':');
            if(colon==std::string::npos)
                throw std::invalid_argument("bridge profile entry malformed");
            sounding.height_m.push_back(
                static_cast<float>(std::stod(entry.substr(0U,colon))));
            sounding.temp_k.push_back(
                static_cast<float>(std::stod(entry.substr(colon+1U))));
            if(separator==std::string::npos)break;
            position=separator+1U;
        }
        if(sounding.height_m.empty())
            throw std::invalid_argument("bridge profile is empty");
    };
    const auto unpack_faces=[](const std::string& text,std::vector<float>& values) {
        std::size_t position=0;
        while(position<text.size()) {
            const auto separator=text.find(';',position);
            values.push_back(static_cast<float>(std::stod(
                text.substr(position,separator==std::string::npos
                            ?std::string::npos:separator-position))));
            if(separator==std::string::npos)break;
            position=separator+1U;
        }
    };
    std::vector<BridgeObservation> observations;
    while(std::getline(file,line)) {
        if(line.empty())continue;
        std::vector<std::string> fields;
        std::string token;
        std::istringstream stream(line);
        while(std::getline(stream,token,','))fields.push_back(token);
        if(fields.size()<=std::max(std::max(std::max(temperature,humidity),
                                           std::max(cloud,pressure)),ceiling))
            throw std::invalid_argument("bridge CSV short row");
        BridgeObservation observation;
        observation.air_temperature_k=std::stod(fields[temperature]);
        observation.humidity_pct=std::stod(fields[humidity]);
        observation.opaque_cloud_tenths=std::stod(fields[cloud]);
        observation.pressure_mb=std::stod(fields[pressure]);
        observation.ceiling_hundreds_ft=std::stoi(fields[ceiling]);
        observation.justa=std::stoi(fields[justa]);
        unpack_profile(fields[aa_profile],observation.aa);
        unpack_profile(fields[bb_profile],observation.bb);
        unpack_faces(fields[aa_face],observation.aa_face);
        unpack_faces(fields[bb_face],observation.bb_face);
        observations.push_back(observation);
    }
    if(observations.empty())throw std::invalid_argument("bridge CSV has no hours");
    return observations;
}

// U/V/W of a CALMET.DAT 2.1 wind source for the same intervals. Missing
// (9999.) values are rejected instead of being silently interpolated.
struct WindLevels {
    std::vector<std::vector<float>> u,v,w;
};
WindLevels read_wind_levels(const std::string& path,
                            const CalmetDat21Dataset& reference) {
    const auto wind=read_calmet_dat21(path);
    const auto& h=wind.header;
    const auto& r=reference.header;
    const auto same=[&](bool condition,const char* message) {
        if(!condition)throw std::invalid_argument(
            std::string("wind source incompatible: ")+message);
    };
    same(h.nx==r.nx&&h.ny==r.ny&&h.nz==r.nz,"grid extent differs");
    same(h.timezone==r.timezone,"time zone differs");
    same(h.lcalgrd==r.lcalgrd&&h.npsta==r.npsta,"output mode differs");
    same(wind.intervals==reference.intervals,"interval count differs");
    same(wind.fields_per_hour==reference.fields_per_hour,"field count differs");
    for(std::size_t hour=0;hour<reference.intervals;++hour) {
        const auto& a=wind.hourly[hour*wind.fields_per_hour].begin;
        const auto& b=reference.hourly[hour*reference.fields_per_hour].begin;
        same(calmet21_pack(a)==calmet21_pack(b)&&a.seconds==b.seconds,
             "hourly time labels differ");
    }
    const auto cells=static_cast<std::size_t>(h.nx)*static_cast<std::size_t>(h.ny);
    WindLevels out;
    out.u.resize(static_cast<std::size_t>(h.nz));
    out.v.resize(static_cast<std::size_t>(h.nz));
    out.w.resize(static_cast<std::size_t>(h.nz));
    for(auto& level:out.u)level.assign(reference.intervals*cells,0.f);
    for(auto& level:out.v)level.assign(reference.intervals*cells,0.f);
    for(auto& level:out.w)level.assign(reference.intervals*cells,0.f);
    for(std::size_t hour=0;hour<wind.intervals;++hour)
        for(std::size_t j=0;j<wind.fields_per_hour;++j) {
            const auto& field=wind.hourly[hour*wind.fields_per_hour+j];
            const auto level=[&]() -> int {
                if(field.label.rfind("U-LEV",0)==0||field.label.rfind("V-LEV",0)==0||
                   field.label.rfind("WFACE",0)==0)
                    return std::stoi(field.label.substr(5U));
                return 0;
            }();
            if(level<1||level>h.nz)continue;
            const auto index=static_cast<std::size_t>(level-1);
            for(std::size_t k=0;k<cells;++k) {
                const float value=field.real_values[k];
                if(value==calmet21_missing)
                    throw std::invalid_argument(
                        "wind source has missing values in "+field.label);
                if(!std::isfinite(value))
                    throw std::invalid_argument("wind source nonfinite "+field.label);
                const auto target=hour*cells+k;
                if(field.label.rfind("U-LEV",0)==0)out.u[index][target]=value;
                else if(field.label.rfind("V-LEV",0)==0)out.v[index][target]=value;
                else out.w[index][target]=value;
            }
        }
    return out;
}

void emit_realcase(const std::string& reference_path,const std::string& observations_path,
                   const std::string& out_path,const std::string& report_path,
                   float ha1,float ha2,float hb1,float hb2,
                   const std::string& wind_path,int mnmdav,float hafang,
                   const std::string& geo_path,float hc1,float hc2,float hc3,
                   const std::string& state_prefix,float fcoriol) {
    const auto reference=read_calmet_dat21(reference_path);
    // Optional wind source: the frozen original U/V/W for the same intervals.
    std::optional<WindLevels> wind;
    if(!wind_path.empty())wind=read_wind_levels(wind_path,reference);
    std::optional<GeoLandUse> geo;
    if(!geo_path.empty()) {
        geo=read_geo_landuse(geo_path,reference.header.nx,reference.header.ny);
        if(geo->ilandu!=reference.statics.ilandu)
            throw std::invalid_argument(
                "GEO.DAT land-use grid does not match the reference ILANDU");
        if(geo->iwat1!=reference.header.iwat1||geo->iwat2!=reference.header.iwat2)
            throw std::invalid_argument("GEO.DAT water range does not match the reference");
    }
    const auto observations=read_bridge_csv(observations_path);
    const auto& h=reference.header;
    if(observations.size()!=reference.intervals)
        throw std::invalid_argument("observation hours do not match the reference intervals");
    if(h.nssta<1||reference.statics.surface_x_m.empty())
        throw std::invalid_argument("reference carries no surface station coordinate");
    const auto cells=static_cast<std::size_t>(h.nx)*static_cast<std::size_t>(h.ny);
    // TRADKM=500 from met.lst; the kernel works in grid units.
    SurfvarMultistationInput surf;
    surf.nx=static_cast<std::size_t>(h.nx);
    surf.ny=static_cast<std::size_t>(h.ny);
    surf.dgrid_m=h.dgrid_m;
    surf.trad_grid=500000.f/h.dgrid_m;
    surf.irad=1;
    surf.numts=5;
    surf.stations.resize(1U);
    // XSSTA/YSSTA are stored in metres relative to the SW corner of grid cell
    // (1,1) - the same metres as the stored DGRID - so the source expression
    // `r2=(xxx*dgridi-xc)**2+(yyy*dgridi-yc)**2` is reproduced exactly.
    surf.stations[0].x_m=reference.statics.surface_x_m[0];
    surf.stations[0].y_m=reference.statics.surface_y_m[0];
    CalmetDat21Header candidate_header=h;
    candidate_header.comments={"P6E partial CALMET.DAT 2.1 candidate for the real 2022 case",
        "Geometry, station coordinates and time labels are copied run inputs",
        "Only TEMPK, IRH and T-LEV1 are produced; every other field is 9999. missing"};
    candidate_header.run_hours=static_cast<int>(reference.intervals);
    const auto labels=calmet21_hourly_labels(h.nz,h.irtype,h.lcalgrd,h.npsta);
    const int base_timezone=timezone_hours_from_axtz(h.timezone);
    // Grid solar geometry through the documented interface
    // cteas/p6e/surface_solar.hpp: UTM zone 50N / WGS-84 cell centres.
    const auto solar_grid=surface_solar_grid(
        static_cast<std::size_t>(h.nx),static_cast<std::size_t>(h.ny),50,
        h.xorigin_m,h.yorigin_m,h.dgrid_m);
    std::vector<CalmetDat21Field> hourly;
    std::size_t produced_temperature=0U,missing_humidity=0U;
    // Previous-hour MIXHT state for the ZICONV/DPTT memory (starts at zero,
    // the original SETUP initialisation for land cells).
    std::vector<float> previous_ziconv(cells,0.f),previous_dptt(cells,0.f);
    // Exported chain state (hour-major, i-fastest) when --state-prefix is set.
    std::vector<float> exported_ziconv,exported_zi;
    for(std::size_t hour=0;hour<reference.intervals;++hour) {
        const auto& observation=observations[hour];
        surf.stations[0].temp_k=static_cast<float>(observation.air_temperature_k);
        surf.stations[0].rh_pct=observation.humidity_pct>=9999.0
            ?9999:static_cast<int>(observation.humidity_pct);
        if(surf.stations[0].rh_pct==9999)++missing_humidity; else ++produced_temperature;
        auto surface=surfvar_multistation(surf);
        const auto& reference_first=
            reference.hourly[hour*reference.fields_per_hour];
        if(wind) {
            // SURFVAR L12878-12881: IAVET=1 averages TEMP2D with the
            // first-level diagnostic wind before anything else uses it.
            AvetmpInput a;
            a.nx=static_cast<std::size_t>(h.nx);
            a.ny=static_cast<std::size_t>(h.ny);
            a.mnmdav=mnmdav;a.hafang_deg=hafang;a.dgrid_m=h.dgrid_m;
            a.u_ms.resize(cells);a.v_ms.resize(cells);
            for(std::size_t k=0;k<cells;++k) {
                a.u_ms[k]=wind->u[0][hour*cells+k];
                a.v_ms[k]=wind->v[0][hour*cells+k];
            }
            a.temperature_k=surface.temp2d_k;
            surface.temp2d_k=avetmp(a);
        }
        // Documented interface: cteas/p6e/surface_solar.hpp.
        const float cloud_fraction=heatfx_cloud_fraction(
            static_cast<int>(observation.opaque_cloud_tenths));
        const auto sinalpc=sinalpc_grid(solar_grid,reference_first.begin.julian_day,
                                        reference_first.begin.hour,base_timezone);
        // airden() L2085: RHO(i,j) = 0.3484321 * PRES(nears(i,j)) / TEMP2D(i,j)
        if(!(observation.pressure_mb>0.0)||observation.pressure_mb>=9998.0)
            throw std::invalid_argument(
                "station surface pressure missing or out of range for airden");
        const float station_pressure_mb=static_cast<float>(observation.pressure_mb);
        std::vector<int> stability;
        if(wind) {
            // PGTSTB L13295-13498 with the first-level wind, MCLOUD=1 station
            // cloud/ceiling and the case land-use grid.
            PgtInput p;
            p.nx=static_cast<std::size_t>(h.nx);
            p.ny=static_cast<std::size_t>(h.ny);
            p.mcloud=1;p.water_first=h.iwat1;p.water_last=h.iwat2;
            p.u10.resize(cells);p.v10.resize(cells);
            p.sinalp=sinalpc;
            p.ccgrid.assign(cells,0.f);
            p.ceilgrid.assign(cells,0);
            p.landuse=reference.statics.ilandu;
            p.nearest.assign(cells,1);
            p.station_cloud={static_cast<int>(observation.opaque_cloud_tenths)};
            p.station_ceil={observation.ceiling_hundreds_ft};
            for(std::size_t k=0;k<cells;++k) {
                p.u10[k]=wind->u[0][hour*cells+k];
                p.v10[k]=wind->v[0][hour*cells+k];
            }
            stability=pgtstb(p);
        }
        std::vector<float> density(cells,0.f);
        for(std::size_t k=0;k<cells;++k)
            density[k]=0.3484321f*station_pressure_mb/surface.temp2d_k[k];
        std::vector<float> ustar,el,heat_flux,heatfx_shortwave_field;
        std::vector<float> zi_height,ziconv,wstar_field;
        if(geo&&wind) {
            // HEATFX L7999-8122 with the case's gridded land-use properties.
            HeatFluxInput hx;
            hx.nx=static_cast<std::size_t>(h.nx);
            hx.ny=static_cast<std::size_t>(h.ny);
            hx.mcloud=1;hx.water_first=h.iwat1;hx.water_last=h.iwat2;
            hx.ha1=ha1;hx.ha2=ha2;hx.hb1=hb1;hx.hb2=hb2;
            hx.hc1=hc1;hx.hc2=hc2;hx.hc3p1=hc3+1.f;
            hx.station_cloud={static_cast<int>(observation.opaque_cloud_tenths)};
            hx.nearest.assign(cells,1);
            hx.landuse=reference.statics.ilandu;
            hx.sun=sinalpc;
            hx.temp_k=surface.temp2d_k;
            hx.cloud_grid.assign(cells,0.f);
            hx.albedo=geo->albedo;
            hx.bowen=geo->bowen;
            hx.soil_heat_fraction=geo->soil_heat_fraction;
            hx.anthropogenic_wm2=geo->anthropogenic_wm2;
            hx.initial_qh.assign(cells,0.f);
            const auto flux=heatfx(hx);
            heat_flux=flux.sensible_wm2;
            heatfx_shortwave_field=flux.shortwave_wm2;
            // ELUSTR L6333-6579: one branch per cell, cells of the other
            // branch are marked as water so the kernel skips them.
            std::vector<int> stable_use(cells),day_use(cells);
            for(std::size_t k=0;k<cells;++k) {
                const bool land=reference.statics.ilandu[k]<h.iwat1||
                                reference.statics.ilandu[k]>h.iwat2;
                stable_use[k]=(land&&heat_flux[k]<0.f)?
                    reference.statics.ilandu[k]:h.iwat1;
                day_use[k]=(land&&heat_flux[k]>=0.f)?
                    reference.statics.ilandu[k]:h.iwat1;
            }
            StableElustr stable;
            stable.nx=static_cast<std::size_t>(h.nx);
            stable.ny=static_cast<std::size_t>(h.ny);
            stable.mcloud=1;stable.water_first=h.iwat1;stable.water_last=h.iwat2;
            stable.zm=0.5f*(reference.statics.zface_m[0]+reference.statics.zface_m[1]);
            stable.landuse=stable_use;
            stable.nearest.assign(cells,1);
            stable.station_cloud={static_cast<int>(observation.opaque_cloud_tenths)};
            stable.z0=reference.statics.z0_m;
            stable.u1.resize(cells);stable.v1.resize(cells);
            for(std::size_t k=0;k<cells;++k) {
                stable.u1[k]=wind->u[0][hour*cells+k];
                stable.v1[k]=wind->v[0][hour*cells+k];
            }
            stable.rho=density;stable.temp_k=surface.temp2d_k;
            stable.cloud_grid.assign(cells,0.f);
            stable.heat_flux_initial=heat_flux;
            const auto stable_result=stable_elustr(stable);
            DaytimeElustrInput day;
            day.nx=static_cast<std::size_t>(h.nx);
            day.ny=static_cast<std::size_t>(h.ny);
            day.water_first=h.iwat1;day.water_last=h.iwat2;day.ipsifcn=0;
            day.zm=stable.zm;
            day.landuse=day_use;
            day.z0=reference.statics.z0_m;
            day.u1=stable.u1;day.v1=stable.v1;
            day.rho=density;day.temp_k=surface.temp2d_k;
            day.heat_flux_initial=heat_flux;
            day.initial_ustar.assign(cells,0.05f);
            day.initial_el.assign(cells,-9.9e9f);
            const auto day_result=elustr_daytime(day);
            ustar.resize(cells);el.resize(cells);
            for(std::size_t k=0;k<cells;++k) {
                // ELUSTR updates the surface heat flux in place, and MIXHT /
                // WSTARR consume that updated value (the /HFLUX/ QH array).
                if(heat_flux[k]<0.f) {
                    ustar[k]=stable_result.ustar_ms[k];
                    el[k]=stable_result.monin_obukhov_m[k];
                    heat_flux[k]=stable_result.qh_wm2[k];
                } else {
                    ustar[k]=day_result.ustar_ms[k];
                    el[k]=day_result.monin_obukhov_m[k];
                    heat_flux[k]=day_result.qh_wm2[k];
                }
            }
            // MIXHT L10411-10683 (IMIXH=1 Maul-Carson, the real-case closure).
            MixhtObservedMcInput mh;
            mh.nx=static_cast<std::size_t>(h.nx);
            mh.ny=static_cast<std::size_t>(h.ny);
            mh.water_first=h.iwat1;mh.water_last=h.iwat2;mh.iupt=-1;
            mh.imixh=1;mh.izicrlx=1;mh.timestep_s=3600;
            mh.constn=2400.f;mh.zimin=50.f;mh.zimax=3000.f;
            mh.threshl=0.f;mh.dptmin=0.001f;mh.dzzi=200.f;
            mh.onedte=4140.f;mh.twodte=1080.f;mh.tzicrlx=800.f;
            mh.sounding[0]={observation.aa};
            mh.sounding[1]={observation.bb};
            mh.justa={observation.justa};
            mh.nearu.assign(cells,1);
            mh.landuse=reference.statics.ilandu;
            mh.qh=heat_flux;mh.ustar=ustar;mh.el=el;mh.rho=density;
            mh.cmech.resize(cells);mh.fcori.resize(cells);
            for(std::size_t k=0;k<cells;++k) {
                // Original ZIPARM.FCORIOL rule (L9606-9616): a user value is
                // used verbatim; only FCORIOL>998 selects the latitude-derived
                // field f=2*(2*pi/86400)*sin(lat). Either way FCORI is floored
                // at 0.25e-4 and CMECH=CONSTB/sqrt(FCORI).
                float f=fcoriol;
                if(fcoriol>998.f) {
                    const float lat=0.0174533f*solar_grid.latitude_deg[k];
                    f=2.f*(6.2831853f/86400.f)*std::sin(lat);
                }
                mh.fcori[k]=std::max(std::fabs(f),0.25e-4f);
                mh.cmech[k]=1.41f/std::sqrt(mh.fcori[k]);
            }
            mh.initial_zi.assign(cells,-7777.f);
            mh.initial_ziconv=previous_ziconv;
            mh.initial_dptt=previous_dptt;
            const auto mixing=mixht_observed_mc(mh);
            zi_height=mixing.zi;
            ziconv=mixing.ziconv;
            // COMP L4715: IAVEZI=1 averages ZI and ZICONV upwind with the
            // ILEVZI=1 wind level before WSTARR and the output write.
            {
                AvetmpInput av;
                av.nx=static_cast<std::size_t>(h.nx);
                av.ny=static_cast<std::size_t>(h.ny);
                av.mnmdav=mnmdav;av.hafang_deg=hafang;av.dgrid_m=h.dgrid_m;
                av.u_ms.resize(cells);av.v_ms.resize(cells);
                for(std::size_t k=0;k<cells;++k) {
                    av.u_ms[k]=wind->u[0][hour*cells+k];
                    av.v_ms[k]=wind->v[0][hour*cells+k];
                }
                const auto mixed=avemix(av,zi_height,ziconv);
                zi_height=mixed.zi_m;
                ziconv=mixed.ziconv_m;
            }
            previous_ziconv=ziconv;
            previous_dptt=mixing.dptt;
            if(!state_prefix.empty()) {
                exported_ziconv.insert(exported_ziconv.end(),
                                       ziconv.begin(),ziconv.end());
                exported_zi.insert(exported_zi.end(),
                                   zi_height.begin(),zi_height.end());
            }
            // WSTARR L36884-36949.
            WstarrInput sw;
            sw.nx=static_cast<std::size_t>(h.nx);
            sw.ny=static_cast<std::size_t>(h.ny);
            sw.water_first=h.iwat1;sw.water_last=h.iwat2;
            sw.zi_conv_m=ziconv;
            sw.heat_flux_wm2=heat_flux;
            sw.temp_k=surface.temp2d_k;
            sw.rho_kgm3=density;
            sw.initial_wstar_ms.assign(cells,0.f);
            sw.landuse=reference.statics.ilandu;
            wstar_field=wstarr(sw);
        }
        std::vector<float> shortwave(cells,0.f);
        if(heatfx_shortwave_field.empty())
            for(std::size_t k=0;k<cells;++k)
                shortwave[k]=heatfx_shortwave(sinalpc[k],cloud_fraction,ha1,ha2,hb1,hb2);
        else
            shortwave=heatfx_shortwave_field;
        for(const std::string& label:labels) {
            CalmetDat21Field field;
            field.label=label;
            field.begin=reference_first.begin;
            field.end=reference_first.end;
            field.integer=label=="IPGT"||label=="IRH";
            const bool surface_temperature=label=="TEMPK";
            const bool layer_one=label=="T-LEV  1"; // OUTHR pads the level index
            if(field.integer) {
                field.integer_values.assign(cells,calmet21_missing_int);
                if(label=="IRH")
                    for(std::size_t k=0;k<cells;++k)
                        field.integer_values[k]=surface.rh2d_pct[k];
                if(label=="IPGT"&&wind)
                    for(std::size_t k=0;k<cells;++k)
                        field.integer_values[k]=stability[k];
            } else {
                field.real_values.assign(cells,calmet21_missing);
                if(surface_temperature||layer_one)
                    for(std::size_t k=0;k<cells;++k)
                        field.real_values[k]=surface.temp2d_k[k];
                if(label=="QSW")
                    for(std::size_t k=0;k<cells;++k)
                        field.real_values[k]=shortwave[k];
                if(label=="USTAR"&&!ustar.empty())
                    for(std::size_t k=0;k<cells;++k)
                        field.real_values[k]=ustar[k];
                if(label=="EL"&&!el.empty())
                    for(std::size_t k=0;k<cells;++k)
                        field.real_values[k]=el[k];
                if(label=="ZI"&&!zi_height.empty())
                    for(std::size_t k=0;k<cells;++k)
                        field.real_values[k]=zi_height[k];
                if(label=="WSTAR"&&!wstar_field.empty())
                    for(std::size_t k=0;k<cells;++k)
                        field.real_values[k]=wstar_field[k];
                if(label=="RHO")
                    for(std::size_t k=0;k<cells;++k)
                        field.real_values[k]=density[k];
            }
            hourly.push_back(std::move(field));
        }
    }
    write_calmet_dat21(out_path,candidate_header,reference.statics,hourly);
    if(!state_prefix.empty()) {
        if(exported_ziconv.size()!=reference.intervals*cells||
           exported_zi.size()!=reference.intervals*cells)
            throw std::invalid_argument(
                "state export requires a wind and GEO source for every hour");
        const auto write_binary=[&](const std::string& path,
                                    const std::vector<float>& values) {
            std::ofstream file(path,std::ios::binary|std::ios::trunc);
            if(!file)throw std::invalid_argument("cannot write "+path);
            file.write(reinterpret_cast<const char*>(values.data()),
                       static_cast<std::streamsize>(values.size()*sizeof(float)));
            if(!file)throw std::invalid_argument("short write to "+path);
        };
        write_binary(state_prefix+".ziconv.f32",exported_ziconv);
        write_binary(state_prefix+".zi.f32",exported_zi);
        std::string meta="{\n";
        meta+="  \"producer\": \"p6e_calmet_dat21 emit-realcase\",\n";
        meta+="  \"source\": \"chain-internal MIXHT state advanced hour by hour; NOT copied from the reference met.dat\",\n";
        meta+="  \"fields\": [\"ziconv\",\"zi\"],\n";
        meta+="  \"units\": \"m\",\n";
        meta+="  \"dtype\": \"float32 little-endian\",\n";
        meta+="  \"layout\": \"hour-major, then j-major rows with i fastest, exactly as CALMET.DAT field payloads\",\n";
        meta+="  \"shape\": ["+std::to_string(reference.intervals)+", "+
              std::to_string(h.ny)+", "+std::to_string(h.nx)+"],\n";
        meta+="  \"values_per_file\": "+std::to_string(exported_zi.size())+",\n";
        meta+="  \"water_sentinel\": -7777.0,\n";
        meta+="  \"missing_sentinel\": 9999.0,\n";
        meta+="  \"negative_dptt_or_zero_ziconv_is_physical\": true,\n";
        meta+="  \"wind_source\": "+json_string(wind_path)+",\n";
        meta+="  \"geo_source\": "+json_string(geo_path)+",\n";
        meta+="  \"candidate_artifact\": "+json_string(out_path)+",\n";
        meta+="  \"difference_from_met_dat_ZI\": \"CALMET.DAT carries ZI (max of convective and mechanical, after AVEMIX averaging); ZICONV is the internal convective height that CGAMMA consumes. This export provides both from the P6E chain; the ZI field here still disagrees with the frozen reference on 4488 of 357500 cells (1.26%, max abs 51.06 m, concentrated in 42 daylight hours), so use ZICONV for wiring until the convective-growth audit closes.\",\n";
        meta+="  \"hour_labels\": [";
        for(std::size_t hour=0;hour<reference.intervals;++hour) {
            const auto& begin=reference.hourly[hour*reference.fields_per_hour].begin;
            meta+=std::string(hour?", ":"")+"{\"packed\": "+std::to_string(calmet21_pack(begin))+
                  ", \"seconds\": "+std::to_string(begin.seconds)+
                  ", \"year\": "+std::to_string(begin.year)+
                  ", \"julian_day\": "+std::to_string(begin.julian_day)+
                  ", \"hour\": "+std::to_string(begin.hour)+"}";
        }
        meta+="]\n}\n";
        std::ofstream metadata(state_prefix+".state.json",std::ios::trunc);
        if(!metadata)throw std::invalid_argument("cannot write state metadata");
        metadata<<meta;
    }
    std::string report="{\n";
    report+="  \"candidate\": "+json_string(out_path)+",\n";
    if(!state_prefix.empty())report+="  \"state_export_prefix\": "+json_string(state_prefix)+",\n";
    report+="  \"reference\": "+json_string(reference_path)+",\n";
    report+="  \"observations\": "+json_string(observations_path)+",\n";
    report+="  \"intervals\": "+std::to_string(reference.intervals)+",\n";
    report+="  \"produced_fields\": [\"TEMPK\",\"IRH\",\"T-LEV  1\",\"QSW\",\"RHO\"";
    if(wind)report+=",\"IPGT\"";
    if(geo&&wind)report+=",\"USTAR\",\"EL\",\"ZI\",\"WSTAR\"";
    report+="],\n";
    report+="  \"wind_source\": "+json_string(wind_path)+",\n";
    report+="  \"avetmp_mnmdav\": "+std::to_string(mnmdav)+
            ", \"avetmp_hafang_deg\": "+json_number(hafang)+",\n";
    report+="  \"produced_real_hours\": "+std::to_string(produced_temperature)+",\n";
    report+="  \"hours_with_missing_station_humidity\": "+std::to_string(missing_humidity)+",\n";
    report+="  \"shortwave_constants\": {\"ha1\": "+json_number(ha1)+", \"ha2\": "+json_number(ha2)+
            ", \"hb1\": "+json_number(hb1)+", \"hb2\": "+json_number(hb2)+"},\n";
    report+="  \"base_timezone_hours\": "+std::to_string(base_timezone)+",\n";
    report+="  \"missing_sentinel_fields\": [";
    {
        bool first=true;
        for(const auto& label:labels) {
            if(label=="TEMPK"||label=="IRH"||label=="T-LEV  1"||label=="QSW"||
               label=="RHO")continue;
            if(label=="IPGT"&&wind)continue;
            if((label=="USTAR"||label=="EL"||label=="ZI"||label=="WSTAR")&&geo&&wind)continue;
            report+=(first?"":", ")+json_string(label);
            first=false;
        }
    }
    report+="],\n";
    report+="  \"status\": \"PARTIAL_REAL_INPUT_CANDIDATE\",\n";
    report+="  \"status_detail\": \"only wind-independent surface fields are produced; all other fields are the original 9999. missing sentinel\"\n}\n";
    std::cout<<report;
    if(!report_path.empty()) {
        std::ofstream file(report_path,std::ios::trunc);
        if(!file)throw std::invalid_argument("cannot write "+report_path);
        file<<report;
    }
}

// ------------------------------------------------------------------ check
std::string footer(const std::string& status,const std::string& detail) {
    std::string out="  \"status\": "+json_string(status)+",\n";
    out+="  \"status_detail\": "+json_string(detail)+"\n}\n";
    return out;
}

int run_check(const std::string& path,const std::string& out_path,
              const std::string& rewrite_path) {
    const auto dataset=read_calmet_dat21(path);
    if(!rewrite_path.empty())
        write_calmet_dat21(rewrite_path,dataset.header,dataset.statics,dataset.hourly);
    const auto& h=dataset.header;
    const auto cells=static_cast<std::size_t>(h.nx)*static_cast<std::size_t>(h.ny);
    const auto labels=calmet21_hourly_labels(h.nz,h.irtype,h.lcalgrd,h.npsta);
    std::map<std::string,std::vector<double>> stats;
    for(const auto& label:labels)
        stats.emplace(label,std::vector<double>{0.0,0.0,
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity(),0.0});
    double layer1_mismatch=0.0,wstar_negative=0.0,zi_negative=0.0;
    double ustar_below_floor_land=0.0,irh_out_of_range=0.0,ipgt_out_of_range=0.0;
    for(std::size_t hour=0;hour<dataset.intervals;++hour) {
        const auto base=hour*dataset.fields_per_hour;
        const CalmetDat21Field* temperature=nullptr;
        const CalmetDat21Field* surface=nullptr;
        for(std::size_t j=0;j<dataset.fields_per_hour;++j) {
            const auto& field=dataset.hourly[base+j];
            auto& bucket=stats[field.label];
            for(std::size_t k=0;k<cells;++k) {
                if(field.integer) {
                    const auto v=field.integer_values[k];
                    if(v==calmet21_missing_int){bucket[1]+=1.0;continue;}
                    if(field.label=="IRH"&&(v<0||v>100))irh_out_of_range+=1.0;
                    if(field.label=="IPGT"&&(v<1||v>8))ipgt_out_of_range+=1.0;
                    bucket[0]+=1.0;
                    bucket[2]=std::min(bucket[2],static_cast<double>(v));
                    bucket[3]=std::max(bucket[3],static_cast<double>(v));
                    bucket[4]+=static_cast<double>(v);
                } else {
                    const float v=field.real_values[k];
                    if(v==calmet21_missing){bucket[1]+=1.0;continue;}
                    bucket[0]+=1.0;
                    bucket[2]=std::min(bucket[2],static_cast<double>(v));
                    bucket[3]=std::max(bucket[3],static_cast<double>(v));
                    bucket[4]+=static_cast<double>(v);
                    if(field.label=="WSTAR"&&v<0.f)wstar_negative+=1.0;
                    if(field.label=="ZI"&&v<0.f)zi_negative+=1.0;
                    if(field.label=="USTAR"&&dataset.statics.ilandu[k]<h.iwat1&&v<0.05f)
                        ustar_below_floor_land+=1.0;
                }
            }
            if(field.label=="T-LEV1")temperature=&field;
            if(field.label=="TEMPK")surface=&field;
        }
        if(temperature!=nullptr&&surface!=nullptr)
            for(std::size_t k=0;k<cells;++k)
                if(temperature->real_values[k]!=surface->real_values[k])
                    layer1_mismatch+=1.0;
    }
    const auto order=labels;
    std::string out="{\n";
    out+="  \"file\": "+json_string(path)+",\n";
    out+="  \"header\": {\n";
    out+="    \"dataset\": "+json_string(h.dataset)+", \"version\": "+json_string(h.version)+",\n";
    out+="    \"timezone\": "+json_string(h.timezone)+", \"irtype\": "+
         std::to_string(h.irtype)+", \"iwfcod\": "+std::to_string(h.iwfcod)+",\n";
    out+="    \"nx\": "+std::to_string(h.nx)+", \"ny\": "+std::to_string(h.ny)+
         ", \"nz\": "+std::to_string(h.nz)+", \"dgrid_m\": "+json_number(h.dgrid_m)+",\n";
    out+="    \"nssta\": "+std::to_string(h.nssta)+", \"nusta\": "+std::to_string(h.nusta)+
         ", \"npsta\": "+std::to_string(h.npsta)+", \"nowsta\": "+std::to_string(h.nowsta)+",\n";
    out+="    \"iwat1\": "+std::to_string(h.iwat1)+", \"iwat2\": "+std::to_string(h.iwat2)+
         ", \"lcalgrd\": "+(h.lcalgrd?std::string("true"):std::string("false"))+",\n";
    out+="    \"irlg\": "+std::to_string(h.run_hours)+", \"comments\": "+
         std::to_string(h.comments.size())+"\n";
    out+="  },\n";
    out+="  \"intervals\": "+std::to_string(dataset.intervals)+",\n";
    out+="  \"fields_per_hour\": "+std::to_string(dataset.fields_per_hour)+",\n";
    out+="  \"cells\": "+std::to_string(cells)+",\n";
    out+="  \"record_bytes\": "+std::to_string(
        calmet21_field_header_bytes+4U*cells)+",\n";
    out+="  \"hourly_labels\": [";
    for(std::size_t j=0;j<order.size();++j)
        out+=(j?", ":"")+json_string(order[j]);
    out+="],\n";
    const auto bindings=calmet21_field_bindings();
    std::vector<std::string> unimplemented;
    out+="  \"field_inventory\": [\n";
    for(std::size_t j=0;j<order.size();++j) {
        const auto& label=order[j];
        const auto& b=stats[label];
        std::string unit="unknown",owner="unknown",source="";
        for(const auto& binding:bindings) {
            if(label.rfind(binding.label,0)!=0)continue;
            if(binding.label.size()>8U&&label.size()!=binding.label.size()+3U)continue;
            unit=unit_name(binding.unit);owner=owner_name(binding.owner);
            source=binding.source;
            break;
        }
        if(owner=="p6d"||owner=="case_input")unimplemented.push_back(label);
        const double denominator=std::max(1.0,b[0]);
        out+="    {\"label\": "+json_string(label)+", \"unit\": "+json_string(unit)+
             ", \"owner\": "+json_string(owner)+
             ", \"values\": "+json_number(b[0])+
             ", \"missing\": "+json_number(b[1])+
             ", \"min\": "+json_number(b[2])+", \"max\": "+json_number(b[3])+
             ", \"mean\": "+json_number(b[4]/denominator)+
             (source.empty()?"":", \"source\": "+json_string(source))+"}";
        out+=(j+1U==order.size())?"\n":",\n";
    }
    out+="  ],\n";
    out+="  \"not_implemented_in_p6e\": [";
    for(std::size_t j=0;j<unimplemented.size();++j)
        out+=(j?", ":"")+json_string(unimplemented[j]);
    out+="],\n";
    out+="  \"internal_only_absent\": ";
    {
        bool absent=true;
        const auto internals=calmet21_internal_only_fields();
        for(const auto& internal:internals)
            for(const auto& label:order)if(label==internal)absent=false;
        out+=absent?std::string("true"):std::string("false");
    }
    out+=",\n";
    out+="  \"invariants\": {\n";
    out+="    \"T_LEV1_equals_TEMPK_mismatches\": "+json_number(layer1_mismatch)+",\n";
    out+="    \"WSTAR_negative_values\": "+json_number(wstar_negative)+",\n";
    out+="    \"ZI_negative_values\": "+json_number(zi_negative)+",\n";
    out+="    \"USTAR_below_0p05_over_land\": "+json_number(ustar_below_floor_land)+",\n";
    out+="    \"IRH_outside_0_100\": "+json_number(irh_out_of_range)+",\n";
    out+="    \"IPGT_outside_1_8\": "+json_number(ipgt_out_of_range)+"\n";
    out+="  },\n";
    out+=footer("PASS","layout validated against the original OUTHD/OUTHR record semantics");
    std::cout<<out;
    if(!out_path.empty()) {
        std::ofstream file(out_path,std::ios::trunc);
        if(!file)throw std::invalid_argument("cannot write "+out_path);
        file<<out;
    }
    return 0;
}

// ---------------------------------------------------------------- compare
int run_compare(const std::string& reference_path,const std::string& candidate_path,
                const std::string& out_path) {
    const auto reference=read_calmet_dat21(reference_path);
    const auto candidate=read_calmet_dat21(candidate_path);
    const auto& rh=reference.header;
    const auto& ch=candidate.header;
    const auto require=[&](bool ok,const char* message) {
        if(!ok)throw std::invalid_argument(std::string("CALMET.DAT 2.1 comparison ")+message);
    };
    require(rh.nx==ch.nx&&rh.ny==ch.ny&&rh.nz==ch.nz,"grid extent mismatch");
    require(rh.timezone==ch.timezone,"time zone mismatch");
    require(rh.iwat1==ch.iwat1&&rh.iwat2==ch.iwat2,"water range mismatch");
    require(reference.fields_per_hour==candidate.fields_per_hour,
            "field count mismatch");
    require(reference.intervals==candidate.intervals,"interval count mismatch");
    for(std::size_t j=0;j<reference.fields_per_hour;++j)
        require(reference.hourly[j].label==candidate.hourly[j].label,
                "field label mismatch");
    for(std::size_t hour=0;hour<reference.intervals;++hour) {
        const auto& rb=reference.hourly[hour*reference.fields_per_hour].begin;
        const auto& cb=candidate.hourly[hour*candidate.fields_per_hour].begin;
        require(calmet21_pack(rb)==calmet21_pack(cb)&&rb.seconds==cb.seconds,
                "hourly time label mismatch");
    }
    struct Aggregate {
        double denominator=0.0,missing_reference=0.0,missing_candidate=0.0;
        double max_abs=0.0,max_rel=0.0,exceedances=0.0;
        std::vector<std::string> samples;
    };
    std::map<std::string,Aggregate> aggregates;
    std::vector<std::string> order;
    double exceedances=0.0;
    for(std::size_t hour=0;hour<reference.intervals;++hour)
        for(std::size_t j=0;j<reference.fields_per_hour;++j) {
            const auto& rf=reference.hourly[hour*reference.fields_per_hour+j];
            const auto& cf=candidate.hourly[hour*candidate.fields_per_hour+j];
            auto& aggregate=aggregates[rf.label];
            if(aggregate.denominator==0.0&&aggregate.missing_reference==0.0&&
               aggregate.missing_candidate==0.0)
                order.push_back(rf.label);
            const auto tolerance=tolerance_for(rf.label);
            const auto cells=rf.integer?rf.integer_values.size():rf.real_values.size();
            for(std::size_t k=0;k<cells;++k) {
                double a=0.0,b=0.0;
                if(rf.integer) {
                    const auto ai=rf.integer_values[k];
                    const auto bi=cf.integer_values[k];
                    if(ai==calmet21_missing_int){aggregate.missing_reference+=1.0;continue;}
                    if(bi==calmet21_missing_int){aggregate.missing_candidate+=1.0;continue;}
                    a=static_cast<double>(ai);b=static_cast<double>(bi);
                } else {
                    const auto ar=rf.real_values[k];
                    const auto br=cf.real_values[k];
                    if(ar==calmet21_missing){aggregate.missing_reference+=1.0;continue;}
                    if(br==calmet21_missing){aggregate.missing_candidate+=1.0;continue;}
                    a=static_cast<double>(ar);b=static_cast<double>(br);
                }
                aggregate.denominator+=1.0;
                const double error=std::fabs(a-b);
                aggregate.max_abs=std::max(aggregate.max_abs,error);
                if(a!=0.0)aggregate.max_rel=std::max(aggregate.max_rel,error/std::fabs(a));
                if(error>tolerance.abs_limit+tolerance.rel_limit*std::fabs(a)) {
                    aggregate.exceedances+=1.0;
                    exceedances+=1.0;
                    if(aggregate.samples.size()<4U) {
                        aggregate.samples.push_back(
                            "{\"hour\": "+std::to_string(hour+1U)+", \"cell\": "+
                            std::to_string(k+1U)+", \"reference\": "+json_number(a)+
                            ", \"candidate\": "+json_number(b)+
                            ", \"abs\": "+json_number(error)+"}");
                    }
                }
            }
        }
    std::string out="{\n";
    out+="  \"reference\": "+json_string(reference_path)+",\n";
    out+="  \"candidate\": "+json_string(candidate_path)+",\n";
    out+="  \"intervals\": "+std::to_string(reference.intervals)+",\n";
    out+="  \"fields_per_hour\": "+std::to_string(reference.fields_per_hour)+",\n";
    out+="  \"fields\": [\n";
    for(std::size_t j=0;j<order.size();++j) {
        const auto& label=order[j];
        const auto& a=aggregates[label];
        const auto tolerance=tolerance_for(label);
        const auto& binding_source=[&] {
            for(const auto& binding:calmet21_field_bindings())
                if(label.rfind(binding.label,0)==0&&
                   (binding.label.size()<=8U||label.size()==binding.label.size()+3U))
                    return std::string(binding.source);
            return std::string();
        }();
        out+="    {\"label\": "+json_string(label)+
             ", \"denominator\": "+json_number(a.denominator)+
             ", \"missing_reference\": "+json_number(a.missing_reference)+
             ", \"missing_candidate\": "+json_number(a.missing_candidate)+
             ", \"max_abs_error\": "+json_number(a.max_abs)+
             ", \"max_rel_error\": "+json_number(a.max_rel)+
             ", \"abs_limit\": "+json_number(tolerance.abs_limit)+
             ", \"rel_limit\": "+json_number(tolerance.rel_limit)+
             ", \"exceedances\": "+json_number(a.exceedances)+
             ", \"source\": "+json_string(binding_source)+
             ", \"first_exceedances\": ["+
             [&] {
                 std::string joined;
                 for(std::size_t s=0;s<a.samples.size();++s)
                     joined+=(s?", ":"")+a.samples[s];
                 return joined;
             }()+
             "]}";
        out+=(j+1U==order.size())?"\n":",\n";
    }
    out+="  ],\n";
    out+="  \"total_exceedances\": "+json_number(exceedances)+",\n";
    out+=footer(exceedances==0.0?"PASS":"FAIL_NUMERIC",
                exceedances==0.0
                  ?"every compared cell is inside the frozen source-gate tolerance"
                  :"one or more cells exceed the frozen source-gate tolerance");
    std::cout<<out;
    if(!out_path.empty()) {
        std::ofstream file(out_path,std::ios::trunc);
        if(!file)throw std::invalid_argument("cannot write "+out_path);
        file<<out;
    }
    return exceedances==0.0?0:1;
}

std::string argument(int argc,char** argv,const std::string& flag,
                     const std::string& fallback=std::string()) {
    for(int k=1;k+1<argc;++k)if(flag==argv[k])return argv[k+1];
    return fallback;
}
int number_argument(int argc,char** argv,const std::string& flag,int fallback) {
    const auto text=argument(argc,argv,flag);
    return text.empty()?fallback:std::stoi(text);
}
} // namespace

int main(int argc,char** argv) {
    try {
        if(argc<2) {
            std::cerr<<"usage: p6e_calmet_dat21 <emit-synthetic|emit-realcase|check|compare> ...\n";
            return 2;
        }
        const std::string mode=argv[1];
        if(mode=="emit-synthetic") {
            const auto out=argument(argc,argv,"--out");
            if(out.empty())throw std::invalid_argument("emit-synthetic needs --out");
            emit_synthetic(out,
                           static_cast<std::size_t>(number_argument(argc,argv,"--hours",3)),
                           number_argument(argc,argv,"--nx",50),
                           number_argument(argc,argv,"--ny",50),
                           number_argument(argc,argv,"--nz",9));
            return 0;
        }
        if(mode=="emit-realcase") {
            const auto reference_file=argument(argc,argv,"--reference");
            const auto observations=argument(argc,argv,"--observations");
            const auto out=argument(argc,argv,"--out");
            if(reference_file.empty()||observations.empty()||out.empty())
                throw std::invalid_argument(
                    "emit-realcase needs --reference, --observations and --out");
            emit_realcase(reference_file,observations,out,
                          argument(argc,argv,"--report"),
                          static_cast<float>(std::stod(argument(argc,argv,"--ha1","990."))),
                          static_cast<float>(std::stod(argument(argc,argv,"--ha2","-30."))),
                          static_cast<float>(std::stod(argument(argc,argv,"--hb1","-0.75"))),
                          static_cast<float>(std::stod(argument(argc,argv,"--hb2","3.4"))),
                          argument(argc,argv,"--wind-file"),
                          number_argument(argc,argv,"--mnmdav",1),
                          static_cast<float>(std::stod(argument(argc,argv,"--hafang","30."))),
                          argument(argc,argv,"--geo-file"),
                          static_cast<float>(std::stod(argument(argc,argv,"--hc1","5.31e-13"))),
                          static_cast<float>(std::stod(argument(argc,argv,"--hc2","60."))),
                          static_cast<float>(std::stod(argument(argc,argv,"--hc3","0.12"))),
                          argument(argc,argv,"--state-prefix"),
                          static_cast<float>(std::stod(
                              argument(argc,argv,"--fcoriol","1.0e-4"))));
            return 0;
        }
        if(mode=="check") {
            const auto file=argument(argc,argv,"--file");
            if(file.empty())throw std::invalid_argument("check needs --file");
            return run_check(file,argument(argc,argv,"--out"),
                             argument(argc,argv,"--rewrite"));
        }
        if(mode=="compare") {
            const auto reference=argument(argc,argv,"--reference");
            const auto candidate=argument(argc,argv,"--candidate");
            if(reference.empty())throw std::invalid_argument("compare needs --reference");
            if(candidate.empty())throw std::invalid_argument("compare needs --candidate");
            return run_compare(reference,candidate,argument(argc,argv,"--out"));
        }
        std::cerr<<"unknown mode "<<mode<<"\n";
        return 2;
    } catch(const std::exception& error) {
        std::cerr<<"p6e_calmet_dat21: "<<error.what()<<"\n";
        return 3;
    }
}
