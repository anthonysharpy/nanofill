#pragma once

#include "events/event.hpp"
#include "orderbookleveldatapool.hpp"
#include "concurrency/concurrency.hpp"
#include <climits>
#include <cstdlib>
#include <thread>

namespace nanofill::orderbook {

using events::Event;
using events::EventType;

constexpr std::size_t order_book_size = 500000;

// Note that this order book only supports one stock index (in our data - Microsoft).
// For an order book that supports multiple, the choices here would probably be a lot
// different (e.g. maybe stronger emphasis on rationing memory).
// この注文板は一つの株価指数に対応している（Microsoft）。
// 複数の株価指数に対応するために、ここでされている選択が全く違うほうがいいかもしれない（例えば、メモリ
// の割り当ての制限を重視する）。
class OrderBook {
public:
    OrderBook() noexcept {
        levels_orders.resize(order_book_size);

        // Each level will get two pools by default. This provides us an opportunity for later
        // optimisations in the hot loop because all we have to do is check if .growth_requested
        // is set to false.
        //
        // As an optimisation, we'll get this main thread to do initial_grow(), so that the cache
        // is warm on this core.
        // 各レベルはデフォルトで２つのプールをもらう。そうすると、後でホットループで、.growth_requestedがfalse
        // かどうかをチェックするだけで、最適化の機会になる。
        //
        // 最適化として、このメインスレッドでキャッシュが温まるために、このスレッドにinitial_grow()を呼び出させる。
        for (auto& level : levels_orders) {
            level.initial_grow();
        }

        levels_last_modified.resize(order_book_size);
        levels_size.resize(order_book_size);

        memory_allocator_thread = std::jthread([](std::stop_token stop){
            concurrency::pin_thread_to_core(1);

            orderbook::OrderBookLevelDataPool* data[GROWTH_BUFFER_SIZE];

            while (!stop.stop_requested()) {
                auto count = orderbook::growth_buffer.pop_many(data, GROWTH_BUFFER_SIZE);

                for (std::size_t n = 0; n < count; n++) {
                    data[n]->grow();
                }
            }
        });
    }

    // Returns true if the event was actioned, false if not.
    // 処理したら、trueを返す。または、false。
    [[gnu::always_inline]]
    bool process_event(const Event event) noexcept {
        // Ordered from most to least common.
        // 多い順に並べっている。
        switch (event.type) {
            case EventType::Submission:
                process_submission_event(event);
                return true;
            case EventType::Deletion:
                return process_deletion_event(event);
            case EventType::ExecutionVisible:
                return process_visible_execution_event(event);
            case EventType::Cancellation:
                return process_cancellation_event(event);
            default:
            // Probably a hidden order was executed. This means we never had it in our order book,
            // and so there is no real order to process.
            // 多分隠した注文が実行した。つまり、板にないので、処理できる注文が実はない。
            return false;
        }
    }

    [[gnu::always_inline]]
    std::uint32_t get_last_modified_for_price(const std::uint32_t price) const noexcept {
        return levels_last_modified[price];
    }

    [[gnu::always_inline]]
    std::uint32_t get_total_order_size_for_price(const std::uint32_t price) const noexcept {
        return levels_size[price];
    }

    [[gnu::always_inline]]
    std::size_t get_order_count_for_price(const std::uint32_t price) noexcept {
        return levels_orders[price].get_order_count();
    }

    [[gnu::always_inline]]
    LevelPoolData* get_order_by_id_and_price(
        const std::uint32_t order_id,
        const std::uint32_t price
    ) noexcept {
        return levels_orders[price].get_order_data_by_order_id(order_id);
    }
    
private:
    // Data for all order book levels. We'll store this in vectors instead of structs,
    // as this can give us better cache locality. Each index of each array will represent
    // each possible price.
    // 注文板のレベルのデータ。structの代わりにvectorに格納することで、キャッシュ局所性を改善できる。
    // 各配列の各インデックスがすべての価格を表す。

    // The time of the last event on each level (according to the event).
    // 各レベルの最後のイベントの時（イベントによって）。
    std::vector<std::uint32_t> levels_last_modified;
    // The number of shares on each level.
    // 各レベルの株の数。
    std::vector<std::uint32_t> levels_size;
    // The orders on each level.
    // 各レベルの注文。
    std::vector<OrderBookLevelDataPool> levels_orders;

    std::jthread memory_allocator_thread;

    // An order has been entirely deleted.
    // 注文が完全に削除された。
    [[gnu::always_inline]]
    bool process_deletion_event(const Event event) noexcept {
        return remove_order(event);
    }

    // An order we have on our order book has been executed.
    // 板にある注文が実行された。
    [[gnu::always_inline]]
    bool process_visible_execution_event(const Event event) noexcept {
        return remove_order(event);
    }

    // We received a new order.
    // 新しい注文を受け取った。
    [[gnu::always_inline]]
    void process_submission_event(const Event event) noexcept {
        insert_order(event);
    }

    // Remove an order from the order book. Returns true if an order was removed.
    // 板から注文を削除する。注文を削除できたら、trueを返す。
    [[gnu::always_inline]]
    bool remove_order(const Event event) noexcept {
        auto size = levels_orders[event.price].remove_order_by_id(event.order_id);
        
        if (size == std::numeric_limits<decltype(size)>::max()) {
            // Order not found.
            return false;
        }

        levels_last_modified[event.price] = event.time;
        levels_size[event.price] -= size;

        return true;
    }

    // An order has had its quantity decreased by the given amount (partial cancellation).
    // Returns true if processed.
    // 注文のサイズが減って。
    // 処理したら、trueを返す。
    [[gnu::always_inline]]
    bool process_cancellation_event(const Event event) noexcept {
        auto size_ptr = levels_orders[event.price].get_event_size_by_order_id(event.order_id);

        if (size_ptr == nullptr) {
            return false;
        }

        levels_size[event.price] -= event.size;
        *size_ptr -= event.get_size_with_direction();
        levels_last_modified[event.price] = event.time;

        return true;
    }

    // Insert an order into the order book.
    // 板に注文を入れる。
    [[gnu::always_inline]]
    void insert_order(const Event event) noexcept {
        levels_last_modified[event.price] = event.time;
        levels_size[event.price] += event.size;

        levels_orders[event.price].push(event);
    }
};

}
