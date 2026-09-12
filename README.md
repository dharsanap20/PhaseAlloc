# PhaseAlloc

PhaseAlloc is a phase-based arena allocator written in C.

It provides a simple way to allocate multiple objects from an arena and reclaim them together by resetting the arena.

Unlike a fixed-size arena, PhaseAlloc can automatically grow when the current memory region does not have enough space for an allocation.

---

## What is an Arena Allocator?

An arena allocator obtains a large block of memory and uses that block to satisfy smaller allocation requests.

Instead of individually freeing every allocation, an arena can reset its allocation offset and reuse the memory for another phase of work.

The basic allocation process is:

1. Create an arena with a specified initial capacity.
2. Allocate memory from the arena.
3. Use the allocated memory.
4. Reset the arena when the allocations are no longer needed.
5. Reuse the arena for another phase of work.
6. Destroy the arena when it is no longer needed.

The key idea is that individual allocations do not need to be freed one at a time.

For example, instead of:

```text
allocate A → free A

allocate B → free B

allocate C → free C
```

an arena can do:

```text
allocate A

allocate B

allocate C

     ↓

   reset

     ↓

all allocations become reusable
```

---

## How PhaseAlloc Works

PhaseAlloc uses a chunk-based arena.

Each `PhaseChunk` contains:

* `buffer` — pointer to the chunk's allocated memory.
* `capacity` — total size of the chunk in bytes.
* `offset` — current position used to determine where the next allocation will begin.
* `next` — pointer to the next chunk.

The `PhaseArena` keeps track of:

* `first_chunk` — the first chunk in the arena.
* `current_chunk` — the chunk currently being used for allocation.

The arena also maintains:

* `buffer` — pointer to the current chunk's memory.
* `capacity` — capacity of the current chunk.
* `offset` — current allocation position within the current chunk.
* `peak_offset` — highest total amount of arena memory used during the current lifetime of the arena.

### The Offset

The `offset` represents how much of the current chunk has been used.

For example, if an arena has a capacity of 1024 bytes:

```text
Arena

┌──────────────────────────────────────────┐
│ Used Memory │ Available Memory           │
└──────────────────────────────────────────┘
              ↑
            offset
```

If 200 bytes have been allocated:

```text
Arena

┌────────────────────┬─────────────────────┐
│    Used Memory     │   Available Memory  │
│      200 bytes     │     824 bytes       │
└────────────────────┴─────────────────────┘
                    ↑
                  offset
```

After another 100-byte allocation:

```text
Arena

┌──────────────────────────────┬────────────┐
│        Used Memory           │ Available  │
│          300 bytes           │  724 bytes │
└──────────────────────────────┴────────────┘
                              ↑
                            offset
```

The allocator does not need to search for a free block.

It simply uses the current offset to determine where the next allocation begins.

---

## Allocation

When memory is requested, PhaseAlloc:

1. Checks that the arena and allocation request are valid.
2. Checks the current chunk for available space.
3. Aligns the allocation to an 8-byte boundary.
4. Returns a pointer to the available memory.
5. Advances the chunk's offset.
6. Updates the arena's allocation state and peak memory usage.

Conceptually:

```text
Current Chunk

┌──────────────────────────────────────────┐
│ Allocated │       Available Memory       │
└──────────────────────────────────────────┘
            ↑
          offset
```

The next allocation begins at the updated offset.

---

## Dynamic Growth

A fixed-size arena eventually runs out of space.

PhaseAlloc handles this by using multiple chunks.

The basic structure is:

```text
PhaseArena

│
├── first_chunk
│       │
│       ▼
│   ┌─────────────┐
│   │   Chunk 1   │
│   │             │
│   └──────┬──────┘
│          │
│          ▼
│   ┌─────────────┐
│   │   Chunk 2   │
│   │             │
│   └──────┬──────┘
│          │
│          ▼
│   ┌─────────────┐
│   │   Chunk 3   │
│   │             │
│   └─────────────┘
│
└── current_chunk
```

When the current chunk does not have enough space, PhaseAlloc searches existing chunks.

If an existing chunk can satisfy the allocation, it is reused.

If no existing chunk can satisfy the allocation, PhaseAlloc creates a new chunk.

### Growth Strategy

PhaseAlloc uses geometric growth for new chunks.

For example:

```text
Initial chunk

     64 bytes

        ↓

New chunk

    128 bytes

        ↓

New chunk

    256 bytes

        ↓

New chunk

    512 bytes

        ↓

New chunk

   1024 bytes
```

The next chunk normally has twice the capacity of the previous chunk.

If the requested allocation is larger than the calculated growth size, the new chunk is instead made large enough for the request.

For example:

```text
Current chunk:        64 bytes

Requested allocation: 1000 bytes

New chunk:            1000+ bytes
```

This allows PhaseAlloc to handle allocations larger than its original capacity.

---

## Why Use Chunks?

PhaseAlloc uses chunks instead of growing a single buffer with `realloc()`.

The reason is that `realloc()` can move the existing memory to a different address.

If PhaseAlloc returned pointers into that memory, moving it could invalidate previously returned pointers.

With chunks:

```text
Chunk 1 → Chunk 2 → Chunk 3
```

old chunks remain at their original addresses.

Therefore, allocations made before a growth event remain valid.

Conceptually:

```text
Before growth:

Chunk 1

┌───────────────────┐
│ Allocation A      │
│ Allocation B      │
└───────────────────┘


After growth:

Chunk 1                Chunk 2

┌───────────────────┐  ┌───────────────────┐
│ Allocation A      │  │ Allocation C      │
│ Allocation B      │  │ Allocation D      │
└───────────────────┘  └───────────────────┘
        ↑
   remains valid
```

---

## Reset and Reuse

When the arena is reset, PhaseAlloc resets the offset of every existing chunk to zero.

The chunks themselves are **not freed**.

For example:

```text
Before reset:

Chunk 1 → Chunk 2 → Chunk 3

  used       used       used
```

After reset:

```text
Chunk 1 → Chunk 2 → Chunk 3

  free       free       free
   ↑
current chunk
```

The arena returns to the first chunk and can reuse the existing memory.

This means a reset does not require new allocations from the system.

The underlying memory remains owned by the arena until `phase_arena_destroy()` is called.

The `peak_offset` value tracks the highest amount of arena memory used and is useful for analyzing memory behavior during workloads.

---

## Destroy

When the arena is destroyed, PhaseAlloc walks through the chunk list and frees:

1. Each chunk's memory buffer.
2. Each `PhaseChunk` structure.

After destruction, the arena's state is cleared.

```text
buffer          → NULL

capacity        → 0

offset          → 0

peak_offset     → 0

first_chunk     → NULL

current_chunk   → NULL
```

The destroy operation is also safe to call on an already-destroyed arena.

---

## Current Features

* Arena creation with a configurable initial capacity
* Sequential memory allocation
* Dynamic chunk-based growth
* Geometric growth strategy
* Preservation of existing allocations during growth
* Reuse of existing chunks
* 8-byte pointer alignment
* Bounds checking
* Allocation failure handling
* Zero-byte allocation protection
* `NULL` arena safety
* Overflow-safe growth calculations
* Arena reset for memory reuse
* Peak arena memory tracking
* Repeated reset and reuse
* Safe arena destruction
* Safe repeated destruction
* Automated tests
* Stress testing
* Basic usage example
* Repeatable performance benchmark suite
* Multiple benchmark workloads
* Write/read/checksum verification during benchmarking
* Real JSON parsing workload
* Equivalent `malloc/free` and PhaseAlloc JSON implementations
* JSON correctness verification
* JSON allocation instrumentation
* JSON peak-memory measurement
* Multiple JSON workload characteristics
* Recursive checksum verification

---

## Project Structure

```text
PhaseAlloc/

├── include/
│   └── phasealloc.h
│
├── src/
│   ├── phasealloc.c
│   ├── phasealloc_internal.h
│   ├── json.h
│   ├── json.c
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
│   ├── test_json.c
│   └── test_json_phase.c
│
├── examples/
│   ├── basic.c
│   └── json_phase.c
│
├── benchmarks/
│   ├── benchmark.c
│   └── benchmark_json.c
│
├── README.md
├── LICENSE
└── .gitignore
```

`phasealloc.h` contains the public API.

`phasealloc.c` contains the allocator implementation.

`phasealloc_internal.h` contains implementation-specific definitions for the internal chunk system.

The internal header is kept separate from the public `include/` directory because users of the library do not need direct access to the chunk implementation.

`json.h` contains the JSON value structures and JSON parser API.

`json.c` contains the standard `malloc/free` JSON parser.

`json_phase.c` contains the equivalent JSON parser using PhaseAlloc.

`benchmark.c` contains the controlled allocation performance benchmark.

`benchmark_json.c` contains the real JSON workload benchmark.

---

# API

## `phase_arena_create`

```c
PhaseArena phase_arena_create(size_t capacity);
```

Creates an arena with the requested initial capacity in bytes.

The requested capacity becomes the capacity of the first chunk.

If the underlying memory allocation fails, the returned arena has a `NULL` buffer.

Example:

```c
PhaseArena arena = phase_arena_create(1024);
```

---

## `phase_arena_alloc`

```c
void *phase_arena_alloc(PhaseArena *arena, size_t size);
```

Allocates a block of memory from the arena and returns a pointer to that memory.

The allocation is aligned to an 8-byte boundary.

If the current chunk does not have enough space, PhaseAlloc searches existing chunks.

If no existing chunk can satisfy the request, a new chunk is created automatically.

Returns `NULL` if:

* The arena is `NULL`.
* The arena has no valid current chunk.
* The requested size is zero.
* A required memory allocation fails.
* The requested allocation cannot be safely represented.

---

## `phase_arena_reset`

```c
void phase_arena_reset(PhaseArena *arena);
```

Resets the allocation state of all existing chunks.

The chunks and their underlying memory are retained for reuse.

After reset, the arena returns to its first chunk.

---

## `phase_arena_destroy`

```c
void phase_arena_destroy(PhaseArena *arena);
```

Releases all memory owned by the arena.

Every chunk and its buffer are freed.

After destruction:

```text
buffer          → NULL
capacity        → 0
offset          → 0
peak_offset     → 0
first_chunk     → NULL
current_chunk   → NULL
```

The function safely handles:

* A `NULL` arena.
* An already-destroyed arena.

---

# Basic Usage

```c
#include <stdio.h>
#include "phasealloc.h"

int main(void)
{
    PhaseArena arena = phase_arena_create(1024);

    if (arena.buffer == NULL)
    {
        return 1;
    }

    int *numbers = phase_arena_alloc(&arena, 5 * sizeof(int));

    if (numbers == NULL)
    {
        phase_arena_destroy(&arena);
        return 1;
    }

    for (int i = 0; i < 5; i++)
    {
        numbers[i] = (i + 1) * 10;
    }

    phase_arena_reset(&arena);
    phase_arena_destroy(&arena);

    return 0;
}
```

The basic lifecycle is:

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

If more memory is required during a phase:

```text
Create

  ↓

Allocate

  ↓

Current chunk becomes full

  ↓

Create new chunk

  ↓

Continue allocating

  ↓

Reset

  ↓

Reuse existing chunks

  ↓

Destroy
```

---

# Testing

PhaseAlloc includes an automated test suite covering the allocator's core functionality, correctness, failure handling, dynamic growth, reset/reuse behavior, stress behavior, and JSON integration.

## `test_basic.c`

Tests:

* Arena creation
* Memory allocation
* Writing to allocated memory
* Basic allocation behavior
* Allocation larger than the initial capacity
* Automatic arena growth
* Reset
* Destruction

## `test_alignment.c`

Tests:

* 8-byte pointer alignment
* Multiple differently sized allocations

## `test_edge_cases.c`

Tests:

* Zero-byte allocations
* Allocations larger than the current chunk
* Exact-capacity allocations followed by growth
* Multiple allocations
* `NULL` arena handling

## `test_overflow.c`

Tests:

* Extremely large allocation requests
* `SIZE_MAX` allocation requests
* Failed allocations preserving the arena's previous state
* Continued arena operation after a failed large allocation

## `test_reset.c`

Tests:

* Resetting the allocation offset
* Reusing memory after reset
* Repeated reset and reuse cycles
* Preservation of the arena buffer
* Preservation of arena capacity

## `test_destroy.c`

Tests:

* Clearing the arena buffer after destruction
* Resetting capacity and offset
* Safe repeated destruction
* Safe destruction of a `NULL` arena
* Creating a new arena after destruction

## `test_growth.c`

Tests:

* Basic dynamic growth
* Multiple growth events
* Allocations larger than the current chunk
* Preservation of old allocations after growth
* Reset after growth
* Reuse after reset
* Allocation failure cases
* Stress testing across many allocations

## `test_json.c`

Tests the standard `malloc/free` JSON parser for:

* JSON parsing
* Numbers
* Strings
* Booleans
* Null values
* Arrays
* Objects
* Nested structures
* Correct parsed values

## `test_json_phase.c`

Tests the PhaseAlloc JSON parser for:

* JSON parsing
* Numbers
* Strings
* Booleans
* Null values
* Arrays
* Objects
* Nested structures
* Correct parsed values
* Arena-based allocation

---

# Running the Tests

Each test can be compiled against the PhaseAlloc implementation.

For example:

```bash
gcc src/phasealloc.c tests/test_basic.c -Iinclude -o test_basic
./test_basic
```

On Windows:

```powershell
gcc src/phasealloc.c tests/test_basic.c -Iinclude -o test_basic.exe
.\test_basic.exe
```

For example:

```powershell
gcc src/phasealloc.c tests/test_growth.c -Iinclude -o test_growth.exe
.\test_growth.exe
```

JSON tests require the JSON parser sources:

```powershell
gcc src/phasealloc.c src/json.c src/json_phase.c tests/test_json.c -Iinclude -Isrc -o test_json.exe
.\test_json.exe
```

The current test suite covers:

* Basic functionality
* Correctness
* Alignment
* Dynamic growth
* Failure handling
* Overflow handling
* Reset/reuse behavior
* Destruction behavior
* Stress behavior
* JSON parsing
* PhaseAlloc JSON integration

---

# Performance Benchmarking

PhaseAlloc includes repeatable benchmark suites comparing its allocation strategy against standard `malloc/free`.

The controlled benchmark performs the same memory-use operations for both implementations:

1. Allocate memory.
2. Write data to the allocated memory.
3. Read the data back.
4. Maintain a checksum.
5. Reclaim the memory.

For `malloc/free`, each allocation is individually released with `free()`.

For PhaseAlloc, allocations are reclaimed together using `phase_arena_reset()`.

## Benchmark Workloads

The controlled benchmark suite tests four workloads:

| Workload | Allocations | Allocation Size |
| -------- | ----------: | --------------: |
| A        |     100,000 |        32 bytes |
| B        |     100,000 |       256 bytes |
| C        |      10,000 |            4 KB |
| D        |       1,000 |        32 bytes |

Each workload is repeated for 10 iterations.

## Benchmark Results

The following results are from a benchmark run on the Windows development environment:

| Workload            | `malloc/free` | PhaseAlloc | PhaseAlloc / malloc | Improvement |
| ------------------- | ------------: | ---------: | ------------------: | ----------: |
| A — 100,000 × 32 B  |    0.013458 s | 0.010330 s |              76.76% |  **23.24%** |
| B — 100,000 × 256 B |    0.084244 s | 0.058819 s |              69.82% |  **30.18%** |
| C — 10,000 × 4 KB   |    0.119481 s | 0.099230 s |              83.05% |  **16.95%** |
| D — 1,000 × 32 B    |    0.000126 s | 0.000109 s |              86.77% |  **13.23%** |

All four workloads passed checksum verification.

### Interpreting the Results

`PhaseAlloc / malloc` represents how much time PhaseAlloc used relative to the `malloc/free` baseline.

For example:

```text
PhaseAlloc / malloc = 69.82%
```

means PhaseAlloc took 69.82% as much time as `malloc/free`.

Therefore:

```text
100% - 69.82% = 30.18%
```

So PhaseAlloc used **30.18% less measured time** for that workload.

The current controlled benchmark run showed PhaseAlloc with lower measured total workload time in all four tested workloads.

However, these results are specific to the tested workloads and the current Windows development environment. They do **not** establish that PhaseAlloc is universally faster than `malloc/free`.

The purpose of the benchmark is to collect evidence about which workloads benefit from arena allocation rather than assuming that arena allocation is always faster.

### Benchmark Limitations

The controlled benchmark:

* Uses Windows-specific timing behavior.
* Tests a limited set of allocation workloads.
* Represents synthetic allocation patterns rather than a complete real-world application.
* Has not yet been tested across multiple operating systems.
* Does not directly measure memory-management behavior inside a real application.

The JSON benchmark provides a second level of testing using a practical parsing workload.

---

# Real Workload Benchmark — JSON

Stage 5 extends the benchmark methodology to a real JSON parsing workload.

The goal is to compare two equivalent implementations:

```text
JSON input
    │
    ├── malloc/free parser
    │
    └── PhaseAlloc parser
```

Both parsers construct equivalent JSON structures and are then processed using the same recursive checksum function.

This allows the benchmark to measure the effect of the memory-management strategy while keeping the JSON workload itself equivalent.

## JSON Workloads

Four JSON workloads are currently tested:

| Workload              | Description                                  |
| --------------------- | -------------------------------------------- |
| Small JSON            | Small object containing basic values         |
| Medium JSON           | Object containing multiple arrays and values |
| Large Nested JSON     | Nested objects and arrays                    |
| Allocation-Heavy JSON | JSON document containing 500 small objects   |

Each workload is parsed 10,000 times.

The benchmark records:

* `malloc` calls
* `realloc` calls
* Total `malloc/realloc` operations
* Average `malloc/realloc` operations per parse
* PhaseAlloc allocation calls
* Average PhaseAlloc allocations per parse
* Peak PhaseAlloc memory per parse
* Total parsing time
* Relative performance
* Recursive checksum

## JSON Benchmark Results

The following results are from the current benchmark run:

| Workload              | Avg. malloc/realloc operations per parse | Avg. PhaseAlloc allocations per parse | Peak PhaseAlloc memory per parse | Improvement |
| --------------------- | ---------------------------------------: | ------------------------------------: | -------------------------------: | ----------: |
| Small JSON            |                                    22.00 |                                 16.00 |                        400 bytes |  **62.50%** |
| Medium JSON           |                                    73.00 |                                 51.00 |                      1,767 bytes |  **58.14%** |
| Large Nested JSON     |                                    99.00 |                                 71.00 |                      2,264 bytes |  **57.14%** |
| Allocation-Heavy JSON |                                 6,006.00 |                              4,514.00 |                    128,328 bytes |  **64.49%** |

All four workloads produced matching checksums.

### JSON Timing Results

```text
Small JSON

malloc/free time: 0.024000 seconds
PhaseAlloc time:  0.009000 seconds
Relative improvement: 62.50%


Medium JSON

malloc/free time: 0.043000 seconds
PhaseAlloc time:  0.018000 seconds
Relative improvement: 58.14%


Large Nested JSON

malloc/free time: 0.049000 seconds
PhaseAlloc time: 0.021000 seconds
Relative improvement: 57.14%


Allocation-Heavy JSON

malloc/free time: 3.177000 seconds
PhaseAlloc time: 1.128000 seconds
Relative improvement: 64.49%
```

These measurements are from one benchmark run on the development system. Exact timings and percentages can vary between runs and machines.

### Allocation Activity

The allocation-heavy workload produced:

```text
malloc/free:

6,006 malloc/realloc operations per parse

PhaseAlloc:

4,514 PhaseAlloc allocation calls per parse
```

The allocation-heavy workload also produced the largest measured improvement in this benchmark run:

```text
64.49%
```

This workload creates many short-lived JSON objects, making it a useful test of PhaseAlloc's phase-based memory-management model.

### Why PhaseAlloc Benefits This Workload

The JSON parser creates many objects that belong to the same parsed document.

During parsing, objects such as:

```text
JSON objects
JSON arrays
JSON strings
JSON numbers
JSON booleans
JSON null values
```

are created and remain useful for approximately the lifetime of the parsed document.

With standard `malloc/free`, these allocations are individually managed and eventually individually released.

With PhaseAlloc, the allocations can remain inside the same arena and the entire collection can be reclaimed together.

Conceptually:

```text
malloc/free

allocate object A
allocate object B
allocate object C
allocate object D

       ↓

free A
free B
free C
free D
```

versus:

```text
PhaseAlloc

allocate A
allocate B
allocate C
allocate D

       ↓

destroy/reset arena

       ↓

all allocations reclaimed together
```

This matches the fundamental lifetime pattern that arena allocators are designed for.

### Important Interpretation

The benchmark results support the hypothesis that PhaseAlloc can provide an advantage when:

* Many allocations are performed.
* Allocations have similar lifetimes.
* Individual objects do not need to be freed independently.
* The workload can reclaim a group of objects together.
* Allocation overhead is a meaningful part of the workload.

The results do not show that PhaseAlloc is always faster.

Instead, they provide evidence for the specific research question:

> **For what types of workloads does an arena allocator actually outperform standard `malloc/free`, and why?**

### Memory Measurement

PhaseAlloc records `peak_offset` to measure the highest amount of arena memory used during a JSON parse.

The current measurements represent the amount of memory accounted for as used by the arena across its chunks.

They do **not** represent the total memory reserved from the operating system, and they are not a direct measurement of the malloc parser's peak heap usage.

Therefore, the current JSON benchmark should not be used to claim that PhaseAlloc uses less memory than `malloc/free`.

### Current JSON Benchmark Limitations

The current JSON implementation is intentionally focused on evaluating the allocator.

Some implementation details remain candidates for future improvement:

* Object keys currently use the JSON string parser, which creates a temporary `JsonValue` structure.
* Growing JSON arrays allocates new pointer storage inside the arena.
* Previous pointer-storage allocations remain occupied because the arena does not individually free allocations.
* The current parser is a benchmark workload rather than a complete production JSON library.
* JSON parsing does not yet support every feature of the JSON specification.

These limitations are documented rather than hidden because they provide potential areas for future allocator and workload optimization.

---

# Design Goal

PhaseAlloc is designed around the idea of **phase-based memory management**.

A phase is a period of computation during which a group of objects is needed together.

Instead of individually releasing each object, the arena can reclaim the entire group by resetting its allocation state.

This makes the allocator particularly suited to workloads where many objects have similar lifetimes and can be discarded together.

For example, a program could have:

```text
Phase 1

├── allocate object A
├── allocate object B
├── allocate object C
└── finish phase

         ↓

       reset

         ↓

Phase 2

├── reuse memory
├── allocate object D
├── allocate object E
└── finish phase
```

Dynamic growth allows a phase to exceed the arena's initial capacity without invalidating allocations that have already been returned.

The JSON workload provides a practical example of this model because objects created while parsing a document generally share the lifetime of that document.

---

# Research Question

PhaseAlloc is being developed around the following research question:

> **For what types of workloads does an arena allocator actually outperform standard `malloc/free`, and why?**

The Stage 4 benchmark suite provides a controlled comparison using several allocation patterns.

Stage 5 extends the investigation to a real JSON parsing workload.

The results from the current JSON benchmark show lower measured parsing time for PhaseAlloc across all four tested JSON workloads, with the largest improvement occurring in the allocation-heavy workload.

The evidence suggests that PhaseAlloc's advantage becomes particularly relevant when a workload performs many allocations whose lifetimes are naturally grouped together.

---

# Development Stages

### Stage 1 — Basic Arena

Completed:

* Fixed-size arena
* Sequential allocation
* Reset
* Destroy
* Basic tests
* Basic usage example

### Stage 2 — Robustness & Correctness

Completed:

* Alignment
* Edge-case handling
* Overflow checks
* Allocation failure handling
* Additional correctness tests

### Stage 3 — Dynamic Growth

Completed:

* Chunk-based architecture
* Automatic growth
* Geometric growth
* Large allocation support
* Preservation of existing allocations
* Chunk reuse after reset

### Stage 4 — Performance & Benchmarking

Completed:

* `malloc/free` baseline
* PhaseAlloc comparison
* Write/read/checksum workload
* Multiple allocation workloads
* Performance calculations
* Throughput analysis
* Repeatable benchmark suite

### Stage 5 — Real Workload / JSON Integration

**Completed.**

Completed work:

* Integrated PhaseAlloc into a JSON parsing workload.
* Built an equivalent `malloc/free` JSON parser.
* Built an equivalent PhaseAlloc JSON parser.
* Verified equivalent parsing results.
* Added recursive checksum verification.
* Benchmarked repeated JSON parsing.
* Tested small JSON workloads.
* Tested medium JSON workloads.
* Tested large nested JSON workloads.
* Tested allocation-heavy JSON workloads.
* Instrumented `malloc` and `realloc` activity.
* Instrumented PhaseAlloc allocation calls.
* Added peak PhaseAlloc memory tracking.
* Compared allocation activity between the two approaches.
* Analyzed why arena allocation benefits the tested workload.
* Documented benchmark limitations.

### Stage 6 — Production-Quality Library

Planned work:

* Improve the public API
* Review API naming and consistency
* Improve internal organization
* Improve error handling
* Improve documentation
* Review memory-safety behavior
* Improve portability
* Expand testing
* Add more allocator diagnostics
* Review performance opportunities without sacrificing correctness

### Stage 7 — Open Source & External Validation

Future work:

* Prepare a polished public release
* Improve README and documentation
* Add usage examples
* Add cross-platform testing
* Publish benchmark methodology
* Encourage external testing
* Collect real usage feedback
* Investigate real-world workloads
* Potentially publish PhaseAlloc as a reusable C library

---

# Current Status

**Stage 5 — Real Workload / JSON Integration: Complete**

PhaseAlloc currently has:

* A dynamically growing chunk-based arena allocator
* 8-byte alignment
* Overflow and bounds checks
* Allocation failure handling
* Automated correctness tests
* Stress testing
* Reset and reuse support
* Safe destruction
* Peak arena memory tracking
* A repeatable multi-workload benchmark suite
* Measured comparisons against standard `malloc/free`
* A real JSON parsing workload
* Equivalent malloc/free and PhaseAlloc JSON implementations
* Recursive checksum verification
* Allocation instrumentation
* Peak PhaseAlloc memory measurement
* Workload-specific performance analysis

The next development milestone is:

**Stage 6 — Production-Quality Library**

---

# License

See `LICENSE` for the terms under which PhaseAlloc is distributed.
