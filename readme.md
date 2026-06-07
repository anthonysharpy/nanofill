# NanoFill

NanoFill is a C++ low-latency market data orderbook and trading engine with support for multiple markets.

The system is designed with a number of low-latency techniques:

- Memory-aligned SPSC ring buffers for fast communication between threads.
- Single-threaded orderbook logic to avoid caching and locking slowdowns.
- Pre-reserved memory pools to minimise expensive initial allocations.
- Custom memory pool allocator running on a separate thread, optimised for minimal thread contention. 
- Core-pinning to avoid unnecessary sharing of cache.
- Ordered and compact POD structs optimised for cache locality.
- Selective use of structs of arrays optimised for SIMD instructions.
- Carefully-chosen types small enough to fit into cache and big enough to avoid extra widening operations on the CPU.
- Avoidance of branches to avoid mispredictions, with optimised branch ordering where they must exist.
- Performance-guided optimisation (PGO) build process, resulting in faster binaries.
- Compiler flags set for aggressive optimisation. 
- Highly aggressive inlining of small, often invoked and rarely repeated code, avoiding function call overhead.

The software comes complete with the ability to produce its own latency distribution performance analysis:

![Software output](/media/output.png)

The purpose of this is to demonstrate the implementation of fast, low-latency programming techniques. It does not particularly show off any complex algorithms. For an example of complex mathematics and algorithms, see https://github.com/anthonysharpy/theta-surface.

This program was optimised based on the provided data, namely, the ~6,700,000 market order events. The optimisation decisions made in this program do not necessarily reflect those that might be advisable in a real production system, where data and throughput would likely differ greatly.

# Usage

Note that the build process is designed to run on Linux and may require some extra software.

- **Release build**: `make release`
- **Profile build**: `make profile` (likely requires extra software)
- **Run tests**: `make test`
- **Normal build (not recommended)**: `make`
- **Dump assembly to file**: `make assembly`
- **Build for benchmarking**: `make benchmark`

Once built, simply run the executable.

# Sources

Example market data is from https://data.lobsterdata.com/info/DataStructure.php.

## Recent Changes

### 6 June 2026
- Add support for multiple markets as well as some other changes and improvements in latency (details below). P75-P99 has seen an average 20-26% speedup, P50 is ~10.7% faster and P0 is also about 1% faster on average.
- Add support for multiple markets. Markets are configured and represented by an enum.

    The code works similarly to how it did before, except each market now gets its own copy of the data. Initially, this caused the program to take up 8GB of memory for just 10 markets. However, upon noticing that we were supporting fractional pence even though the data doesn't have any fractional pence in it, it was posible to cut the memory footprint by 100x. Thus, the program now only uses around 80MB of memory. This probably contributed to the large performance improvements seen; the memory has become more compact and thus has less CPU cache turnover.

    The code does not interact with each market directly; instead, it must go through the `OrderBook` and `TradingEngine` classes, whose members have been made static with `[[gnu::always_inline]]` in order to turn them into zero-cost wrappers.

    We duplicate the data for each market, meaning we are now processing 10x as much data as before. The incoming event data has the markets shuffled. For example, the first event will be for market 1, the second event for market 2, and so on. This means that we are constantly putting pressure on the cache since the market we are accessing is constantly changing, making it quite a realistic test.
- Fix a rather bad bug that caused 1/8th of all orders to be unfindable, causing massive slowdowns as well as incorrect behaviour.

    The reason this bug was hard to find was due to the fact that the tests weren't thorough enough. In order for it to happen, at least 8 orders had to be created - the 8th would always be unfindable.

    The reason for this was because of this code:

    ```
    int mask = _mm256_movemask_epi8(cmp);

    if (mask > 0) {
        // Divide by four to get position.
        return n + (__builtin_ctz(mask) >> 2);
    }
    ```

    `mask` shows where (and if) an order ID was found during a SIMD operation (basically, we put a bunch of order IDs into a 256-bit register to check 8 order IDs at once).

    However, if the order ID were found in the 8th slot, this would have set the highest 4 bits to 1. Because `mask` was an int, this made the number negative, meaning  `mask > 0` would always be false for the 8th order and it would never be found.

    The fix was simply to make this an `unsigned int`.
- Add a 4 second delay into the benchmarking script between runs. Previously, the CPU was getting throttled, which caused it to heat up and make the performance data drift in strange ways. Now it's much more stable, even after an equivalent number of runs.
- Replace the `std::unique_ptr` that points to the next market data memory pool with an `std::atomic<OrderBookLevelDataPool*>`, and add a destructor to clean this up manually.
    
    The main reason for doing this is because when we were accessing the memory pool pointed to by `next` from the main thread and the pool allocator thread, we were reading and writing from it without any kind of formal synchronisation mechanism. In reality this worked fine because x86 supports this kind of behaviour, but technically this is undefined behaviour in C++; the compiler is free to rearrange/optimise operations within each thread, so there's a theoretical chance it could lead to inconsistent behaviour, like the `next` pointer becoming available to the other thread while the data it's pointing to has not even been initialised yet. The best fix was to make this atomic. In doing so, the `std::unique_ptr` turned into an additional layer of complication, and it made more sense to ditch it.
- Replace uses of `std::cout` with the more modern `std::println`. It produces much nicer code.
- Replace some vectors that never grew with arrays, which also helps us sidestep issues caused by the implicitly deleted copy operators caused by use of atomics.
- Make the trading engine code .hpp only (95% of it already was). From benchmarking, .hpp files generally seem to provide the compiler a better chance of performing as many optimisations as possible as it reduces the number of code boundaries it has to reason over, even when using link-time optimisation.
- Make the program output its memory usage on startup.
- Fix alignment of the `next` pointer inside the data pools - the start of it was aligned but not the end, possibly causing a very minor slowdown.

### 1 June 2026
- Replace unsigned types (`std::size_t` etc) with `ptrdiff_t` in loops. `ptrdiff_t` is more easily optimisable by the compiler since it's a signed type. P75 and above gets a ~1.5% speed boost.

### 8 May 2026
- Upgrade makefile to use GCC 16 instead of 12. Seems to increase performance by just over 1%. 

### 11 April 2026
- Add a custom pool allocator instead of relying on vector growth.

    Previously, orders for each price were stored in a vector of default capacity 100.

    Now, each price has a pool. A pool can hold 48 orders. Extra pools can be created (every price starts with two pools by default to roughly match the previous size of 100). Each pool points to the next one in a linked-list fashion. When another pool is required, the main thread puts a pointer to the pool into an SPSC ring buffer. A separate thread processes this and allocates a new pool. It then simply updates the `next` pointer in the pool to point to the new pool, thus largely sidestepping thread contention.

    The result is that P50 becomes ~3ns (~7%) slower, but P75 to P99 becomes ~4-5% faster. P99.9 becomes ~13ns (~11%) slower. However, P100 becomes ~40% (19,930ns) faster. The average event time decreases by 2.34%.

    Overall this is a very successful change. Technically the extra complexity adds some delay, and the vector approach worked reasonably given the current data, but vectors would not have scaled well at higher order counts, as growing a vector usually involves doubling its size. Previous tests showed that we were only growing vectors 0.0074% of the time. In other words, the vectors were never really put to the test, and likely would have failed under real-world conditions. These dynamically allocated pools on the other hand can be grown and pruned on-demand without interrupting the main thread.
- Add core pinning to make sure that important cache does not get spread across cores.
- Do all initialisation on the main thread so that the cache is warm on that core.
- Store order IDs as a separate array in the orderbook. This lets us perform SIMD to check up to 8 order IDs at once when searching for an order. Store other data as a struct in a separate array; having separate arrays for each data point stresses the cache out too much.
- Keep a pointer to the next non-full pool in the first pool for faster insertion. When we remove an order from a pool, if that pool is younger, set the pointer to that pool.
- Update profiling recipe to record activity from all CPU cores.

### 6 Apr 2026
- Add debugging code that prints the final capacities of the market data vectors. Tells us that 37 of the vectors are growing during (exceeding capacity 100) execution whereas 499,963 are not.

### 29 Mar 2026
- Record cache misses in profile data.

### 27 Mar 2026
- Only use P999 data for calculating average event processing time in order to reduce noise during benchmarking. 
- Make buffer size a configurable value.
- Reduce event processing time by ~2.5% by shrinking buffer size.
- Reduce event processing time by ~1.6% by making event direction a larger type, preventing wasted CPU cycles on type widening.

### 25 Mar 2026
- Improve average event processing time by ~4.6% by using smaller integer types. P50min: 34ns -> 33ns. P75min: 54ns -> 53ns. P95min: 73ns -> 67ns.
- Simplify code in some places.
- Fix bug where size type was not large enough to fit some massive orders. Unfortunately increases average event time by ~5.9%.

### 24 Mar 2026
- Add benchmarking script.

### 22 Mar 2026
- Add --benchmark flag that repeats the program multiple times for more thorough benchmarking.
- Add average event time to status output.
- Store event size as an unsigned int, and store whether this is negative or positive separately in the struct. Decreases average event processing time by 0.34%. P50 gets a ~1ns reduction, at the cost of P75+ being ~2ns slower.

### 20 Mar 2026
- Use fixed 10ns intervals on latency graph for easier run-by-run comparison.

### 19 Mar 2026
- Fix weird compile error that was probably caused by different standard library versions.
- Improve average event time by ~1.1% with use of emplace_back instead of push.