#pragma once
// P6G G14B: one file-backed source authority for multi-site CALMET hours.
// The original-format GEO.DAT / SURF.DAT / per-US UP.DAT plus met.inp's
// SSn/USn station roster enter the SAME 17-stage stateful scheduler.
// No synthetic observations, station reordering or original met.dat backfill.
#include "cteas/p6/p6g_unified_hourly.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cteas::calmet::p6g {
struct CaseInputPaths {
    std::string geo_file;
    std::string surface_file;
    std::vector<std::string> upper_files;
    std::string met_inp_file;
    GeoGridSpec expected_grid{};
    int surface_count=0,upper_count=0;
};

inline UnifiedMultistationHourlyScheduler load_unified_multistation_case(
    const CaseInputPaths& paths) {
    if(paths.geo_file.empty()||paths.surface_file.empty()||
       paths.met_inp_file.empty()||paths.surface_count<=0||
       paths.upper_count<=0||
       paths.upper_files.size()!=static_cast<std::size_t>(paths.upper_count))
        throw std::invalid_argument("P6G G14B requires full original-format case files");
    std::ifstream deck_file(paths.met_inp_file);
    if(!deck_file)
        throw std::runtime_error("P6G G14B met.inp unavailable");
    const std::string text((std::istreambuf_iterator<char>(deck_file)),
                           std::istreambuf_iterator<char>());
    if(deck_file.bad()||text.empty())
        throw std::runtime_error("P6G G14B met.inp unreadable");
    // GEO.DAT origin and met.inp station origin are taken from one
    // authoritative numerical-grid config, never geographic case centre.
    auto geo=read_geo_dynamic(paths.geo_file,paths.expected_grid);
    auto stations=original_calmet_multi_station_deck(
        text,geo.xorigin_km,geo.yorigin_km,
        paths.surface_count,paths.upper_count);
    std::vector<int> surface_ids,upper_ids;
    surface_ids.reserve(stations.surface.size());
    upper_ids.reserve(stations.upper.size());
    for(const auto& s:stations.surface)surface_ids.push_back(s.id);
    for(const auto& s:stations.upper)upper_ids.push_back(s.id);
    auto surface=read_surface_dynamic(paths.surface_file,surface_ids);
    auto upper=read_upper_station_set(paths.upper_files,upper_ids);
    return UnifiedMultistationHourlyScheduler(
        std::move(geo),std::move(stations),
        std::move(surface),std::move(upper));
}
} // namespace cteas::calmet::p6g
