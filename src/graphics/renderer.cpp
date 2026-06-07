#include "renderer.hpp"
#include <array>
#include <print>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nanofill::graphics {

void print_latency_percentiles(std::vector<std::vector<std::uint32_t>> performance_data) {
    // Create sorted version of data so we can measure percentiles etc.
    // パーセンタイルを測るために、ソートしたバージョンを作る。
    for (auto& run_data : performance_data) {
        std::sort(run_data.begin(), run_data.end());
    }

    float p0{}, p50{}, p75{}, p90{}, p95{}, p99{}, p999{}, p100{};

    for (const auto& run_data : performance_data) {
        p0 += run_data[0];
        p50 += run_data[std::roundl(run_data.size() * 0.5)];
        p75 += run_data[std::roundl(run_data.size() * 0.75)];
        p90 += run_data[std::roundl(run_data.size() * 0.90)];
        p95 += run_data[std::roundl(run_data.size() * 0.95)];
        p99 += run_data[std::roundl(run_data.size() * 0.99)];
        p999 += run_data[std::roundl(run_data.size() * 0.999)];
        p100 += run_data.back();
    }

    p0 /= performance_data.size();
    p50 /= performance_data.size();
    p75 /= performance_data.size();
    p90 /= performance_data.size();
    p95 /= performance_data.size();
    p99 /= performance_data.size();
    p999 /= performance_data.size();
    p100 /= performance_data.size();

    // Print stats.
    // 統計情報を出力する。
    std::println(
        "===== Per-event latency percentiles =====\n"
        "P0: {}ns\n"
        "P50: {}ns\n"
        "P75: {}ns\n"
        "P90: {}ns\n"
        "P95: {}ns\n"
        "P99: {}ns\n"
        "P99.9: {}ns\n"
        "P100: {}ns\n",
        p0, p50, p75, p90, p95, p99, p999, p100
    );
}

void print_latency_distribution(const std::vector<std::uint32_t>& flattened_p999_data) {
    // The frequency table will go up in 10ns increments up to 200ns.
    // 度数表は200nsまで10ns刻みで増加する。
    constexpr int frequency_table_rows = 20;
    constexpr int frequency_table_increment_size = 10;
    std::array<int, frequency_table_rows> frequency_table{};

    // Calculate the frequency table.
    // 度数表を計算する。
    for (const auto& latency : flattened_p999_data) {
        int row = latency / frequency_table_increment_size;

        if (row >= frequency_table_rows) {
            row = frequency_table_rows-1;
        }

        ++frequency_table[row];
    }

    // Figure out what the highest frequency was.
    // 一番度数が高い度数をチェックする。
    int highest_frequency = 0;

    for (const auto& frequency : frequency_table) {
        if (frequency > highest_frequency) {
            highest_frequency = frequency;
        }
    }

    // Print the frequency table.
    // 度数表を出力する。
    std::println("===== P99.9 latency distribution =====");

    for (std::size_t i = 0; i < frequency_table.size(); ++i) {
        std::string label = std::to_string((i + 1) * frequency_table_increment_size) + "ns";
        
        if (i == frequency_table_rows-1) {
            label += "+";
        }
        
        label.insert(0, label_size - label.size(), ' ');

        int bar_width = std::lroundf((static_cast<float>(frequency_table[i]) / highest_frequency) * chart_width);
        std::string bar(bar_width, '|');
        bar.insert(bar.size(), chart_width - bar.size(), ' ');

        std::string n = "(" + std::to_string(frequency_table[i]) + ")";

        std::println("{} | {} | {}", label, bar, n);
    }
}

void print_stats(const std::vector<std::uint32_t>& flattened_p999_data) {
    std::uint64_t total = 0;
        
    for (const auto& time : flattened_p999_data) {
        total += time;
    }

    double average = static_cast<double>(total) / flattened_p999_data.size();

    std::println(
        "===== Stats =====\n"
        "Average event time: {}ns\n",
        average
    );
}

std::vector<std::uint32_t>
get_flattened_performance_data(std::vector<std::vector<std::uint32_t>> raw_performance_data) {
    // Flatten vectors into a single vector.
    // Vectorを一つのvectorにまとめる。
    std::vector<std::uint32_t> flattened_data;
    flattened_data.reserve(raw_performance_data.size() * raw_performance_data[0].size());

    for (auto& data : raw_performance_data) {
        flattened_data.insert(
            flattened_data.end(),
            std::make_move_iterator(data.begin()),
            std::make_move_iterator(data.end())
        );
    }

    // Sort.
    // 並べ替える。
    std::sort(flattened_data.begin(), flattened_data.end());

    return flattened_data;
}

std::vector<std::uint32_t>
get_flattened_p999_performance_data(std::vector<std::uint32_t> raw_flattened_performance_data) {
    // Keep only p99.9 data.
    // p99.9データしか要らない。
    std::vector<std::uint32_t> p999_data;
    const int p999_count = raw_flattened_performance_data.size() * 0.999;

    p999_data.reserve(p999_count);

    p999_data.insert(
        p999_data.end(),
        std::make_move_iterator(raw_flattened_performance_data.begin()),
        std::make_move_iterator(raw_flattened_performance_data.begin() + p999_count)
    );

    return p999_data;
}

// Using the latency performance data we collected, draw a nice chart in the console that
// shows the latency distribution.
// さっき収集したレイテンシ性能のデータで、レイテンシ分布を示すために、コンソールでいい表を作ろう。
void render_latency_chart(std::vector<std::vector<std::uint32_t>>& performance_data) {
    std::println("");

    auto flattened_data = get_flattened_performance_data(performance_data);
    auto flattened_p999_data = get_flattened_p999_performance_data(flattened_data); 

    print_stats(flattened_p999_data);
    print_latency_percentiles(performance_data);
    print_latency_distribution(flattened_p999_data);
}

}