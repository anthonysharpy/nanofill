#pragma once

#include <cstddef>

namespace nanofill::orderbook {

enum class OrderBookMarketType : std::uint32_t {
    MICROSOFT,
    NVIDIA,
    APPLE,
    GOOGLE,
    AIRBNB,
    OPENAI,
    UBER,
    SONY,
    TOSHIBA,
    MERCEDES,
    // For counting the number of markets.
    // 市場の数を数えるためだ。
    COUNT,
};

constexpr int MARKET_COUNT = static_cast<int>(OrderBookMarketType::COUNT);
constexpr size_t MARKET_COUNT_SIZE_T = static_cast<size_t>(OrderBookMarketType::COUNT);

}