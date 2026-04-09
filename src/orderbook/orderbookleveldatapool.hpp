#pragma once

#include "events/event.hpp"
#include "consts/consts.hpp"
#include "concurrency/spscringbuffer.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include <limits>
#include <new>
#include <atomic>
#include <array>
#include <immintrin.h>
#include <chrono>

namespace nanofill::orderbook {
    
    using events::Event;

    constexpr std::size_t POOL_SIZE = 48;
    constexpr std::size_t GROWTH_BUFFER_SIZE = 32;

    struct OrderBookLevelDataPool;

    alignas (std::hardware_destructive_interference_size) static concurrency::SPSCRingBuffer<OrderBookLevelDataPool*, GROWTH_BUFFER_SIZE> growth_buffer;

    struct LevelPoolData
    {
        std::uint32_t price;
        std::uint32_t time;
        std::int32_t size;

        LevelPoolData(
            std::uint32_t price,
            std::uint32_t time,
            std::int32_t size
        ) : price(price), time(time), size(size) {}

        LevelPoolData() = default;
    };

    struct OrderBookLevelDataPool
    {
        // Combine other data into an array of structs so that we can update all data without
        // having to touch multiple different cache lines.
        std::array<LevelPoolData, POOL_SIZE> data;
        // Order IDs gets their own contiguous array so we can easily run SIMD over it.
        std::array<std::uint32_t, POOL_SIZE> order_ids;
        alignas (std::hardware_destructive_interference_size) std::unique_ptr<OrderBookLevelDataPool> next = nullptr;
        OrderBookLevelDataPool* next_pool_available_for_insert = this;
        std::uint64_t created_at = std::chrono::steady_clock::now().time_since_epoch().count();
        std::size_t size = 0;
        bool growth_requested = false;

        // Returns the order's size on success, or std::numeric_limits<std::int32_t>::max() on failure.
        [[gnu::always_inline]]
        std::int32_t remove_order_by_id(const std::uint32_t order_id) noexcept {
            OrderBookLevelDataPool* pool = this;

            while (true) {
                auto index = pool->find_order_id_in_pool(order_id);

                if (index < pool->size) {
                    auto size = pool->data[index].size;

                    pool->data[index] = pool->data[pool->size-1];
                    pool->order_ids[index] = pool->order_ids[pool->size-1];
                    
                    --pool->size;

                    // Prefer to write to younger pools.
                    if (pool->created_at < next_pool_available_for_insert->created_at) {
                        next_pool_available_for_insert = pool;
                    }

                    return size;
                }

                // Keep searching if we can.
                if (pool->next) {
                    pool = pool->next.get();
                    continue;
                }

                // The order doesn't exist.
                return std::numeric_limits<std::int32_t>::max();
            }
        }
        
        // Get a pointer to the event size of the order with the given id, or nullptr if it doesn't exist.
        [[gnu::always_inline]]
        std::int32_t* get_event_size_by_order_id(const std::uint32_t order_id) noexcept {
            OrderBookLevelDataPool* pool = this;

            while (true) {
                auto index = pool->find_order_id_in_pool(order_id);

                if (index < pool->size) {
                    return &pool->data[index].size;
                }

                // Keep searching if we can.
                if (pool->next) {
                    pool = pool->next.get();
                    continue;
                }

                // The order doesn't exist.
                return nullptr;
            }
        }

        [[gnu::always_inline]]
        void push(const Event event) noexcept {
            next_pool_available_for_insert->data[next_pool_available_for_insert->size] =
                LevelPoolData(event.price, event.time, event.size);
            next_pool_available_for_insert->order_ids[next_pool_available_for_insert->size] =
                event.order_id;

            ++next_pool_available_for_insert->size;

            // Grow if this is the last pool.
            if (!next_pool_available_for_insert->growth_requested) {
               next_pool_available_for_insert->request_grow();
            }

            while (next_pool_available_for_insert->size == POOL_SIZE) {
                while(!next_pool_available_for_insert->next) {
                    _mm_pause();
                }

                next_pool_available_for_insert = next_pool_available_for_insert->next.get();
            }
        }

        [[gnu::always_inline]]
        void request_grow() noexcept {
            growth_requested = true;

            while (!growth_buffer.push(this)) {}
        }

        [[gnu::always_inline]]
        void grow() noexcept {
            this->next = std::make_unique<orderbook::OrderBookLevelDataPool>();
        }

        // Same as grow() except this is called when the program starts to initialise initial
        // pools.
        [[gnu::always_inline]]
        void initial_grow() noexcept {
            this->growth_requested = true;
            this->grow();
        }

    private:
        // Return the index of this order ID in the pool, or the max value of size_t if it doesn't
        // exist. May return false positives where the result is greater or equal to .size.
        // For performance reasons, we don't check that here; the caller must check.
        //
        // Won't traverse connected pools.
        [[gnu::always_inline]]
        std::size_t find_order_id_in_pool(const std::uint32_t order_id) noexcept {
            __m256i target = _mm256_set1_epi32(order_id);

            // Check this pool. Use explicit SIMD to check 8 values at a time.
            //
            // It's actually slightly faster to not do n < size because then we don't
            // need to fetch the size from cache.
            for (std::size_t n = 0; n < POOL_SIZE; n += 8) {
                __m256i ids = _mm256_loadu_si256((__m256i*)&order_ids[n]);
                __m256i cmp = _mm256_cmpeq_epi32(ids, target);
                int mask = _mm256_movemask_epi8(cmp);

                if (mask > 0) {
                    // Divide by four to get position.
                    return n + (__builtin_ctz(mask) >> 2);
                }
            }

            return std::numeric_limits<std::size_t>::max();
        }
    };
}