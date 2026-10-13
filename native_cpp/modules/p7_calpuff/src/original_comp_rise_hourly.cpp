#include "cteas/puff/original_comp_rise_hourly.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace cteas::puff {
RiseCompHour OriginalCompRiseMultiPuffHours::advance_hour(
    const OriginalMetHour& met,const Calmet21Static& statics,
    const NeutralCompControls& controls,const DispersionSetup& pg,
    bool urban,const std::vector<RisePointAdmission>& admissions) {
    int last=last_id_;
    for(const auto& admission:admissions){
        if(admission.puff_id<=last)
            throw std::invalid_argument("noncanonical rising point release IDs");
        last=admission.puff_id;
    }
    // Copy *all* native rise, motion and mass state at the hour boundary.
    // No puff (old or newly admitted) is committed if ANY puff fails.
    auto trial=puffs_;
    auto ids=active_ids_;
    for(const auto& a:admissions){
        trial.admit(a.puff_id,a.point,a.mass);
        ids.push_back(a.puff_id);
    }
    RiseCompHour result;
    result.hour_index=hours_;
    result.puffs.reserve(ids.size());
    for(const int id:ids){
        result.puffs.push_back({id,
            trial.step_pg_rise(id,met,statics,controls,pg,urban)});
    }
    result.active_after=ids.size();
    puffs_=std::move(trial);
    active_ids_=std::move(ids);
    last_id_=last;
    ++hours_;
    return result;
}
OriginalPuffMotionState OriginalCompRiseMultiPuffHours::motion(int id)const {
    return puffs_.motion(id);
}
NativeVerticalPuffState OriginalCompRiseMultiPuffHours::vertical(int id)const{
    return puffs_.vertical(id);
}
std::size_t OriginalCompRiseMultiPuffHours::puff_steps(int id)const{
    return puffs_.steps(id);
}
std::size_t OriginalCompRiseMultiPuffHours::completed_hours()const noexcept{
    return hours_;
}
std::size_t OriginalCompRiseMultiPuffHours::active_count()const noexcept{
    return active_ids_.size();
}
const std::vector<int>& OriginalCompRiseMultiPuffHours::canonical_ids()const noexcept{
    return active_ids_;
}
void OriginalCompRiseMultiPuffHours::retire(int id){
    if(std::find(active_ids_.begin(),active_ids_.end(),id)==active_ids_.end())
        throw std::out_of_range("retire unknown rising puff");
    auto next=puffs_;
    next.retire(id);
    puffs_=std::move(next);
    active_ids_.erase(
        std::remove(active_ids_.begin(),active_ids_.end(),id),
        active_ids_.end());
}
} // namespace cteas::puff
