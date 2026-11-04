# C++ Matching Engine

A single-threaded limit-order matching engine in C++20 with price-time
priority, GFD/IOC orders, cancellation, modification, and a text interface.

## Build and test

Requires a C++20 compiler and CMake 3.16 or newer. No external libraries or
test frameworks are required.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run the example:

```bash
./build/matching-engine < examples/orders.txt
```

Expected output is in `examples/expected.txt`.

Alternatively, build directly with GCC:

```bash
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -I include \
    src/OrderPool.cpp src/PriceLevel.cpp src/TradeReporter.cpp \
    src/OrderBook.cpp src/CommandProcessor.cpp src/main.cpp \
    -o matching-engine
./matching-engine < examples/orders.txt
```

For address and undefined-behavior checks with GCC or Clang:

```bash
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DMATCHING_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --parallel
ctest --test-dir build-sanitize --output-on-failure
```

## Commands

| Command | Format |
| --- | --- |
| Buy | `BUY GFD|IOC price quantity id` |
| Sell | `SELL GFD|IOC price quantity id` |
| Cancel | `CANCEL id` |
| Modify | `MODIFY id BUY|SELL price quantity` |
| Print | `PRINT` |

The `|` symbol denotes alternatives, not a literal command character.
Each command occupies one line. IDs are case-sensitive, non-whitespace
tokens. Prices and quantities are positive signed 64-bit integers.
Malformed lines, extra fields, invalid values, unknown IDs for cancellation
or modification, and duplicate active IDs for new orders are ignored.

Orders match the best available opposite price first, then FIFO within the
same price. Unfilled GFD quantities remain in the book; unfilled IOC quantities
are discarded. Modification cancels and resubmits a GFD order, losing its
previous time priority. The side can change. IDs can be reused after an order
is cancelled or fully executed.

Each execution prints the resting order first:

```text
TRADE restingId restingPrice quantity incomingId incomingPrice quantity
```

Both original limit prices are printed to follow this exercise's format.
`PRINT` displays `SELL:` then `BUY:`, with aggregated quantities in descending
price order in **both** sections.

## Structure

| File | Responsibility |
| --- | --- |
| `include/matching/Order.h` | Side, order type, and intrusive order node |
| `OrderPool.h` / `src/OrderPool.cpp` | Stable-address node ownership and free list |
| `PriceLevel.h` / `src/PriceLevel.cpp` | FIFO links and aggregate quantity |
| `OrderBook.h` / `src/OrderBook.cpp` | Matching, cancellation, modification, book snapshots |
| `TradeReporter.h` / `src/TradeReporter.cpp` | Trade event interface and text formatting |
| `CommandProcessor.h` / `src/CommandProcessor.cpp` | Line parsing and command dispatch |
| `src/main.cpp` | Standard input/output setup and fatal-error reporting |
| `tests/MatchingEngineTests.cpp` | Specification examples and behavioral/component checks |

All headers are under `include/matching/`. The core library is the CMake target
`matching_engine`; the command-line executable is `matching-engine`.

## Design and performance

- Ordered maps hold active price levels. An unordered map indexes active IDs.
- Each price level uses an intrusive doubly linked list for FIFO matching and
  constant-time unlinking once the order is found.
- A growing block pool reuses order nodes without moving existing nodes.
- Initial node capacity and hash bucket reservation can be configured through
  the `OrderBook` constructor. The default is 1,024 active orders.
- A synchronous `TradeReporter` separates execution reporting from matching.
  Reporters must copy IDs if retaining events; event IDs are borrowed views.

For L active price levels, level lookup is O(log L). ID lookup is average
O(1), with worst-case O(N) hash lookup. Matching also depends on the number of
orders executed. Printing visits all active levels. String hashing/comparison
cost depends on ID length.

## Limits and tradeoffs

This is an educational, single-threaded engine, not a production exchange.
It has no persistence, concurrency, market connectivity, or measured latency
guarantees. Standard maps, strings, hash entries, and pool growth still allocate
memory. Stream parsing and synchronous text output contribute to latency.

Price-level totals must fit in `int64_t`; overflow is detected and reported
as a fatal error by the executable. Allocation/reporter exceptions propagate.
Resting-order registration rolls back its own changes on failure, but a whole
command is not transactional: already completed fills are not rolled back, and
a failed modification may have removed the original order.

`PriceLevel` operations have documented membership preconditions; debug
assertions catch some misuse but do not validate all foreign-node cases.
`OrderPool::release` accepts only detached nodes obtained from that same pool.
Returned best-order pointers are borrowed and can become invalid or refer to
reused storage after subsequent mutations. Reporters must not re-enter the book.

## Validation

The test executable checks all five specification examples, FIFO execution,
modification priority, side changes, IOC behavior, invalid commands, duplicate
and missing IDs, descending snapshots, best-price queries, pool growth/reuse,
linked-list removal, and aggregate overflow rollback.

GitHub Actions builds and tests Release and Debug configurations, with
sanitizers enabled for Debug.

The initial refactor was verified locally with GCC: 21 checks passed in
Release and with AddressSanitizer/UBSan. Leak detection was disabled because
the execution environment does not support LeakSanitizer. All six public
headers compiled independently, and the CLI example matched its expected
output. CMake/CTest and the GitHub Actions workflow have not yet been run in
that environment because CMake was unavailable.
