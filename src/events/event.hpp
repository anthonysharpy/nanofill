#pragma once

#include "consts/consts.hpp"
#include <cstdint>
#include <vector>
#include <new>

namespace nanofill::events {

enum class EventType : std::uint8_t {
    // Submission of a new limit order.
    // 新しい指値注文の出し。
    Submission = 1,
    // Partial deletion of a limit order (reduce quantity).
    // 指値注文の一部削除（量が減る）。
    Cancellation = 2,
    // Total deletion of a limit order.
    // 指値注文の全削除。
    Deletion = 3,
    // Execution of a visible limit order.
    // 見える指値注文の約定。
    ExecutionVisible = 4,
    // Execution of a hidden limit order (original order not present in order book).
    // 見えない指値注文の削除（元の注文は板にない）。
    ExecutionHidden = 5,
};

// Make our struct as small as possible as well as ordering it appropriately so that
// we can fit more into the CPU cache.
//
// Some of our data types are quite small, but they fit our data so they're fine for
// our purposes.
// このstructをなるべく小さくすること上に適当に要素を並ぶことで、CPUキャッシュにデータをもっと詰め込める。
//
// データ型の一部はかなり小さいが、今のデータにはデータ型のサイズが足りているから問題ない。

// A trading event.
// 取引のイベント。
struct Event {
    // Dollar price times 10,000.
    // 10,000倍したドラの価格。
    std::uint32_t price;
    // Seconds after midnight the event happened.
    // イベントが発生したときからの零時から数秒。
    std::uint32_t time;
    std::uint32_t order_id;
    // Number of shares. Negative means this is a sell order.
    // 株の数。ネガチブなら、これは売り注文だ。
    std::int16_t size;
    EventType type;
};

std::vector<Event>
events_from_csv_data(const std::vector<consts::TradingDataCSVFormat>& csv_data);

void print_event(Event event);

}