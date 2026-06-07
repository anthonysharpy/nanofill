#pragma once

#include "events/event.hpp"
#include "consts/consts.hpp"
#include "concurrency/spscringbuffer.hpp"
#include "concurrency/concurrency.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include <limits>
#include <new>
#include <atomic>
#include <array>
#include <immintrin.h>
#include <chrono>
#include <thread>

namespace nanofill::orderbook {

using events::Event;

struct OrderBookLevelDataPool;

constexpr std::size_t DATA_POOL_GROWTH_BUFFER_SIZE = std::bit_ceil(32 * MARKET_COUNT_SIZE_T);
constexpr std::size_t DATA_POOL_SIZE = 48;

alignas (std::hardware_destructive_interference_size) static inline concurrency::SPSCRingBuffer<OrderBookLevelDataPool*, DATA_POOL_GROWTH_BUFFER_SIZE> growth_buffer;

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
    // 複数のキャッシュラインを触らずにすべてのデータを変えるため、他のデータを一つのstructの配列に纏める。
    std::array<LevelPoolData, DATA_POOL_SIZE> data;
    // Order IDs gets their own contiguous array so we can easily run SIMD over it.
    // 簡単にSIMDを使えるため、注文IDは専用の連続した配列をもらう。
    std::array<std::uint32_t, DATA_POOL_SIZE> order_ids;
    alignas (std::hardware_destructive_interference_size) std::atomic<OrderBookLevelDataPool*> next = nullptr;
    alignas (std::hardware_destructive_interference_size) OrderBookLevelDataPool* next_pool_available_for_insert = this;
    std::uint64_t created_at = std::chrono::steady_clock::now().time_since_epoch().count();
    std::size_t size = 0;
    bool growth_requested = false;

    OrderBookLevelDataPool() = default;

    ~OrderBookLevelDataPool() {
        delete next.load(std::memory_order_relaxed);
    }

    // Returns the order's size on success, or std::numeric_limits<std::int32_t>::max() on failure.
    // 成功した場合は注文のサイズを返し、失敗した場合はstd::numeric_limits<std::int32_t>::max()を返す。
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

                // Prefer to write to older pools.
                // 古いプールに書くのを優先する。
                if (pool->created_at < next_pool_available_for_insert->created_at) {
                    next_pool_available_for_insert = pool;
                }

                return size;
            }

            pool = pool->next.load(std::memory_order_acquire);

            // Keep searching if we can.
            // できれば、探しつづける。
            if (pool) {
                continue;
            }

            // The order doesn't exist.
            // 注文は存在しない。
            return std::numeric_limits<std::int32_t>::max();
        }
    }
    
    // Get a pointer to the size of the order with the given id, or nullptr if it doesn't exist.
    // 指定されたIDの注文が存在する場合は、サイズのポインターを返し、存在しない場合は、nullptrを返す。
    [[gnu::always_inline]]
    std::int32_t* get_event_size_by_order_id(const std::uint32_t order_id) noexcept {
        OrderBookLevelDataPool* pool = this;

        while (true) {
            auto index = pool->find_order_id_in_pool(order_id);

            if (index < pool->size) {
                return &pool->data[index].size;
            }

            pool = pool->next.load(std::memory_order_acquire);

            // Keep searching if we can.
            // できれば、探しつづける。
            if (pool) {
                continue;
            }

            // The order doesn't exist.
            // 注文は存在しない。
            return nullptr;
        }
    }

    [[gnu::always_inline]]
    LevelPoolData* get_order_data_by_order_id(const std::uint32_t order_id) noexcept {
        OrderBookLevelDataPool* pool = this;

        while (true) {
            auto index = pool->find_order_id_in_pool(order_id);

            if (index < pool->size) {
                return &pool->data[index];
            }

            pool = pool->next.load(std::memory_order_acquire);

            // Keep searching if we can.
            // できれば、探しつづける。
            if (pool) {
                continue;
            }

            // The order doesn't exist.
            // 注文は存在しない。
            return nullptr;
        }
    }

    [[gnu::always_inline]]
    std::size_t get_order_count() noexcept {
        OrderBookLevelDataPool* pool = this;
        std::size_t count = 0;

        while (pool) {
            count += pool->size;
            pool = pool->next.load(std::memory_order_acquire);
        }

        return count;
    }

    [[gnu::always_inline]]
    void push(const Event event) noexcept {
        next_pool_available_for_insert->data[next_pool_available_for_insert->size] =
            LevelPoolData(event.price, event.time, event.get_size_with_direction());
        next_pool_available_for_insert->order_ids[next_pool_available_for_insert->size] =
            event.order_id;

        ++next_pool_available_for_insert->size;

        // Grow if this is the last pool.
        // これが最後のプールなら、プールを増加する。
        if (!next_pool_available_for_insert->growth_requested) {
            next_pool_available_for_insert->request_grow();
        }

        while (next_pool_available_for_insert->size == DATA_POOL_SIZE) {
            auto next_pool = next_pool_available_for_insert->next.load(std::memory_order_acquire);

            while(!next_pool) {
                _mm_pause();
                next_pool = next_pool_available_for_insert->next.load(std::memory_order_acquire);
            }

            next_pool_available_for_insert = next_pool;
        }
    }

    [[gnu::always_inline]]
    void request_grow() noexcept {
        growth_requested = true;

        while (!growth_buffer.push(this)) {}
    }

    [[gnu::always_inline]]
    void grow() noexcept {
        this->next.store(new OrderBookLevelDataPool(), std::memory_order_release);
    }

    // Same as grow() except this is called when the program starts to initialise initial
    // pools.
    // grow()と同じだが、プログラムが始まる時に初期のプールを初期化するために使われる。
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
    // 指定された注文が存在したら、注文のインデックスを返し、存在しなかったらsize_tの最大値を返す。戻り値が
    // .size以上の偽陽性を返す可能性もあり、性能上、呼び出し側が戻り値をチェックする必要がある。
    //
    // 繋がっているプールを辿らない。
    [[gnu::always_inline]]
    std::size_t find_order_id_in_pool(const std::uint32_t order_id) noexcept {
        __m256i target = _mm256_set1_epi32(order_id);

        // Check this pool. Use explicit SIMD to check 8 values at a time.
        //
        // It's actually slightly faster to not do n < size because then we don't
        // need to fetch the size from cache.
        // このプールをチェックする。一斉に８つの値をチェックするため、明示的なSIMDを使う。
        //
        // 実は、キャッシュを読み込む必要がなくなるから、n < sizeをチェックしないほうが速い。
        for (ptrdiff_t n = 0; n < static_cast<ptrdiff_t>(DATA_POOL_SIZE); n += 8) {
            __m256i ids = _mm256_loadu_si256((__m256i*)&order_ids[n]);
            __m256i cmp = _mm256_cmpeq_epi32(ids, target);
            unsigned int mask = _mm256_movemask_epi8(cmp);

            if (mask > 0) {
                // Divide by four to get position.
                // 位置を求めるため４で割る。
                return n + (__builtin_ctz(mask) >> 2);
            }
        }

        return std::numeric_limits<std::size_t>::max();
    }
};

static inline std::jthread memory_allocator_thread = std::jthread([](std::stop_token stop){
    concurrency::pin_thread_to_core(1);

    OrderBookLevelDataPool* data[DATA_POOL_GROWTH_BUFFER_SIZE];

    while (!stop.stop_requested()) {
        auto count = growth_buffer.pop_many(data, DATA_POOL_GROWTH_BUFFER_SIZE);

        for (ptrdiff_t n = 0; n < static_cast<ptrdiff_t>(count); ++n) {
            data[n]->grow();
        }
    }
});

}