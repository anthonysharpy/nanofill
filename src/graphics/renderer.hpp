#pragma once

#include <vector>

namespace nanofill::graphics {

constexpr int chart_width = 60;
// The size of each chart label.
// 表の目盛りの広さ。
constexpr int label_size = 8;

void render_latency_chart(std::vector<std::vector<unsigned int>>& performance_data);

}