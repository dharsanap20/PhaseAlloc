# PhaseAlloc

A dynamically growing, phase-based arena allocator written in C.

## Download

Latest release:

**PhaseAlloc v1.1.0**

Download the latest release package here:

https://github.com/dharsanap20/PhaseAlloc/releases/latest

The release includes:

* Source code
* Examples
* Tests
* Benchmarks
* Makefile
* MIT License

## Requirements

* C compiler with C11 support
* Make
* MinGW (Windows) or GCC/Clang (Linux/macOS)

## Quick Start

Clone the repository:

```bash
git clone https://github.com/dharsanap20/PhaseAlloc.git
cd PhaseAlloc
```

Build the project:

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
```

Run the JSON benchmark:

```bash
make benchmark-json
```

## What PhaseAlloc Does

PhaseAlloc is designed for workloads where many objects are created during a phase of computation and later released together.

Instead of:

```text
allocate object
allocate object
allocate object
free object
free object
free object
```

PhaseAlloc allows:

```text
create arena
     |
allocate many objects
     |
use objects
     |
reset arena
     |
reuse memory
```

This removes the need to individually free every allocation.

## Features

* Dynamically growing arena allocator
* Chunk-based memory expansion
* Sequential allocation
* Arena reset and reuse
* Alignment using `max_align_t`
* Overflow protection
* Allocation failure handling
* Opaque public API
* Current memory tracking
* Peak memory tracking
* Allocation statistics
* Chunk statistics
* Automated regression tests

## How It Works

PhaseAlloc manages memory using linked chunks:

```text
PhaseArena

    |
    v

+-------------+
|   Chunk 1   |
| allocations |
|   offset    |
+-------------+
      |
      v
+-------------+
|   Chunk 2   |
| allocations |
|   offset    |
+-------------+
```

When the current chunk does not have enough space, PhaseAlloc creates a new chunk while keeping previous allocations valid.

## Public API

Create an arena:

```c
PhaseArena *phase_arena_create(size_t capacity);
```

Allocate memory:

```c
void *phase_arena_alloc(PhaseArena *arena, size_t size);
```

Reset the arena:

```c
void phase_arena_reset(PhaseArena *arena);
```

Destroy the arena:

```c
void phase_arena_destroy(PhaseArena *arena);
```

Get statistics:

```c
size_t phase_arena_get_current_memory(const PhaseArena *arena);
size_t phase_arena_get_peak_memory(const PhaseArena *arena);
size_t phase_arena_get_allocation_count(const PhaseArena *arena);
size_t phase_arena_get_chunk_count(const PhaseArena *arena);
```

## Example

```c
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

## Benchmarks

PhaseAlloc includes two benchmark systems.

### Allocation Benchmark

Compares:

```text
malloc/free
```

against:

```text
PhaseAlloc allocation/reset
```

using controlled allocation workloads.

Test cases include:

* 100,000 × 32-byte allocations
* 100,000 × 256-byte allocations
* 10,000 × 4 KB allocations
* 1,000 × 32-byte allocations

### JSON Workload Benchmark

PhaseAlloc includes an equivalent JSON parsing workload comparing:

```text
malloc/free parser
```

against:

```text
PhaseAlloc parser
```

The benchmark measures:

* Parsing time
* Allocation count
* Reallocation count
* PhaseAlloc allocations
* Peak arena memory
* Result correctness using checksums

Benchmark results depend on hardware, compiler, operating system, and workload.

## Testing

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
* JSON parsing
* PhaseAlloc JSON parsing
* Growth stress testing

Current test status:

```text
All PhaseAlloc tests passed.
```

## Project Structure

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
├── tests/
│   ├── allocator tests
│   └── JSON tests
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
└── LICENSE
```

## When To Use PhaseAlloc

PhaseAlloc works best when:

* Many allocations share the same lifetime
* Objects are created during a temporary phase
* Individual freeing is unnecessary
* Allocation overhead matters

Examples:

* Parsers
* Compilers
* Game engines
* Simulation systems
* Temporary data processing

PhaseAlloc is not intended to replace `malloc/free` for every workload. Programs requiring frequent individual deallocation may be better suited for traditional allocators.

## Research Question

PhaseAlloc explores:

> For what types of workloads does an arena allocator actually outperform standard malloc/free, and why?

The project uses controlled experiments and real workloads to study allocator behavior.

## License

PhaseAlloc is released under the MIT License.
