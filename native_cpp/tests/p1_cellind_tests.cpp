#include "cteas/geo/terrain_grid.hpp"
#include <stdexcept>
#include <iostream>
using namespace cteas::reconstruction::geo;
int main() {
  TerrainGridSpec s{1000,2000,100,2,2};
  auto check=[&](float x,float y,int wantx,int wanty){
    const auto v=cell_index_terrel(x,y,s);
    if(wantx<0){if(v)throw std::runtime_error("unexpected grid hit");}
    else if(!v || static_cast<int>(v->first)!=wantx||static_cast<int>(v->second)!=wanty)
      throw std::runtime_error("CELLIND boundary mismatch");
  };
  check(1000,2000,0,0);
  check(1099,2099,0,0);
  check(1100,2000,1,0);
  check(1000,2100,0,1);
  check(999,2000,-1,-1);
  check(1000,1999,-1,-1);
  check(1200,2150,-1,-1);
  std::cout<<"P1 original CELLIND boundary subset PASS\n";
}
