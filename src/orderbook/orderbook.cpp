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
    levels_size[event.price] += event.size;
    levels_orders[event.price].push_back(event);
}

// Same as remove_order, except slightly faster because we already know the order's index.
[[gnu::always_inline]] inline
void OrderBook::remove_order_with_index(const Event event, const unsigned int index) noexcept {
    levels_last_modified[event.price] = event.time;
    levels_size[event.price] -= event.size;
    levels_orders[event.price][index] = levels_orders[event.price].back();
    levels_orders[event.price].pop_back();
}

// Remove an order from the order book. Prefer remove_order_with_index if possible.
// Returns true if an order was removed.
[[gnu::always_inline]] inline
bool OrderBook::remove_order(const Event event) noexcept {
    unsigned int index = get_order_index_by_price_and_id(event.price, event.order_id);
    
    if (index == UINT_MAX) {
        // Order not found.
        return false;
    }

    levels_last_modified[event.price] = event.time;
    levels_size[event.price] -= event.size;
    levels_orders[event.price][index] = levels_orders[event.price].back();
    levels_orders[event.price].pop_back();

    return true;
}

// We received a new order.
void OrderBook::process_submission_event(const Event event) noexcept {
    insert_order(event);
}

// Get the index of this order with the given price and id, or UINT_MAX if it doesn't exist.
[[gnu::always_inline]] inline
unsigned int
OrderBook::get_order_index_by_price_and_id(const std::uint32_t price, const std::uint32_t order_id) const noexcept {
    auto start = levels_orders[price].data();
    auto position = start;
    auto end = start + levels_orders[price].size();

    while (position != end) {
        if (position->order_id == order_id) {
            return static_cast<unsigned int>(position - start);
        }

        ++position;
    }

    return UINT_MAX;
}

// An order has had its quantity decreased by the given amount (partial cancellation).
// Returns true if actioned.
bool OrderBook::process_cancellation_event(const Event event) noexcept {
    unsigned int order_index = get_order_index_by_price_and_id(event.price, event.order_id);

    if (order_index == UINT_MAX) {
        return false;
    }

    Event old_event = levels_orders[event.price][order_index];

    remove_order_with_index(old_event, order_index);

    old_event.time = event.time;
    old_event.size -= event.size;

    insert_order(old_event);

    return true;
}

// An order has been entirely deleted.
bool OrderBook::process_deletion_event(const Event event) noexcept {
    return remove_order(event);
}

// An order we have on our order book has been executed.
bool OrderBook::process_visible_execution_event(const Event event) noexcept {
    return remove_order(event);
}

}