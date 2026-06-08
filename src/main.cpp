#include "fileio/fileio.hpp"
#include "fileio/csv.hpp"
#include "events/event.hpp"
#include "linux/linux.hpp"
#include "orderbook/orderbookconfig.hpp"
#include "concurrency/spscringbuffer.hpp"
#include "threads/threads.hpp"
#include "graphics/renderer.hpp"
#include "consts/consts.hpp"
#include "concurrency/concurrency.hpp"
#include <chrono>
#include <thread>
#include <algorithm>
#include <print>

using nanofill::events::Event;
using nanofill::concurrency::SPSCRingBuffer;
using nanofill::orderbook::OrderBookMarketType;
using nanofill::orderbook::MARKET_COUNT_SIZE_T;
using nanofill::orderbook::MARKET_COUNT;

// Duplicate every event we have once for each market so that we have test data
// for all markets.
// すべての市場にテストデータがあるように、各イベントを複製する。
std::vector<Event> duplicate_events_for_markets(const std::vector<Event>& events) {
    std::vector<Event> output;

    output.reserve(events.size() * MARKET_COUNT_SIZE_T);

    for (const auto& event : events) {
        for (int i = 0; i < MARKET_COUNT; ++i) {
            output.push_back(event);
            output.back().market = static_cast<OrderBookMarketType>(i);
        }
    }

    return output;
}

std::vector<Event> parse_events(const std::vector<nanofill::consts::TradingDataCSVFormat>& csv_data) {
    std::println("Parsing {} events...", csv_data.size());
    auto clock_start = std::chrono::steady_clock::now();

    auto events = nanofill::events::events_from_csv_data(csv_data);
    events = duplicate_events_for_markets(events);

    auto clock_end = std::chrono::steady_clock::now();
    std::int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_start).count();
    double elapsed_seconds = elapsed / 1000000.0;
    std::println("Done in {} seconds", elapsed_seconds);

    return events;
}

std::vector<std::uint32_t>
process_events(const std::vector<Event>& events) {
    std::vector<std::uint32_t> performance_data;
    performance_data.resize(events.size());
    SPSCRingBuffer<Event, nanofill::consts::SPSC_BUFFER_SIZE> buffer;
    
    std::thread event_producer_thread(
        nanofill::threads::event_producer<nanofill::consts::SPSC_BUFFER_SIZE>,
        std::ref(buffer),
        std::ref(events)
    );
    std::thread event_consumer_thread(
        nanofill::threads::event_consumer<nanofill::consts::SPSC_BUFFER_SIZE>,
        std::ref(buffer),
        std::ref(performance_data)
    );

    event_producer_thread.join();
    event_consumer_thread.join();
    
    return performance_data;
}

bool is_benchmark_mode(int argc, char* argv[]) {
    auto end = argv + argc;

    return std::find_if(
        argv, end,
        [](const auto& x){ return std::strcmp(x, "--benchmark") == 0; }
    ) != end;
}

int main(int argc, char* argv[]) {
    nanofill::concurrency::pin_thread_to_core(0);

    std::println("Program is using {}mb of memory...", nanofill::linux::get_ram_usage_mb());

    std::println("Opening data file...");
    std::vector<std::string> file_data = nanofill::fileio::open_text_file("./data/MSFT_2012-06-21_34200000_57600000_message_10.csv");

    std::println("Parsing CSV data...");
    auto csv_data = nanofill::fileio::parse_csv_data<nanofill::consts::TradingDataCSVFormat>(file_data);
    std::println("Done!");

    int runs = is_benchmark_mode(argc, argv) ? 60 : 1;
    std::vector<std::vector<std::uint32_t>> performance_data;
    performance_data.resize(runs);

    auto events = parse_events(csv_data);

    std::println(
        "Processing {} events {} times for each of the {} markets ({} total events)...",
        events.size(), runs, MARKET_COUNT_SIZE_T, events.size() * runs * MARKET_COUNT_SIZE_T
    );

    for (int i = 0; i < runs; ++i) {
        // ===== FROM HERE is where we care about performance ===== //
        // ===== ここから性能が大事だ ===== //
        performance_data[i] = process_events(events);   
        // ===== Don't care about performance after this ===== //
        // ===== ここから性能がどうでもいい ===== //
    }

    std::println("Done!");

    nanofill::graphics::render_latency_chart(performance_data);

    return 0;
}