#include "tradingengine.hpp"
#include "events/event.hpp"
#include <iostream>

namespace nanofill::tradingengine {

TradingEngine::TradingEngine(const int price_spread) noexcept {
    this->price_spread = price_spread;
}

void TradingEngine::process_order_removed_event(const Event event) noexcept {
    // A cancellation/deletion event will always contain the correct size, so we don't need
    // to look it up.

    auto abs_size = std::abs(event.size);
    total_market_price -= abs_size * event.price;
    market_shares -= abs_size;

    average_share_price = total_market_price / market_shares;

    if (event.type == EventType::ExecutionHidden || event.type == EventType::ExecutionVisible) {
        last_execution_order = event;
    }
}

void TradingEngine::process_order_added_event(const Event event) noexcept {
    auto abs_size = std::abs(event.size);
    total_market_price += abs_size * event.price;
    market_shares += abs_size;

    average_share_price = total_market_price / market_shares;
}

// The market value has changed. Update our position.
void TradingEngine::update_position() noexcept {
    if (price_spread > average_share_price) {
        target_buy_price = 0;
    } else {
        target_buy_price = average_share_price - price_spread;
    }

    target_sell_price = average_share_price + price_spread;
}

}