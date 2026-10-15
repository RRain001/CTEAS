// G34B: TEST-ONLY six consecutive native G14 hours. No GUI/production binding.
// G26A/G26B production adapters remain Unknown,false. Nothing is borrowed
// from an original-vendor CALMET.DAT; frozen scientific comparison is separate.
#include "cteas/p6/p6g_g19_file_backed_case.hpp"
#include "cteas/p6/p6g_g26b_file_backed_comp_bridge.hpp"
#include "cteas/p6/p6g_g18d1_p6f_precommit.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace p6g=cteas::calmet::p6g;
namespace p6=cteas::calmet::p6;

class G34BSelectedOnlyHour final : public p6g::MultiHourPhysics {
public:
    G34BSelectedOnlyHour(const p6g::UnifiedMultistationHourlyScheduler& owner,
                         p6g::G18D2StationClockAuthority& clocks,
                         const p6g::G19FileBackedCase& source)
        :source_(source),diagno_(owner,clocks,source),comp_(diagno_,source) {}

    p6::NativeHourStageResult run(p6::NativeHourStage stage,
                                  p6g::MultiHourFrame& frame) override {
        if(cursor_>=17||stage!=p6::kHefeiOriginalStageOrder[cursor_])
            throw std::logic_error("G34B native stage skipped/reordered");
        if(cursor_<12) diagno_.run_selected(stage,frame,true);
        else if(cursor_<16) comp_.run_selected(stage,frame,true);
        else {
            const auto& candidate=comp_.selected_p6f_candidate(frame);
            const auto& heat=comp_.source_comp_heat_for_p6f(frame);
            const auto records=p6g::g18c5_selected_p6f_fields(
                frame,heat,candidate,source_.original_metinp,true);
            const auto dataset=p6g::g18c6_selected_one_hour_dataset(
                frame,records,p6g::source_setcom_nears(frame.grid,frame.stations),
                source_.original_metinp,true);
            const auto bytes=p6g::g18d1_validate_p6f_precommit(frame,dataset);
            if(bytes.empty()||dataset.hourly.size()!=45)
                throw std::logic_error("G34B native P6F candidate incomplete");
            output_.emplace(dataset);
        }
        ++cursor_;
        // Only this opt-in executable converts independently computed
        // selected kernels to a G14 transaction attempt, not production.
        return {p6::NativeHourSource::NativeCpp,true};
    }

    const cteas::p6f::Calmet21File& precommit(
        const p6g::MultiHourFrame& frame) const {
        if(cursor_!=17||!output_)
            throw std::logic_error("G34B missing native 17th stage or output");
        diagno_.require_station_clock_preflight(frame);
        return *output_;
    }

    bool publish_after_commit_noexcept() noexcept {
        return cursor_==17&&diagno_.try_publish_after_selected_g14_commit_noexcept();
    }
private:
    const p6g::G19FileBackedCase& source_;
    p6g::G26SourceFileBackedG14DiagnoBridge diagno_;
    p6g::G26BSourceFileBackedCompBridge comp_;
    std::size_t cursor_=0;
    std::optional<cteas::p6f::Calmet21File> output_;
};

int main(int argc,char** argv) {
    try {
        if(argc!=7)
            throw std::invalid_argument(
                "Usage: g34b <met.inp> <GEO.DAT> <SURF.DAT> <UP1.DAT> <UP2.DAT> <candidate-calmet.dat>");
        const p6g::G19SourceFilePaths paths{argv[1],argv[2],argv[3],{argv[4],argv[5]}};
        const auto source=p6g::g19_load_original_source_case(paths);
        auto scheduler=source.make_scheduler();
        if(source.stations.surface.size()!=2||source.stations.upper.size()!=2||
           source.grid.nx!=12||source.grid.ny!=12||scheduler.planned_hours()!=6)
            throw std::invalid_argument(
                "G34B only original G19 12x12 2SS/2US six-hour test case");
        p6g::G18D2StationClockAuthority clocks;
        std::optional<cteas::p6f::Calmet21File> six_hour;
        for(std::size_t hour=0;hour<6;++hour) {
            std::optional<cteas::p6f::Calmet21File> accepted;
            G34BSelectedOnlyHour* active=nullptr;
            bool published=false;
            scheduler.advance_with_fresh_physics(
                [&](const p6g::MultiHourFrame& live)
                    ->std::unique_ptr<p6g::MultiHourPhysics> {
                    if(live.hour_index!=hour||scheduler.completed_hours()!=hour)
                        throw std::logic_error("G34B duplicate/omitted source hour");
                    auto current=std::make_unique<G34BSelectedOnlyHour>(
                        scheduler,clocks,source);
                    active=current.get();
                    return current;
                },
                [&](const p6g::MultiHourFrame& frame) {
                    if(!active)
                        throw std::logic_error("G34B missing live native executor");
                    accepted=active->precommit(frame);
                    if(!accepted||accepted->hourly.size()!=45)
                        throw std::logic_error("G34B native P6F precommit missing");
                },
                [&](p6g::MultiHourPhysics&,const p6g::MultiHourFrame& frame,
                    const p6g::UnifiedMultistationHourlyScheduler& owner) {
                    if(!active||owner.completed_hours()!=hour||
                       frame.hour_index!=hour)
                        throw std::logic_error("G34B clock preflight detached");
                },
                [&](p6g::MultiHourPhysics& physics,
                    const p6g::UnifiedMultistationHourlyScheduler&) noexcept {
                    auto* native=dynamic_cast<G34BSelectedOnlyHour*>(&physics);
                    published=native&&native->publish_after_commit_noexcept();
                });
            if(!published||!accepted||scheduler.completed_hours()!=hour+1||
               !clocks.has_published_hour())
                throw std::runtime_error(
                    "G34B failed postcommit AA/BB/CGAMMA transaction");
            if(!six_hour) {
                six_hour=std::move(accepted);
            } else {
                auto& result=*six_hour;
                auto& next=*accepted;
                if(result.header.nx!=next.header.nx||
                   result.header.ny!=next.header.ny||
                   result.header.nssta!=next.header.nssta||
                   result.header.nusta!=next.header.nusta||
                   result.statics.zface_m!=next.statics.zface_m||
                   result.statics.nears!=next.statics.nears||
                   result.statics.elev_m!=next.statics.elev_m||
                   result.hourly.size()!=hour*45)
                    throw std::logic_error(
                        "G34B original static inventory drifted across hours");
                result.hourly.insert(result.hourly.end(),
                    std::make_move_iterator(next.hourly.begin()),
                    std::make_move_iterator(next.hourly.end()));
                result.header.irlg=static_cast<int>(hour+1);
                result.header.end_year=next.header.end_year;
                result.header.end_month=next.header.end_month;
                result.header.end_day=next.header.end_day;
                result.header.end_hour=next.header.end_hour;
                result.header.end_second=next.header.end_second;
            }
            std::cout<<"G34B_NATIVE_HOUR_COMMITTED="<<hour+1
                     <<" AA_BB_CGAMMA_CLOCKS=1\n";
        }
        scheduler.require_complete();
        if(!six_hour||six_hour->hourly.size()!=270||
           six_hour->header.irlg!=6)
            throw std::logic_error("G34B six-hour native output lacks 270 fields");
        const auto bytes=cteas::p6f::serialize_calmet_dat21(*six_hour);
        std::ofstream output(argv[6],std::ios::binary|std::ios::trunc);
        if(!output)throw std::runtime_error("G34B output file unavailable");
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        output.flush();
        if(!output)throw std::runtime_error("G34B native output truncated");
        std::cout<<"PASS_G34B_NATIVE_SIX_HOUR_TRANSACTION_TEST_ONLY "
                 <<"HOURS=6 FIELDS=270 STATIONS=2SS_2US "
                 <<"ORIGINAL_VENDOR_PARITY=NOT_YET_CALCULATED\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"BLOCKED_G34B_NATIVE_SIX_HOUR "<<error.what()<<"\n";
        return 2;
    }
}
