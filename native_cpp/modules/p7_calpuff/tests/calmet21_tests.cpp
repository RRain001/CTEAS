#include "cteas/puff/calmet21.hpp"
#include <cstring>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
namespace fs=std::filesystem;
using cteas::puff::Calmet21Reader;
namespace {
void put32(std::vector<std::uint8_t>& b,std::uint32_t v,std::size_t p) {
    assert(p+4<=b.size());
    for (int j=0;j<4;++j) b[p+j]=std::uint8_t(v>>(j*8));
}
void putint(std::vector<std::uint8_t>& b,int x,std::size_t p) {
    const std::int32_t i = static_cast<std::int32_t>(x);
    std::uint32_t bits = 0;
    static_assert(sizeof(i) == sizeof(bits));
    std::memcpy(&bits, &i, sizeof(bits));
    put32(b,bits,p);
}
void putfloat(std::vector<std::uint8_t>& b,float x,std::size_t p) {
    std::uint32_t bits = 0;
    static_assert(sizeof(x) == sizeof(bits));
    std::memcpy(&bits, &x, sizeof(bits));
    put32(b,bits,p);
}
void record(std::ofstream& f,const std::vector<std::uint8_t>& bytes) {
    const auto size=static_cast<std::uint32_t>(bytes.size());
    for (int j=0;j<4;++j) f.put(static_cast<char>(size>>(8*j)));
    f.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    for (int j=0;j<4;++j) f.put(static_cast<char>(size>>(8*j)));
}
std::vector<std::string> field_names() {
    return {"U-LEV  1","V-LEV  1","WFACE  1","U-LEV  2","V-LEV  2","WFACE  2",
            "T-LEV  1","T-LEV  2","IPGT","USTAR","ZI","EL","WSTAR",
            "TEMPK","RHO","QSW","IRH"};
}
void create_fixture(const fs::path& path,bool order=false,bool gap=false,bool nan=false) {
    std::ofstream f(path,std::ios::binary|std::ios::trunc);
    std::vector<std::uint8_t> signature(40,' ');
    const std::string id="CALMET.DAT 2.1";
    for (std::size_t i=0;i<id.size();++i) signature[i]=std::uint8_t(id[i]);
    record(f,signature);
    record(f,std::vector<std::uint8_t>(4,0));
    std::vector<std::uint8_t> h(112,0);
    const std::string tz="UTC+0000";
    for (std::size_t i=0;i<tz.size();++i) h[40+i]=std::uint8_t(tz[i]);
    putint(h,1,52); putint(h,2,56); putint(h,2,60); putint(h,2,64);
    putfloat(h,100.0F,68); putfloat(h,513000.0F,72);
    putfloat(h,3533000.0F,76); putint(h,1,80); putint(h,1,84); putint(h,1,88);
    record(f,h);
    for (int hour=0;hour<2;++hour) {
        auto labels=field_names();
        for (std::size_t i=0;i<labels.size();++i) {
            std::vector<std::uint8_t> b(40,0);
            const auto label=order && hour==0 && i==0 ? "NOTWIND" : labels[i];
            for (std::size_t k=0;k<8;++k) b[k]= k<label.size() ? std::uint8_t(label[k]) : ' ';
            const int shift=gap && hour==1 ? 1 : 0;
            putint(b,202200100+hour+shift,8); putint(b,0,12);
            putint(b,202200101+hour+shift,16); putint(b,0,20);
            for (int k=0;k<4;++k) {
                if (labels[i]=="IPGT" || labels[i]=="IRH") putint(b,11+k,24+4*k);
                else if (nan && hour==0 && i==0 && k==0)
                    put32(b,0x7FC00000U,24+4*k);
                else putfloat(b,2.5F+float(k),24+4*k);
            }
            record(f,b);
        }
    }
}
template<typename F> void must_fail(F fn) {
    bool threw=false;
    try {fn();} catch (const std::runtime_error&) {threw=true;}
    assert(threw);
}
struct Cleanup {fs::path root;~Cleanup(){std::error_code e;fs::remove_all(root,e);} };
}
int main() {
    const auto root=fs::temp_directory_path()/"cteas_p7_calmet21_tests";
    fs::create_directories(root);
    Cleanup cleanup{root};
    const auto good=root/"correct.dat";
    create_fixture(good);
    std::size_t called=0,integers=0;
    const auto summary=Calmet21Reader::read(good.string(),[&](const auto& field){
        ++called;
        if (field.integer) {
            ++integers;
            assert(field.integer_values.size()==4 && field.integer_values.at(0)==11);
        } else {
            assert(field.real_values.size()==4 && field.real_values.at(0)==2.5F);
        }
    });
    assert(summary.intervals==2 && summary.hourly_fields==34);
    assert(summary.header.nx==2 && summary.header.ny==2 && summary.header.nz==2);
    assert(summary.final_end.unix_seconds-summary.first_begin.unix_seconds==7200);
    assert(called==34 && integers==4);
    for (const auto& [name,wrong_order,gap,nan]:
         std::vector<std::tuple<std::string,bool,bool,bool>>{
             {"bad-order",true,false,false},
             {"bad-time",false,true,false},
             {"nan-input",false,false,true}}) {
        const auto p=root/name;
        create_fixture(p,wrong_order,gap,nan);
        must_fail([&]{(void)Calmet21Reader::read(p.string());});
    }
    must_fail([&]{(void)Calmet21Reader::read(good.string(),{},true);});
}
