#include "gtest/gtest.h"
#include "orderbook/orderbook.hpp"

using nanofill::events::Event;
using nanofill::events::EventType;

TEST(Orderbook, OrderbookProcessSubmissionEvent) {
    auto orderbook = nanofill::orderbook::OrderBook();

    Event event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .type = EventType::Submission
    };

    ASSERT_EQ(0U, orderbook.get_last_modified_for_price(10));
    ASSERT_EQ(0U, orderbook.get_orders_for_price(10).size());
    ASSERT_EQ(0U, orderbook.get_total_order_size_for_price(10));

    ASSERT_TRUE(orderbook.process_event(event));

    ASSERT_EQ(100U, orderbook.get_last_modified_for_price(10));
    ASSERT_EQ(10U, orderbook.get_total_order_size_for_price(10));

    auto orders = orderbook.get_orders_for_price(10);

    ASSERT_EQ(1U, orders.size());
    ASSERT_EQ(orders[0].order_id, 1000U);
    ASSERT_EQ(orders[0].price, 10U);
    ASSERT_EQ(orders[0].size, 10);
    ASSERT_EQ(orders[0].time, 100U);
    ASSERT_EQ(orders[0].type, EventType::Submission);

    Event event2 {
        .price = 10,
        .time = 105,
        .order_id = 1001,
        .size = -10,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(event2));

    ASSERT_EQ(105U, orderbook.get_last_modified_for_price(10));
    ASSERT_EQ(20U, orderbook.get_total_order_size_for_price(10));

    orders = orderbook.get_orders_for_price(10);

    ASSERT_EQ(2U, orders.size());
    ASSERT_EQ(orders[0].order_id, 1000U);
    ASSERT_EQ(orders[0].price, 10U);
    ASSERT_EQ(orders[0].size, 10);
    ASSERT_EQ(orders[0].time, 100U);
    ASSERT_EQ(orders[0].type, EventType::Submission);
    ASSERT_EQ(orders[1].order_id, 1001U);
    ASSERT_EQ(orders[1].price, 10U);
    ASSERT_EQ(orders[1].size, -10);
    ASSERT_EQ(orders[1].time, 105U);
    ASSERT_EQ(orders[1].type, EventType::Submission);
}

TEST(Orderbook, OrderbookProcessCancellationEvent) {
    auto orderbook = nanofill::orderbook::OrderBook();

    Event cancellation_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 3,
        .type = EventType::Cancellation
    };

    // Order doesn't exist, so nothing happens.
    ASSERT_FALSE(orderbook.process_event(cancellation_event));

    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(submission_event));
    ASSERT_TRUE(orderbook.process_event(cancellation_event));

    ASSERT_EQ(orderbook.get_last_modified_for_price(10), 105U);
    ASSERT_EQ(orderbook.get_total_order_size_for_price(10), 7U);

    auto orders = orderbook.get_orders_for_price(10);

    ASSERT_EQ(orders.size(), 1U);
    ASSERT_EQ(orders[0].order_id, 1000U);
    ASSERT_EQ(orders[0].price, 10U);
    ASSERT_EQ(orders[0].size, 7);
    ASSERT_EQ(orders[0].time, 100U);
    ASSERT_EQ(orders[0].type, EventType::Submission);
}

TEST(Orderbook, OrderbookProcesVisibleExecutionEvent) {
    auto orderbook = nanofill::orderbook::OrderBook();
}

TEST(Orderbook, OrderbookProcessDeletionEvent) {
    auto orderbook = nanofill::orderbook::OrderBook();
}