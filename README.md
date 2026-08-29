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

- `buffer` — pointer to the arena's allocated memory.
- `capacity` — total size of the arena in bytes.
- `offset` — current position where the next allocation will begin.

When memory is requested, PhaseAlloc returns a pointer to the current offset and advances the offset by the requested size.

Conceptually:

    Arena
    ┌──────────────────────────────────────────┐
    │ Used Memory │ Available Memory           │
    └──────────────────────────────────────────┘
                  ↑
                offset

When the arena is reset, the offset returns to zero, allowing the memory region to be reused.

The underlying memory is not released until `phase_arena_destroy()` is called.

## Current Features

- Arena creation with a configurable capacity
- Sequential memory allocation
- Bounds checking
- Allocation failure handling
- Arena reset for memory reuse
- Arena destruction
- Basic automated tests
- Basic usage example

## Project Structure

    PhaseAlloc/
    ├── include/
    │   └── phasealloc.h
    ├── src/
    │   └── phasealloc.c
    ├── tests/
    │   └── test_basic.c
    ├── examples/
    │   └── basic.c
    ├── README.md
    ├── LICENSE
    └── .gitignore

## API

### `phase_arena_create`

    PhaseArena phase_arena_create(size_t capacity);

Creates an arena with the requested capacity in bytes.

### `phase_arena_alloc`

    void *phase_arena_alloc(PhaseArena *arena, size_t size);

Allocates a block of memory from the arena and returns a pointer to that memory.

Returns `NULL` if the allocation cannot be satisfied.

### `phase_arena_reset`

    void phase_arena_reset(PhaseArena *arena);

Resets the arena's allocation offset to zero so its memory can be reused.

### `phase_arena_destroy`

    void phase_arena_destroy(PhaseArena *arena);

Releases the memory owned by the arena.

## Basic Usage

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

## Testing

PhaseAlloc includes a basic test suite covering:

- Arena creation
- Memory allocation
- Writing to allocated memory
- Out-of-memory protection
- Arena reset
- Arena destruction

The test program is located at `tests/test_basic.c`.

## Example

A basic usage example is located at `examples/basic.c`.

It demonstrates the basic lifecycle of a PhaseAlloc arena:

    Create
      ↓
    Allocate
      ↓
    Use
      ↓
    Reset
      ↓
    Destroy

## Design Goal

PhaseAlloc is designed around the idea of phase-based memory management.

A phase is a period of computation during which a group of objects is needed together. Instead of individually releasing each object, the arena can reclaim the entire group by resetting its allocation offset.

Future development will focus on improving robustness, alignment, testing, benchmarking, and evaluating PhaseAlloc against conventional allocation strategies using realistic workloads.

## License

See `LICENSE` for the terms under which PhaseAlloc is distributed.