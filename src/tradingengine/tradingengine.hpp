#pragma once

#include "events/event.hpp"

// Most of this code is really just an example since this will be mostly business logic.
// This currently only supports one market index.
namespace nanofill::tradingengine {

using events::Event;

class TradingEngine {
    // The last execution order we've seen.
    Event last_execution_order;
    // Sum of the value of all orders in 10,000ths of a dollar. This includes both sell and buy orders.
    // E.g. $100 sell and $100 buy would be $200 (2,000,000).
    std::uint64_t total_market_price = 0;
    // Number of shares wanting to be bought or sold.
    std::uint64_t market_shares = 0;
    // The max order value that the system can make at once, measured in 10,000ths of a dollar.
    std::uint64_t max_order_value = 0;
    // Current average share price in 10,000ths of a dollar.
    std::uint32_t average_share_price = 0;
    // The price the engine wants to buy at.
    std::uint32_t target_buy_price = 0;
    // The price the engine wants to sell at.
    std::uint32_t target_sell_price = 0;
    // The distance from the average market price that we are willing to buy/sell at.
    std::uint32_t price_spread = 0;

    void update_position() noexcept;
    void generate_intent(const std::int32_t amount, const std::uint32_t price) noexcept;
    void cancel_all_orders() noexcept;

public:
    TradingEngine(const int price_spread) noexcept;
    void process_event(const Event event) noexcept;
    void process_order_removed_event(const Event event) noexcept;
    void process_order_added_event(const Event event) noexcept;
};

}