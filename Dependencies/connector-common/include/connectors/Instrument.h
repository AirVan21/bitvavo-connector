#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace connectors {

enum class InstrumentType {
    Spot,
    Future,
    Option
};

constexpr std::string_view kNoExpiryDate = "1970-01-01";

struct Instrument {
    int64_t instrument_id_ = 0;
    InstrumentType type_ = InstrumentType::Spot;
    std::string base_asset_;
    std::string quote_asset_;
    std::string settlement_currency_;
    std::string expiry_;
    std::optional<std::string> cfi_code_;
    std::string attributes_json_;
    std::string display_name_;
};

struct Listing {
    int64_t listing_id_ = 0;
    int64_t instrument_id_ = 0;
    std::string venue_;
    std::string venue_symbol_;
    std::string attributes_json_;
    bool active_ = true;
};

InstrumentType InstrumentTypeFromString(const std::string& type);
std::string InstrumentTypeToString(InstrumentType type);

} // namespace connectors
