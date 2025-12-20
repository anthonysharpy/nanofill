#pragma once

#include <tuple>
#include <cstdint>

namespace nanofill::consts {

using TradingDataCSVFormat = std::tuple<double, std::uint8_t, std::uint32_t, std::uint16_t, std::uint32_t, std::int8_t>;

}