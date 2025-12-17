#include <vector>
#include "gtest/gtest.h"
#include "events/event.hpp"

using nanofill::events::EventType;

TEST(Events, EventsFromCSVData) {
    std::vector<std::tuple<double, std::uint8_t, std::uint32_t, std::uint16_t, std::uint32_t, std::int8_t>> csv_data = {
        { 0, 1, 1001, 10, 25000, 1 },
        { 500, 2, 1002,  5, 24990, -1 },
        { 1000, 3, 1003, 20, 25010, 1 }
    };

    auto events = nanofill::events::events_from_csv_data(csv_data);

    ASSERT_EQ(csv_data.size(), 3U);

    ASSERT_EQ(events[0].order_id, 1001U);
    ASSERT_EQ(events[1].order_id, 1002U);
    ASSERT_EQ(events[2].order_id, 1003U);

    ASSERT_EQ(events[0].price, 25000U);
    ASSERT_EQ(events[1].price, 24990U);
    ASSERT_EQ(events[2].price, 25010U);

    ASSERT_EQ(events[0].size, 10);
    ASSERT_EQ(events[1].size, -5);
    ASSERT_EQ(events[2].size, 20);

    ASSERT_EQ(events[0].time, 0U);
    ASSERT_EQ(events[1].time, 500U);
    ASSERT_EQ(events[2].time, 1000U);

    ASSERT_EQ(events[0].type, EventType::Submission);
    ASSERT_EQ(events[1].type, EventType::Cancellation);
    ASSERT_EQ(events[2].type, EventType::Deletion);
}