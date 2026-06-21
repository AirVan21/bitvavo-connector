#include "connectors/Instrument.h"

namespace connectors {

InstrumentType InstrumentTypeFromString(const std::string& type) {
    if (type == "SPOT") return InstrumentType::Spot;
    if (type == "FUTURE") return InstrumentType::Future;
    if (type == "OPTION") return InstrumentType::Option;
    return InstrumentType::Spot;
}

std::string InstrumentTypeToString(InstrumentType type) {
    switch (type) {
        case InstrumentType::Spot: return "SPOT";
        case InstrumentType::Future: return "FUTURE";
        case InstrumentType::Option: return "OPTION";
    }
    return "SPOT";
}

} // namespace connectors
