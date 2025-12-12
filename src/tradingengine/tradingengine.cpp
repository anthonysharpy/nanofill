#include "tradingengine.hpp"
#include "events/event.hpp"
#include <iostream>

namespace nanofill::tradingengine {

using events::EventType;

TradingEngine::TradingEngine(const int price_spread) noexcept {
    this->price_spread = price_spread;
}

void TradingEngine::process_order_removed_event(const Event event) noexcept {
    if (event.size < 0) {
        total_market_price -= -event.size * event.price;
        market_shares -= -event.size;
    } else {
        total_market_price -= event.size * event.price;
        market_shares -= event.size;
    }

    average_share_price = total_market_price / market_shares;

    if (event.type == EventType::ExecutionHidden || event.type == EventType::ExecutionVisible) {
        last_execution_order = event;
    }
}

void TradingEngine::process_order_added_event(const Event event) noexcept {
    if (event.size < 0) {
        total_market_price += -event.size * event.price;
        market_shares += -event.size;
    } else {
        total_market_price += event.size * event.price;
        market_shares += event.size;
    }

    average_share_price = total_market_price / market_shares;
}

void TradingEngine::process_event(const Event event) noexcept {
    switch (event.type) {
        case EventType::Cancellation:
        case EventType::Deletion:
        case EventType::ExecutionVisible:
            process_order_removed_event(event);
            break;
        case EventType::Submission:
            process_order_added_event(event);
            break;
        case EventType::ExecutionHidden:
            // Processing would lead to strange results since this order was never recorded.
            return;
    }    

    update_position();
}

// The market value has changed. Update our position.
void TradingEngine::update_position() noexcept {
    target_buy_price = average_share_price - price_spread;
    target_sell_price = average_share_price + price_spread;
}

}