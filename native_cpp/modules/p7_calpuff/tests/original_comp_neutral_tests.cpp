#include "cteas/puff/original_comp_neutral.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace cteas::puff;
namespace {
void require(bool condition,const char* msg){
 if(!condition)throw std::runtime_error(msg);
}
template<class F> void rejects(F&& act,const char* why){
 bool threw=false;try{act();}catch(const std::exception&){threw=true;}
 require(threw,why);
}
struct Set {
 OriginalCompNeutralPointDriver driver;
 OriginalMetHour met;
 Calmet21Static statics;
 NeutralCompControls opts;
 SigmaYCurve cy;
 SigmaZCurve cz;
 Set():cy([](double sigma,double dx,double dt)->SigmaCurvePoint{
             return {sigma,dt,dx};
          }),cz([](double sigma,double dx,double dt,double)->SigmaCurvePoint{
             return {sigma,dt,dx};
          }) {
    met.nx=100;met.ny=100;met.nz=2;
    const auto area=100U*100U;
    met.zi.assign(area,100.0F);
    met.ipgt.assign(area,4);
    met.u.assign(area*2,0.002F);met.v.assign(area*2,0.0F);
    statics.cell_face_m={0.0F,100.0F,200.0F};
    NewPuffInputs point;
    point.source_x_km=5.0;
    point.source_y_km=5.0;
    point.effective_height_m=50.0;
    point.sigma_y_source_m=1.0;
    point.sigma_z_source_m=1.0;
    point.stability_class=4;
    InjectionInputs inj;
    inj.species_count=2;
    inj.emission_duration_s=3600.0;
    inj.effective_height_m=50.0;
    inj.mixing_height_m=100.0;
    inj.max_layer_height_m=200.0;
    inj.mass_fraction_mixed=1.0;
    inj.stability_class=4;
    inj.gaussian_distribution=1;
    inj.emission_rate={2.0,3.0};
    driver.admit(1,point,inj);
    opts.max_sampling_steps=1;
 }
};
void run(const int hours) {
 Set s;
 const auto original_mass=s.driver.vertical(1).mass_mixed;
 require(original_mass.size()==2 && original_mass[0]==7200.0F &&
         original_mass[1]==10800.0F,"native two-species source emission incorrect");
 rejects([&]{(void)s.driver.step(999,s.met,s.statics,s.opts,s.cy,s.cz);},
         "nonexistent puff accepted");
 rejects([&]{auto bad=s.opts;bad.seconds=-1;
              (void)s.driver.step(1,s.met,s.statics,bad,s.cy,s.cz);},
         "negative time accepted");
 require(s.driver.steps(1)==0 &&
         s.driver.motion(1).travel_time_s==0.0F,
         "failed prevalidation mutated stored state");

 for(int hour=0;hour<hours;++hour) {
   const auto start=s.driver.motion(1);
   const auto vbefore=s.driver.vertical(1);
   const auto step=s.driver.step(1,s.met,s.statics,s.opts,s.cy,s.cz);
   const auto result=s.driver.motion(1);
   const auto after=s.driver.vertical(1);
   require(step.cell.ix_1based>=1 && step.cell.ix_1based<=100 &&
           step.cadence.sampling_steps==1 && !step.substep.split &&
           step.after.travel_time_s==float((hour+1)*3600),
           "native hour cadence or step time discontinuity");
   require(result.x_cells>start.x_cells &&
           result.y_cells==start.y_cells &&
           result.z_m==50.0F &&
           result.total_distance_m>start.total_distance_m,
           "persisted native motion not applied");
   require(after.completed_substeps==std::size_t(hour+1) &&
           after.committed_lid_steps==std::size_t(hour+1) &&
           step.mass.puff_code==after.puff_code &&
           after.puff_code==1 &&
           after.ziold_m==100.0F && after.zimax_m==100.0F,
           "VMASS/COMP native mixing history inconsistent");
   require(vbefore.mass_mixed==after.mass_mixed &&
           vbefore.mass_above==after.mass_above &&
           after.mass_mixed==original_mass,
           "original source mass not conserved across COMP steps");
   if(hour==0) {
      // Failure during the *late* P7Q phase must also roll back the already
      // computed GETCELL / ADVWND / native advance_puff / SETPUF changes.
      const auto before_m=s.driver.motion(1);
      const auto before_v=s.driver.vertical(1);
      auto badCurve=[](double,double,double)->SigmaCurvePoint {
          return {std::numeric_limits<double>::quiet_NaN(),0,0};
      };
      rejects([&]{(void)s.driver.step(1,s.met,s.statics,
                                      s.opts,badCurve,s.cz);},
              "late sigma failure accepted");
      const auto after_m=s.driver.motion(1);
      const auto after_v=s.driver.vertical(1);
      require(after_m.x_cells==before_m.x_cells &&
              after_m.total_distance_m==before_m.total_distance_m &&
              after_m.travel_time_s==before_m.travel_time_s &&
              after_v.completed_substeps==before_v.completed_substeps &&
              after_v.committed_lid_steps==before_v.committed_lid_steps &&
              s.driver.steps(1)==1,
              "late physics rejection partially mutated two native stores");
   }
 }
 require(s.driver.steps(1)==std::size_t(hours) &&
         s.driver.motion(1).travel_time_s==float(hours*3600) &&
         s.driver.active_count()==1,
         "native COMP longitudinal hour count incomplete");
 s.driver.retire(1);
 require(s.driver.active_count()==0,"native terminal retirement failed");
 rejects([&]{(void)s.driver.motion(1);},"retired puff still visible");
 std::cout<<"P7_NATIVE_COMP_NEUTRAL=PASS HOURS="<<hours
          <<" STEPS="<<hours
          <<" NATIVE_UV_METFM1=YES NATIVE_PUFFDZ=YES"
          <<" NATIVE_SETPUF_SIGMA=YES NATIVE_VMASS_COMP=YES"
          <<" STORE_ATOMICITY=YES MASS_CONSERVED=YES"
          <<" RISE_AND_CALM=UNIMPLEMENTED"
          <<" FROZEN_143H_PHYSICAL_PARITY=NOT_CLAIMED\n";
}
}
int main(int argc,char** argv){
 try{
   if(argc!=2)throw std::invalid_argument("usage: neutral_comp_test [1|6|143]");
   const auto hours=std::stoi(argv[1]);
   if(hours!=1 && hours!=6 && hours!=143)
       throw std::invalid_argument("only 1/6/143 hour fixture supported");
   run(hours);return 0;
 }catch(const std::exception& e) {
   std::cerr<<"P7_NATIVE_COMP_NEUTRAL=FAIL "<<e.what()<<"\n";
   return 1;
 }
}
