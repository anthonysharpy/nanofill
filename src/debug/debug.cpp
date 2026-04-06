#include "debug.hpp"

namespace nanofill::debug
{
    void print_levels_capacities(nanofill::orderbook::OrderBook& orderbook) {
        std::map<unsigned int, unsigned int> data{};

        for (const auto& level : orderbook.get_levels_orders()) {
            ++data[level.capacity()];
        }

        std::cout << "===== Level sizes =====\n";

        for (const auto& pair : data) {
            std::cout << "Capacity " << pair.first << ": " << pair.second << " occurences\n";
        }
    }
}