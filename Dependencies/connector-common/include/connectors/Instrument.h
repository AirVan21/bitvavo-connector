#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace connectors {

/** @brief Classification of a financial instrument. */
enum class InstrumentType {
    Spot,
    Future,
    Option
};

/** @brief Sentinel expiry date used for non-expiring instruments (e.g. spot). */
constexpr std::string_view kNoExpiryDate = "1970-01-01";

/** @brief Exchange-agnostic description of a tradeable instrument. */
struct Instrument {
    int64_t instrument_id_ = 0;
    InstrumentType type_ = InstrumentType::Spot;
    std::string base_asset_;              /**< e.g. "BTC" */
    std::string quote_asset_;             /**< e.g. "EUR" */
    std::string settlement_currency_;
    std::string expiry_;                  /**< ISO 8601 date; kNoExpiryDate for spot instruments. */
    std::optional<std::string> cfi_code_; /**< ISO 10962 CFI code, if available. */
    std::string attributes_json_;         /**< Exchange-specific metadata as a JSON object. */
    std::string display_name_;            /**< Human-readable name, e.g. "BTC/EUR". */
};

/** @brief Maps a canonical Instrument to an exchange-specific trading symbol. */
struct Listing {
    int64_t listing_id_ = 0;
    int64_t instrument_id_ = 0;
    std::string venue_;           /**< Exchange identifier, e.g. "bitvavo". */
    std::string venue_symbol_;    /**< Exchange-specific symbol, e.g. "BTC-EUR". */
    std::string attributes_json_; /**< Exchange-specific listing metadata as a JSON object. */
    bool active_ = true;          /**< false if the listing has been delisted. */
};

InstrumentType InstrumentTypeFromString(const std::string& type);
std::string InstrumentTypeToString(InstrumentType type);

} // namespace connectors
