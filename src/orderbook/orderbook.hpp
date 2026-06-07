#pragma once

#include "orderbookmarket.hpp"
#include "orderbookconfig.hpp"
#include <vector>
#include <thread>
#include <memory>

namespace nanofill::orderbook {

using events::Event;

class OrderBook {
    inline static std::array<OrderBookMarket, MARKET_COUNT_SIZE_T> markets;  

public:
    [[gnu::always_inline]]
    static bool process_event(const Event event) noexcept {
        return markets[static_cast<size_t>(event.market)].process_event(event);
    }

    [[gnu::always_inline]]
    static std::uint32_t
    get_last_modified_for_price(
        const OrderBookMarketType market,
        const std::uint32_t price
    ) noexcept {
        return markets[static_cast<size_t>(market)].levels_last_modified[price];
    }

    [[gnu::always_inline]]
    static std::uint32_t
    get_total_order_size_for_price(
        const OrderBookMarketType market,
        const std::uint32_t price
    ) noexcept {
        return markets[static_cast<size_t>(market)].levels_size[price];
    }

    [[gnu::always_inline]]
    static std::size_t
    get_order_count_for_price(
        const OrderBookMarketType market,
        const std::uint32_t price
    ) noexcept {
        return markets[static_cast<size_t>(market)].levels_orders[price].get_order_count();
    }

    [[gnu::always_inline]]
    static LevelPoolData*
    get_order_by_id_and_price(
        const OrderBookMarketType market,
        const std::uint32_t order_id,
        const std::uint32_t price
    ) noexcept {
        return markets[static_cast<size_t>(market)].levels_orders[price].get_order_data_by_order_id(order_id);
    }

    // Note that this currently causes a data race with the memory allocator thread.
    // This will be fixed later but for now this should only be used within tests.
    // 現在、これがメモリアロケータスレッドとデータ競合を起こす。後で直すつもりだが、とりあえず、
    // テストだけで使うこと。
    [[gnu::always_inline]]
    static void reset_market(const OrderBookMarketType market) noexcept {
        std::destroy_at(&markets[static_cast<size_t>(market)]);
        std::construct_at(&markets[static_cast<size_t>(market)]);
    }
};

}