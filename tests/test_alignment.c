#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "../include/phasealloc.h"

int main(void)
{
    PhaseArena arena = phase_arena_create(1024);

    assert(arena.buffer != NULL);

    // Test allocations of different sizes
    void *first = phase_arena_alloc(&arena, 1);
    void *second = phase_arena_alloc(&arena, 5);
    void *third = phase_arena_alloc(&arena, 13);
    void *fourth = phase_arena_alloc(&arena, 32);

    assert(first != NULL);
    assert(second != NULL);
    assert(third != NULL);
    assert(fourth != NULL);

    // Check that every returned address is 8-byte aligned
    assert((uintptr_t)first % 8 == 0);
    assert((uintptr_t)second % 8 == 0);
    assert((uintptr_t)third % 8 == 0);
    assert((uintptr_t)fourth % 8 == 0);

    printf("[PASS] All allocations are 8-byte aligned.\n");

    phase_arena_destroy(&arena);

    printf("\nAlignment tests passed!\n");

    return 0;
}