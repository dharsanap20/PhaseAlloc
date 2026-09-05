# PhaseAlloc

PhaseAlloc is a phase-based arena allocator written in C.

It provides a simple way to allocate multiple objects from a pre-allocated memory region and reclaim the entire region at once by resetting the arena.

## What is an Arena Allocator?

An arena allocator obtains a large block of memory and uses that block to satisfy smaller allocation requests.

Instead of individually freeing every allocation, an arena can reset its allocation offset and reuse the entire memory region.

The basic allocation process is:

1. Create an arena with a specified capacity.
2. Allocate memory from the arena.
3. Use the allocated memory.
4. Reset the arena when the allocations are no longer needed.
5. Reuse the arena for another phase of work.
6. Destroy the arena when it is no longer needed.

## How PhaseAlloc Works

PhaseAlloc stores three pieces of information in a `PhaseArena`:

* `buffer` — pointer to the arena's allocated memory.
* `capacity` — total size of the arena in bytes.
* `offset` — current position used to determine where the next allocation will begin.

When memory is requested, PhaseAlloc finds an appropriately aligned location in the arena, returns a pointer to that memory, and advances the allocation offset.

Conceptually:

```text
Arena

┌──────────────────────────────────────────┐
│ Used Memory │ Available Memory           │
└──────────────────────────────────────────┘
              ↑
            offset
```

When the arena is reset, the offset returns to zero, allowing the existing memory region to be reused.

The underlying memory is not released when the arena is reset. It remains owned by the arena until `phase_arena_destroy()` is called.

## Current Features

* Arena creation with a configurable capacity
* Sequential memory allocation
* 8-byte pointer alignment
* Bounds checking
* Allocation failure handling
* Zero-byte allocation protection
* NULL arena safety
* Overflow-safe allocation checks
* Arena reset for memory reuse
* Repeated reset and reuse
* Safe arena destruction
* Safe repeated destruction
* Automated tests
* Basic usage example

## Project Structure

```text
PhaseAlloc/
│
├── include/
│   └── phasealloc.h
│
├── src/
│   └── phasealloc.c
│
├── tests/
│   ├── test_basic.c
│   ├── test_alignment.c
│   ├── test_edge_cases.c
│   ├── test_overflow.c
│   ├── test_reset.c
│   └── test_destroy.c
│
├── examples/
│   └── basic.c
│
├── README.md
├── LICENSE
└── .gitignore
```

## API

### `phase_arena_create`

```c
PhaseArena phase_arena_create(size_t capacity);
```

Creates an arena with the requested capacity in bytes.

If the underlying memory allocation fails, the returned arena has a `NULL` buffer.

### `phase_arena_alloc`

```c
void *phase_arena_alloc(PhaseArena *arena, size_t size);
```

Allocates a block of memory from the arena and returns a pointer to that memory.

The allocation is aligned to an 8-byte boundary.

Returns `NULL` if:

* The arena is `NULL`
* The arena has no valid buffer
* The requested size is zero
* The requested allocation does not fit
* The allocation would exceed the arena's available space

### `phase_arena_reset`

```c
void phase_arena_reset(PhaseArena *arena);
```

Resets the arena's allocation offset to zero so its existing memory can be reused.

Resetting an arena does not release its underlying buffer.

### `phase_arena_destroy`

```c
void phase_arena_destroy(PhaseArena *arena);
```

Releases the memory owned by the arena.

After destruction, the arena's state is cleared:

```text
buffer   → NULL
capacity → 0
offset   → 0
```

The function safely handles a `NULL` arena and an already-destroyed arena.

## Basic Usage

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

## Testing

PhaseAlloc includes an automated test suite covering the allocator's core functionality and robustness.

### `test_basic.c`

Tests:

* Arena creation
* Memory allocation
* Writing to allocated memory
* Basic allocation behavior

### `test_alignment.c`

Tests:

* 8-byte pointer alignment
* Multiple differently sized allocations

### `test_edge_cases.c`

Tests:

* Zero-byte allocations
* Out-of-memory allocations
* Exact-capacity allocations
* Multiple allocations
* `NULL` arena handling

### `test_overflow.c`

Tests:

* Extremely large allocation requests
* `SIZE_MAX` allocation requests
* Failed allocations preserving the arena's previous state
* Continued arena operation after a failed large allocation

### `test_reset.c`

Tests:

* Resetting the allocation offset
* Reusing memory after reset
* Repeated reset and reuse cycles
* Preservation of the arena buffer
* Preservation of arena capacity

### `test_destroy.c`

Tests:

* Clearing the arena buffer after destruction
* Resetting capacity and offset
* Safe repeated destruction
* Safe destruction of a `NULL` arena
* Creating a new arena after destruction

## Running the Tests

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

## Design Goal

PhaseAlloc is designed around the idea of phase-based memory management.

A phase is a period of computation during which a group of objects is needed together. Instead of individually releasing each object, the arena can reclaim the entire group by resetting its allocation offset.

This makes the allocator particularly suited to workloads where many objects have similar lifetimes and can be discarded together.

## Current Scope

PhaseAlloc currently uses a fixed-size arena.

If the arena does not have enough available space for an allocation, the allocation fails and returns `NULL`.

Automatic arena growth is not currently implemented.

Future development will focus on:

* Automatic arena growth
* Performance benchmarking
* Comparison with conventional allocation strategies
* Realistic workloads
* JSON parsing integration
* Additional stress testing
* Portability and production-quality improvements

## License

See `LICENSE` for the terms under which PhaseAlloc is distributed.
