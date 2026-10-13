#include "cteas/puff/original_native_terrain.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace cteas::puff;
static void need(bool ok,const char* reason){
    if(!ok)throw std::runtime_error(reason);
}
int main(){
    try{
        Calmet21Static statics;
        constexpr int nx=12,ny=12;
        statics.present=true;
        statics.elevation_m.resize(nx*ny);
        for(int j=0;j<ny;++j)for(int i=0;i<nx;++i)
            statics.elevation_m[static_cast<std::size_t>(j*nx+i)]=
                40.0F+0.3F*static_cast<float>(i)+
                0.1F*static_cast<float>(j);
        const auto terrain=OriginalNativeTerrain::from_calmet(
            statics,nx,ny,100.0F);
        const auto lookup=terrain.constraint_lookup();
        need(lookup.cell_elevation_msl(5.2,6.1)>40 &&
             std::isfinite(lookup.elevation_msl(5.2,6.1)) &&
             std::isfinite(lookup.relief(5.2,6.1,5)) &&
             lookup.valley_width(5.2,6.1,1)>=-1,
             "original TRELIEF/VWIDTH/GETELEV native source cache incomplete");
        // Original CALPUFF GETCELL/GRIDIJ (unlike a strict grid accessor)
        // clamps the last step of a departing puff to its boundary MET
        // cell. Its independent GETELEV bilinear interpolation remains
        // a different, source-defined operation.
        need(lookup.cell_elevation_msl(-0.2,6.1)==
                 statics.elevation_m[6*nx] &&
             lookup.cell_elevation_msl(12.3,6.1)==
                 statics.elevation_m[6*nx+(nx-1)] &&
             lookup.relief(-0.2,6.1,5)==lookup.relief(0.0,6.1,5) &&
             lookup.valley_width(12.3,6.1,1)==
                 lookup.valley_width(11.3,6.1,1),
             "source GETCELL did not clamp out-of-domain final puff substep");
        const OriginalPuffct2Result position{
            .x_m=520.0F,.y_m=610.0F,.z_m=30.0F};
        const auto receiver=terrain.receptor(
            5+6*nx,1,1,nx,ny,position,0.015F,1.2F);
        const auto expected=statics.elevation_m[5+6*nx];
        need(receiver.receptor_ground_elevation_msl==expected &&
             receiver.terrain_peak_elevation_msl>=expected &&
             std::isfinite(receiver.terrain_under_puff_msl),
             "PUFRECS ground/peak/puff GETELEV indices mismatched");
        bool missed=false;
        try {(void)terrain.receptor(nx*ny,1,1,nx,ny,position,0.0F,1.2F);}
        catch(const std::invalid_argument&){missed=true;}
        need(missed,"terrain adapter allowed an invalid receptor index");
        statics.elevation_m.clear();
        bool no_grid=false;
        try{(void)OriginalNativeTerrain::from_calmet(statics,nx,ny,100.0F);}
        catch(const std::invalid_argument&){no_grid=true;}
        need(no_grid,"terrain adapter invented missing CALMET static ELEV");
        std::cout<<"P7_REAL_CALMET_STATIC_TERRAIN_ADAPTER=PASS"
                 <<" TRELIEF=SOURCE_PORTED GETELEV=SOURCE_PORTED"
                 <<" VWIDTH=SOURCE_PORTED"
                 <<" NO_DGM=YES MCTADJ2_ORIGINAL_CONC=NOT_YET_ACCEPTED\\n";
        return 0;
    }catch(const std::exception& e){
        std::cerr<<"P7_REAL_CALMET_STATIC_TERRAIN_ADAPTER=FAIL "
                 <<e.what()<<"\\n";
        return 1;
    }
}
