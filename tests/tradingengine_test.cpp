#include "gtest/gtest.h"
#include "tradingengine/tradingengine.hpp"

using nanofill::events::Event;
using nanofill::events::EventType;
using nanofill::tradingengine::TradingEngine;

TEST(TradingEngine, ProcessAllEvents) {
    auto trading_engine = TradingEngine(20);

    // Submission (buy).
    Event buy_submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .type = EventType::Submission
    };

    ASSERT_EQ(0U, trading_engine.total_market_price);
    ASSERT_EQ(0U, trading_engine.market_shares);
    ASSERT_EQ(0U, trading_engine.average_share_price);
    ASSERT_EQ(0U, trading_engine.target_buy_price);
    ASSERT_EQ(0U, trading_engine.target_sell_price);

    trading_engine.process_event(buy_submission_event);

    ASSERT_EQ(100U, trading_engine.total_market_price);
    ASSERT_EQ(10U, trading_engine.market_shares);
    ASSERT_EQ(10U, trading_engine.average_share_price);

    ASSERT_EQ(0U, trading_engine.target_buy_price);
    ASSERT_EQ(30U, trading_engine.target_sell_price);

    // Submission (sell).
    Event sell_submission_event {
        .price = 20,
        .time = 105,
        .order_id = 1001,
        .size = -10,
        .type = EventType::Submission
    };

    trading_engine.process_event(sell_submission_event);

    ASSERT_EQ(300U, trading_engine.total_market_price);
    ASSERT_EQ(20U, trading_engine.market_shares);
    ASSERT_EQ(15U, trading_engine.average_share_price);

    ASSERT_EQ(0U, trading_engine.target_buy_price);
    ASSERT_EQ(35U, trading_engine.target_sell_price);

    // Partial cancellation.
    Event cancellation_event {
        .price = 20,
        .time = 110,
        .order_id = 1001,
        .size = 5,
        .type = EventType::Cancellation
    };

    trading_engine.process_event(cancellation_event);

    ASSERT_EQ(200U, trading_engine.total_market_price);
    ASSERT_EQ(15U, trading_engine.market_shares);
    ASSERT_EQ(13U, trading_engine.average_share_price); // 200 / 15 = 13.333...

    ASSERT_EQ(0U, trading_engine.target_buy_price);
    ASSERT_EQ(33U, trading_engine.target_sell_price);

    // Deletion.
    Event deletion_event {
        .price = 20,
        .time = 115,
        .order_id = 1001,
        .size = 5,
        .type = EventType::Deletion
    };

    trading_engine.process_event(deletion_event);

    ASSERT_EQ(100U, trading_engine.total_market_price);
    ASSERT_EQ(10U, trading_engine.market_shares);
    ASSERT_EQ(10U, trading_engine.average_share_price);

    ASSERT_EQ(0U, trading_engine.target_buy_price);
    ASSERT_EQ(30U, trading_engine.target_sell_price);

    // Visible Execution.
    Event visible_execution_event {
        .price = 10,
        .time = 120,
        .order_id = 1000,
        .size = 5,
        .type = EventType::ExecutionVisible
    };

    ASSERT_EQ(trading_engine.last_execution_order.order_id, 0U);

    trading_engine.process_event(visible_execution_event);

    ASSERT_EQ(50U, trading_engine.total_market_price);
    ASSERT_EQ(5U, trading_engine.market_shares);
    ASSERT_EQ(10U, trading_engine.average_share_price);

    ASSERT_EQ(0U, trading_engine.target_buy_price);
    ASSERT_EQ(30U, trading_engine.target_sell_price);

    ASSERT_EQ(trading_engine.last_execution_order.order_id, 1000U);
    ASSERT_EQ(trading_engine.last_execution_order.price, 10U);
    ASSERT_EQ(trading_engine.last_execution_order.size, 5);
    ASSERT_EQ(trading_engine.last_execution_order.time, 120U);
    ASSERT_EQ(trading_engine.last_execution_order.type, EventType::ExecutionVisible);

    // Hidden Execution.
    Event hidden_execution_event {
        .price = 10,
        .time = 120,
        .order_id = 1000,
        .size = 5,
        .type = EventType::ExecutionHidden
    };

    // Nothing should change as we don't process hidden execution.
    trading_engine.process_event(hidden_execution_event);

    ASSERT_EQ(50U, trading_engine.total_market_price);
    ASSERT_EQ(5U, trading_engine.market_shares);
    ASSERT_EQ(10U, trading_engine.average_share_price);

    ASSERT_EQ(0U, trading_engine.target_buy_price);
    ASSERT_EQ(30U, trading_engine.target_sell_price);

    ASSERT_EQ(trading_engine.last_execution_order.order_id, 1000U);
    ASSERT_EQ(trading_engine.last_execution_order.price, 10U);
    ASSERT_EQ(trading_engine.last_execution_order.size, 5);
    ASSERT_EQ(trading_engine.last_execution_order.time, 120U);
    ASSERT_EQ(trading_engine.last_execution_order.type, EventType::ExecutionVisible);
}