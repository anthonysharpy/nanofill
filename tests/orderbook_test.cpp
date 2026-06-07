#include "gtest/gtest.h"
#include "orderbook/orderbook.hpp"
#include "events/event.hpp"

using nanofill::events::Event;
using nanofill::events::EventType;
using nanofill::events::TradeDirection;
using nanofill::orderbook::OrderBookMarketType;
using nanofill::orderbook::OrderBook;

TEST(OrderBook, ProcessSubmissionEvent) {
    Event event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_EQ(0U, OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(0U, OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(0U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10));

    ASSERT_TRUE(OrderBook::process_event(event));

    ASSERT_EQ(100U, OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(10U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10));

    auto order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1000, 10);

    ASSERT_EQ(OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10), 1U);
    ASSERT_EQ(order->price, 10U);
    ASSERT_EQ(order->size, 10);
    ASSERT_EQ(order->time, 100U);

    Event event2 {
        .price = 10,
        .time = 105,
        .order_id = 1001,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Negative,
        .type = EventType::Submission
    };

    ASSERT_TRUE(OrderBook::process_event(event2));

    ASSERT_EQ(105U, OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(20U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10));

    auto order1 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1000, 10);
    auto order2 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1001, 10);

    ASSERT_EQ(2U, OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(order1->price, 10U);
    ASSERT_EQ(order1->size, 10);
    ASSERT_EQ(order1->time, 100U);
    ASSERT_EQ(order2->price, 10U);
    ASSERT_EQ(order2->size, -10);
    ASSERT_EQ(order2->time, 105U);

    OrderBook::reset_market(OrderBookMarketType::MICROSOFT);
}

TEST(OrderBook, ProcessBigSubmissionEvent) {
    Event event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 200000U,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(OrderBook::process_event(event));

    ASSERT_EQ(200000U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10));

    auto order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1000, 10);

    ASSERT_EQ(1U, OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(order->price, 10U);
    ASSERT_EQ(order->size, 200000);
    ASSERT_EQ(order->time, 100U);

    Event event2 {
        .price = 10,
        .time = 105,
        .order_id = 1001,
        .size = 200000U,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Negative,
        .type = EventType::Submission
    };

    ASSERT_TRUE(OrderBook::process_event(event2));

    ASSERT_EQ(105U, OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(400000U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10));

    auto order1 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1000, 10);
    auto order2 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1001, 10);

    ASSERT_EQ(2U, OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(order1->price, 10U);
    ASSERT_EQ(order1->size, 200000);
    ASSERT_EQ(order1->time, 100U);
    ASSERT_EQ(order2->price, 10U);
    ASSERT_EQ(order2->size, -200000);
    ASSERT_EQ(order2->time, 105U);

    OrderBook::reset_market(OrderBookMarketType::MICROSOFT);
}

TEST(OrderBook, ProcessCancellationEvent) {
    Event cancellation_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 3,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Cancellation
    };

    // Order doesn't exist, so nothing happens.
    ASSERT_FALSE(OrderBook::process_event(cancellation_event));

    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(OrderBook::process_event(submission_event));
    ASSERT_TRUE(OrderBook::process_event(cancellation_event));

    ASSERT_EQ(OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10), 105U);
    ASSERT_EQ(OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10), 7U);

    auto order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1000, 10);

    ASSERT_EQ(OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10), 1U);
    ASSERT_EQ(order->price, 10U);
    ASSERT_EQ(order->size, 7);
    ASSERT_EQ(order->time, 100U);

    OrderBook::reset_market(OrderBookMarketType::MICROSOFT);
}

TEST(OrderBook, ProcessCancellationEventChangesEventSizeCorrectly) {
    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(OrderBook::process_event(submission_event));

    auto order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1000, 10);

    ASSERT_EQ(order->size, 10);

    Event cancellation_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 3,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Cancellation
    };

    ASSERT_TRUE(OrderBook::process_event(cancellation_event));

    order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1000, 10);

    ASSERT_EQ(order->size, 7);

    // Sell event, make sure it's handled correctly.
    Event submission_event2 {
        .price = 10,
        .time = 100,
        .order_id = 1001,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Negative,
        .type = EventType::Submission
    };

    ASSERT_TRUE(OrderBook::process_event(submission_event2));

    order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1001, 10);

    ASSERT_EQ(order->size, -10);

    Event cancellation_event2 {
        .price = 10,
        .time = 105,
        .order_id = 1001,
        .size = 3,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Negative,
        .type = EventType::Cancellation
    };

    ASSERT_TRUE(OrderBook::process_event(cancellation_event2));

    order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1001, 10);

    ASSERT_EQ(order->size, -7);

    OrderBook::reset_market(OrderBookMarketType::MICROSOFT);
}

TEST(OrderBook, ProcessVisibleExecutionEvent) {
    Event execution_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 0,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::ExecutionVisible
    };

    // Order doesn't exist, so nothing happens.
    ASSERT_FALSE(OrderBook::process_event(execution_event));

    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(OrderBook::process_event(submission_event));
    ASSERT_TRUE(OrderBook::process_event(execution_event));

    ASSERT_EQ(OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10), 105U);
    ASSERT_EQ(OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10), 0U);

    ASSERT_EQ(OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10), 0U);
}

TEST(OrderBook, ProcessDeletionEvent) {
    Event deletion_event {
        .price = 10,
        .time = 105,
        .order_id = 1000,
        .size = 0,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Deletion
    };

    // Order doesn't exist, so nothing happens.
    ASSERT_FALSE(OrderBook::process_event(deletion_event));

    Event submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_TRUE(OrderBook::process_event(submission_event));
    ASSERT_TRUE(OrderBook::process_event(deletion_event));

    ASSERT_EQ(OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10), 105U);
    ASSERT_EQ(OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10), 0U);

    ASSERT_EQ(OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10), 0U);

    OrderBook::reset_market(OrderBookMarketType::MICROSOFT);
}

TEST(OrderBook, DifferentMarketsWork) {
    Event event1 {
        .price = 5,
        .time = 100,
        .order_id = 1000,
        .size = 8,
        .market = OrderBookMarketType::AIRBNB,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };
    Event event2 {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    // Airbnb.
    ASSERT_EQ(0U, OrderBook::get_last_modified_for_price(OrderBookMarketType::AIRBNB, 5));
    ASSERT_EQ(0U, OrderBook::get_order_count_for_price(OrderBookMarketType::AIRBNB, 5));
    ASSERT_EQ(0U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::AIRBNB, 5));
    // Microsoft.
    ASSERT_EQ(0U, OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(0U, OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(0U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10));

    // Process events.
    ASSERT_TRUE(OrderBook::process_event(event1));
    ASSERT_TRUE(OrderBook::process_event(event2));

    // Airbnb.
    ASSERT_EQ(100U, OrderBook::get_last_modified_for_price(OrderBookMarketType::AIRBNB, 5));
    ASSERT_EQ(8U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::AIRBNB, 5));

    auto order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1000, 5);

    ASSERT_EQ(OrderBook::get_order_count_for_price(OrderBookMarketType::AIRBNB, 5), 1U);
    ASSERT_EQ(order->price, 5U);
    ASSERT_EQ(order->size, 8);
    ASSERT_EQ(order->time, 100U);

    // Microsoft.
    ASSERT_EQ(100U, OrderBook::get_last_modified_for_price(OrderBookMarketType::MICROSOFT, 10));
    ASSERT_EQ(10U, OrderBook::get_total_order_size_for_price(OrderBookMarketType::MICROSOFT, 10));

    order = OrderBook::get_order_by_id_and_price(OrderBookMarketType::MICROSOFT, 1000, 10);

    ASSERT_EQ(OrderBook::get_order_count_for_price(OrderBookMarketType::MICROSOFT, 10), 1U);
    ASSERT_EQ(order->price, 10U);
    ASSERT_EQ(order->size, 10);
    ASSERT_EQ(order->time, 100U);
    
    OrderBook::reset_market(OrderBookMarketType::MICROSOFT);
    OrderBook::reset_market(OrderBookMarketType::AIRBNB);
}

TEST(OrderBook, SIMDSearchWorks) {
    Event event1 {
        .price = 5,
        .time = 100,
        .order_id = 1000,
        .size = 8,
        .market = OrderBookMarketType::AIRBNB,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };
    auto event2 = event1;
    auto event3 = event1;
    auto event4 = event1;
    auto event5 = event1;
    auto event6 = event1;
    auto event7 = event1;
    auto event8 = event1;

    event2.order_id = 1001;
    event3.order_id = 1002;
    event4.order_id = 1003;
    event5.order_id = 1004;
    event6.order_id = 1005;
    event7.order_id = 1006;
    event8.order_id = 1007;

    ASSERT_TRUE(OrderBook::process_event(event1));
    ASSERT_TRUE(OrderBook::process_event(event2));
    ASSERT_TRUE(OrderBook::process_event(event3));
    ASSERT_TRUE(OrderBook::process_event(event4));
    ASSERT_TRUE(OrderBook::process_event(event5));
    ASSERT_TRUE(OrderBook::process_event(event6));
    ASSERT_TRUE(OrderBook::process_event(event7));
    ASSERT_TRUE(OrderBook::process_event(event8));

    auto order1 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1000, 5);
    auto order2 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1001, 5);
    auto order3 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1002, 5);
    auto order4 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1003, 5);
    auto order5 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1004, 5);
    auto order6 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1005, 5);
    auto order7 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1006, 5);
    auto order8 = OrderBook::get_order_by_id_and_price(OrderBookMarketType::AIRBNB, 1007, 5);

    ASSERT_NE(order1, nullptr);
    ASSERT_NE(order2, nullptr);
    ASSERT_NE(order3, nullptr);
    ASSERT_NE(order4, nullptr);
    ASSERT_NE(order5, nullptr);
    ASSERT_NE(order6, nullptr);
    ASSERT_NE(order7, nullptr);
    ASSERT_NE(order8, nullptr);

    OrderBook::reset_market(OrderBookMarketType::AIRBNB);
}