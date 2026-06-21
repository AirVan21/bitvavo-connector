#pragma once

#include <cstdint>
#include <future>
#include <string>
#include <vector>

namespace connectors {

struct MarketDataConnector {
    virtual ~MarketDataConnector() = default;

    virtual std::future<bool> Connect() = 0;
    virtual void Disconnect() = 0;
    virtual std::future<bool> SubscribeBBO(std::vector<int64_t> instrument_ids) = 0;
    virtual std::future<bool> SubscribeTrades(std::vector<int64_t> instrument_ids) = 0;
    virtual std::string Venue() const = 0;
};

} // namespace connectors
