#include "renderer.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <algorithm>
#include <cmath>

namespace nanofill::graphics {

// Using the latency performance data we collected, draw a nice chart in the console that
// shows the latency distribution.
// さっき収集したレイテンシ性能のデータで、レイテンシ分布を示すために、コンソールでいい表を作ろう。
void render_latency_chart(std::vector<unsigned int> performance_data) {
    auto p999_n = performance_data.size() * 0.999;

    // Create sorted version of data so we can measure percentiles etc.
    // パーセンタイルを測るために、ソートしたバージョンを作る。
    auto sorted_data = performance_data;
    std::sort(sorted_data.begin(), sorted_data.end());

    std::vector<unsigned int> p999_data(sorted_data.begin(), sorted_data.begin() + p999_n);

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

    // Print stats.
    // 統計情報を出力する。
    std::cout << std::endl
        << "===== Per-event latency percentiles =====" << std::endl
        << "P0: " << sorted_data[0] << "ns" << std::endl
        << "P50: " << sorted_data[std::roundl(sorted_data.size() * 0.5)] << "ns" << std::endl
        << "P75: " << sorted_data[std::roundl(sorted_data.size() * 0.75)] << "ns" << std::endl
        << "P90: " << sorted_data[std::roundl(sorted_data.size() * 0.90)] << "ns" << std::endl
        << "P95: " << sorted_data[std::roundl(sorted_data.size() * 0.95)] << "ns" << std::endl
        << "P99: " << sorted_data[std::roundl(sorted_data.size() * 0.99)] << "ns" << std::endl
        << "P99.9: " << sorted_data[std::roundl(sorted_data.size() * 0.999)] << "ns" << std::endl
        << "P100: " << sorted_data[std::roundl(sorted_data.size() - 1)] << "ns" << std::endl
        << std::endl;

    // Print the frequency table.
    // 度数表を出力する。
    std::cout << "===== P99.9 latency distribution =====" << std::endl;

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

        std::cout << label << " | " << bar << " | " << n << std::endl;
    }
}

}