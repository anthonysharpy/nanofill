#include "tradingengine.hpp"
#include "events/event.hpp"
#include <iostream>

namespace nanofill::tradingengine {

TradingEngine::TradingEngine(const int price_spread) noexcept {
    this->price_spread = price_spread;
}

void TradingEngine::process_order_removed_event(const Event event) noexcept {
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
    target_buy_price = average_share_price - price_spread;
    target_sell_price = average_share_price + price_spread;
}

}