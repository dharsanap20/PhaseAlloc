#include <assert.h>
#include <stdio.h>
#include "../include/phasealloc.h"

int main(void)
{
    // A NULL arena should report zero peak memory.
    assert(phase_arena_get_peak_memory(NULL) == 0);

    PhaseArena *arena = phase_arena_create(64);

    assert(arena != NULL);

    // No allocations have happened yet.
    assert(phase_arena_get_peak_memory(arena) == 0);

    // Allocate some memory and verify the peak increases.
    void *first = phase_arena_alloc(arena, 16);

    assert(first != NULL);

    size_t first_peak = phase_arena_get_peak_memory(arena);

    assert(first_peak >= 16);

    // Another allocation should increase the peak.
    void *second = phase_arena_alloc(arena, 16);

    assert(second != NULL);

    size_t second_peak = phase_arena_get_peak_memory(arena);

    assert(second_peak >= first_peak);

    // Reset should clear current usage but preserve the lifetime peak.
    phase_arena_reset(arena);

    assert(phase_arena_get_peak_memory(arena) == second_peak);

    // Allocate again after reset.
    void *third = phase_arena_alloc(arena, 32);

    assert(third != NULL);

    size_t third_peak = phase_arena_get_peak_memory(arena);

    assert(third_peak >= second_peak);

    // Force the arena to grow.
    void *large = phase_arena_alloc(arena, 128);

    assert(large != NULL);

    size_t growth_peak = phase_arena_get_peak_memory(arena);

    assert(growth_peak >= third_peak);

    phase_arena_destroy(arena);

    printf("All peak memory tests passed!\n");

    return 0;
}