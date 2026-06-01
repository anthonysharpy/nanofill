#pragma once

#include <tuple>
#include <cstdint>

namespace nanofill::consts {

using TradingDataCSVFormat = std::tuple<double, std::uint8_t, std::uint32_t, std::uint32_t, std::uint32_t, std::int8_t>;
constexpr std::size_t SPSC_BUFFER_SIZE = 256;

}