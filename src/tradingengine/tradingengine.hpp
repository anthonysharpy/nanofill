#pragma once

#include "events/event.hpp"
#include "tradingenginemarket.hpp"
#include "orderbook/orderbookconfig.hpp"
#include <cstdlib>

namespace nanofill::tradingengine {

using events::Event;
using events::EventType;
using orderbook::OrderBookMarketType;

class TradingEngine {
    inline static std::array<TradingEngineMarket, orderbook::MARKET_COUNT_SIZE_T> markets;  

public:
    [[gnu::always_inline]]
    static void process_event(const Event event) noexcept {
        markets[static_cast<std::size_t>(event.market)].process_event(event);
    }

    [[gnu::always_inline]]
    static void update_position(const OrderBookMarketType market) noexcept {
        markets[static_cast<std::size_t>(market)].update_position();
    }

    [[gnu::always_inline]]
    static void process_order_removed_event(const Event event) noexcept {
        markets[static_cast<std::size_t>(event.market)].process_order_removed_event(event);
    }

    [[gnu::always_inline]]
    static void process_order_added_event(const Event event) noexcept {
        markets[static_cast<std::size_t>(event.market)].process_order_added_event(event);
    }

    [[gnu::always_inline]]
    static std::uint64_t get_total_market_price(const OrderBookMarketType market) noexcept {
        return markets[static_cast<std::size_t>(market)].total_market_price;
    }

    [[gnu::always_inline]]
    static std::uint64_t get_market_shares(const OrderBookMarketType market) noexcept {
        return markets[static_cast<std::size_t>(market)].market_shares;
    }

    [[gnu::always_inline]]
    static std::uint32_t get_average_share_price(const OrderBookMarketType market) noexcept {
        return markets[static_cast<std::size_t>(market)].average_share_price;
    }

    [[gnu::always_inline]]
    static std::uint32_t get_target_buy_price(const OrderBookMarketType market) noexcept {
        return markets[static_cast<std::size_t>(market)].target_buy_price;
    }

    [[gnu::always_inline]]
    static std::uint32_t get_target_sell_price(const OrderBookMarketType market) noexcept {
        return markets[static_cast<std::size_t>(market)].target_sell_price;
    }

    [[gnu::always_inline]]
    static Event get_last_execution_order(const OrderBookMarketType market) noexcept {
        return markets[static_cast<std::size_t>(market)].last_execution_order;
    }

    [[gnu::always_inline]]
    static void reset_market(const OrderBookMarketType market) noexcept {
        markets[static_cast<size_t>(market)] = TradingEngineMarket();
    }
};

}