#include "cteas/puff/conc_file.hpp"
#include "cteas/puff/receptors.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

std::string read_text(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("P7E cannot read control file: " + path);
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

int receptor_mode(const std::string& path) {
    const auto text = read_text(path);
    const auto receptors = cteas::puff::receptors_from_control(text);
    std::cout << "{\n"
              << "  \"gate\": \"P7E_RECEPTOR_GEOMETRY\",\n"
              << "  \"control\": \"" << path << "\",\n"
              << "  \"gridded\": " << (receptors.gridded ? "true" : "false") << ",\n"
              << "  \"sampling_nx\": " << receptors.sampling_nx << ",\n"
              << "  \"sampling_ny\": " << receptors.sampling_ny << ",\n"
              << "  \"mesh_down\": " << receptors.mesh_down << ",\n"
              << "  \"spacing_km\": " << receptors.spacing_km << ",\n"
              << "  \"grid_receptors\": " << receptors.grid.size() << ",\n"
              << "  \"discrete_receptors\": " << receptors.discrete.size() << "\n"
              << "}\n";
    std::cout << "P7E_RECEPTOR_GEOMETRY=PASS GRIDDED=" << (receptors.gridded ? "true" : "false")
              << " SAMPLING_NX=" << receptors.sampling_nx << " SAMPLING_NY=" << receptors.sampling_ny
              << " MESHDN=" << receptors.mesh_down << " SPACING_KM=" << receptors.spacing_km
              << " GRID_RECEPTORS=" << receptors.grid.size()
              << " DISCRETE_RECEPTORS=" << receptors.discrete.size();
    if (!receptors.grid.empty()) {
        std::cout << " FIRST_X_KM=" << receptors.grid.front().x_km
                  << " FIRST_Y_KM=" << receptors.grid.front().y_km
                  << " LAST_X_KM=" << receptors.grid.back().x_km
                  << " LAST_Y_KM=" << receptors.grid.back().y_km;
    }
    std::cout << '\n';
    return 0;
}

int structure_mode(const std::string& path) {
    const auto summary = cteas::puff::summarize_conc_file(path);
    std::cout << "P7E_CONC_STRUCTURE=PASS RECORDS=" << summary.records << " BYTES=" << summary.bytes
              << " PERIOD_RECORDS=" << summary.period_records
              << " GRID_RECORDS=" << summary.grid_records
              << " SPECIES_FIELDS=" << summary.species_field_records
              << " OTHER=" << summary.unframed_records
              << " FIRST_BEGIN=" << summary.first_begin_date << " LAST_END=" << summary.last_end_date
              << '\n';
    for (const auto& entry : summary.species_field_counts) {
        std::cout << "P7E_CONC_SPECIES_FIELD label=\"" << entry.first << "\" count=" << entry.second
                  << '\n';
    }
    if (!summary.species_field_sizes.empty()) {
        std::size_t smallest = summary.species_field_sizes.front();
        std::size_t largest = summary.species_field_sizes.front();
        for (const auto size : summary.species_field_sizes) {
            smallest = std::min(smallest, size);
            largest = std::max(largest, size);
        }
        std::cout << "P7E_CONC_SPECIES_FIELD_SIZES min=" << smallest << " max=" << largest << '\n';
    }
    for (const auto& entry : summary.label_counts) {
        std::cout << "P7E_CONC_LABEL label=\"" << entry.first << "\" count=" << entry.second << '\n';
    }
    if (!summary.framed_sizes.empty()) {
        std::size_t smallest = summary.framed_sizes.front();
        std::size_t largest = summary.framed_sizes.front();
        for (const auto size : summary.framed_sizes) {
            smallest = std::min(smallest, size);
            largest = std::max(largest, size);
        }
        std::cout << "P7E_CONC_FRAMED_SIZES min=" << smallest << " max=" << largest << '\n';
    }
    return 0;
}

int matrix_mode() {
    // PFSAMP case matrix: icase xold yold xnew ynew xrec yrec sigma_y dist2
    std::size_t cases = 0;
    std::cout.setf(std::ios::scientific, std::ios::floatfield);
    std::cout.precision(12);
    while (true) {
        int icase = 0;
        double xold = 0.0, yold = 0.0, xnew = 0.0, ynew = 0.0;
        double xrec = 0.0, yrec = 0.0, sigma = 0.0, dist2 = 0.0;
        if (!(std::cin >> icase >> xold >> yold >> xnew >> ynew >> xrec >> yrec >> sigma >> dist2)) {
            if (std::cin.eof()) break;
            throw std::invalid_argument("malformed PFSAMP case line");
        }
        const auto integrals = cteas::puff::puff_sampling_integrals(xold, yold, xnew, ynew, xrec, yrec,
                                                                    sigma, dist2);
        std::cout << icase << ' ' << integrals.i1 << ' ' << integrals.i2 << '\n';
        ++cases;
    }
    std::cerr << "P7E_PFSAMP_PROBE=PASS CASES=" << cases << " NOT_A_MODEL_RUN=true\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--receptors") return receptor_mode(argv[2]);
        if (argc == 3 && std::string(argv[1]) == "--structure") return structure_mode(argv[2]);
        if (argc != 1) {
            std::cerr << "Usage: cteas_p7e_probe [--receptors CONTROL_FILE | --structure CONC.DAT]\n"
                         "Without arguments reads the PFSAMP case matrix on stdin:\n"
                         "  icase xold yold xnew ynew xrec yrec sigma_y dist2\n"
                         "and writes: icase xi1 xi2\n";
            return 2;
        }
        return matrix_mode();
    } catch (const std::exception& error) {
        std::cerr << "P7E_PROBE=FAIL: " << error.what() << '\n';
        return 1;
    }
}
