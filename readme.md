# NanoFill

NanoFill is a C++ low-latency market data orderbook and trading engine that records and analyses real market data.

The system is designed around a number of low-latency techniques:

- Memory-aligned SPSC ring buffers for fast communication between event producer and consumer threads.
- Single-threaded orderbook and trading engine to avoid caching and locking slowdowns.
- Ordered compact structs optimised for cache locality.
- Pre-reserved memory pools to minimise allocations.
- Avoidance of branches to avoid mispredictions, with optimised branch ordering where they must exist.
- Performance-guided optimisation (PGO) build process, resulting in faster binaries.
- Compiler flags set for aggressive optimisation. 
- Inlining of small or rarely repeated code to avoid function call overhead.

The software comes complete with the ability to produce its own latency distribution performance metrics analysis:

![Software output](/media/output.png)

# Usage

- **Release build**: `make pgo-gen` -> `make release`
- **Profile build**: `make pgo-gen` -> `make profile` (may require some extra software)
- **Run tests**: `make test`
- **Normal build (not recommended)**: `make`

# Sources

Example market data is from https://data.lobsterdata.com/info/DataStructure.php.