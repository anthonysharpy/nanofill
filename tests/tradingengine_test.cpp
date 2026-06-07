#include "gtest/gtest.h"
#include "tradingengine/tradingengine.hpp"

using nanofill::events::Event;
using nanofill::events::Event;
using nanofill::events::EventType;
using nanofill::events::TradeDirection;
using nanofill::tradingengine::TradingEngine;
using nanofill::orderbook::OrderBookMarketType;

TEST(TradingEngine, ProcessAllEvents) {
    // Submission (buy).
    Event buy_submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    ASSERT_EQ(0U, TradingEngine::get_total_market_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(0U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(0U, TradingEngine::get_average_share_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(0U, TradingEngine::get_target_buy_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(0U, TradingEngine::get_target_sell_price(OrderBookMarketType::MICROSOFT));

    TradingEngine::process_event(buy_submission_event);

    ASSERT_EQ(100U, TradingEngine::get_total_market_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(10U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(10U, TradingEngine::get_average_share_price(OrderBookMarketType::MICROSOFT));

    ASSERT_EQ(0U, TradingEngine::get_target_buy_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(30U, TradingEngine::get_target_sell_price(OrderBookMarketType::MICROSOFT));

    // Submission (sell).
    Event sell_submission_event {
        .price = 20,
        .time = 105,
        .order_id = 1001,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Negative,
        .type = EventType::Submission
    };

    TradingEngine::process_event(sell_submission_event);

    ASSERT_EQ(300U, TradingEngine::get_total_market_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(20U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(15U, TradingEngine::get_average_share_price(OrderBookMarketType::MICROSOFT));

    ASSERT_EQ(0U, TradingEngine::get_target_buy_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(35U, TradingEngine::get_target_sell_price(OrderBookMarketType::MICROSOFT));

    // Partial cancellation.
    Event cancellation_event {
        .price = 20,
        .time = 110,
        .order_id = 1001,
        .size = 5,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Negative,
        .type = EventType::Cancellation
    };

    TradingEngine::process_event(cancellation_event);

    ASSERT_EQ(200U, TradingEngine::get_total_market_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(15U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(13U, TradingEngine::get_average_share_price(OrderBookMarketType::MICROSOFT)); // 200 / 15 = 13.333...

    ASSERT_EQ(0U, TradingEngine::get_target_buy_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(33U, TradingEngine::get_target_sell_price(OrderBookMarketType::MICROSOFT));

    // Deletion.
    Event deletion_event {
        .price = 20,
        .time = 115,
        .order_id = 1001,
        .size = 5,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Deletion
    };

    TradingEngine::process_event(deletion_event);

    ASSERT_EQ(100U, TradingEngine::get_total_market_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(10U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(10U, TradingEngine::get_average_share_price(OrderBookMarketType::MICROSOFT));

    ASSERT_EQ(0U, TradingEngine::get_target_buy_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(30U, TradingEngine::get_target_sell_price(OrderBookMarketType::MICROSOFT));

    // Visible Execution.
    Event visible_execution_event {
        .price = 10,
        .time = 120,
        .order_id = 1000,
        .size = 5,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::ExecutionVisible
    };

    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).order_id, 0U);

    TradingEngine::process_event(visible_execution_event);

    ASSERT_EQ(50U, TradingEngine::get_total_market_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(5U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(10U, TradingEngine::get_average_share_price(OrderBookMarketType::MICROSOFT));

    ASSERT_EQ(0U, TradingEngine::get_target_buy_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(30U, TradingEngine::get_target_sell_price(OrderBookMarketType::MICROSOFT));

    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).order_id, 1000U);
    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).price, 10U);
    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).size, 5U);
    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).time, 120U);
    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).type, EventType::ExecutionVisible);

    // Hidden Execution.
    Event hidden_execution_event {
        .price = 10,
        .time = 120,
        .order_id = 1000,
        .size = 5,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::ExecutionHidden
    };

    // Nothing should change as we don't process hidden execution.
    TradingEngine::process_event(hidden_execution_event);

    ASSERT_EQ(50U, TradingEngine::get_total_market_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(5U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(10U, TradingEngine::get_average_share_price(OrderBookMarketType::MICROSOFT));

    ASSERT_EQ(0U, TradingEngine::get_target_buy_price(OrderBookMarketType::MICROSOFT));
    ASSERT_EQ(30U, TradingEngine::get_target_sell_price(OrderBookMarketType::MICROSOFT));

    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).order_id, 1000U);
    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).price, 10U);
    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).size, 5U);
    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).time, 120U);
    ASSERT_EQ(TradingEngine::get_last_execution_order(OrderBookMarketType::MICROSOFT).type, EventType::ExecutionVisible);

    TradingEngine::reset_market(OrderBookMarketType::MICROSOFT);
}

TEST(TradingEngine, DoesntCrashWhenSharesReachZero) {
    // Submission (buy).
    Event buy_submission_event {
        .price = 10,
        .time = 100,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Submission
    };

    TradingEngine::process_event(buy_submission_event);

    ASSERT_EQ(10U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));

    // Deletion.
    Event deletion_event {
        .price = 20,
        .time = 115,
        .order_id = 1000,
        .size = 10,
        .market = OrderBookMarketType::MICROSOFT,
        .direction = TradeDirection::Positive,
        .type = EventType::Deletion
    };

    TradingEngine::process_event(deletion_event);

    ASSERT_EQ(0U, TradingEngine::get_market_shares(OrderBookMarketType::MICROSOFT));

    TradingEngine::reset_market(OrderBookMarketType::MICROSOFT);
}