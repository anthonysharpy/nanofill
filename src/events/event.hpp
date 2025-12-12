#pragma once

#include <cstdint>
#include <vector>
#include <new>

namespace nanofill::events {

enum class EventType : std::uint8_t {
    // Submission of a new limit order.
    Submission = 1,
    // Partial deletion of a limit order (reduce quantity).
    Cancellation = 2,
    // Total deletion of a limit order.
    Deletion = 3,
    // Execution of a visible limit order.
    ExecutionVisible = 4,
    // Execution of a hidden limit order (original order not present in order book).
    ExecutionHidden = 5,
};

// Make our struct as small as possible as well as ordering it appropriately so that
// we can fit more into the CPU cache.
//
// Some of our data types are quite small, but they fit our data so they're fine for
// our purposes.

// A trading event.
struct Event {
    // Dollar price times 10,000.
    std::uint32_t price;
    // Seconds after midnight the event happened.
    std::uint32_t time;
    std::uint32_t order_id;
    // Number of shares. Negative means this is a sell order.
    std::int32_t size;
    EventType type;
};

std::vector<Event>
events_from_csv_data(const std::vector<std::tuple<double, std::uint8_t, std::uint32_t, std::uint16_t, std::uint32_t, std::int8_t>>& csv_data);

void print_event(Event event);

}