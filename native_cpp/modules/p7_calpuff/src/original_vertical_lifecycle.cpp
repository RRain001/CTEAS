#include "cteas/puff/original_vertical_lifecycle.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace cteas::puff {
namespace {
void require(bool ok,const char* what) {
    if(!ok) throw std::invalid_argument(std::string("P7 native vertical: ")+what);
}
float real4(double value,const char* name) {
    require(std::isfinite(value),name);
    const float f=static_cast<float>(value);
    require(std::isfinite(f),name);
    return f;
}
}
void OriginalVerticalLifecycle::admit_point(
    int id,const NewPuffInputs& source,const InjectionInputs& injection) {
    require(id>0 && source.source_type==1 && injection.species_count>0,
            "invalid point puff admission");
    require(puffs_.find(id)==puffs_.end(),"duplicate active puff");
    require(real4(source.effective_height_m,"release height")==
            real4(injection.effective_height_m,"injection height"),
            "POINTS1/INJECT inconsistent effective height");
    require(source.stability_class==injection.stability_class,
            "POINTS1/INJECT inconsistent release stability");
    const auto release=new_puff_state(source);
    const auto allocated=inject_emissions(injection);
    NativeVerticalPuffState state;
    state.sigma_y_m=real4(release.sigma_y_m,"source sigma-y");
    state.sigma_z_m=real4(release.sigma_z_m,"source sigma-z");
    state.ziold_m=real4(allocated.previous_mixing_height_m,"ZIOLD");
    state.zimax_m=real4(allocated.unlimited_mixing_height_m,"ZIMAX");
    state.puff_code=allocated.puff_code;
    require(state.sigma_y_m>=0 && state.sigma_z_m>=0 &&
            state.ziold_m>=0 && state.zimax_m>=0 &&
            state.puff_code>0,"invalid initial vertical state");
    for(double mass:allocated.mixed_layer_mass) {
        const auto value=real4(mass,"mixed source mass");
        require(value>=0,"negative mixed source mass");
        state.mass_mixed.push_back(value);
    }
    for(double mass:allocated.above_layer_mass) {
        const auto value=real4(mass,"upper source mass");
        require(value>=0,"negative upper source mass");
        state.mass_above.push_back(value);
    }
    require(state.mass_mixed.size()==static_cast<std::size_t>(injection.species_count) &&
            state.mass_above.size()==state.mass_mixed.size(),
            "incomplete source mass state");
    puffs_.emplace(id,std::move(state));
}
NativeVerticalPuffState OriginalVerticalLifecycle::state(int id) const {
    const auto it=puffs_.find(id);
    if(it==puffs_.end())throw std::out_of_range("unknown vertical puff");
    return it->second;
}
NativeVerticalWind OriginalVerticalLifecycle::sample(
    int id,const OriginalMetHour& met,const Calmet21Static& statics,
    float x,float y,float height) const {
    const auto s=state(id);
    const auto z=real4(height,"native HTMET");
    const auto cell=original_met_cell(x,y,met.nx,met.ny);
    require(cell.offset_0based<met.zi.size() &&
            cell.offset_0based<met.ipgt.size(),
            "METFM1 missing real original surface fields");
    const auto mixing=met.zi[cell.offset_0based];
    const auto stable=met.ipgt[cell.offset_0based];
    require(std::isfinite(mixing) && mixing>=0 &&
            stable>=1 && stable<=6,
            "METFM1 mixing depth or stability invalid");
    PuffExtentInputs input;
    input.puff_code=s.puff_code;
    input.puff=true;
    input.stability_class=stable;
    input.puff_height_m=z;
    input.mixing_height_m=mixing;
    input.sigma_z_begin_m=s.sigma_z_m;
    input.previous_mixing_height_m=s.ziold_m;
    input.maximum_mixing_height_m=s.zimax_m;
    for(float q:s.mass_mixed)input.mixed_layer_mass.push_back(q);
    for(float q:s.mass_above)input.above_layer_mass.push_back(q);
    const auto extent=puff_extent(input);
    const auto wind=original_met_cell_advect(
        met,statics,cell,real4(extent.z_bottom_m,"ZBOT"),
        real4(extent.z_top_m,"ZTOP"));
    return {cell,extent,wind,stable,mixing};
}
SigmaUpdateResult OriginalVerticalLifecycle::commit_sigma(
    int id,const SigmaUpdateInputs& inputs,
    const SigmaYCurve& yc,const SigmaZCurve& zc) {
    const auto it=puffs_.find(id);
    if(it==puffs_.end())throw std::out_of_range("unknown vertical puff");
    const auto& previous=it->second;
    require(real4(inputs.step.sigma_y_start_m,"sigma-y begin")==previous.sigma_y_m &&
            real4(inputs.step.sigma_z_start_m,"sigma-z begin")==previous.sigma_z_m &&
            inputs.step.puff_code==previous.puff_code,
            "stale or incorrect sigma update state");
    auto next=sigma_update(inputs,yc,zc);
    const float sy=real4(next.adjust.stored_sigma_y_m,"sigma-y result");
    const float sz=real4(next.adjust.stored_sigma_z_m,"sigma-z result");
    require(sy>=0 && sz>=0,"negative stored native dispersion sigma");
    it->second.sigma_y_m=sy;
    it->second.sigma_z_m=sz;
    ++it->second.completed_substeps;
    return next;
}
void OriginalVerticalLifecycle::retire(int id) {
    if(puffs_.erase(id)!=1U)throw std::out_of_range("retire unknown vertical puff");
}
std::size_t OriginalVerticalLifecycle::active_count()const noexcept {
    return puffs_.size();
}
} // namespace cteas::puff
