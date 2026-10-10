#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
struct LU {
    int id = 0;
    int out = 0;
    float z0 = 0, albedo = 0, bowen = 0, soil = 0, qf = 0, lai = 0;
};
struct Setup {
    int nx=50,ny=50,nincat=38,noutcat=14,numwat=5,imiss=55;
    float cfract=0.5f,flumin=0.96f;
    std::string title;
    std::map<std::string,std::string> opts;
    std::vector<int> categories,output_categories,water;
    std::vector<LU> table;
};
std::string strip(const std::string& s) {
    auto a=s.find_first_not_of(" \t\r\n");
    if(a==std::string::npos)return {};
    return s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);
}
std::vector<float> numbers(std::string s) {
    std::replace(s.begin(),s.end(),',',' ');
    std::istringstream in(s);
    std::vector<float> v;float t=0;
    while(in>>t) {
        if(!std::isfinite(t))throw std::runtime_error("nonfinite MAKEGEO numeric control");
        v.push_back(t);
    }
    if(!in.eof())throw std::runtime_error("unexpected nonnumeric MAKEGEO control");
    return v;
}
int strict_int(float x) {
    if(!std::isfinite(x)||x!=static_cast<float>(static_cast<int>(x)))
        throw std::runtime_error("invalid integer in MAKEGEO controls");
    return static_cast<int>(x);
}
void require(bool ok,const std::string& msg) {
    if(!ok)throw std::runtime_error("P3: "+msg);
}
int numopt(const Setup& c,const std::string& name) {
    auto it=c.opts.find(name);
    require(it!=c.opts.end(),"missing MAKEGEO control "+name);
    auto n=numbers(it->second);
    require(n.size()==1,"malformed numeric control "+name);
    return strict_int(n[0]);
}
float floatopt(const Setup& c,const std::string& name) {
    auto it=c.opts.find(name);
    require(it!=c.opts.end(),"missing MAKEGEO control "+name);
    auto n=numbers(it->second);
    require(n.size()==1,"malformed numeric control "+name);
    return n[0];
}
std::string str_opt(const Setup& c,const std::string& name) {
    auto it=c.opts.find(name);
    require(it!=c.opts.end(),"missing MAKEGEO control "+name);
    return strip(it->second);
}
Setup parse_controls(const std::string& path) {
    std::ifstream in(path);
    require(bool(in),"cannot open original MAKEGEO control");
    Setup c;
    std::string line;
    while(std::getline(in,line)) {
        auto begin=line.find('!');
        if(begin==std::string::npos)continue;
        auto end=line.find('!',begin+1);
        if(end==std::string::npos)continue;
        std::string segment=strip(line.substr(begin+1,end-begin-1));
        auto eq=segment.find('=');
        if(eq==std::string::npos)continue;
        auto key=strip(segment.substr(0,eq));
        auto value=strip(segment.substr(eq+1));
        if(key=="X") {
            auto n=numbers(value);
            require(n.size()==8,"expected 8 parameters for each input landuse");
            c.table.push_back({strict_int(n[0]),strict_int(n[7]),n[1],n[2],n[3],n[4],n[5],n[6]});
        } else if(key=="OUTCAT") {
            for(float f:numbers(value))c.output_categories.push_back(strict_int(f));
        } else if(key=="IWAT") {
            for(float f:numbers(value))c.water.push_back(strict_int(f));
        } else {
            if(c.opts.contains(key))require(c.opts[key]==value,"duplicate inconsistent MAKEGEO key "+key);
            c.opts[key]=value;
        }
    }
    require(str_opt(c,"LTERR")=="T","study path requires LTERR=T");
    require(str_opt(c,"LLU2")=="F","LU2 supplementation unsupported");
    require(str_opt(c,"LSNOW")=="F","snow processing unsupported");
    require(numopt(c,"NSPLIT")==0,"category split unsupported");
    require(str_opt(c,"PMAP")=="UTM","only UTM coordinates supported");
    require(numopt(c,"IUTMZN")==50,"only zone 50 supported");
    require(str_opt(c,"UTMHEM")=="N","only Northern UTM supported");
    require(str_opt(c,"DATUM")=="WGS-84","only WGS84 supported");
    c.nx=numopt(c,"NX");c.ny=numopt(c,"NY");
    require(c.nx==50&&c.ny==50&&std::fabs(floatopt(c,"DGRIDKM")-0.1f)<1e-6,
            "only Hefei 50x50 at 100m supported");
    c.nincat=numopt(c,"NINCAT");
    c.noutcat=numopt(c,"NOUTCAT");
    c.numwat=numopt(c,"NUMWAT");
    c.imiss=numopt(c,"IMISS");
    c.cfract=floatopt(c,"CFRACT");
    c.flumin=floatopt(c,"FLUMIN");
    require(c.nincat==38&&c.noutcat==14&&c.numwat==5,
            "expected frozen 38 input and 14 output classes");
    require(c.table.size()==38&&c.output_categories.size()==14&&c.water.size()==5,
            "class table/category header incomplete");
    std::set<int> seen;
    for(const LU& t:c.table) {
        require(seen.insert(t.id).second,"duplicate input category");
        require(t.z0>0&&std::isfinite(t.z0),"z0 must be positive for log weights");
        require(std::isfinite(t.albedo)&&std::isfinite(t.bowen)&&
                std::isfinite(t.soil)&&std::isfinite(t.qf)&&std::isfinite(t.lai),
                "nonfinite surface parameter");
        c.categories.push_back(t.id);
    }
    for(int w:c.water)require(seen.contains(w),"input water category absent");
    return c;
}
struct Cell {
    std::array<float,38> fractions{};
    float elev=0;
    int dominant=0;
    std::array<float,6> params{};  // z0, albedo, bowen, soil, qf, LAI
};
std::vector<Cell> read_landuse(const std::string& path,const Setup& cfg) {
    std::ifstream in(path);
    require(bool(in),"cannot open LU.DAT");
    std::string s;
    std::array<std::string,11> head;
    for(auto& h:head)require(bool(std::getline(in,h)),"LU.DAT header truncated");
    require(head[0].find("LU.DAT")!=std::string::npos&&head[0].find("2.1")!=std::string::npos,
            "expected LU.DAT v2.1");
    require(strip(head[4])=="FRACTION"&&strip(head[5])=="UTM"&&
            head[6].find("50N")!=std::string::npos&&
            head[7].find("WGS-84")!=std::string::npos,
            "LU.DAT CRS/type mismatch");
    {std::istringstream shape(head[8]);int x=0,y=0;
     shape>>x>>y;require(x==50&&y==50,"LU.DAT grid shape mismatch");}
    {std::istringstream classes(head[10]);int id=0;
     for(int k=0;k<cfg.nincat;k++){
        require(bool(classes>>id),"LU.DAT category header truncated");
        require(id==cfg.table.at(static_cast<std::size_t>(k)).id,"LU.DAT category order mismatch");
     }
     require(!(classes>>id),"LU.DAT category count mismatch");
    }
    std::vector<Cell> out(2500);
    for(std::size_t n=0;n<out.size();n++) {
        require(bool(std::getline(in,s)),"LU.DAT missing grid cell");
        std::istringstream stream(s);int i=0,j=0;
        require(bool(stream>>i>>j),"LU.DAT bad cell index");
        require(i==static_cast<int>(n%50+1)&&j==static_cast<int>(n/50+1),
                "LU.DAT row order mismatch");
        for(float& f:out[n].fractions) {
            require(bool(stream>>f),"LU.DAT fraction missing");
            require(std::isfinite(f)&&f>=0&&f<=1,"bad input LU fraction");
        }
        std::string marker,extra;
        require(bool(stream>>marker)&&marker=="."&&!(stream>>extra),"LU.DAT row suffix mismatch");
    }
    require(!std::getline(in,s)||strip(s).empty(),"LU.DAT unexpected extra rows");
    return out;
}
void read_terrain(const std::string& path,std::vector<Cell>& cells) {
    std::ifstream in(path);
    require(bool(in),"cannot open TERREL.DAT");
    std::string s;std::array<std::string,10> head;
    for(auto& h:head)require(bool(std::getline(in,h)),"TERREL header incomplete");
    require(head[0].find("TERREL.DAT")!=std::string::npos&&
            head[0].find("2.0")!=std::string::npos,"TERREL source identity mismatch");
    require(strip(head[4])=="UTM"&&head[5].find("50N")!=std::string::npos&&
            head[6].find("WGS-84")!=std::string::npos&&
            head[9].find("W_E N_S")!=std::string::npos,"TERREL source CRS/axis mismatch");
    {std::istringstream dims(head[7]);int nx=0,ny=0;
     dims>>nx>>ny;require(nx==50&&ny==50,"TERREL grid shape mismatch");}
    // Original terrain prints north-to-south; landuse is indexed south-to-north.
    for(int j=49;j>=0;j--)for(int i=0;i<50;i++) {
        float z=0;
        require(bool(in>>z),"TERREL height file truncated");
        require(std::isfinite(z)&&z>=-998,"terrain missing; no fill allowed");
        cells[static_cast<std::size_t>(j*50+i)].elev=z;
    }
    float extra=0;
    require(!(in>>extra),"TERREL file has extra numeric fields");
}
std::array<float,6> weights(const Cell& cell,const Setup& c) {
    std::array<float,6> n{};
    float denom=0.0f;
    for(int k=0;k<c.nincat;k++) {
        const float f=cell.fractions[static_cast<std::size_t>(k)];
        const auto& t=c.table.at(static_cast<std::size_t>(k));
        denom+=f;
        n[0]+=f*std::log(t.z0);
        n[1]+=f*t.albedo;
        n[2]+=f*t.bowen;
        n[3]+=f*t.soil;
        n[4]+=f*t.qf;
        n[5]+=f*t.lai;
    }
    require(denom>0,"zero landuse weight");
    for(float& p:n)p/=denom;
    n[0]=std::exp(n[0]);
    return n;
}
void compute(std::vector<Cell>& cells,const Setup& cfg) {
    for(Cell& cell:cells) {
        float total=0,watsum=0;
        for(int k=0;k<cfg.nincat;k++) {
            const float f=cell.fractions[static_cast<std::size_t>(k)];
            total+=f;
            if(std::find(cfg.water.begin(),cfg.water.end(),cfg.table[static_cast<std::size_t>(k)].id)
               !=cfg.water.end()) watsum+=f;
        }
        // The original has special IMISS fallback below 0.001; not in this study.
        require(total>=cfg.flumin&&total<=1.005f,
                "partial/missing landuse unsupported in this path");
        const bool water=watsum>cfg.cfract+0.0001f;
        float largest=-99.9f;
        int best=-1;
        for(int k=0;k<cfg.nincat;k++) {
            const bool this_water=std::find(cfg.water.begin(),cfg.water.end(),cfg.table[k].id)!=cfg.water.end();
            if(this_water!=water)continue;
            const float f=cell.fractions[static_cast<std::size_t>(k)];
            if(f>largest) {largest=f;best=k;}
        }
        require(best>=0,"dominant category not found");
        cell.dominant=cfg.table[static_cast<std::size_t>(best)].out;
        cell.params=weights(cell,cfg);
    }
}
void csv(const std::string& file,const std::vector<Cell>& cells) {
    std::ofstream out(file);require(bool(out),"cannot write P3 CSV");
    out<<"i,j,category,elevation_m,z0_m,albedo,bowen,soil_heat,anthropogenic_heat,lai\n";
    out<<std::fixed<<std::setprecision(9);
    for(std::size_t i=0;i<cells.size();i++) {
        const auto& c=cells[i];
        out<<i%50+1<<","<<i/50+1<<","<<c.dominant<<","<<c.elev;
        for(float p:c.params)out<<","<<p;
        out<<"\n";
    }
    require(bool(out),"P3 CSV write failed");
}
} // namespace
int main(int argc,char** argv) {
    if(argc!=5) {std::cerr<<"usage: cteas_p3_makegeo MAKEGEO.INP LU.DAT TERREL.DAT OUTPUT.CSV\n";return 2;}
    try {
        auto c=parse_controls(argv[1]);
        auto cells=read_landuse(argv[2],c);
        read_terrain(argv[3],cells);
        compute(cells,c);
        csv(argv[4],cells);
        std::cout<<"P3_SCOPED_CELLS="<<cells.size()<<" FIELDS=8\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<"P3 validation failed: "<<e.what()<<"\n";
        return 1;
    }
}
