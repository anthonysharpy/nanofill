#include "orderbook.hpp"
#include <stdexcept>

namespace nanofill::orderbook {

using events::Event;

OrderBook::OrderBook() noexcept {
    levels_orders.resize(order_book_size);
    levels_last_modified.resize(order_book_size);
    levels_size.resize(order_book_size);

    for (size_t i = 0; i < order_book_size; ++i) {
        levels_orders[i].reserve(50);
    }
}

// Insert an order into the order book.
void OrderBook::insert_order(const Event event) noexcept {
    levels_last_modified[event.price] = event.time;
    levels_size[event.price] += std::abs(event.size);
    levels_orders[event.price].push_back(event);
}

// An order has had its quantity decreased by the given amount (partial cancellation).
// Returns true if actioned.
bool OrderBook::process_cancellation_event(const Event event) noexcept {
    Event* current_event = get_order_by_price_and_id(event.price, event.order_id);

    if (current_event == nullptr) {
        return false;
    }

    levels_size[event.price] -= event.size;
    current_event->size -= event.size;
    levels_last_modified[event.price] = event.time;

    return true;
}

}