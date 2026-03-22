#include "renderer.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nanofill::graphics {

void print_latency_percentiles(std::vector<std::vector<unsigned int>> performance_data) {
    // Create sorted version of data so we can measure percentiles etc.
    // パーセンタイルを測るために、ソートしたバージョンを作る。
    for (auto& run_data : performance_data) {
        std::sort(run_data.begin(), run_data.end());
    }

    float p0{}, p50{}, p75{}, p90{}, p95{}, p99{}, p999{}, p100{};

    for (auto& run_data : performance_data) {
        p0 += run_data[0];
        p50 += run_data[std::roundl(run_data.size() * 0.5)];
        p75 += run_data[std::roundl(run_data.size() * 0.75)];
        p90 += run_data[std::roundl(run_data.size() * 0.90)];
        p95 += run_data[std::roundl(run_data.size() * 0.95)];
        p99 += run_data[std::roundl(run_data.size() * 0.99)];
        p999 += run_data[std::roundl(run_data.size() * 0.999)];
        p100 += run_data[std::roundl(run_data.size() - 1)];
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
    std::cout << "===== Per-event latency percentiles =====\n"
        << "P0: " << p0 << "ns\n"
        << "P50: " << p50 << "ns\n"
        << "P75: " << p75 << "ns\n"
        << "P90: " << p90 << "ns\n"
        << "P95: " << p95 << "ns\n"
        << "P99: " << p99 << "ns\n"
        << "P99.9: " << p999 << "ns\n"
        << "P100: " << p100 << "ns\n\n";
}

void print_latency_distribution(std::vector<std::vector<unsigned int>> performance_data) {
    // Flatten vectors into a single vector.
    // Vectorを一つのvectorにまとめる。
    std::vector<unsigned int> combined_data;
    combined_data.reserve(performance_data.size() * performance_data[0].size());

    for (auto& data : performance_data) {
        combined_data.insert(
            combined_data.end(),
            std::make_move_iterator(data.begin()),
            std::make_move_iterator(data.end())
        );
    }

    // Sort.
    // 並べ替える。
    std::sort(combined_data.begin(), combined_data.end());

    // Keep only p99.9 data.
    // p99.9データしか要らない。
    std::vector<unsigned int> p999_data;
    p999_data.reserve(combined_data.size() * 0.999);

    p999_data.insert(
        p999_data.end(),
        std::make_move_iterator(combined_data.begin()),
        std::make_move_iterator(combined_data.end())
    );

    // The frequency table will go up in 10ns increments up to 200ns.
    // 度数表は200nsまで10ns刻みで増加する。
    constexpr int frequency_table_rows = 20;
    constexpr int frequency_table_increment_size = 10;
    std::array<int, frequency_table_rows> frequency_table{};

    // Calculate the frequency table.
    // 度数表を計算する。
    for (auto latency : p999_data) {
        unsigned int row = latency / frequency_table_increment_size;

        if (row >= frequency_table_rows) {
            row = frequency_table_rows-1;
        }

        ++frequency_table[row];
    }

    // Figure out what the highest frequency was.
    // 一番度数が高い度数をチェックする。
    int highest_frequency = 0;

    for (auto frequency : frequency_table) {
        if (frequency > highest_frequency) {
            highest_frequency = frequency;
        }
    }

    // Print the frequency table.
    // 度数表を出力する。
    std::cout << "===== P99.9 latency distribution =====\n";

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

        std::cout << label << " | " << bar << " | " << n << "\n";
    }
}

void print_stats(std::vector<std::vector<unsigned int>> performance_data) {
    std::uint64_t total = 0;
    std::uint64_t event_count = 0;

    for (auto& run: performance_data) {
        for (auto& time : run) {
            total += time;
            ++event_count;
        }
    }

    double average = static_cast<double>(total) / event_count;

    std::cout << "===== Stats =====\n"
        << "Average event time: " << average << "ns\n\n";
}

// Using the latency performance data we collected, draw a nice chart in the console that
// shows the latency distribution.
// さっき収集したレイテンシ性能のデータで、レイテンシ分布を示すために、コンソールでいい表を作ろう。
void render_latency_chart(std::vector<std::vector<unsigned int>>& performance_data) {
    std::cout << "\n";

    print_stats(performance_data);
    print_latency_percentiles(performance_data);
    print_latency_distribution(performance_data);
}

}