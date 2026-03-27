#pragma once

#include <tuple>
#include <cstdint>

namespace nanofill::consts {

using TradingDataCSVFormat = std::tuple<double, std::uint8_t, std::uint32_t, std::uint32_t, std::uint32_t, std::int8_t>;
constexpr unsigned int SPSC_BUFFER_SIZE = 1024;

}