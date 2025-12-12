#pragma once

#include "events/event.hpp"
#include <climits>

namespace nanofill::orderbook {

using events::Event;
using events::EventType;

constexpr std::size_t order_book_size = 500000;

// Note that this order book only supports one stock index (in our data - Microsoft).
// For an order book that supports multiple, the choices here would probably be a lot
// different (e.g. maybe stronger emphasis on rationing memory).
class OrderBook {
public:
    OrderBook() noexcept;

    // Returns true if the event was actioned, false if not.
    [[gnu::always_inline]] inline
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
    
private:
    Event best_bid;
    Event best_ask;
    // Data for all order book levels. We'll store this in arrays instead of structs,
    // as this can give us better cache locality. Each index of each array will represent
    // each possible price.

    // The time of the last event on each level (according to the event).
    std::vector<double> levels_last_modified;
     // The number of shares on each level.
    std::vector<std::uint32_t> levels_size;
    // The orders on each level.
    std::vector<std::vector<Event>> levels_orders;

    void insert_order(const Event event) noexcept;
    bool remove_order(const Event event) noexcept;
    void remove_order_with_index(const Event event, const unsigned int index) noexcept;
    void process_submission_event(const Event event) noexcept;
    bool process_cancellation_event(const Event event) noexcept;
    bool process_deletion_event(const Event event) noexcept;
    bool process_visible_execution_event(const Event event) noexcept;
    void process_hidden_execution_event(const Event event) noexcept;
    void process_trading_halted_event(const Event event) noexcept;
    std::uint32_t get_order_index_by_price_and_id(const std::uint32_t price, const std::uint32_t order_id) const noexcept;
};

}
