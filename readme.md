# NanoFill

NanoFill is a C++ low-latency market data orderbook and trading engine that records and analyses real market data.

The system is designed with a number of low-latency techniques:

- Memory-aligned SPSC ring buffers for fast communication between event producer and consumer threads.
- Single-threading to avoid caching and locking slowdowns.
- Ordered and compact POD structs optimised for cache locality and reducing CPU cycles.
- Structs of arrays instead of arrays of structs to reduce cache turnover.
- Pre-reserved memory pools to minimise allocations.
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

### 6 Apr 2026
- Add debugging code that prints the final capacities of the market data vectors. Tells us that 37 of the vectors are growing during execution whereas 499,963 are not.

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