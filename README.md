# PhaseAlloc

PhaseAlloc is a dynamically growing, phase-based arena allocator written in C.

It is designed for workloads where many objects are created during a phase of computation and can be reclaimed together. Instead of individually freeing each allocation, the arena can reset its allocation state and reuse its existing memory.

PhaseAlloc also supports dynamic growth through linked memory chunks, allowing allocations to exceed the arena's initial capacity without moving previously allocated objects.

The project is built around the research question:

> **For what types of workloads does an arena allocator actually outperform standard `malloc/free`, and why?**

The project investigates this through controlled allocation benchmarks and a real JSON parsing workload.

---

## How PhaseAlloc Works

At its core, PhaseAlloc manages a region of memory and moves an allocation offset forward as memory is requested.

```text
PhaseArena
    │
    ▼
┌────────────────────────────────────────┐
│                Chunk 1                  │
│                                        │
│ [ Allocation A ][ Allocation B ][ free │
│                                  space ]│
│                              ▲         │
│                            offset      │
└────────────────────────────────────────┘
```

Each chunk contains:

* A memory buffer
* A capacity
* An allocation offset
* A pointer to the next chunk

The public `PhaseArena` type is opaque, so these implementation details remain private to the library.

### Allocation

When memory is requested, PhaseAlloc:

```text
phase_arena_alloc()
        │
        ▼
   Validate request
        │
        ▼
   Find available space
        │
        ▼
   Align allocation
        │
        ▼
   Return pointer
        │
        ▼
   Advance offset
        │
        ▼
   Update statistics
```

Allocations are sequential within a chunk. There is no need to search for individually freed blocks because PhaseAlloc does not support individual deallocation.

Allocations are aligned to the platform's `_Alignof(max_align_t)` requirement.

---

## Dynamic Growth

A fixed-size arena becomes limiting when the workload requires more memory than the initial capacity.

PhaseAlloc solves this by linking additional chunks together:

```text
PhaseArena
    │
    ▼
┌──────────────┐
│   Chunk 1    │
│    64 B      │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Chunk 2    │
│   128 B      │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Chunk 3    │
│   256 B      │
└──────────────┘
```

Chunks normally grow geometrically:

```text
64 B → 128 B → 256 B → 512 B → ...
```

If a requested allocation is larger than the calculated growth size, PhaseAlloc creates a chunk large enough to satisfy the request.

Using separate chunks means previously returned pointers remain valid when the arena grows.

---

## Reset and Reuse

One of the main advantages of an arena allocator is that many allocations can be reclaimed together.

Before reset:

```text
┌────────────────────────────────────────┐
│ [ A ][ B ][ C ][ D ][   free space   ]│
│                         ▲             │
│                       offset          │
└────────────────────────────────────────┘
```

After reset:

```text
┌────────────────────────────────────────┐
│ [              reusable space        ]│
│  ▲                                     │
│ offset = 0                             │
└────────────────────────────────────────┘
```

`phase_arena_reset()` resets the allocation offsets of all existing chunks without freeing their memory.

This allows the same arena to be reused for another phase of computation.

Previously returned pointers should no longer be used after reset.

---

## Public API

`include/phasealloc.h` contains the public allocator interface.

### Create

```c
PhaseArena *phase_arena_create(size_t capacity);
```

Creates an arena with the specified initial capacity.

Returns `NULL` if the capacity is zero or required memory allocation fails.

### Allocate

```c
void *phase_arena_alloc(PhaseArena *arena, size_t size);
```

Allocates memory from the arena.

Returns `NULL` for invalid arenas, zero-size requests, or allocation failures.

### Reset

```c
void phase_arena_reset(PhaseArena *arena);
```

Resets the arena so its existing memory can be reused.

### Destroy

```c
void phase_arena_destroy(PhaseArena *arena);
```

Releases all memory owned by the arena.

Safe to call with `NULL`.

### Diagnostics

```c
size_t phase_arena_get_current_memory(const PhaseArena *arena);
size_t phase_arena_get_peak_memory(const PhaseArena *arena);
size_t phase_arena_get_allocation_count(const PhaseArena *arena);
size_t phase_arena_get_chunk_count(const PhaseArena *arena);
```

These functions provide allocator statistics without exposing internal implementation details.

* `current_memory` — current memory accounted as used.
* `peak_memory` — highest recorded memory usage.
* `allocation_count` — number of successful allocations.
* `chunk_count` — number of chunks currently owned by the arena.

---

## Basic Usage

```c
#include <stdio.h>
#include "phasealloc.h"

int main(void)
{
    PhaseArena *arena = phase_arena_create(1024);

    if (arena == NULL)
    {
        return 1;
    }

    int *numbers = phase_arena_alloc(arena, 5 * sizeof(int));

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

The typical lifecycle is:

```text
create
  ↓
allocate
  ↓
use
  ↓
reset
  ↓
reuse
  ↓
destroy
```

---

## Real Workload: JSON Parsing

PhaseAlloc includes a JSON parsing workload to evaluate arena allocation in a practical setting.

Two equivalent parsers are benchmarked:

```text
                 JSON input
                     │
              ┌──────┴──────┐
              ▼             ▼
         malloc/free    PhaseAlloc
           parser          parser
              │             │
              └──────┬──────┘
                     ▼
             recursive checksum
```

Both parsers construct equivalent JSON structures and use the same recursive checksum to verify their results.

The tested workloads include:

* **Small JSON** — small objects containing basic values.
* **Medium JSON** — objects containing multiple arrays and values.
* **Large Nested JSON** — nested objects and arrays.
* **Allocation-Heavy JSON** — many small objects and allocations.

The JSON benchmark records:

* `malloc` calls
* `realloc` calls
* PhaseAlloc allocation calls
* Parsing time
* Peak PhaseAlloc memory
* Recursive checksums

All tested workloads currently produce matching checksums between the two implementations.

---

## Benchmarking

PhaseAlloc includes two benchmark suites.

### Controlled Allocation Benchmark

`benchmarks/benchmark.c` compares equivalent allocation workloads:

* 100,000 × 32-byte allocations
* 100,000 × 256-byte allocations
* 10,000 × 4 KB allocations
* 1,000 × 32-byte allocations

Each workload is repeated for 10 iterations.

The benchmark compares:

```text
malloc allocation + free
```

against:

```text
PhaseAlloc allocation + reset
```

Both implementations perform equivalent memory writes, reads, and checksum calculations.

The goal is to determine **when** arena allocation provides an advantage rather than assuming it is universally faster.

### JSON Benchmark

`benchmarks/benchmark_json.c` repeatedly parses the four JSON workloads.

The benchmark uses 10,000 iterations per workload and measures both performance and allocation behavior.

Exact timings vary between machines and runs, so results are treated as measurements of specific experiments rather than universal performance claims.

---

## Building and Testing

PhaseAlloc uses a `Makefile` for repeatable builds and tests.

With MinGW on Windows:

```powershell
mingw32-make test
```

Build and run the controlled benchmark:

```powershell
mingw32-make benchmark
```

Build and run the JSON benchmark:

```powershell
mingw32-make benchmark-json
```

Build the examples:

```powershell
mingw32-make
```

Clean generated executables:

```powershell
mingw32-make clean
```

The test suite covers:

* Basic allocation
* Alignment
* Edge cases
* Overflow handling
* Reset and reuse
* Destruction
* Dynamic growth
* Peak memory tracking
* API contracts
* Allocation statistics
* Standard JSON parsing
* PhaseAlloc JSON parsing
* Stress growth behavior

The project is compiled with:

```text
-Wall -Wextra -std=c11
```

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
│   ├── test_peak.c
│   ├── test_api_contracts.c
│   ├── test_stats.c
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
├── Makefile
├── README.md
├── LICENSE
└── .gitignore
```

### Implementation organization

`phasealloc.h` contains the public allocator interface.

`phasealloc.c` contains the allocator implementation.

`phasealloc_internal.h` contains private chunk and arena definitions.

`json.c` implements the standard `malloc/free` JSON parser.

`json_phase.c` implements the equivalent JSON parser using PhaseAlloc.

`benchmark.c` contains controlled allocation benchmarks.

`benchmark_json.c` contains the real JSON workload benchmark.

---

## Current Design Characteristics

PhaseAlloc currently provides:

* Opaque public arena API
* Sequential arena allocation
* Dynamically growing chunks
* Geometric chunk growth
* Preservation of existing allocations during growth
* Reuse of existing chunks after reset
* Platform-correct `max_align_t` alignment
* Bounds checking
* Zero-size allocation protection
* Allocation failure handling
* Overflow-safe growth calculations
* Current memory tracking
* Peak memory tracking
* Allocation counting
* Chunk counting
* Automated regression testing
* Controlled performance benchmarks
* Real JSON workload integration
* Allocation instrumentation
* Recursive checksum verification

---

## Limitations

PhaseAlloc is designed for phase-based allocation rather than as a replacement for `malloc/free` in every workload.

It is best suited to situations where:

* Many allocations are created together.
* Objects have similar lifetimes.
* Individual objects do not need independent deallocation.
* Groups of objects can be reclaimed together.
* Allocation overhead is significant enough to matter.

It is less suitable when individual objects must frequently be freed while other allocations remain active.

The current JSON parser is a **benchmark workload**, not a complete production JSON library. Its purpose is to provide a realistic allocation pattern for evaluating PhaseAlloc.

The JSON implementation also contains areas for future optimization, including temporary allocations for object keys and arena storage used during dynamic JSON array growth.

Peak PhaseAlloc memory represents memory accounted for as used by the arena. It is not a direct measurement of total operating-system memory consumption.

Benchmark results depend on the workload, compiler, operating system, hardware, and system state.

---

## Development Progress

### Stage 1 — Basic Arena

**Complete**

Fixed-size arena, sequential allocation, reset, destruction, tests, and examples.

### Stage 2 — Robustness & Correctness

**Complete**

API contracts, edge-case handling, overflow protection, allocation failure handling, and correctness testing.

### Stage 3 — Dynamic Growth

**Complete**

Chunk-based growth, geometric expansion, large allocations, and preservation of existing allocations.

### Stage 4 — Performance & Benchmarking

**Complete**

Controlled `malloc/free` comparison, multiple allocation workloads, checksums, and repeatable benchmarking.

### Stage 5 — Real Workload / JSON Integration

**Complete**

Equivalent `malloc/free` and PhaseAlloc JSON parsers, allocation instrumentation, peak-memory measurement, and workload-specific benchmarking.

### Stage 6 — Production-Quality Library

**In Progress**

The allocator has been refactored around an opaque API and now includes:

* API contracts
* Internal encapsulation
* O(1) peak-memory tracking
* Platform-correct alignment
* Allocation statistics
* Chunk statistics
* Automated regression testing
* Repeatable build and benchmark commands

### Stage 7 — Open Source & External Validation

**Planned**

Future work includes cross-platform testing, public release preparation, external validation, additional real workloads, and feedback from external users.

---

## Research Question

PhaseAlloc is ultimately an experiment in understanding when arena allocation provides a measurable advantage over general-purpose allocation.

> **For what types of workloads does an arena allocator actually outperform standard `malloc/free`, and why?**

The current benchmark results show that PhaseAlloc can outperform `malloc/free` substantially on several allocation-heavy workloads, while performance can vary depending on allocation size and workload characteristics.

Further benchmarking and external validation are needed before drawing broader conclusions.

---

## License

See `LICENSE` for the terms under which PhaseAlloc is distributed.
