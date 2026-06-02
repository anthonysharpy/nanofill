#include "gtest/gtest.h"
#include "orderbook/orderbookmarket.hpp"
#include "events/event.hpp"

using nanofill::events::Event;
using nanofill::events::EventType;
using nanofill::events::TradeDirection;
using nanofill::orderbook::OrderBookMarket;

TEST(OrderBook, ProcessSubmissionEvent) {
    auto orderbook = OrderBookMarket();

    Event event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_EQ(0U, orderbook.get_last_modified_for_price(10));
    ASSERT_EQ(0U, orderbook.get_order_count_for_price(10));
    ASSERT_EQ(0U, orderbook.get_total_order_size_for_price(10));

    ASSERT_TRUE(orderbook.process_event(event));

    ASSERT_EQ(100U, orderbook.get_last_modified_for_price(10));
    ASSERT_EQ(10U, orderbook.get_total_order_size_for_price(10));

    auto order = orderbook.get_order_by_id_and_price(1000, 10);

    ASSERT_EQ(orderbook.get_order_count_for_price(10), 1U);
    ASSERT_EQ(order->price, 10U);
    ASSERT_EQ(order->size, 10);
    ASSERT_EQ(order->time, 100U);

    Event event2 {
        .price = 10,
        .time = 105,
        .order_id = 1001,
        .size = 10,
        .direction = TradeDirection::Negative,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(event2));

    ASSERT_EQ(105U, orderbook.get_last_modified_for_price(10));
    ASSERT_EQ(20U, orderbook.get_total_order_size_for_price(10));

    auto order1 = orderbook.get_order_by_id_and_price(1000, 10);
    auto order2 = orderbook.get_order_by_id_and_price(1001, 10);

    ASSERT_EQ(2U, orderbook.get_order_count_for_price(10));
    ASSERT_EQ(order1->price, 10U);
    ASSERT_EQ(order1->size, 10);
    ASSERT_EQ(order1->time, 100U);
    ASSERT_EQ(order2->price, 10U);
    ASSERT_EQ(order2->size, -10);
    ASSERT_EQ(order2->time, 105U);
}

TEST(OrderBook, ProcessBigSubmissionEvent) {
    auto orderbook = OrderBookMarket();

    Event event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 200000U,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(event));

    ASSERT_EQ(200000U, orderbook.get_total_order_size_for_price(10));

    auto order = orderbook.get_order_by_id_and_price(1000, 10);

    ASSERT_EQ(1U, orderbook.get_order_count_for_price(10));
    ASSERT_EQ(order->price, 10U);
    ASSERT_EQ(order->size, 200000);
    ASSERT_EQ(order->time, 100U);

    Event event2 {
        .price = 10,
        .time = 105,
        .order_id = 1001,
        .size = 200000U,
        .direction = TradeDirection::Negative,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(event2));

    ASSERT_EQ(105U, orderbook.get_last_modified_for_price(10));
    ASSERT_EQ(400000U, orderbook.get_total_order_size_for_price(10));

    auto order1 = orderbook.get_order_by_id_and_price(1000, 10);
    auto order2 = orderbook.get_order_by_id_and_price(1001, 10);

    ASSERT_EQ(2U, orderbook.get_order_count_for_price(10));
    ASSERT_EQ(order1->price, 10U);
    ASSERT_EQ(order1->size, 200000);
    ASSERT_EQ(order1->time, 100U);
    ASSERT_EQ(order2->price, 10U);
    ASSERT_EQ(order2->size, -200000);
    ASSERT_EQ(order2->time, 105U);
}

TEST(OrderBook, ProcessCancellationEvent) {
    auto orderbook = OrderBookMarket();

    Event cancellation_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 3,
        .direction = TradeDirection::Positive,
        .type = EventType::Cancellation
    };

    // Order doesn't exist, so nothing happens.
    ASSERT_FALSE(orderbook.process_event(cancellation_event));

    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(submission_event));
    ASSERT_TRUE(orderbook.process_event(cancellation_event));

    ASSERT_EQ(orderbook.get_last_modified_for_price(10), 105U);
    ASSERT_EQ(orderbook.get_total_order_size_for_price(10), 7U);

    auto order = orderbook.get_order_by_id_and_price(1000, 10);

    ASSERT_EQ(orderbook.get_order_count_for_price(10), 1U);
    ASSERT_EQ(order->price, 10U);
    ASSERT_EQ(order->size, 7);
    ASSERT_EQ(order->time, 100U);
}

TEST(OrderBook, ProcessCancellationEventChangesEventSizeCorrectly) {
    auto orderbook = OrderBookMarket();

    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(submission_event));

    auto order = orderbook.get_order_by_id_and_price(1000, 10);

    ASSERT_EQ(order->size, 10);

    Event cancellation_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 3,
        .direction = TradeDirection::Positive,
        .type = EventType::Cancellation
    };

    ASSERT_TRUE(orderbook.process_event(cancellation_event));

    order = orderbook.get_order_by_id_and_price(1000, 10);

    ASSERT_EQ(order->size, 7);

    // Sell event, make sure it's handled correctly.
    Event submission_event2 {
        .price = 10,
        .time = 100,
        .order_id = 1001,
        .size = 10,
        .direction = TradeDirection::Negative,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(submission_event2));

    order = orderbook.get_order_by_id_and_price(1001, 10);

    ASSERT_EQ(order->size, -10);

    Event cancellation_event2 {
        .price = 10,
        .time = 105,
        .order_id = 1001,
        .size = 3,
        .direction = TradeDirection::Negative,
        .type = EventType::Cancellation
    };

    ASSERT_TRUE(orderbook.process_event(cancellation_event2));

    order = orderbook.get_order_by_id_and_price(1001, 10);

    ASSERT_EQ(order->size, -7);
}

TEST(OrderBook, ProcessVisibleExecutionEvent) {
    auto orderbook = OrderBookMarket();

    Event execution_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 0,
        .direction = TradeDirection::Positive,
        .type = EventType::ExecutionVisible
    };

    // Order doesn't exist, so nothing happens.
    ASSERT_FALSE(orderbook.process_event(execution_event));

    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(submission_event));
    ASSERT_TRUE(orderbook.process_event(execution_event));

    ASSERT_EQ(orderbook.get_last_modified_for_price(10), 105U);
    ASSERT_EQ(orderbook.get_total_order_size_for_price(10), 0U);

    ASSERT_EQ(orderbook.get_order_count_for_price(10), 0U);
}

TEST(OrderBook, ProcessDeletionEvent) {
    auto orderbook = OrderBookMarket();

    Event deletion_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 0,
        .direction = TradeDirection::Positive,
        .type = EventType::Deletion
    };

    // Order doesn't exist, so nothing happens.
    ASSERT_FALSE(orderbook.process_event(deletion_event));

    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(orderbook.process_event(submission_event));
    ASSERT_TRUE(orderbook.process_event(deletion_event));

    ASSERT_EQ(orderbook.get_last_modified_for_price(10), 105U);
    ASSERT_EQ(orderbook.get_total_order_size_for_price(10), 0U);

    ASSERT_EQ(orderbook.get_order_count_for_price(10), 0U);
}