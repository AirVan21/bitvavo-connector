#include "connectors/InstrumentClient.h"

#include <ctime>
#include <iomanip>
#include <sstream>

#include <google/protobuf/util/json_util.h>
#include <grpcpp/grpcpp.h>

#include "instrument.grpc.pb.h"

namespace connectors {

namespace {

std::string TimestampToDateString(const google::protobuf::Timestamp& timestamp) {
    const std::time_t seconds = timestamp.seconds();
    std::tm tm;
    gmtime_r(&seconds, &tm);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
    return buffer;
}

std::string StructToJsonString(const google::protobuf::Struct& struct_value) {
    std::string json;
    google::protobuf::util::MessageToJsonString(struct_value, &json);
    return json;
}

Instrument ToInstrument(const instrument::v1::Instrument& proto) {
    Instrument instrument;
    instrument.instrument_id_ = proto.instrument_id();
    instrument.type_ = InstrumentTypeFromString(proto.type());
    instrument.base_asset_ = proto.base_asset();
    instrument.quote_asset_ = proto.quote_asset();
    instrument.settlement_currency_ = proto.settlement_currency();
    instrument.expiry_ = TimestampToDateString(proto.expiry());
    if (!proto.cfi_code().empty()) {
        instrument.cfi_code_ = proto.cfi_code();
    }
    instrument.attributes_json_ = StructToJsonString(proto.attributes());
    instrument.display_name_ = proto.display_name();
    return instrument;
}

Listing ToListing(const instrument::v1::Listing& proto) {
    Listing listing;
    listing.listing_id_ = proto.listing_id();
    listing.instrument_id_ = proto.instrument_id();
    listing.venue_ = proto.venue();
    listing.venue_symbol_ = proto.venue_symbol();
    listing.attributes_json_ = StructToJsonString(proto.attributes());
    listing.active_ = proto.active();
    return listing;
}

std::string CacheKey(const std::string& venue, int64_t instrument_id) {
    return venue + ":" + std::to_string(instrument_id);
}

std::string SymbolKey(const std::string& venue, const std::string& venue_symbol) {
    return venue + ":" + venue_symbol;
}

} // namespace

struct InstrumentClient::InstrumentClientImpl {
    explicit InstrumentClientImpl(const std::string& server_address)
        : stub_(instrument::v1::InstrumentService::NewStub(
              grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()))) {}

    std::unique_ptr<instrument::v1::InstrumentService::Stub> stub_;
};

InstrumentClient::InstrumentClient(std::string server_address)
    : server_address_(std::move(server_address)),
      impl_(std::make_unique<InstrumentClientImpl>(server_address_)) {}

InstrumentClient::~InstrumentClient() = default;

void InstrumentClient::CacheListing(const Listing& listing) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    listing_cache_[CacheKey(listing.venue_, listing.instrument_id_)] = listing;
    symbol_to_instrument_id_[SymbolKey(listing.venue_, listing.venue_symbol_)] =
        listing.instrument_id_;
}

std::optional<Instrument> InstrumentClient::GetInstrument(int64_t instrument_id) {
    instrument::v1::GetInstrumentRequest request;
    request.set_instrument_id(instrument_id);

    instrument::v1::Instrument response;
    grpc::ClientContext context;
    const grpc::Status status = impl_->stub_->GetInstrument(&context, request, &response);
    if (!status.ok()) {
        return std::nullopt;
    }
    return ToInstrument(response);
}

std::optional<Listing> InstrumentClient::GetListing(int64_t listing_id) {
    instrument::v1::GetListingRequest request;
    request.set_listing_id(listing_id);

    instrument::v1::Listing response;
    grpc::ClientContext context;
    const grpc::Status status = impl_->stub_->GetListing(&context, request, &response);
    if (!status.ok()) {
        return std::nullopt;
    }

    Listing listing = ToListing(response);
    CacheListing(listing);
    return listing;
}

std::optional<Listing> InstrumentClient::ResolveListing(int64_t instrument_id,
                                                         const std::string& venue) {
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        const auto key = CacheKey(venue, instrument_id);
        const auto it = listing_cache_.find(key);
        if (it != listing_cache_.end()) {
            return it->second;
        }
    }

    instrument::v1::ResolveListingRequest request;
    request.set_instrument_id(instrument_id);
    request.set_venue(venue);

    instrument::v1::Listing response;
    grpc::ClientContext context;
    const grpc::Status status = impl_->stub_->ResolveListing(&context, request, &response);
    if (!status.ok()) {
        return std::nullopt;
    }

    Listing listing = ToListing(response);
    CacheListing(listing);
    return listing;
}

std::vector<Listing> InstrumentClient::ListListings(int64_t instrument_id, bool active_only) {
    instrument::v1::ListListingsRequest request;
    request.set_instrument_id(instrument_id);
    request.set_active_only(active_only);

    instrument::v1::ListListingsResponse response;
    grpc::ClientContext context;
    const grpc::Status status = impl_->stub_->ListListings(&context, request, &response);
    if (!status.ok()) {
        return {};
    }

    std::vector<Listing> listings;
    listings.reserve(response.listings_size());
    for (const auto& proto_listing : response.listings()) {
        Listing listing = ToListing(proto_listing);
        CacheListing(listing);
        listings.push_back(std::move(listing));
    }
    return listings;
}

std::optional<int64_t> InstrumentClient::ResolveInstrumentId(const std::string& venue,
                                                              const std::string& venue_symbol) {
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        const auto key = SymbolKey(venue, venue_symbol);
        const auto it = symbol_to_instrument_id_.find(key);
        if (it != symbol_to_instrument_id_.end()) {
            return it->second;
        }
    }
    return std::nullopt;
}

} // namespace connectors
