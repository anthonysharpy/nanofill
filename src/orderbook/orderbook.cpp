#include "orderbook.hpp"
#include <stdexcept>

namespace nanofill::orderbook {

using events::Event;

OrderBook::OrderBook() noexcept {
    levels_orders.resize(order_book_size);
    levels_last_modified.resize(order_book_size);
    levels_size.resize(order_book_size);

    for (size_t i = 0; i < order_book_size; ++i) {
        levels_orders[i].reserve(100);
    }
}

// Insert an order into the order book.
// 板に注文を入れる。
void OrderBook::insert_order(const Event event) noexcept {
    levels_last_modified[event.price] = event.time;
    levels_size[event.price] += std::abs(event.size);

    OrderBookEntry entry = {
        .price = event.price,
        .time = event.time,
        .order_id = event.order_id,
        .size = event.size
    };

    levels_orders[event.price].push_back(entry);
}

// An order has had its quantity decreased by the given amount (partial cancellation).
// Returns true if processed.
// 注文のサイズが減って。
// 処理したら、trueを返す。
bool OrderBook::process_cancellation_event(const Event event) noexcept {
    OrderBookEntry* current_event = get_order_by_price_and_id(event.price, event.order_id);

    if (current_event == nullptr) {
        return false;
    }

    levels_size[event.price] -= event.size;
    current_event->size -= event.size;
    levels_last_modified[event.price] = event.time;

    return true;
}

}