# Riverbed

Lock-free single-producer single-consumer primitives for high-throughput byte streams and message passing in C++17.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Performance](#performance)
- [Design](#design)
- [Limitations](#limitations)
- [Platform Support](#platform-support)
- [License](#license)

## Overview

Riverbed provides two core abstractions for low-latency, wait-free communication between one producer and one consumer:

- **River** — A lock-free ring buffer for raw bytes. You reserve a writable or readable region, use it (e.g. `recv` into it or `send` from it), then commit. No extra copying between producer and consumer; the same memory is handed to both sides. Ideal for I/O pipelines, socket echo servers, and streaming data paths where you want to avoid heap churn and unnecessary copies.

- **SpscQueue&lt;T&gt;** — A lock-free single-producer single-consumer queue for trivially copyable, trivially destructible types. Uses acquire/release semantics for correct visibility. Suited for task queues, event pumps, and message passing between two threads without mutexes or condition variables.

This design is ideal for:

- **I/O pipelines** — decouple network read/write with a bounded buffer
- **Echo and proxy servers** — stream bytes through a ring buffer between recv and send
- **Inter-thread messaging** — pass items between one producer and one consumer with minimal overhead
- **Embedded and real-time** — predictable, lock-free behavior and no dynamic allocation in the hot path

## Features

- **Lock-free** — River and SpscQueue use atomics only; no mutexes or condition variables
- **Cache-line aware** — producer and consumer indices are padded to avoid false sharing
- **Zero-copy friendly** — River exposes direct pointers into the buffer for reserve/commit
- **Power-of-two capacity** — ring indexing via mask for fast wrap-around
- **Correct memory ordering** — acquire/release on the shared indices
- **Zero dependencies** — C++17 and the standard library only
- **Modern CMake** — sanitizer options, export compile commands

## Installation

### CMake (FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
  riverbed
  GIT_REPOSITORY https://github.com/youruser/riverbed.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(riverbed)

target_link_libraries(your_target PRIVATE riverbed_lib)
```

### Manual

```bash
git clone https://github.com/youruser/riverbed.git
cd riverbed
cmake -S . -B build
cmake --build build -j
```

Link against `riverbed_lib` and add `include/` to your include path.

## Quick Start

### River (byte ring buffer)

```cpp
#include "Riverbed/river.hpp"
#include <sys/socket.h>
#include <unistd.h>

int main() {
    Riverbed::River river(64 * 1024);  // 64 KiB, power-of-two

    // Producer: reserve, fill, commit
    auto [dst, writable] = river.reserve_write();
    ssize_t n = ::recv(sock_fd, const_cast<uint8_t*>(dst), writable, 0);
    if (n > 0)
        river.commit_write(static_cast<size_t>(n));

    // Consumer: reserve, drain, commit
    auto [src, readable] = river.reserve_read();
    ssize_t sent = ::send(sock_fd, src, readable, 0);
    if (sent > 0)
        river.commit_read(static_cast<size_t>(sent));
}
```

### SpscQueue (message queue)

```cpp
#include "Riverbed/queue.hpp"
#include <thread>

int main() {
    Riverbed::SpscQueue<int> queue(1024);  // power-of-two capacity

    std::thread producer([&] {
        for (int i = 0; i < 1000; ++i)
            while (!queue.push(i)) { /* full, spin or yield */ }
    });

    std::thread consumer([&] {
        int value;
        while (queue.pop(value))
            process(value);
    });

    producer.join();
    consumer.join();
}
```

## API Reference

### `Riverbed::River`

Lock-free byte ring buffer for one producer and one consumer.

#### Constructor

```cpp
explicit River(size_t capacity);
```

Creates a ring buffer with the given capacity. **Capacity must be a power of two** (e.g. 4096, 65536).

#### `reserve_write()`

```cpp
std::pair<const uint8_t*, size_t> reserve_write();
```

Returns a pointer and length for the next contiguous writable region. The producer may write up to that many bytes, then call `commit_write(n)`.

| Returns | Description |
|--------|-------------|
| `.first` | Pointer to writable memory |
| `.second` | Number of bytes that can be written (may be 0 if full) |

#### `commit_write(n)`

```cpp
void commit_write(size_t n);
```

Marks `n` bytes as written. Must not exceed the length returned by the last `reserve_write()`.

#### `reserve_read()`

```cpp
std::pair<const uint8_t*, size_t> reserve_read();
```

Returns a pointer and length for the next contiguous readable region. The consumer may read up to that many bytes, then call `commit_read(n)`.

| Returns | Description |
|--------|-------------|
| `.first` | Pointer to readable memory |
| `.second` | Number of bytes available (may be 0 if empty) |

#### `commit_read(n)`

```cpp
void commit_read(size_t n);
```

Marks `n` bytes as consumed. Must not exceed the length returned by the last `reserve_read()`.

#### `capacity_bytes()`

```cpp
size_t capacity_bytes() const;
```

Returns the buffer capacity in bytes.

---

### `Riverbed::SpscQueue<T>`

Lock-free single-producer single-consumer queue. `T` must be trivially copyable and trivially destructible.

#### Constructor

```cpp
explicit SpscQueue(size_t capacity);
```

Creates a queue with the given capacity. **Capacity must be a power of two.**

#### `push(value)`

```cpp
bool push(T value);
```

Enqueues `value`. Returns `true` on success, `false` if the queue is full.

#### `pop(out)`

```cpp
bool pop(T& out);
```

Dequeues into `out`. Returns `true` on success, `false` if the queue is empty.

---

### Project structure

```
riverbed/
├── include/
│   └── Riverbed/
│       ├── river.hpp   # River API
│       └── queue.hpp   # SpscQueue API
├── src/
│   └── river.cpp       # River implementation
├── examples/
│   ├── river.cpp       # Echo server (simple vs River-backed)
│   ├── queue.cpp       # SpscQueue vs mutex+queue benchmark
│   └── client.cpp      # TCP echo client for throughput
└── CMakeLists.txt
```

## Performance

Riverbed is built for **predictable, low-overhead** transfer between one producer and one consumer. Throughput is dominated by the underlying I/O or workload; the primitives add minimal contention and no locking.

### TCP echo throughput

The `client_bench` program drives a TCP echo server with 100,000 round-trips of 1 KiB messages (send + receive). Total data moved is ~204.8 MB (100k × 1024 × 2). Typical result on a single connection:

| Metric        | Value        |
|---------------|--------------|
| Iterations    | 100,000      |
| Message size  | 1024 bytes   |
| Total bytes   | 2.048×10⁸ B  |
| Throughput    | ~153 MB/s    |

*Example: Apple Silicon, Release build with `-O3`. Run the server with `./build/riverbed_bench river` in one terminal and `./build/client_bench` in another.*

This reflects end-to-end TCP echo throughput. River’s role is to provide a bounded, lock-free buffer between `recv` and `send` so the server can stream bytes without extra allocations or copies. For higher throughput, scale out with more connections or use the queue for parallelism.

### SpscQueue vs mutex + std::queue

The `queue_bench` program compares `SpscQueue<int>` against a `std::queue<int>` guarded by a mutex and condition variable (1M items, one producer, one consumer). Lock-free SPSC typically shows significantly higher throughput and more consistent latency under contention; run `./build/queue_bench` on your machine for numbers.

### Why the design is efficient

1. **No locks** — producer and consumer never block each other; progress is guaranteed.
2. **Cache-line separation** — write and read indices are on different cache lines to reduce false sharing.
3. **Contiguous regions** — River exposes one contiguous chunk per reserve call, so I/O can use it directly.
4. **Power-of-two sizing** — wrap-around is a single mask instead of a branch or modulo.

## Design

### River: reserve / commit

The API is two-phase: reserve a region, use it, then commit. This keeps the buffer consistent without copying:

```
Producer                              Buffer (ring)                    Consumer
   |                                        |                               |
   | reserve_write()  ------------------->  [..............]                 |
   |   -> (ptr, len)                        ^ writable                       |
   |   write(ptr, len)                      |                                |
   | commit_write(len)  ---------------->  write += len                     |
   |                                        |                                |
   |                                        |  reserve_read()  <-------------|
   |                                        |  -> (ptr, len)   readable ^   |
   |                                        |                    read(ptr)   |
   |                                        |  commit_read(len) <-------------|
   |                                        read += len                      |
```

The ring is indexed with a mask (`pos & (capacity - 1)`). Reserve always returns at most one “chunk” (either to the end of the buffer or to the logical write/read head), so a single recv/send often uses one reserve/commit pair.

### SpscQueue: single producer, single consumer

Only one thread may call `push` and only one may call `pop`. This allows a single-producer single-consumer ring of items with two atomics (write index, read index) and acquire/release semantics so that the item written by `push` is visible to `pop` without a full sequential consistency barrier.

### Project structure (reference)

```
riverbed/
├── include/
│   └── Riverbed/
│       ├── river.hpp      # River API
│       └── queue.hpp      # SpscQueue API
├── src/
│   └── river.cpp          # River implementation
├── examples/
│   ├── river.cpp          # Echo server (simple | river)
│   ├── queue.cpp          # Queue benchmark
│   └── client.cpp         # Echo client
└── CMakeLists.txt
```

## Limitations

| Limitation | Explanation |
|------------|-------------|
| SPSC only | One producer and one consumer per River or SpscQueue; no MPMC |
| Power-of-two capacity | Required for fast mask-based indexing |
| Blocking when full/empty | Push/pop and reserve return “full”/“empty”; caller must spin, wait, or back off |
| No timeout or wait API | No built-in condition variable or blocking wait |
| Trivial types (queue) | SpscQueue requires trivially copyable and trivially destructible `T` |

### When NOT to use Riverbed

- You need multiple producers or multiple consumers (use an MPMC queue or other structure).
- You need blocking semantics (e.g. “block until a slot is free”); you can build this on top with a condition variable.
- You need dynamic capacity; both River and SpscQueue have fixed capacity at construction.

## Platform Support

| Platform | Status |
|----------|--------|
| macOS   | ✅ Supported |
| Linux   | ✅ Supported |
| Windows | ❌ Not tested (C++17 and POSIX sockets in examples) |

### Requirements

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.20+
- Examples that use sockets assume a POSIX environment (`sys/socket.h`, etc.)

## Building

```bash
# Standard build
cmake -S . -B build
cmake --build build -j

# With AddressSanitizer
cmake -S . -B build-asan -RIVER_ASAN=ON
cmake --build build-asan -j

# With ThreadSanitizer
cmake -S . -B build-tsan -RIVER_TSAN=ON
cmake --build build-tsan -j

# Run TCP echo benchmark (terminal 1: server, terminal 2: client)
./build/riverbed_bench river
./build/client_bench

# Run queue benchmark
./build/queue_bench
```

The CMake configuration uses `-O3 -Wall -Wextra -Wpedantic` by default for the library and benchmarks.

## References

- [1024cores](https://www.1024cores.net/) — Lock-free and wait-free algorithms
- Anthony Williams, *C++ Concurrency in Action* — Memory ordering and lock-free design
- Dmitry Vyukov, [Bounded MPMC queue](https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue) — Ring buffer and index semantics

## License

MIT License. See [LICENSE](LICENSE) for details.

---

**Riverbed** — Lock-free SPSC byte streams and message queues for C++.
