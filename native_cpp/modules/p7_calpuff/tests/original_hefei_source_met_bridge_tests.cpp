// P7 frozen Hefei input-authority bridge: unmodified Group 13b point source
// plus ORIGINAL METFM1 full hourly winds. This is not PRFIN or CALCPF parity.
#include "cteas/puff/original_met_hours.hpp"
#include "cteas/puff/original_release_wind.hpp"
#include "cteas/puff/original_native_terrain.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void demand(bool condition,const char* reason){
    if(!condition)throw std::runtime_error(reason);
}
struct FrozenSource {
    std::string name;
    std::vector<float> row;
};
FrozenSource read_selected_point(const std::string& filename){
    std::ifstream in(filename);
    demand(static_cast<bool>(in),"missing original puff.inp");
    const std::regex identity(R"(^\s*1\s*!\s*SRCNAM\s*=\s*([^!]+)\s*!\s*$)");
    const std::regex data(R"(^\s*1\s*!\s*X\s*=\s*([^!]+)!\s*$)");
    const std::regex count(R"(!\s*NPT1\s*=\s*1\s*!)");
    const std::regex no_external(R"(!\s*NPT2\s*=\s*0\s*!)");
    const std::regex no_variable(R"(!\s*NSPT1\s*=\s*0\s*!)");
    FrozenSource source;
    int names=0,rows=0;
    bool one_point=false,no_file=false,no_variation=false;
    std::string line;
    while(std::getline(in,line)){
        std::smatch m;
        if(std::regex_search(line,count))one_point=true;
        if(std::regex_search(line,no_external))no_file=true;
        if(std::regex_search(line,no_variable))no_variation=true;
        if(std::regex_match(line,m,identity)){
            source.name=m[1].str();
            const auto end=source.name.find_last_not_of(" \t\r\n");
            if(end!=std::string::npos)source.name.resize(end+1);
            ++names;
        }
        if(std::regex_match(line,m,data)){
            std::string csv=m[1].str();
            std::replace(csv.begin(),csv.end(),',',' ');
            std::istringstream values(csv);
            double value;
            while(values>>value){
                demand(std::isfinite(value),"nonfinite original point field");
                source.row.push_back(static_cast<float>(value));
            }
            demand(values.eof(),"malformed original Group 13b value");
            ++rows;
        }
    }
    demand(one_point&&no_file&&no_variation&&names==1&&rows==1&&
           source.name=="HTO_STACK"&&source.row.size()==10,
           "not the locked original single constant point source");
    const auto& r=source.row;
    demand(r[0]==513.385F&&r[1]==3533.769F&&r[2]==70.8F&&
           r[3]==0.0F&&r[4]==1.6F&&r[5]==17.26F&&
           r[6]==298.15F&&r[7]==0.0F&&r[8]==834000.0F&&
           r[9]==0.0F,
           "frozen 2022 real source changed: no synthetic substitution");
    return source;
}
} // namespace

int main(int argc,char** argv){
    try{
        if(argc!=3)throw std::invalid_argument(
            "usage: original_hefei_source_met_bridge_tests MET.DAT puff.inp");
        const auto source=read_selected_point(argv[2]);
        std::size_t hours=0;
        float min_ws=std::numeric_limits<float>::infinity();
        float max_ws=0.0F;
        std::size_t calms=0;
        float source_x_cells=0.0F,source_y_cells=0.0F;
        const auto summary=cteas::puff::OriginalMetHourlyReader::read(
            argv[1],[&](const cteas::puff::Calmet21Header& header,
                         const cteas::puff::Calmet21Static& statics,
                         const cteas::puff::OriginalMetHour& hour){
                demand(header.nx==50&&header.ny==50&&header.nz==9&&
                       header.dx_m==100.0F&&
                       statics.elevation_m.size()==2500,
                       "original 2022 MET grid or static terrain changed");
                // Original XPT1GRD/YPT1GRD are RELATIVE met-grid units.
                // Do not pass UTM km 513.385/3533.769 to the puff solver.
                const float x=static_cast<float>(
                    (source.row[0]*1000.0F-header.origin_x_m)/header.dx_m);
                const float y=static_cast<float>(
                    (source.row[1]*1000.0F-header.origin_y_m)/header.dx_m);
                demand(std::isfinite(x)&&std::isfinite(y)&&
                       std::abs(x-25.0F)<0.001F&&std::abs(y-25.0F)<0.001F,
                       "original source / CALMET origin grid mismatch");
                if(hours==0){
                    source_x_cells=x;source_y_cells=y;
                    const auto n=static_cast<std::size_t>(header.nx)*header.ny;
                    const auto cell=cteas::puff::original_met_cell(
                        x,y,header.nx,header.ny).offset_0based;
                    demand(hour.tempk.size()==n&&hour.zi.size()==n&&
                           hour.ipgt.size()==n&&hour.el.size()==n&&
                           hour.t.size()==n*static_cast<std::size_t>(header.nz),
                           "first original MET hour lacks stack temperature or stability");
                    const auto layer=cteas::puff::original_release_wind_metfm1(
                        hour,statics,{.x_cells=x,.y_cells=y,
                            .release_height_m=source.row[2],
                            .calm_threshold_ms=0.5F}).layer_1based;
                    const auto column=(static_cast<std::size_t>(layer)-1U)*n+cell;
                    std::cout<<"P7_FIRST_HOUR_RELEASE_MET"
                             <<" YEAR="<<hour.begin.year
                             <<" JULIAN="<<hour.begin.julian_day
                             <<" HOUR="<<hour.begin.hour
                             <<" CELL="<<cell
                             <<" STACK_LAYER="<<layer
                             <<" PGT="<<hour.ipgt[cell]
                             <<" TEMP2D_K="<<hour.tempk[cell]
                             <<" TEMP3D_STACK_K="<<hour.t[column]
                             <<" DPBL_M="<<hour.zi[cell]
                             <<" EL_M="<<hour.el[cell]
                             <<" ROUGHNESS_M="<<statics.roughness_m[cell]
                             <<" DIAMETER_M="<<source.row[4]
                             <<" EXIT_MS="<<source.row[5]
                             <<" STACK_K="<<source.row[6]<<"\\n";
                }else demand(x==source_x_cells&&y==source_y_cells,
                            "source shifted between hourly MET snapshots");
                const auto stack=cteas::puff::original_release_wind_metfm1(
                    hour,statics,{
                        .x_cells=x,.y_cells=y,
                        .release_height_m=source.row[2],
                        .calm_threshold_ms=0.5F
                    });
                demand(stack.wind_speed_raw_ms>=0.0F&&
                       std::isfinite(stack.wind_speed_raw_ms)&&
                       stack.layer_1based>=1&&stack.layer_1based<=9,
                       "native real RLSMET stack wind invalid");
                min_ws=std::min(min_ws,stack.wind_speed_raw_ms);
                max_ws=std::max(max_ws,stack.wind_speed_raw_ms);
                calms+=static_cast<std::size_t>(stack.calm);
                ++hours;
            },true);
        demand(hours==143&&summary.intervals==143,
               "native case did not consume all 143 real MET hours");
        std::cout<<"P7_HEFEI_SOURCE_MET_STACK_BRIDGE=PASS HOURS="<<hours
                 <<" SOURCE_NAME="<<source.name
                 <<" XPT1GRD="<<source_x_cells
                 <<" YPT1GRD="<<source_y_cells
                 <<" STACK_HEIGHT_M="<<source.row[2]
                 <<" EMISSION_BQ_S="<<source.row[8]
                 <<" WIND_MIN_MS="<<min_ws<<" WIND_MAX_MS="<<max_ws
                 <<" CALM_HOURS="<<calms
                 <<" REAL_MET_DRIVER=YES DGM_DRIVING_INPUT=NO"
                 <<" NATIVE_PRFIN=NOT_YET_ACCEPTED"
                 <<" MDRY_CALCPF_PARITY=NOT_YET_ACCEPTED\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"P7_HEFEI_SOURCE_MET_STACK_BRIDGE=FAIL "<<e.what()<<'\n';
        return 1;
    }
}
