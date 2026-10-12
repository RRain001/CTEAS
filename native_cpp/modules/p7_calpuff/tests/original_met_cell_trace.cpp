#include "cteas/puff/original_met_cell.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Record {std::array<std::int32_t,24> i;std::array<float,64> f;};
static_assert(sizeof(Record)==352U);
static_assert(std::endian::native==std::endian::little);
void require(bool valid,const char* reason) {
    if(!valid)throw std::runtime_error(reason);
}
template<class F> void must_reject(F&& action) {
    bool rejected=false;
    try{action();}catch(const std::invalid_argument&){rejected=true;}
    catch(const std::out_of_range&){rejected=true;}
    require(rejected,"off-grid/bad MET input accepted");
}
}
int main(int argc,char** argv) {
    try {
        using namespace cteas::puff;
        require(argc==3,"usage: original_met_cell_trace ORIGINAL_MET_DAT FIRST_HOUR_DGM");
        must_reject([]{(void)original_met_cell(-0.01F,25.0F,50,50);});
        must_reject([]{(void)original_met_cell(50.0F,25.0F,50,50);});
        must_reject([]{(void)original_met_cell(25.0F,std::nanf(""),50,50);});
        require(original_met_cell(25.0F,25.0F,50,50).ix_1based==26,
                "original exact integer coordinate indexed incorrectly");
        require(original_met_cell(24.613815F,25.903828F,50,50).ix_1based==25,
                "original fractional grid coordinates indexed incorrectly");
        std::ifstream input(argv[2],std::ios::binary);
        require(static_cast<bool>(input),"original first-hour DGM absent");
        std::vector<Record> records;
        Record next{};
        while(input.read(reinterpret_cast<char*>(&next),sizeof(next)))
            records.push_back(next);
        require(input.eof() && input.gcount()==0 && records.size()==1275U,
                "original first-hour DGM truncated or unexpected row count");
        std::size_t matched_cell=0,wind_exact=0,wind_mismatch=0,stable_match=0;
        float max_u=0.0F,max_v=0.0F;
        const auto summary=OriginalMetHourlyReader::read(
            argv[1],[&](const Calmet21Header& header,const Calmet21Static& statics,
                          const OriginalMetHour& hour) {
                if(hour.begin.year!=2022 || hour.begin.julian_day!=1 ||
                   hour.begin.hour!=0)return;
                require(header.nx==50 && header.ny==50 && header.nz==9,
                        "frozen original MET dimensions mismatch");
                for(std::size_t n=0;n<records.size();++n) {
                    const auto& r=records[n];
                    require(r.i[0]==1 && r.i[1]==202200100 && r.i[2]==0,
                            "DGM row outside frozen original first hour");
                    const auto cell=original_met_cell(r.f[0],r.f[1],
                                                     header.nx,header.ny);
                    if(cell.ix_1based!=r.i[5] || cell.iy_1based!=r.i[6]) {
                        std::cerr<<"P7_GETCELL_FIRST_DIFFERENCE ROW="<<n+1
                                 <<" ORIGINAL=("<<r.i[5]<<","<<r.i[6]<<")"
                                 <<" NATIVE=("<<cell.ix_1based<<","
                                 <<cell.iy_1based<<")\n";
                        throw std::runtime_error("original GETCELL index diverged");
                    }
                    ++matched_cell;
                    if(hour.ipgt.at(cell.offset_0based)==r.i[8])++stable_match;
                    const auto wind=original_met_cell_advect(
                        hour,statics,cell,r.f[8],r.f[7]);
                    const float u=static_cast<float>(wind.u_ms);
                    const float v=static_cast<float>(wind.v_ms);
                    if(u==r.f[9] && v==r.f[10])++wind_exact;
                    else {
                        ++wind_mismatch;
                        max_u=std::max(max_u,std::abs(u-r.f[9]));
                        max_v=std::max(max_v,std::abs(v-r.f[10]));
                        if(wind_mismatch<=4U)
                            std::cerr<<"P7_MET_ADVECT_FIRST_DIFFERENCE ROW="<<n+1
                                <<" CELL=("<<cell.ix_1based<<","<<cell.iy_1based<<")"
                                <<" NATIVE_U="<<u<<" ORIGINAL_U="<<r.f[9]
                                <<" NATIVE_V="<<v<<" ORIGINAL_V="<<r.f[10]<<"\n";
                    }
                }
            },true);
        require(summary.intervals==143 && matched_cell==1275U,
                "MET reading or original cell-selection first hour incomplete");
        std::cout<<"P7_NATIVE_GETCELL_INDEX=PASS FIRST_HOUR_RECORDS="<<matched_cell
                 <<" NATIVE_CELL_AND_ORIGINAL=EXACT\n";
        std::cout<<"P7_NATIVE_ADVECT_PROFILE_DIAGNOSTIC EXACT="<<wind_exact
                 <<" DIFFERENT="<<wind_mismatch
                 <<" PGT_MATCH="<<stable_match
                 <<" MAX_U_DIFF="<<max_u<<" MAX_V_DIFF="<<max_v<<"\n";
        // Original cell selection is an accepted assertion; wind diagnostic
        // deliberately does NOT declare parity unless exact at every puff.
        // A nonzero wind mismatch is a physical integration blocker, not a
        // regression of the already-accepted isolated Fortran ADVWND kernel.
        if(wind_mismatch!=0) {
            std::cerr<<"P7_NATIVE_METFM1_FULL_ADVECT=NOT_YET_EQUIVALENT"
                     <<" DIAGNOSE_GETCELL_SPATIAL_INTERPOLATION\n";
            return 2;
        }
        std::cout<<"P7_NATIVE_METFM1_ADVECT=PASS RECORDS=1275 REAL4_EXACT=TRUE\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<"P7_NATIVE_METFM1_ADVECT=FAIL "<<e.what()<<"\n";
        return 1;
    }
}
