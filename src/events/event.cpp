#include "event.hpp"
#include "consts/consts.hpp"
#include <tuple>
#include <cstdint>
#include <iostream>

namespace nanofill::events {

// Print an event to the console for debugging.
// デバッギングのために、コンソールにイベントを出力する。
void print_event(const Event event) {
    std::cout << "orderid=" << event.order_id
        << "time=" << event.time
        << "price=" << event.price
        << "size=" << event.size
        << "type=" << static_cast<int>(event.type)
        << std::endl;
}

std::vector<Event>
events_from_csv_data(const std::vector<consts::TradingDataCSVFormat>& csv_data) {
    std::vector<Event> events;
    events.resize(csv_data.size());

    for (std::size_t i = 0; i < csv_data.size(); ++i) {
        const auto event_data = csv_data[i];

        events[i].time = std::get<0>(event_data);
        events[i].type = static_cast<EventType>(std::get<1>(event_data));
        events[i].order_id = std::get<2>(event_data);
        events[i].size = std::get<3>(event_data) * std::get<5>(event_data);
        events[i].price = std::get<4>(event_data);
    }

    return events;
}

}