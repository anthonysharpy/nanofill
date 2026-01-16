# NanoFill

NanoFill is a C++ low-latency market data orderbook and trading engine that records and analyses real market data.

The system is designed with a number of low-latency techniques:

- Memory-aligned SPSC ring buffers for fast communication between event producer and consumer threads.
- Single-threading to avoid caching and locking slowdowns.
- Ordered and compact POD structs optimised for cache locality.
- Structs of arrays instead of arrays of structs to reduce cache turnover.
- Pre-reserved memory pools to minimise allocations.
- Avoidance of branches to avoid mispredictions, with optimised branch ordering where they must exist.
- Performance-guided optimisation (PGO) build process, resulting in faster binaries.
- Compiler flags set for aggressive optimisation. 
- Highly aggressive inlining of small, often invoked and rarely repeated code, avoiding function call overhead.

The software comes complete with the ability to produce its own latency distribution performance metrics analysis:

![Software output](/media/output.png)

The purpose of this is to demonstrate the implementation of fast, low-latency programming techniques. It does not particularly show off any complex algorithms. For an example of complex mathematics and algorithms, see https://github.com/anthonysharpy/theta-surface.

# Usage

Note that the build process is designed to run on Linux and may require some extra software.

- **Release build**: `make pgo-gen` -> `make release`
- **Profile build**: `make pgo-gen` -> `make profile` (likely requires extra software)
- **Run tests**: `make test`
- **Normal build (not recommended)**: `make`

Once built, simply run the executable.

# Sources

Example market data is from https://data.lobsterdata.com/info/DataStructure.php.

# AI disclaimer

AI was used in this project for research and looking-up information. **None** of the code or text in this project is AI-generated.