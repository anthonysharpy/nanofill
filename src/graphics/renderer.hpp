#pragma once

#include <vector>

namespace nanofill::graphics {

// The number of columns the chart has.
// 表の列の数。
constexpr int chart_rows = 15;
constexpr int chart_width = 60;
// The size of each chart label.
// 表の目盛りの広さ。
constexpr int label_size = 8;

void render_latency_chart(std::vector<unsigned int> performance_data);

}