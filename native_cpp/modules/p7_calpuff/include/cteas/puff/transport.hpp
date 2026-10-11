#pragma once
#include "cteas/puff/control.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
namespace cteas::puff {
// P7C: only dry emission + prescribed-wind transport. NO CALPUFF puff parity,
// plume rise, dispersion, deposition or concentration claim.
struct PointRelease {
    double x_m=0, y_m=0, z_m=0;
    std::vector<double> rate_per_s; // Original input mass units, not Bq.
};
struct PuffPacket {
    std::uint64_t id=0;
    double x_m=0, y_m=0, z_m=0;
    std::vector<double> mass;
    bool active=true;
};
struct WindField {
    int nx=0,ny=0,nz=0;
    double origin_x_m=0,origin_y_m=0,spacing_m=0;
    std::vector<double> level_m;
    // Native CALMET fields [z][j][i], i fastest.
    std::vector<float> u,v,w;
    void validate() const;
    bool contains(double x,double y,double z) const;
    std::vector<double> velocity(double x,double y,double z) const;
};
struct MassBalance {
    std::vector<double> released, inventory, outflow;
    std::size_t active=0,total=0;
};
class PuffTransport {
public:
    explicit PuffTransport(std::size_t species);
    void emit(const PointRelease&,double dt_seconds);
    void advect(const WindField&,double dt_seconds);
    [[nodiscard]] MassBalance balance() const;
    [[nodiscard]] const std::vector<PuffPacket>& packets() const {return packets_;}
private:
    std::size_t species_;
    std::uint64_t next_id_=1;
    std::vector<double> released_,outflow_;
    std::vector<PuffPacket> packets_;
};
[[nodiscard]] PointRelease point_source_from_control(const ControlFile&);
[[nodiscard]] std::vector<double> level_centers_from_zface(const std::vector<double>&);
} // namespace cteas::puff
