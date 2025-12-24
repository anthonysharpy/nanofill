#include "tradingengine.hpp"
#include "events/event.hpp"
#include <iostream>

namespace nanofill::tradingengine {

TradingEngine::TradingEngine(const int price_spread) noexcept {
    this->price_spread = price_spread;
}

}