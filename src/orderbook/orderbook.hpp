#pragma once

#include "events/event.hpp"
#include <climits>

namespace nanofill::orderbook {

using events::Event;
using events::EventType;

constexpr std::size_t order_book_size = 500000;

// A trading event.
struct OrderBookEntry {
    // Dollar price times 10,000.
    std::uint32_t price;
    // Seconds after midnight the event happened.
    std::uint32_t time;
    std::uint32_t order_id;
    // Number of shares. Negative means this is a sell order.
    std::int32_t size;
};

// Note that this order book only supports one stock index (in our data - Microsoft).
// For an order book that supports multiple, the choices here would probably be a lot
// different (e.g. maybe stronger emphasis on rationing memory).
class OrderBook {
public:
    OrderBook() noexcept;

    // Returns true if the event was actioned, false if not.
    [[gnu::always_inline]]
    bool process_event(const Event event) noexcept {
        // Try and order these from most to least common.
        switch (event.type) {
            case EventType::Submission:
                process_submission_event(event);
                return true;
            case EventType::Cancellation:
                return process_cancellation_event(event);
            case EventType::ExecutionVisible:
                return process_visible_execution_event(event);
            case EventType::Deletion:
                return process_deletion_event(event);
            case EventType::ExecutionHidden:
                // A hidden order was executed. This means we never had it in our order book,
                // and so there is no real order to process.
                return false;
        }

        return false;
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
    std::vector<OrderBookEntry> get_orders_for_price(const std::uint32_t price) const noexcept {
        return levels_orders[price];
    }
    
private:
    // Data for all order book levels. We'll store this in arrays instead of structs,
    // as this can give us better cache locality. Each index of each array will represent
    // each possible price.

    // The time of the last event on each level (according to the event).
    std::vector<double> levels_last_modified;
     // The number of shares on each level.
    std::vector<std::uint32_t> levels_size;
    // The orders on each level.
    std::vector<std::vector<OrderBookEntry>> levels_orders;

    void insert_order(const Event event) noexcept;
    bool process_cancellation_event(const Event event) noexcept;
    void process_hidden_execution_event(const Event event) noexcept;
    void process_trading_halted_event(const Event event) noexcept;

    // An order has been entirely deleted.
    [[gnu::always_inline]]
    bool process_deletion_event(const Event event) noexcept {
        return remove_order(event);
    }

    // An order we have on our order book has been executed.
    [[gnu::always_inline]]
    bool process_visible_execution_event(const Event event) noexcept {
        return remove_order(event);
    }

    // We received a new order.
    [[gnu::always_inline]]
    void process_submission_event(const Event event) noexcept {
        insert_order(event);
    }

    // Same as remove_order, except slightly faster because we already know the order's index.
    [[gnu::always_inline]]
    void remove_order_with_index(const Event event, const unsigned int index) noexcept {
        levels_last_modified[event.price] = event.time;
        levels_size[event.price] -= event.size;
        levels_orders[event.price][index] = levels_orders[event.price].back();
        levels_orders[event.price].pop_back();
    }

    // Remove an order from the order book. Prefer remove_order_with_index if possible.
    // Returns true if an order was removed.
    [[gnu::always_inline]]
    bool remove_order(const Event event) noexcept {
        unsigned int index = get_order_index_by_price_and_id(event.price, event.order_id);
        
        if (index == UINT_MAX) {
            // Order not found.
            return false;
        }

        levels_last_modified[event.price] = event.time;
        levels_size[event.price] -= levels_orders[event.price][index].size;
        levels_orders[event.price][index] = levels_orders[event.price].back();
        levels_orders[event.price].pop_back();

        return true;
    }

    // Get a pointer to the order with the given price and id, or nullptr if it doesn't exist.
    [[gnu::always_inline]]
    OrderBookEntry* get_order_by_price_and_id(const std::uint32_t price, const std::uint32_t order_id) noexcept {
        auto start = levels_orders[price].data();
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

    // Get the index of this order with the given price and id, or UINT_MAX if it doesn't exist.
    [[gnu::always_inline]]
    unsigned int
    get_order_index_by_price_and_id(const std::uint32_t price, const std::uint32_t order_id) const noexcept {
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
};

}
