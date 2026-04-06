#pragma once

#include "orderbook/orderbook.hpp"
#include <map>
#include <iostream>

namespace nanofill::debug
{
    void print_levels_capacities(nanofill::orderbook::OrderBook& orderbook);
}