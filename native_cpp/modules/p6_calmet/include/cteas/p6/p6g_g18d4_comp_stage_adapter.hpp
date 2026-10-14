#pragma once
// G18D4: selected source-ordered COMP *stage execution* on one G14 frame:
// GRID MIXHT -> AVEMIX -> WSTARR -> TEMP3D; no precomputed end-of-hour
// candidate may impersonate independently completed original CALMET stages.
// Every calculation delegates to already tested G18A/G18C kernels.
// Production MultiHourPhysics::run deliberately remains Unknown/false:
// selected fixture inputs (notably G18A MIXHT coefficients) are NOT
// source-proven 17-stage native CALMET acceptance.
#include "cteas/p6/p6g_g18c4_selected_hour_candidate.hpp"
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cteas::calmet::p6g {
class G18D4SelectedCompStageAdapter final : public MultiHourPhysics {
public:
    G18D4SelectedCompStageAdapter(
        G17CSelectedHeatResult heat,
        G17C2SelectedElustrResult elustr,
        std::vector<G17C3StationDiagi> diagi,
        G18BUpperHour observed_upper,
        G18ASelectedCompControls mixht,
        G18C1AvemixControls avemix,
        G18C3Temp3dControls temp3d)
        : heat_(std::move(heat)),
          elustr_(std::move(elustr)),
          diagi_(std::move(diagi)),
          upper_(std::move(observed_upper)),
          mixht_(std::move(mixht)), avemix_(avemix), temp3d_(temp3d) {}

    // Never mark fixture-sourced partial stages as production NativeCpp.
    p6::NativeHourStageResult run(
        p6::NativeHourStage,MultiHourFrame&) override {
        return {p6::NativeHourSource::Unknown,false};
    }

    // G18D4 selected fixture gate. The stage supplied here must match
    // the NEXT COMP entry in immutable PR #53 kHefeiOriginalStageOrder.
    // It is not a second hourly scheduler; G14 alone selects input hour.
    void run_selected(p6::NativeHourStage stage,MultiHourFrame& frame,
                      bool explicitly_test_only) {
        static constexpr p6::NativeHourStage sequence[]={
            p6::NativeHourStage::CompGridMixht,
            p6::NativeHourStage::CompAvemix,
            p6::NativeHourStage::CompWstarr,
            p6::NativeHourStage::CompTemp3d};
        if(!explicitly_test_only||stage_index_>=4||
           stage!=sequence[stage_index_]||
           (bound_&&(frame.hour_index!=hour_index_||
                     frame.input.end!=source_end_))||
           !frame.source_controls||
           frame.input.end!=frame.surface.end||
           upper_.end!=frame.input.end||
           heat_.end!=frame.input.end||
           elustr_.end!=frame.input.end)
            throw std::invalid_argument("G18D4 original COMP stage or source hour mismatch");
        if(stage_index_==0) {
            const auto pre=selected_comp_mixht_with_observed_upper(
                frame,heat_,elustr_,diagi_,upper_,mixht_);
            mixed_.emplace(pre);
            bound_=true;hour_index_=frame.hour_index;source_end_=frame.input.end;
        }else if(stage_index_==1) {
            averaged_.emplace(g18c1_selected_after_mixht(
                frame,*mixed_,avemix_,true));
        }else if(stage_index_==2) {
            conv_.emplace(g18c2_wstarr_after_avemix(
                frame,*mixed_,*averaged_,heat_,elustr_,true));
        }else{
            // TEMP3D must consume actual physical AA/BB FACET profiles
            // for this same source hour and POST-AVEMIX ZICONV.
            const auto faces=g18b5_selected_native_temperature_faces(
                frame,upper_);
            const auto temp=g18c3_selected_native_temp3d(
                frame,upper_,faces,heat_,*conv_,temp3d_,true);
            result_.emplace(g18c4_assemble_selected_hour(
                frame,heat_,elustr_,*conv_,temp,true));
            // A G14 candidate, NOT a committed earlier-hour history.
            frame.next=result_->candidate_memory;
            frame.fields=result_->candidate_fields;
        }
        ++stage_index_;
    }
    [[nodiscard]] bool selected_complete() const noexcept {
        return stage_index_==4&&result_.has_value();
    }
    [[nodiscard]] const G18C4SelectedCandidate& selected_candidate() const {
        if(!selected_complete())
            throw std::logic_error("G18D4 COMP source stages incomplete");
        return *result_;
    }
private:
    G17CSelectedHeatResult heat_;
    G17C2SelectedElustrResult elustr_;
    std::vector<G17C3StationDiagi> diagi_;
    G18BUpperHour upper_;
    G18ASelectedCompControls mixht_;
    G18C1AvemixControls avemix_;
    G18C3Temp3dControls temp3d_;
    std::optional<G18ASelectedCompResult> mixed_;
    std::optional<G18C1AvemixOutput> averaged_;
    std::optional<G18C2SourceCompAfterAverage> conv_;
    std::optional<G18C4SelectedCandidate> result_;
    std::size_t stage_index_=0,hour_index_=0;
    bool bound_=false;
    std::chrono::sys_seconds source_end_{};
};
} // namespace cteas::calmet::p6g
