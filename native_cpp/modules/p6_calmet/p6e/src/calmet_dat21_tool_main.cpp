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
#include "cteas/p6e/mixht_observed_mc.hpp"
#include "cteas/p6e/temp3d_observed_land.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
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
            std::cerr<<"usage: p6e_calmet_dat21 <emit-synthetic|check|compare> ...\n";
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
