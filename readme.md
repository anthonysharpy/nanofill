# NanoFill

NanoFill is a C++ low-latency market data orderbook and trading engine that records and analyses real market data.

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

This program was optimised based on the provided data, namely, the ~670,000 market order events. The optimisation decisions made in this program do not necessarily reflect those that might be advisable in a real production system, where data and throughput would likely differ greatly.

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

### 8 May 2026
- Upgrade makefile to use GCC 16 instead of 12. Seems to increase performance by just over 1%. 

### 11 April 2026
- Add a custom pool allocator instead of relying on vector growth.

    Previously, orders for each price were stored in a vector of default capacity 100.

    Now, each price has a pool. A pool can hold 48 orders. Extra pools can be created (every price starts with two pools by default to roughly match the previous size of 100). Each pool points to the next one in a linked-list fashion. When another pool is required, the main thread puts a pointer to the pool into an SPSC ring buffer. A separate thread processes this and allocates a new pool. It then simply updates the `next` pointer in the pool to point to the new pool, thus almost completely avoiding thread contention.

    The result is that P50 becomes ~3ns (~7%) slower, but P75 to P99 becomes ~4-5% faster. P99.9 becomes ~13ns (~11%) slower. However, P100 becomes ~40% (19,930ns) faster. The average event time decreases by 2.34%.

    Overall this is a very successful change. Technically the extra complexity adds some delay, and the vector approach worked reasonably given the current data, but vectors would not have scaled well at higher order counts, as growing a vector usually involves doubling its size. Previous tests showed that we were only growing vectors 0.0074% of the time. In other words, the vectors were never really put to the test, and likely would have failed under real-world conditions. These dynamically allocated pools on the other hand can be grown and pruned on-demand virtually for free.
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