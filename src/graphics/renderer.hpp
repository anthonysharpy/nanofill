#pragma once

#include <vector>

namespace nanofill::graphics {

// The number of columns the chart has. In reality, this is rendered as rows, not columns.
constexpr int chart_columns = 15;
// This is called chart height but in this case the chart extends from the left to the right,
// so this is actually the width.
constexpr int chart_height = 60;
// The size of each chart label.
constexpr int label_size = 8;

void render_latency_chart(std::vector<unsigned int> performance_data);

}