#pragma once

#include "events/event.hpp"
#include <climits>
#include <cstdlib>

namespace nanofill::orderbook {

using events::Event;
using events::EventType;

constexpr std::size_t order_book_size = 500000;

// A trading event.
// 取引のイベント。
struct OrderBookEntry {
    // Dollar price times 10,000.
    // 10,000倍したドラの価格。
    std::uint32_t price;
    // Seconds after midnight the event happened.
    // イベントが発生したときからの零時から数秒。
    std::uint32_t time;
    std::uint32_t order_id;
    // 株の数。ネガチブなら、これは売り注文だ。
    // Number of shares. Negative means this is a sell order.
    std::int32_t size;
};

// Note that this order book only supports one stock index (in our data - Microsoft).
// For an order book that supports multiple, the choices here would probably be a lot
// different (e.g. maybe stronger emphasis on rationing memory).
// この注文板は一つの株価指数に対応している（Microsoft）。
// 複数の株価指数に対応するために、ここでされている選択が全く違うほうがいいかもしれない（例えば、メモリ
// の割り当ての制限を重視する）。
class OrderBook {
public:
    OrderBook() noexcept;

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
    const std::vector<OrderBookEntry>& get_orders_for_price(const std::uint32_t price) const noexcept {
        return levels_orders[price];
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
    std::vector<std::vector<OrderBookEntry>> levels_orders;

    void insert_order(const Event event) noexcept;
    bool process_cancellation_event(const Event event) noexcept;
    void process_hidden_execution_event(const Event event) noexcept;
    void process_trading_halted_event(const Event event) noexcept;

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

    // Same as remove_order, except slightly faster because we already know the order's index.
    // remove_orderと同じだが、注文のインデックスがもう分かるので、もう少し速い。
    [[gnu::always_inline]]
    void remove_order_with_index(const Event event, const unsigned int index) noexcept {
        levels_last_modified[event.price] = event.time;
        levels_size[event.price] -= std::abs(event.size);
        levels_orders[event.price][index] = levels_orders[event.price].back();
        levels_orders[event.price].pop_back();
    }

    // Remove an order from the order book. Prefer remove_order_with_index if possible.
    // Returns true if an order was removed.
    // 板から注文を削除する。できれば、remove_order_with_indexを使って。注文を削除できたら、trueを
    // 返す。
    [[gnu::always_inline]]
    bool remove_order(const Event event) noexcept {
        auto entry = get_order_by_price_and_id(event.price, event.order_id);
        
        if (entry == nullptr) {
            // Order not found.
            return false;
        }

        levels_last_modified[event.price] = event.time;
        levels_size[event.price] -= std::abs(entry->size);
        *entry = levels_orders[event.price].back();
        levels_orders[event.price].pop_back();

        return true;
    }

    // Get a pointer to the order with the given price and id, or nullptr if it doesn't exist.
    // この価格とIDがある注文のポインタを返す。ないと、nullptrを返す。
    [[gnu::always_inline]]
    OrderBookEntry* get_order_by_price_and_id(const std::uint32_t price, const std::uint32_t order_id) noexcept {
        auto start = &levels_orders[price][0];
        auto position = start;
        auto end = start + levels_orders[price].size();

        while (position != end) {
            if (position->order_id == order_id) {
                return position;
            }

            ++position;
        }

        return nullptr;
    }
};

}
