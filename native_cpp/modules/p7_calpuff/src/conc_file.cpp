#include "cteas/puff/conc_file.hpp"

#include "cteas/puff/fortran_records.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cteas::puff {
namespace {

void require_true(bool condition, const std::string& message) {
    if (!condition) throw std::invalid_argument("P7E conc file: " + message);
}

std::string padded(std::string value, std::size_t width, const std::string& what) {
    require_true(value.size() <= width, what + " is longer than " + std::to_string(width) + " characters");
    value.resize(width, ' ');
    return value;
}

void append_bytes(std::vector<std::uint8_t>& buffer, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

void append_int(std::vector<std::uint8_t>& buffer, std::int32_t value) {
    append_bytes(buffer, &value, sizeof(value));
}

void append_float(std::vector<std::uint8_t>& buffer, float value) {
    append_bytes(buffer, &value, sizeof(value));
}

// Sequential-unformatted framing: 4-byte marker, payload, 4-byte trailer.
void write_framed(std::ostream& out, const std::vector<std::uint8_t>& payload) {
    const std::int32_t marker = static_cast<std::int32_t>(payload.size());
    out.write(reinterpret_cast<const char*>(&marker), sizeof(marker));
    out.write(reinterpret_cast<const char*>(payload.data()),
              static_cast<std::streamsize>(payload.size()));
    out.write(reinterpret_cast<const char*>(&marker), sizeof(marker));
    require_true(bool(out), "failed to write a sequential-unformatted record");
}

bool valid_period(int year, int julian, int hour, int seconds) {
    return year >= 1900 && year <= 2100 && julian >= 1 && julian <= 366 && hour >= 0 && hour <= 23 &&
           seconds >= 0 && seconds <= 3600;
}

bool valid_date(int packed) {
    const int year = packed / 100000;
    const int day = (packed % 100000) / 100;
    const int hour = packed % 100;
    return year >= 1900 && year <= 2100 && day >= 1 && day <= 366 && hour >= 0 && hour <= 24;
}

} // namespace

struct ConcFileWriter::Impl {
    std::ofstream stream;
    bool closed = false;
};

ConcFileWriter::ConcFileWriter(const std::string& path, std::string data_version,
                               std::string model_version, std::vector<std::string> comments)
    : impl_(new Impl()) {
    impl_->stream.open(path, std::ios::binary | std::ios::trunc);
    require_true(bool(impl_->stream), "cannot create " + path);

    // Version record: 'CONC.DAT' label plus data and model version strings, the
    // same shape and size (96 bytes) as the vendor WROUT1 version record.
    std::vector<std::uint8_t> version;
    const std::string label = padded("CONC.DAT", 16U, "concentration file label");
    append_bytes(version, label.data(), label.size());
    const std::string data = padded(std::move(data_version), 16U, "data version");
    append_bytes(version, data.data(), data.size());
    const std::string model = padded(std::move(model_version), 64U, "model version");
    append_bytes(version, model.data(), model.size());
    require_true(version.size() == 96U, "version record must be 96 bytes");
    write_framed(impl_->stream, version);

    std::vector<std::uint8_t> count;
    append_int(count, static_cast<std::int32_t>(comments.size()));
    write_framed(impl_->stream, count);

    for (const auto& comment : comments) {
        std::vector<std::uint8_t> line;
        const std::string text = padded(comment, 132U, "comment record");
        append_bytes(line, text.data(), text.size());
        write_framed(impl_->stream, line);
    }
}

ConcFileWriter::~ConcFileWriter() {
    if (impl_ != nullptr) {
        if (!impl_->closed) impl_->stream.flush();
        delete impl_;
    }
}

void ConcFileWriter::write_record(const ConcRecord& record) {
    require_true(impl_ != nullptr && !impl_->closed, "writer is closed");
    validate_conc_record(record);
    std::vector<std::uint8_t> payload;
    const std::string label = padded(record.label, 8U, "record label");
    append_bytes(payload, label.data(), label.size());
    append_int(payload, record.begin_date);
    append_int(payload, record.begin_seconds);
    append_int(payload, record.end_date);
    append_int(payload, record.end_seconds);
    payload.reserve(payload.size() + record.values.size() * sizeof(float));
    for (const float value : record.values) append_float(payload, value);
    write_framed(impl_->stream, payload);
    ++records_written_;
}

void ConcFileWriter::write_period_record(const ConcPeriod& period) {
    require_true(impl_ != nullptr && !impl_->closed, "writer is closed");
    const auto valid = [](int year, int julian, int hour, int seconds) {
        return year >= 1900 && year <= 2100 && julian >= 1 && julian <= 366 && hour >= 0 &&
               hour <= 23 && seconds >= 0 && seconds <= 3600;
    };
    require_true(valid(period.begin_year, period.begin_julian, period.begin_hour, period.begin_seconds),
                 "invalid begin period");
    require_true(valid(period.end_year, period.end_julian, period.end_hour, period.end_seconds),
                 "invalid end period");
    std::vector<std::uint8_t> payload;
    for (const int value : {period.begin_year, period.begin_julian, period.begin_hour,
                            period.begin_seconds, period.end_year, period.end_julian,
                            period.end_hour, period.end_seconds}) {
        append_int(payload, value);
    }
    write_framed(impl_->stream, payload);
    ++records_written_;
}

ConcGridDescriptor original_met_grid_descriptor(
    float origin_x_m, float origin_y_m, int discrete_receptors) {
    require_true(std::isfinite(origin_x_m) && std::isfinite(origin_y_m),
                 "original MET.DAT grid origin must be finite");
    require_true(discrete_receptors >= 0,
                 "original discrete receptor count must not be negative");
    constexpr float kOriginalMetresToKm = 0.001F;
    ConcGridDescriptor result;
    result.discrete_receptors = discrete_receptors;
    result.group_index = 1;
    result.group_name = "TOTAL";
    // Preserve original REAL*4 multiply, not double or text conversion.
    result.x_origin_km = static_cast<double>(origin_x_m * kOriginalMetresToKm);
    result.y_origin_km = static_cast<double>(origin_y_m * kOriginalMetresToKm);
    return result;
}

ConcSourceDescriptor original_total_source_descriptor(float origin_x_m,float origin_y_m) {
    const auto legacy=original_met_grid_descriptor(origin_x_m,origin_y_m);
    return {0,1,"TOTAL",legacy.x_origin_km,legacy.y_origin_km};
}

void ConcFileWriter::write_source_descriptor(const ConcSourceDescriptor& source) {
    require_true(impl_!=nullptr && !impl_->closed,"writer is closed");
    require_true(source.source_type>=0 && source.source_type<=8,
                 "invalid OUTPUT source KTYPE");
    require_true(source.source_index>=1,
                 "invalid OUTPUT source KSOURCE");
    require_true(!source.source_name.empty() && source.source_name.size()<=16U,
                 "invalid OUTPUT source CSRCNAM");
    require_true(std::isfinite(source.x_map_km) && std::isfinite(source.y_map_km),
                 "invalid OUTPUT source XY coordinates");
    if(source.source_type==0)
        require_true(source.source_index==1 && source.source_name=="TOTAL",
                     "TOTAL must have KTYPE=0 KSOURCE=1 name TOTAL");
    std::vector<std::uint8_t> payload;
    append_int(payload,source.source_type);
    append_int(payload,source.source_index);
    const auto name=padded(source.source_name,16U,"source name");
    append_bytes(payload,name.data(),name.size());
    append_float(payload,static_cast<float>(source.x_map_km));
    append_float(payload,static_cast<float>(source.y_map_km));
    require_true(payload.size()==32U,"OUTPUT source descriptor must be 32 bytes");
    write_framed(impl_->stream,payload);
    ++records_written_;
}

void ConcFileWriter::write_grid_descriptor(const ConcGridDescriptor& descriptor) {
    // Backward-compatible TOTAL-only alias. These fields NEVER describe
    // receptor grid dimensions in vendor OUTPUT L57912-57913.
    require_true(descriptor.discrete_receptors==0 &&
                 descriptor.group_index==1 && descriptor.group_name=="TOTAL",
                 "legacy TOTAL descriptor cannot represent individual source");
    write_source_descriptor({0,1,"TOTAL",
        descriptor.x_origin_km,descriptor.y_origin_km});
}

void ConcFileWriter::write_species_field(const std::string& label, int species_index, int nx, int ny,
                                        const std::vector<float>& values) {
    require_true(impl_ != nullptr && !impl_->closed, "writer is closed");
    require_true(species_index >= 1, "species index must be at least 1");
    require_true(nx > 0 && ny > 0, "field dimensions must be positive");
    const auto expected = static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny);
    require_true(values.size() == expected,
                 "field holds " + std::to_string(values.size()) + " values, expected " +
                     std::to_string(expected));
    for (const float value : values) {
        require_true(std::isfinite(value), "field values must be finite");
    }
    std::vector<std::uint8_t> payload;
    const std::string name = padded(label, 8U, "species label");
    append_bytes(payload, name.data(), name.size());
    // Original WROUT1 writes an I7 numeric field, right-justified with
    // LEADING spaces. Padded("1",7) used to left-justify it ("1      "),
    // which escaped structure-only CONC.DAT tests but changes the bytes.
    std::string index = std::to_string(species_index);
    require_true(index.size() <= 7U, "species index exceeds original I7 field");
    index.insert(0U, 7U - index.size(), ' ');
    append_bytes(payload, index.data(), index.size());
    payload.reserve(payload.size() + values.size() * sizeof(float));
    for (const float value : values) append_float(payload, value);
    write_framed(impl_->stream, payload);
    ++records_written_;
}

void ConcFileWriter::close() {
    if (impl_ == nullptr || impl_->closed) return;
    impl_->stream.close();
    require_true(!impl_->stream.fail(), "failed to close the concentration file");
    impl_->closed = true;
}

void validate_conc_record(const ConcRecord& record) {
    require_true(!record.label.empty() && record.label.size() <= 8U,
                 "record label must be 1 to 8 characters");
    require_true(valid_date(record.begin_date), "invalid begin date YYYYJJJHH");
    require_true(valid_date(record.end_date), "invalid end date YYYYJJJHH");
    require_true(record.begin_seconds >= 0 && record.begin_seconds <= 3600,
                 "begin seconds must be in 0..3600");
    require_true(record.end_seconds >= 0 && record.end_seconds <= 3600,
                 "end seconds must be in 0..3600");
    require_true(record.end_date > record.begin_date ||
                     (record.end_date == record.begin_date &&
                      record.end_seconds >= record.begin_seconds),
                 "record end time must not precede its begin time");
    for (const float value : record.values) {
        require_true(std::isfinite(value), "record values must be finite");
    }
}

void validate_conc_field(const ConcRecord& record, int nx, int ny) {
    require_true(nx > 0 && ny > 0, "field dimensions must be positive");
    validate_conc_record(record);
    const auto expected = static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny);
    require_true(record.values.size() == expected,
                 "field record holds " + std::to_string(record.values.size()) + " values, expected " +
                     std::to_string(expected));
}

ConcFileSummary summarize_conc_file(const std::string& path) {
    FortranRecordReader reader(path, 4, RecordByteOrder::LittleEndian);
    ConcFileSummary summary;
    std::vector<std::uint8_t> record;
    bool first_period = true;
    while (reader.next(record)) {
        ++summary.records;
        summary.bytes += record.size() + 8U;
        summary.record_sizes.push_back(record.size());
        const auto int_at = [&record](std::size_t offset) {
            std::int32_t value = 0;
            std::memcpy(&value, record.data() + offset, sizeof(value));
            return value;
        };
        if (record.size() == 32U) {
            // (year, julian, hour, seconds) x2 = the sampling period record, or
            // (nrec, group index, 'TOTAL', XORIGKM, YORIGKM) = the grid descriptor.
            const bool period = valid_period(int_at(0), int_at(4), int_at(8), int_at(12)) &&
                                valid_period(int_at(16), int_at(20), int_at(24), int_at(28));
            if (period) {
                ++summary.period_records;
                if (first_period) {
                    summary.first_begin_date = int_at(0) * 1000 + int_at(4);
                    first_period = false;
                }
                summary.last_end_date = int_at(16) * 1000 + int_at(20);
                continue;
            }
            std::string name(reinterpret_cast<const char*>(record.data() + 8U), 16U);
            if (name.rfind("TOTAL", 0U) == 0U) {
                ++summary.grid_records;
                continue;
            }
        }
        if (record.size() >= 15U) {
            const std::string label(reinterpret_cast<const char*>(record.data()), 8U);
            const std::size_t values = record.size() - 15U;
            if (values > 0U && values % sizeof(float) == 0U && !label.empty() &&
                label.find_first_not_of(" ") != std::string::npos) {
                ++summary.species_field_records;
                summary.species_field_sizes.push_back(record.size());
                const auto found = std::find_if(
                    summary.species_field_counts.begin(), summary.species_field_counts.end(),
                    [&label](const auto& entry) { return entry.first == label; });
                if (found == summary.species_field_counts.end()) {
                    summary.species_field_counts.emplace_back(label, 1U);
                } else {
                    ++found->second;
                }
                continue;
            }
        }
        ++summary.unframed_records;
        const std::string key = record.size() >= 8U
                                    ? std::string(reinterpret_cast<const char*>(record.data()), 8U)
                                    : std::string("<short>");
        const auto found = std::find_if(summary.label_counts.begin(), summary.label_counts.end(),
                                        [&key](const auto& entry) { return entry.first == key; });
        if (found == summary.label_counts.end()) {
            summary.label_counts.emplace_back(key, 1U);
        } else {
            ++found->second;
        }
    }
    require_true(summary.records > 0U, "no records found in " + path);
    return summary;
}

std::vector<ConcRecord> read_conc_records(const std::string& path) {
    FortranRecordReader reader(path, 4, RecordByteOrder::LittleEndian);
    std::vector<ConcRecord> records;
    std::vector<std::uint8_t> record;
    while (reader.next(record)) {
        if (record.size() < 24U) continue;
        ConcRecord parsed;
        parsed.label.assign(reinterpret_cast<const char*>(record.data()), 8U);
        std::int32_t begin = 0, begin_seconds = 0, end = 0, end_seconds = 0;
        std::memcpy(&begin, record.data() + 8U, sizeof(begin));
        std::memcpy(&begin_seconds, record.data() + 12U, sizeof(begin_seconds));
        std::memcpy(&end, record.data() + 16U, sizeof(end));
        std::memcpy(&end_seconds, record.data() + 20U, sizeof(end_seconds));
        if (!valid_date(begin) || !valid_date(end)) continue;
        parsed.begin_date = begin;
        parsed.begin_seconds = begin_seconds;
        parsed.end_date = end;
        parsed.end_seconds = end_seconds;
        const std::size_t values = (record.size() - 24U) / sizeof(float);
        parsed.values.resize(values);
        if (values > 0U) {
            std::memcpy(parsed.values.data(), record.data() + 24U, values * sizeof(float));
        }
        records.push_back(std::move(parsed));
    }
    require_true(!records.empty(), "no framed records found in " + path);
    return records;
}

std::vector<ConcPeriod> read_conc_periods(const std::string& path) {
    FortranRecordReader reader(path, 4, RecordByteOrder::LittleEndian);
    std::vector<ConcPeriod> periods;
    std::vector<std::uint8_t> record;
    while (reader.next(record)) {
        if (record.size() != 32U) continue;
        const auto int_at = [&record](std::size_t offset) {
            std::int32_t value = 0;
            std::memcpy(&value, record.data() + offset, sizeof(value));
            return value;
        };
        if (!valid_period(int_at(0), int_at(4), int_at(8), int_at(12)) ||
            !valid_period(int_at(16), int_at(20), int_at(24), int_at(28))) {
            continue;
        }
        ConcPeriod period;
        period.begin_year = int_at(0);
        period.begin_julian = int_at(4);
        period.begin_hour = int_at(8);
        period.begin_seconds = int_at(12);
        period.end_year = int_at(16);
        period.end_julian = int_at(20);
        period.end_hour = int_at(24);
        period.end_seconds = int_at(28);
        periods.push_back(period);
    }
    require_true(!periods.empty(), "no sampling period records found in " + path);
    return periods;
}

std::vector<ConcSpeciesField> read_conc_species_fields(const std::string& path, int nx, int ny) {
    require_true(nx > 0 && ny > 0, "expected field dimensions must be positive");
    const auto expected = static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny);
    FortranRecordReader reader(path, 4, RecordByteOrder::LittleEndian);
    std::vector<ConcSpeciesField> fields;
    std::vector<std::uint8_t> record;
    while (reader.next(record)) {
        if (record.size() != 15U + expected * sizeof(float)) continue;
        ConcSpeciesField field;
        field.label.assign(reinterpret_cast<const char*>(record.data()), 8U);
        std::string index(reinterpret_cast<const char*>(record.data() + 8U), 7U);
        const auto first = index.find_first_not_of(" \t");
        require_true(first != std::string::npos, "species field index is blank");
        const auto last = index.find_last_not_of(" \t");
        const std::string digits = index.substr(first, last - first + 1);
        int parsed = 0;
        const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), parsed);
        require_true(end == digits.data() + digits.size() && error == std::errc{},
                     "species field index is not an integer: " + digits);
        field.species_index = parsed;
        field.values.resize(expected);
        std::memcpy(field.values.data(), record.data() + 15U, expected * sizeof(float));
        fields.push_back(std::move(field));
    }
    require_true(!fields.empty(), "no species field records of the expected size found in " + path);
    return fields;
}

} // namespace cteas::puff
