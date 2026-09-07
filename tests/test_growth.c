#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("Running Stage 3 growth tests...\n");

    PhaseArena arena = phase_arena_create(64); // Create an arena with an initial capacity of 64 bytes

    assert(arena.buffer != NULL); // Ensure the arena was created successfully
    assert(arena.capacity == 64); // Check that the arena's capacity is as expected
    assert(arena.offset == 0); // Check that the arena's offset is initially 0

    void *ptr1 = phase_arena_alloc(&arena, 32); // Allocate 32 bytes from the arena
    assert(ptr1 != NULL); // Ensure the allocation was successful

    void *ptr2 = phase_arena_alloc(&arena, 128); // Allocate 128 bytes from the arena
    assert(ptr2 != NULL); // Ensure the allocation was successful

    assert(ptr1 != ptr2); // Ensure that the two allocations do not overlap

    phase_arena_destroy(&arena); // Clean up and destroy the arena

    printf("[PASS] Basic growth test passed!\n");

    return 0;
}