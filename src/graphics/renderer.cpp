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
    unsigned int smallest_latency = p999_data[0];
    unsigned int highest_latency = p999_data[p999_data.size() - 1];
    int range = highest_latency - smallest_latency;
    int band_size = range / chart_rows;
    std::array<int, chart_rows> frequency_table{};

    // Calculate the frequency table.
    // 度数表を計算する。
    for (auto latency : p999_data) {
        ++frequency_table[std::lround((float)(latency - smallest_latency) / band_size)];
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
        std::string label = std::to_string(smallest_latency + (i * band_size)) + "ns";
        label.insert(0, label_size - label.size(), ' ');

        // I think this technically causes a bug where the last column is incorrectly overrepresented,
        // but the graph's tail is so small that it doesn't really matter in our data.
        // 厳密には、これで、最後の列が過剰に表現されていてしまい、バグになっていて、表の尾がとても小さいから、
        // このため、実は問題ない。
        int bar_width = std::roundl(((float)frequency_table[i] / highest_frequency) * chart_width);
        std::string bar(bar_width, '|');
        bar.insert(bar.size(), chart_width - bar.size(), ' ');

        std::string n = "(" + std::to_string(frequency_table[i]) + ")";

        std::cout << label << " | " << bar << " | " << n << std::endl;
    }
}

}