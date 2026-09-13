#include <assert.h>
#include <stdio.h>
#include "../include/phasealloc.h"

int main(void)
{
    // NULL arenas should return zero for all statistics.
    assert(phase_arena_get_current_memory(NULL) == 0);
    assert(phase_arena_get_peak_memory(NULL) == 0);
    assert(phase_arena_get_allocation_count(NULL) == 0);
    assert(phase_arena_get_chunk_count(NULL) == 0);

    PhaseArena *arena = phase_arena_create(64);

    assert(arena != NULL);

    // A new arena starts with one empty chunk.
    assert(phase_arena_get_current_memory(arena) == 0);
    assert(phase_arena_get_peak_memory(arena) == 0);
    assert(phase_arena_get_allocation_count(arena) == 0);
    assert(phase_arena_get_chunk_count(arena) == 1);

    // Successful allocations increase current memory and allocation count.
    void *first = phase_arena_alloc(arena, 16);

    assert(first != NULL);
    assert(phase_arena_get_current_memory(arena) >= 16);
    assert(phase_arena_get_peak_memory(arena) >= 16);
    assert(phase_arena_get_allocation_count(arena) == 1);
    assert(phase_arena_get_chunk_count(arena) == 1);

    void *second = phase_arena_alloc(arena, 16);

    assert(second != NULL);
    assert(phase_arena_get_allocation_count(arena) == 2);
    assert(phase_arena_get_peak_memory(arena) >= phase_arena_get_current_memory(arena));

    // Failed allocations should not increase the allocation count.
    assert(phase_arena_alloc(arena, 0) == NULL);
    assert(phase_arena_get_allocation_count(arena) == 2);

    // Force the arena to create another chunk.
    void *large = phase_arena_alloc(arena, 128);

    assert(large != NULL);
    assert(phase_arena_get_allocation_count(arena) == 3);
    assert(phase_arena_get_chunk_count(arena) == 2);
    assert(phase_arena_get_current_memory(arena) >= 128);

    size_t peak_before_reset = phase_arena_get_peak_memory(arena);
    size_t allocations_before_reset = phase_arena_get_allocation_count(arena);

    // Reset clears current usage but preserves lifetime statistics.
    phase_arena_reset(arena);

    assert(phase_arena_get_current_memory(arena) == 0);
    assert(phase_arena_get_peak_memory(arena) == peak_before_reset);
    assert(phase_arena_get_allocation_count(arena) == allocations_before_reset);
    assert(phase_arena_get_chunk_count(arena) == 2);

    // Allocation after reset should increase current memory and allocation count.
    void *reused = phase_arena_alloc(arena, 32);

    assert(reused != NULL);
    assert(phase_arena_get_current_memory(arena) >= 32);
    assert(phase_arena_get_allocation_count(arena) == allocations_before_reset + 1);
    assert(phase_arena_get_peak_memory(arena) >= peak_before_reset);

    phase_arena_destroy(arena);

    printf("All statistics tests passed!\n");

    return 0;
}