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
│      200 bytes     │      824 bytes      │
└────────────────────┴─────────────────────┘
                     ↑
                   offset
```

After another 100-byte allocation:

```text
Arena

┌──────────────────────────────┬────────────┐
│        Used Memory           │ Available  │
│          300 bytes           │ 724 bytes  │
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

If no existing chunk can satisfy the request, PhaseAlloc creates a new chunk.

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
Current chunk:       64 bytes
Requested allocation: 1000 bytes

New chunk:          1000+ bytes
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

Chunk 1              Chunk 2
┌───────────────────┐ ┌───────────────────┐
│ Allocation A      │ │ Allocation C      │
│ Allocation B      │ │ Allocation D      │
└───────────────────┘ └───────────────────┘
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
  used      used      used
```

After reset:

```text
Chunk 1 → Chunk 2 → Chunk 3
  free      free      free
   ↑
current chunk
```

The arena returns to the first chunk and can reuse the existing memory.

This means a reset does not require new allocations from the system.

The underlying memory remains owned by the arena until `phase_arena_destroy()` is called.

---

## Destroy

When the arena is destroyed, PhaseAlloc walks through the chunk list and frees:

1. Each chunk's memory buffer.
2. Each `PhaseChunk` structure.

After destruction, the arena's state is cleared.

```text
buffer         → NULL
capacity       → 0
offset         → 0
first_chunk    → NULL
current_chunk  → NULL
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
* Repeated reset and reuse
* Safe arena destruction
* Safe repeated destruction
* Automated tests
* Stress testing
* Basic usage example

---

## Project Structure

```text
PhaseAlloc/

│
├── include/
│   └── phasealloc.h
│
├── src/
│   ├── phasealloc.c
│   └── phasealloc_internal.h
│
├── tests/
│   ├── test_basic.c
│   ├── test_alignment.c
│   ├── test_edge_cases.c
│   ├── test_overflow.c
│   ├── test_reset.c
│   ├── test_destroy.c
│   └── test_growth.c
│
├── examples/
│   └── basic.c
│
├── README.md
├── LICENSE
└── .gitignore
```

`phasealloc.h` contains the public API.

`phasealloc.c` contains the allocator implementation.

`phasealloc_internal.h` contains implementation-specific definitions for the internal chunk system.

The internal header is kept separate from the public `include/` directory because users of the library do not need direct access to the chunk implementation.

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
buffer         → NULL
capacity       → 0
offset         → 0
first_chunk    → NULL
current_chunk  → NULL
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

PhaseAlloc includes an automated test suite covering the allocator's core functionality, correctness, failure handling, dynamic growth, reset/reuse behavior, and stress behavior.

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

The same pattern can be used for the other test files.

For example:

```powershell
gcc src/phasealloc.c tests/test_growth.c -Iinclude -o test_growth.exe
.\test_growth.exe
```

The current test suite covers the allocator's basic functionality, correctness, dynamic growth, failure handling, reset/reuse behavior, destruction behavior, and stress behavior.

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

---

# Current Scope

PhaseAlloc currently provides a dynamically growing, chunk-based arena allocator.

The allocator:

* Starts with an initial memory chunk.
* Allocates sequentially within chunks.
* Searches existing chunks when possible.
* Creates additional chunks when necessary.
* Uses geometric growth for new chunks.
* Preserves existing allocations during growth.
* Retains chunks after reset for reuse.
* Frees all chunks when destroyed.

Current development has focused on allocator correctness, memory safety, dynamic growth, reset/reuse behavior, failure handling, and automated testing.

Future development will focus on:

* Performance benchmarking
* Comparison with conventional allocation strategies
* Allocation throughput measurements
* Memory usage analysis
* Realistic workloads
* JSON parsing integration
* Benchmarking PhaseAlloc against `malloc`/`free`
* Additional portability improvements
* Production-quality API improvements
* Documentation and release preparation

---

# License

See `LICENSE` for the terms under which PhaseAlloc is distributed.
