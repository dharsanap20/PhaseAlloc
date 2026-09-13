#include <assert.h>
#include <stdio.h>
#include "../include/phasealloc.h"

int main(void)
{
    // Creating an arena with zero capacity should fail.
    PhaseArena *zero_arena = phase_arena_create(0);

    assert(zero_arena == NULL);

    // NULL arena operations should fail safely.
    assert(phase_arena_alloc(NULL, 10) == NULL);
    assert(phase_arena_get_peak_memory(NULL) == 0);

    // Create a normal arena.
    PhaseArena *arena = phase_arena_create(64);

    assert(arena != NULL);

    // Zero-size allocations should fail.
    assert(phase_arena_alloc(arena, 0) == NULL);

    // A normal allocation should succeed.
    void *memory = phase_arena_alloc(arena, 16);

    assert(memory != NULL);
    assert(phase_arena_get_peak_memory(arena) >= 16);

    // An allocation larger than the initial chunk should trigger growth.
    void *large_memory = phase_arena_alloc(arena, 128);

    assert(large_memory != NULL);
    assert(phase_arena_get_peak_memory(arena) >= 144);

    // Reset should allow the arena to be reused.
    phase_arena_reset(arena);

    void *reused_memory = phase_arena_alloc(arena, 16);

    assert(reused_memory != NULL);

    // Peak memory is a high-water mark and should not decrease after reset.
    assert(phase_arena_get_peak_memory(arena) >= 144);

    // Destroying the arena should complete safely.
    phase_arena_destroy(arena);

    // Destroying NULL should also be safe.
    phase_arena_destroy(NULL);

    printf("All API contract tests passed!\n");

    return 0;
}