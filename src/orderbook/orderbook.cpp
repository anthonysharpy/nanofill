#include "orderbook.hpp"
#include <stdexcept>

namespace nanofill::orderbook {

using events::Event;

OrderBook::OrderBook() noexcept {
    levels_orders.resize(order_book_size);
    levels_last_modified.resize(order_book_size);
    levels_size.resize(order_book_size);

    for (size_t i = 0; i < order_book_size; ++i) {
        levels_orders[i].reserve(100);
    }
}

}