#pragma once

#include "events/event.hpp"

// Most of this code is really just a simple example since this will be mostly business logic.
// This currently only supports one market index.
// これは普通にビジネスロジックだから、これは大体ただの簡単な例だ。
// これは一つの株価指数に対応している。
namespace nanofill::tradingengine {

using events::Event;
using events::EventType;

class TradingEngine {
public:
    // Sum of the value of all orders in 10,000ths of a dollar. This includes both sell and buy orders.
    // E.g. $100 sell and $100 buy would be $200 (2,000,000).
    // 10,000倍したドラの価格での各注文の価値の合計。売り注文と買い注文が含まれている。たとえば、$100売り注文と
    // $100買い注文は$200になる。
    std::uint64_t total_market_price = 0;
    // Number of shares wanting to be bought or sold.
    // 買ってもらいたいと売ってもらいたい株の合計。
    std::uint64_t market_shares = 0;
    // Current average share price in 10,000ths of a dollar.
    // 10,000倍したドラの価格での平均株価。
    std::uint32_t average_share_price = 0;
    // The last execution order we've seen.
    // 直近の実行された注文。
    Event last_execution_order{};
    // The price the engine wants to buy at.
    // 取引処理エンジンが買ってもらいたい価格。
    std::uint32_t target_buy_price = 0;
    // The price the engine wants to sell at.
    // 取引処理エンジンが売ってもらいたい価格。
    std::uint32_t target_sell_price = 0;
    
    TradingEngine(const int price_spread);

    [[gnu::always_inline]] inline
    void process_event(const Event event) {
        if (event.type == EventType::Submission) {
            process_order_added_event(event);
        } else if (event.type != EventType::ExecutionHidden) {
            process_order_removed_event(event);
        } else {
            // Processing would lead to strange results since this order was never recorded.
            // この注文は記録されていないので、処理すれば、変なことが起こる。
            return;
        }

        update_position();
    }

private:
    // The distance from the average market price that we are willing to buy/sell at.
    // 売ってもらいたい・買ってもらいたい平均株価からの距離。
    std::uint32_t price_spread = 0;

    void update_position();
    void generate_intent(const std::int32_t amount, const std::uint32_t price);
    void cancel_all_orders();
    void process_order_removed_event(const Event event);
    void process_order_added_event(const Event event);
};

}