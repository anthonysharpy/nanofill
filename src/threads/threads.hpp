#pragma once

#include "events/event.hpp"
#include "concurrency/spscringbuffer.hpp"
#include "orderbook/orderbook.hpp"
#include "tradingengine/tradingengine.hpp"
#include <array>
#include <chrono>

namespace nanofill::threads {

using concurrency::SPSCRingBuffer;
using orderbook::OrderBook;
using tradingengine::TradingEngine;
using events::Event;

// Pushes events into the event buffer.
// イベントバッファにイベントを入れる。
template<size_t N>
void event_producer(SPSCRingBuffer<Event, N>& event_buffer, const std::vector<Event>& events) noexcept {
    // I don't know why but the profiler said that using pointers was faster than the [] operator.
    // 理由が分からないけど、プロファイラによって、[]を使うのより、ポインタを使うほうが速い。

    auto position = &events[0];
    auto end = position + events.size();

    // It is much more efficient to push multiple events at once, however I've not done that as
    // it's sort-of cheating... in the real world, events come in one by one (usually). We also
    // don't want to wait for more events before pushing as that would introduce latency.
    // 複数のイベントを一発で入れるほうが速いが、そうするとちょっと狡いので、遠慮した。本当の世界では、イベントが
    // 一つずつ来る。その上、イベントが溜まるのを待つと、遅延が増えってしまう。
    while (position != end) {
        while (!event_buffer.push(*position)) {}
        ++position;
    }
}

// Reads and processes events from the event buffer.
// イベントバッファからのイベントを読み取って、処理する。
template<size_t N>
void event_consumer(
    SPSCRingBuffer<Event, N>& event_buffer,
    OrderBook& order_book,
    TradingEngine& trading_engine,
    std::vector<unsigned int>& performance_data
) noexcept {
    std::size_t events_consumed = 0;
    Event events[8];
    unsigned int events_found = 0;
    std::chrono::steady_clock::time_point clock_start;
    std::chrono::steady_clock::time_point clock_end;
    unsigned int i = 0;

    // Consume all the events. We'll stop when we've processed them all. In the real world,
    // this would keep going.
    // すべてのイベントを処理する。それから、止める。本当の世界では、これが続く。
    while (events_consumed != 668765) {
        events_found = event_buffer.pop_many(events, 8);

        for (i = 0; i < events_found; ++i) {
            // Logging on this hot path is probably not a good idea for performance.
            // このホットパスでログするのは性能に悪いはずだ。
            clock_start = std::chrono::steady_clock::now();

            if (order_book.process_event(events[i])) {
                // If the order book deemed an event to be invalid, then we should probably
                // ignore it in the trading engine too.
                // 板がイベントを無効だと判断したら、取引処理エンジンには無視したほうがいいかもしれない。
                trading_engine.process_event(events[i]);
            }

            clock_end = std::chrono::steady_clock::now();
            performance_data[events_consumed] = 
                std::chrono::duration_cast<std::chrono::nanoseconds>(clock_end - clock_start).count();

            ++events_consumed;
        }
    }    
}

}