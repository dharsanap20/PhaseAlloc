# PhaseAlloc

**PhaseAlloc is a dynamically growing, phase-based arena allocator written in C.**

It provides fast sequential memory allocation for workloads where many objects are created during a phase of execution and can be released together.

---

# Download

## Latest Release

**[Download PhaseAlloc v1.1.0](https://github.com/dharsanap20/PhaseAlloc/releases/latest)**

The release package includes:

* Complete source code
* Examples
* Test suite
* Benchmarks
* Makefile
* MIT License

## Clone the Repository

To get the latest development version:

```bash
git clone https://github.com/dharsanap20/PhaseAlloc.git
cd PhaseAlloc
```

---

# Quick Start

## Requirements

* C11-compatible compiler
* Make
* GCC, Clang, or MinGW

### Windows — MinGW

Build:

```powershell
mingw32-make
```

Run tests:

```powershell
mingw32-make test
```

Run benchmarks:

```powershell
mingw32-make benchmark
mingw32-make benchmark-json
```

### Linux / macOS

Build:

```bash
make
```

Run tests:

```bash
make test
```

Run benchmarks:

```bash
make benchmark
make benchmark-json
```

---

# Basic Usage

A minimal PhaseAlloc program:

```c
#include "phasealloc.h"

int main(void)
{
    PhaseArena *arena = phase_arena_create(1024);

    if (arena == NULL)
    {
        return 1;
    }

    int *numbers = phase_arena_alloc(
        arena,
        5 * sizeof(int)
    );

    if (numbers == NULL)
    {
        phase_arena_destroy(arena);
        return 1;
    }

    for (int i = 0; i < 5; i++)
    {
        numbers[i] = (i + 1) * 10;
    }

    phase_arena_reset(arena);

    phase_arena_destroy(arena);

    return 0;
}
```

Typical lifecycle:

```text
Create
  ↓
Allocate
  ↓
Use
  ↓
Reset
  ↓
Reuse
  ↓
Destroy
```

---

# What Is PhaseAlloc?

PhaseAlloc is an **arena allocator** designed around the idea of **phase-based memory management**.

With traditional allocation:

```text
malloc → use → free
malloc → use → free
malloc → use → free
malloc → use → free
```

With PhaseAlloc:

```text
Create arena
     ↓
Allocate many objects
     ↓
Use objects
     ↓
Reset arena
     ↓
Reuse memory
```

Instead of individually freeing every object, an entire group of allocations can be reclaimed together.

This makes arena allocation useful when many objects have similar lifetimes.

### Good Workloads

PhaseAlloc is particularly suited to workloads such as:

* JSON parsing
* Compilers
* Game engines
* Simulations
* Temporary data processing
* Systems programming workloads

PhaseAlloc is **not intended to replace `malloc/free` for every workload**.

---

# How PhaseAlloc Works

PhaseAlloc stores allocations inside dynamically managed memory chunks.

A simplified arena:

```text
PhaseArena
    │
    ▼
┌──────────────────────────────┐
│           Chunk 1            │
│                              │
│ [ A ][ B ][ C ][ free space ]│
│                    ▲         │
│                  offset      │
└──────────────────────────────┘
```

When the current chunk runs out of space, PhaseAlloc creates another chunk:

```text
┌──────────────┐
│   Chunk 1    │
│ A B C D      │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Chunk 2    │
│ E F G        │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Chunk 3    │
│ H I          │
└──────────────┘
```

Previously allocated objects remain valid when additional chunks are created.

---

# Features

## Sequential Allocation

PhaseAlloc advances an internal offset to allocate memory sequentially.

```text
Before:

[ used memory ][ available memory ]
               ↑
             offset


After allocating 100 bytes:

[ used memory ][ 100 bytes ][ available ]
                            ↑
                          offset
```

This avoids the need to search for an individual free block.

---

## Dynamic Growth

The arena automatically creates additional chunks when the current chunk cannot satisfy an allocation.

Growth normally follows increasing chunk sizes:

```text
64 B → 128 B → 256 B → 512 B → ...
```

Large allocations can receive a chunk large enough to satisfy the request.

---

## Reset and Reuse

`phase_arena_reset()` resets allocation offsets while keeping the arena's chunks available for reuse.

```text
Before reset:

[ A ][ B ][ C ][ D ][ free ]
                    ↑
                  offset


After reset:

[          reusable memory          ]
↑
offset = 0
```

Previously returned pointers should not be used after a reset.

---

## Alignment

PhaseAlloc aligns allocations using the alignment requirements of `max_align_t`.

The test suite verifies alignment across multiple allocation sizes and repeated allocations.

---

## Overflow Protection

PhaseAlloc checks allocation calculations before performing pointer arithmetic.

This prevents integer overflow in size calculations from producing invalid memory ranges.

---

## Allocation Failure Handling

Failed allocations return `NULL`.

The allocator also cleans up newly created chunks if a subsequent allocation unexpectedly fails.

---

## Memory Statistics

PhaseAlloc provides statistics for observing allocator behavior:

```c
size_t phase_arena_get_current_memory(
    const PhaseArena *arena
);

size_t phase_arena_get_peak_memory(
    const PhaseArena *arena
);

size_t phase_arena_get_allocation_count(
    const PhaseArena *arena
);

size_t phase_arena_get_chunk_count(
    const PhaseArena *arena
);
```

These provide:

* Current memory usage
* Peak memory usage
* Successful allocation count
* Current chunk count

---

## Opaque API

The public API hides PhaseAlloc's internal chunk structures.

Users interact with the allocator through:

```c
PhaseArena *phase_arena_create(size_t capacity);

void *phase_arena_alloc(
    PhaseArena *arena,
    size_t size
);

void phase_arena_reset(
    PhaseArena *arena
);

void phase_arena_destroy(
    PhaseArena *arena
);
```

Internal implementation details remain private to the library.

---

# Public API

## Create

```c
PhaseArena *phase_arena_create(size_t capacity);
```

Creates an arena with the requested initial capacity.

Returns `NULL` if the requested capacity is zero or memory allocation fails.

---

## Allocate

```c
void *phase_arena_alloc(
    PhaseArena *arena,
    size_t size
);
```

Allocates memory from the arena.

Returns `NULL` for invalid requests or allocation failure.

---

## Reset

```c
void phase_arena_reset(
    PhaseArena *arena
);
```

Resets the arena so its memory can be reused.

---

## Destroy

```c
void phase_arena_destroy(
    PhaseArena *arena
);
```

Releases all memory owned by the arena.

Safe to call with `NULL`.

---

# JSON Workload

PhaseAlloc includes a JSON parsing workload to evaluate arena allocation against standard `malloc/free` in a realistic allocation-heavy workload.

Two equivalent parsing implementations are benchmarked:

```text
                 JSON Input
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
     malloc/free            PhaseAlloc
        parser                parser
          │                     │
          └──────────┬──────────┘
                     ▼
             Result verification
```

Both implementations construct equivalent JSON structures.

Correctness is verified using recursive checksums.

## JSON Workloads

The benchmark currently includes:

* Small JSON
* Medium JSON
* Large Nested JSON
* Allocation-Heavy JSON

The benchmark records:

* Parsing time
* `malloc` calls
* `realloc` calls
* PhaseAlloc allocation calls
* Peak PhaseAlloc memory
* Checksum correctness

---

# Benchmarks

The benchmark suite compares PhaseAlloc against standard `malloc/free`.

The main research question is:

> **For what types of workloads does an arena allocator actually outperform standard `malloc/free`, and why?**

The benchmark results below were measured on the development machine used for PhaseAlloc.

**These results are machine- and workload-dependent and should not be interpreted as universal performance guarantees.**

## How Improvement Is Calculated

The reported improvement is calculated as:

```text
Improvement =
((malloc/free time - PhaseAlloc time) / malloc/free time) × 100
```

A positive percentage means PhaseAlloc completed the benchmark faster.

---

# Allocation Benchmark Results

## Summary

| Workload        | malloc/free | PhaseAlloc | Improvement |
| --------------- | ----------: | ---------: | ----------: |
| 100,000 × 32 B  |  0.013237 s | 0.008472 s |  **36.00%** |
| 100,000 × 256 B |  0.086803 s | 0.063041 s |  **27.38%** |
| 10,000 × 4 KB   |  0.116933 s | 0.109241 s |   **6.58%** |
| 1,000 × 32 B    |  0.000103 s | 0.000069 s |  **33.00%** |

---

## A — 100,000 × 32-Byte Allocations

| Metric               | malloc/free |        PhaseAlloc |
| -------------------- | ----------: | ----------------: |
| Time                 |  0.013237 s |        0.008472 s |
| Relative improvement |           — | **36.00% faster** |
| Checksum             |        PASS |              PASS |

PhaseAlloc completed this workload approximately **36.00% faster** on the test machine.

---

## B — 100,000 × 256-Byte Allocations

| Metric               | malloc/free |        PhaseAlloc |
| -------------------- | ----------: | ----------------: |
| Time                 |  0.086803 s |        0.063041 s |
| Relative improvement |           — | **27.38% faster** |
| Checksum             |        PASS |              PASS |

PhaseAlloc completed this workload approximately **27.38% faster**.

---

## C — 10,000 × 4 KB Allocations

| Metric               | malloc/free |       PhaseAlloc |
| -------------------- | ----------: | ---------------: |
| Time                 |  0.116933 s |       0.109241 s |
| Relative improvement |           — | **6.58% faster** |
| Checksum             |        PASS |             PASS |

The performance difference was smaller for these larger allocations.

This demonstrates that arena allocation does not provide the same advantage for every allocation pattern.

---

## D — 1,000 × 32-Byte Allocations

| Metric               | malloc/free |        PhaseAlloc |
| -------------------- | ----------: | ----------------: |
| Time                 |  0.000103 s |        0.000069 s |
| Relative improvement |           — | **33.00% faster** |
| Checksum             |        PASS |              PASS |

PhaseAlloc completed this workload approximately **33.00% faster**.

---

# JSON Benchmark Results

## Summary

| Workload         | malloc/free | PhaseAlloc | Improvement | malloc/realloc ops | PhaseAlloc ops | Peak PhaseAlloc Memory |
| ---------------- | ----------: | ---------: | ----------: | -----------------: | -------------: | ---------------------: |
| Small            |  0.026709 s | 0.007221 s |  **72.97%** |                 22 |             16 |                  432 B |
| Medium           |  0.047419 s | 0.019106 s |  **59.71%** |                 73 |             51 |                1,815 B |
| Large Nested     |  0.057176 s | 0.021764 s |  **61.94%** |                 99 |             71 |                2,368 B |
| Allocation-Heavy |  3.523209 s | 1.184236 s |  **66.39%** |              6,006 |          4,514 |              136,336 B |

All JSON checksums passed for both implementations.

---

## Small JSON

| Metric                 | malloc/free |        PhaseAlloc |
| ---------------------- | ----------: | ----------------: |
| Parsing time           |  0.026709 s |        0.007221 s |
| Allocation operations  |          22 |                16 |
| Peak PhaseAlloc memory |           — |             432 B |
| Checksum               |        PASS |              PASS |
| Improvement            |           — | **72.97% faster** |

---

## Medium JSON

| Metric                 | malloc/free |        PhaseAlloc |
| ---------------------- | ----------: | ----------------: |
| Parsing time           |  0.047419 s |        0.019106 s |
| Allocation operations  |          73 |                51 |
| Peak PhaseAlloc memory |           — |           1,815 B |
| Checksum               |        PASS |              PASS |
| Improvement            |           — | **59.71% faster** |

---

## Large Nested JSON

| Metric                 | malloc/free |        PhaseAlloc |
| ---------------------- | ----------: | ----------------: |
| Parsing time           |  0.057176 s |        0.021764 s |
| Allocation operations  |          99 |                71 |
| Peak PhaseAlloc memory |           — |           2,368 B |
| Checksum               |        PASS |              PASS |
| Improvement            |           — | **61.94% faster** |

---

## Allocation-Heavy JSON

| Metric                 | malloc/free |        PhaseAlloc |
| ---------------------- | ----------: | ----------------: |
| Parsing time           |  3.523209 s |        1.184236 s |
| Allocation operations  |       6,006 |             4,514 |
| Peak PhaseAlloc memory |           — |         136,336 B |
| Checksum               |        PASS |              PASS |
| Improvement            |           — | **66.39% faster** |

---

# Benchmark Interpretation

The benchmarks show that PhaseAlloc can provide significant performance improvements for some allocation-heavy, phase-based workloads.

The results also demonstrate why an arena allocator should not simply be described as "faster than `malloc/free`."

In these tests:

* 32-byte allocation workloads showed improvements of approximately **33–36%**.
* 256-byte allocations showed an improvement of **27.38%**.
* 4 KB allocations showed a smaller improvement of **6.58%**.
* JSON workloads showed improvements ranging from **59.71% to 72.97%**.
* The Allocation-Heavy JSON workload showed a **66.39%** improvement.

The advantage depends on the workload's allocation pattern, object lifetime, allocation size, and the amount of allocator overhead involved.

The benchmark suite is intended to investigate **when** PhaseAlloc helps rather than claim that it is always faster.

For meaningful comparisons, run the benchmarks on your own system.

---

# Peak Memory Tracking

PhaseAlloc tracks its own current and peak memory usage.

For example:

```text
Current memory
      ↓
Memory currently used by PhaseAlloc allocations

Peak memory
      ↓
Highest PhaseAlloc memory usage reached
```

The current benchmark suite does **not** instrument the internal memory usage of the system `malloc/free` implementation.

Therefore, the JSON benchmark reports peak memory for **PhaseAlloc only**, rather than presenting an unsupported apples-to-apples peak-memory comparison.

---

# Testing

PhaseAlloc includes a regression test suite covering:

* Basic allocation
* Alignment
* Edge cases
* Overflow protection
* Reset and reuse
* Destruction
* Dynamic growth
* Peak memory tracking
* API contracts
* Allocation statistics
* JSON parsing
* PhaseAlloc JSON parsing

Run:

```bash
make test
```

Windows with MinGW:

```powershell
mingw32-make test
```

The current test suite passes without compiler warnings using:

```text
-Wall -Wextra -std=c11
```

---

# Project Structure

```text
PhaseAlloc/

├── include/
│   └── phasealloc.h
│
├── src/
│   ├── phasealloc.c
│   ├── phasealloc_internal.h
│   ├── json.c
│   ├── json.h
│   └── json_phase.c
│
├── examples/
│   ├── basic.c
│   └── json_phase.c
│
├── tests/
│   ├── test_basic.c
│   ├── test_alignment.c
│   ├── test_edge_cases.c
│   ├── test_overflow.c
│   ├── test_reset.c
│   ├── test_destroy.c
│   ├── test_growth.c
│   ├── test_peak.c
│   ├── test_api_contracts.c
│   ├── test_stats.c
│   ├── test_json.c
│   └── test_json_phase.c
│
├── benchmarks/
│   ├── benchmark.c
│   └── benchmark_json.c
│
├── Makefile
├── README.md
├── LICENSE
└── .gitignore
```

---

# Development Status

| Stage                                       | Status         |
| ------------------------------------------- | -------------- |
| Stage 1 — Basic Arena                       | ✅ Complete     |
| Stage 2 — Robustness & Correctness          | ✅ Complete     |
| Stage 3 — Dynamic Growth                    | ✅ Complete     |
| Stage 4 — Performance & Benchmarking        | ✅ Complete     |
| Stage 5 — JSON Workload Integration         | ✅ Complete     |
| Stage 6 — Production-Quality Library        | ✅ Complete     |
| Stage 7 — Open Source & External Validation | 🚧 In Progress |

---

# Limitations

PhaseAlloc works best when many objects share similar lifetimes.

Good fits include:

* Parsers
* Compilers
* Game engines
* Simulations
* Temporary data processing

It is less suitable when applications require frequent individual object deletion.

PhaseAlloc does not provide individual `free()` operations for allocations made inside the arena.

The included JSON parser is a benchmark workload and is not intended to replace established production JSON libraries.

Benchmark results vary between systems.

---

# Research Question

PhaseAlloc explores:

> **For what types of workloads does an arena allocator actually outperform standard `malloc/free`, and why?**

The project uses controlled allocation experiments and a real JSON parsing workload to investigate the performance characteristics of arena allocation.

---

# License

PhaseAlloc is released under the MIT License.

See `LICENSE` for the complete license text.
