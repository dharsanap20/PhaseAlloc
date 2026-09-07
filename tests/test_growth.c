#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

void test_basic_growth(void)
{
    printf("Testing basic growth tests...\n");

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
}

void test_multiple_growths(void)
{
    printf("Testing multiple growth events...\n");

    PhaseArena arena = phase_arena_create(64); // Create an arena with an initial capacity of 64 bytes

    assert(arena.buffer != NULL); // Ensure the arena was created successfully
    assert(arena.capacity == 64); // Check that the arena's capacity is as expected
    assert(arena.offset == 0); // Check that the arena's offset is initially 0

    void *ptr1 = phase_arena_alloc(&arena, 32); // Allocate 32 bytes from the arena
    assert(ptr1 != NULL); // Ensure the allocation was successful

    void *ptr2 = phase_arena_alloc(&arena, 128); // Allocate 128 bytes from the arena
    assert(ptr2 != NULL); // Ensure the allocation was successful

    void *ptr3 = phase_arena_alloc(&arena, 256); // Allocate 256 bytes from the arena
    assert(ptr3 != NULL); // Ensure the allocation was successful

    void *ptr4 = phase_arena_alloc(&arena, 512); // Allocate 512 bytes from the arena
    assert(ptr4 != NULL); // Ensure the allocation was successful

    assert(ptr1 != ptr2); // Ensure that the first and second allocations do not overlap
    assert(ptr2 != ptr3); // Ensure that the second and third allocations do not overlap
    assert(ptr3 != ptr4); // Ensure that the third and fourth allocations do not overlap

    phase_arena_destroy(&arena); // Clean up and destroy the arena

    printf("[PASS] Multiple growths test passed!\n");
}

void test_old_allocations_survive_growth(void)
{
    printf("Testing old allocations after growth...\n");

    PhaseArena arena = phase_arena_create(64); // Create an arena with an initial capacity of 64 bytes

    int *value = phase_arena_alloc(&arena, sizeof(int)); // Allocate space for an integer
    assert(value != NULL); // Ensure the allocation was successful

    *value = 42; // Store a value in the allocated space

    void *ptr2 = phase_arena_alloc(&arena, 128); // Allocate more memory to trigger growth
    assert(ptr2 != NULL); // Ensure the allocation was successful

    void *ptr3 = phase_arena_alloc(&arena, 256); // Allocate even more memory to trigger another growth
    assert(ptr3 != NULL); // Ensure the allocation was successful

    assert(*value == 42); // Ensure that the value stored in the first allocation is still intact

    phase_arena_destroy(&arena); // Clean up and destroy the arena

    printf("[PASS] Old allocations survive growth test passed!\n");
}

int main(void)
{
    printf("Running Stage 3 growth tests...\n");

    test_basic_growth(); // Run the basic growth test
    test_multiple_growths(); // Run the multiple growths test
    test_old_allocations_survive_growth(); // Run the old allocations survive growth test
    printf("\nAll growth tests passed!\n");

    return 0;
}