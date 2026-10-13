// P6F source-independent grid/static provenance audit.
// The actual candidate's CALMET.DAT is read only for comparison: every
// expected grid value comes independently from GEO.DAT 2.1 and met.inp.
#include "cteas/p6f/calmet_dat21_writer.hpp"
#include "cteas/p6/read_geo.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::string input_text(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("missing model input " + path);
    std::ostringstream out;
    out << file.rdbuf();
    if (file.bad()) throw std::runtime_error("failed model input read " + path);
    return out.str();
}
std::string field(const std::string& text, const std::string& key) {
    const std::regex re("!\\s*" + key + "\\s*=\\s*([^!]*)!",
                        std::regex_constants::icase);
    std::smatch m;
    if (!std::regex_search(text, m, re))
        throw std::runtime_error("met.inp missing " + key);
    auto value=m[1].str();
    const auto first=value.find_first_not_of(" \t\r\n");
    if(first==std::string::npos)throw std::runtime_error("empty met.inp " + key);
    const auto last=value.find_last_not_of(" \t\r\n");
    return value.substr(first,last-first+1);
}
int number_i(const std::string& text, const std::string& key) {
    const auto value=field(text,key);
    std::size_t at=0;
    const int result=std::stoi(value,&at);
    if(at!=value.size())throw std::runtime_error("bad integer "+key);
    return result;
}
float number_f(const std::string& text, const std::string& key) {
    const auto value=field(text,key);
    std::size_t at=0;
    const float result=std::stof(value,&at);
    if(at!=value.size()||!std::isfinite(result))
        throw std::runtime_error("bad float "+key);
    return result;
}
std::vector<float> face_list(const std::string& text) {
    std::string list=field(text,"ZFACE");
    for(auto& c:list)if(c==',')c=' ';
    std::istringstream stream(list);
    std::vector<float> result;
    float x;
    while(stream>>x)result.push_back(x);
    if(!stream.eof()||result.empty())
        throw std::runtime_error("invalid met.inp ZFACE");
    return result;
}
void require(bool yes,const std::string& why) {
    if(!yes)throw std::runtime_error("P6F_SRC_STATIC_NOT_PASS: "+why);
}
bool near(float actual,float expected,float tol=0.0f) {
    return std::isfinite(actual)&&std::isfinite(expected)&&
           std::abs(actual-expected)<=tol;
}
std::vector<float> station_xy(const std::string& deck,const std::string& key) {
    auto row=field(deck,key);
    // Original met.inp has a quoted 4-character station name followed by
    // station ID, UTM easting/northing in KM, region and measurement height.
    const auto quoted=row.find('\'');
    require(quoted!=std::string::npos,"missing station quote "+key);
    const auto close=row.find('\'',quoted+1);
    require(close!=std::string::npos,"station quote unmatched "+key);
    std::istringstream rest(row.substr(close+1));
    std::string identifier;
    float east=0.f,north=0.f;
    rest>>identifier>>east>>north;
    require(!identifier.empty()&&rest.good()&&east>0.f&&north>0.f,
            "station coordinates malformed "+key);
    return {east*1000.f,north*1000.f};
}
template<class A,class B> void check_array(const A& actual,const B& expected,
                                             const std::string& label,
                                             std::size_t& compared) {
    require(actual.size()==expected.size(),label+" count/shape disagreement");
    for(std::size_t i=0;i<actual.size();++i) {
        const float a=static_cast<float>(actual[i]);
        const float b=static_cast<float>(expected[i]);
        if(!near(a,b)) {
            throw std::runtime_error("P6F_SRC_STATIC_NOT_PASS: "+label+
                                     " mismatches GEO.DAT at index="+
                                     std::to_string(i)+
                                     " actual="+std::to_string(a)+
                                     " expected="+std::to_string(b));
        }
        ++compared;
    }
}
void check_scalar(int actual,int expected,const std::string& name) {
    require(actual==expected,name+" not reproduced");
}
}
int main(int argc,char** argv) {
    try {
        if(argc!=7||std::string(argv[1])!="--geo"||
           std::string(argv[3])!="--deck"||
           std::string(argv[5])!="--candidate")
            throw std::runtime_error(
                "usage: p6f_geo_static_provenance --geo GEO.DAT --deck met.inp --candidate CALMET.DAT");
        const auto deck=input_text(argv[4]);
        const auto file=cteas::p6f::read_calmet_dat21(argv[6]);
        const auto& hdr=file.header;
        const auto& stat=file.statics;
        check_scalar(hdr.nx,number_i(deck,"NX"),"NX");
        check_scalar(hdr.ny,number_i(deck,"NY"),"NY");
        check_scalar(hdr.nz,number_i(deck,"NZ"),"NZ");
        check_scalar(hdr.nssta,number_i(deck,"NSSTA"),"NSSTA");
        check_scalar(hdr.nusta,number_i(deck,"NUSTA"),"NUSTA");
        check_scalar(hdr.npsta,number_i(deck,"NPSTA"),"NPSTA");
        check_scalar(hdr.nowsta,number_i(deck,"NOWSTA"),"NOWSTA");
        check_scalar(hdr.irtype,number_i(deck,"IRTYPE"),"IRTYPE");
        require(near(hdr.dgrid_m,number_f(deck,"DGRIDKM")*1000.f),"DGRID");
        require(near(hdr.xorigin_m,number_f(deck,"XORIGKM")*1000.f,0.3f),"XORIG");
        require(near(hdr.yorigin_m,number_f(deck,"YORIGKM")*1000.f,0.3f),"YORIG");
        require(hdr.projection.pmap=="UTM"||hdr.projection.pmap=="UTM     ",
                "projection PMAP");
        require(hdr.projection.datum.rfind("WGS-84",0)==0,"projection DATUM");
        require(hdr.projection.utmhem.rfind("N",0)==0,"projection hemisphere");
        check_scalar(hdr.projection.iutmzn,number_i(deck,"IUTMZN"),
                     "projection IUTMZN");
        const auto geo=cteas::calmet::p6::read_geo_50(argv[2],
                number_f(deck,"XORIGKM"),number_f(deck,"YORIGKM"));
        check_scalar(hdr.nlu,geo.nlu,"NLU");
        check_scalar(hdr.iwat1,geo.iwat1,"IWAT1");
        check_scalar(hdr.iwat2,geo.iwat2,"IWAT2");

        std::size_t compared=0;
        check_array(stat.zface_m,face_list(deck),"ZFACE/met.inp",compared);
        check_array(stat.z0_m,geo.z0_m,"Z0/GEO.DAT",compared);
        check_array(stat.ilandu,geo.landuse,"ILANDU/GEO.DAT",compared);
        check_array(stat.elev_m,geo.terrain_m,"ELEV/GEO.DAT",compared);
        check_array(stat.xlai,geo.lai,"XLAI/GEO.DAT",compared);
        if(hdr.nssta==1) {
            const auto xy=station_xy(deck,"SS1");
            check_array(stat.surface_x_m,std::vector<float>{xy[0]},
                        "XSSTA/met.inp",compared);
            check_array(stat.surface_y_m,std::vector<float>{xy[1]},
                        "YSSTA/met.inp",compared);
            // Only one surface station => nearest station ID is deterministically 1.
            check_array(stat.nears,std::vector<int>(2500,1),
                        "NEARS/single-station",compared);
        }
        if(hdr.nusta==1) {
            const auto xy=station_xy(deck,"US1");
            check_array(stat.upper_x_m,std::vector<float>{xy[0]},
                        "XUSTA/met.inp",compared);
            check_array(stat.upper_y_m,std::vector<float>{xy[1]},
                        "YUSTA/met.inp",compared);
        }
        std::cout<<"{\"status\":\"PASS_P6F_INDEPENDENT_SOURCE_STATIC_VALUES\","
                 <<"\"compared_static_values\":"<<compared<<","
                 <<"\"grid\":["<<hdr.nx<<","<<hdr.ny<<","<<hdr.nz<<"],"
                 <<"\"source\":\"GEO.DAT+met.inp; never derived from original CALMET.DAT\","
                 <<"\"header_comments_independent\":false,"
                 <<"\"all_stations_independent\":false,"
                 <<"\"full_P6F_acceptance\":false}\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"FAIL_P6F_SOURCE_STATIC_AUDIT: "<<e.what()<<'\n';
        return 1;
    }
}
