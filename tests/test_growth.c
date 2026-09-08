#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"
#include "../src/phasealloc_internal.h"

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

void test_reset_after_growth(void)
{
    printf("Testing reset after growth...\n");

    PhaseArena arena = phase_arena_create(64); // Create an arena with an initial capacity of 64 bytes

    void *ptr1 = phase_arena_alloc(&arena, 32); // Allocate 32 bytes from the arena
    assert(ptr1 != NULL); // Ensure the allocation was successful

    void *ptr2 = phase_arena_alloc(&arena, 128); // Allocate 128 bytes from the arena
    assert(ptr2 != NULL); // Ensure the allocation was successful

    void *ptr3 = phase_arena_alloc(&arena, 256); // Allocate 256 bytes from the arena
    assert(ptr3 != NULL); // Ensure the allocation was successful

    phase_arena_reset(&arena); // Reset the arena

    assert(arena.current_chunk == arena.first_chunk); // Ensure the current chunk is reset to the first chunk
    assert(arena.offset == 0); // Ensure the offset is reset to 0
    assert(arena.buffer == arena.first_chunk->buffer); // Ensure the buffer points to the first chunk's buffer
    assert(arena.capacity == arena.first_chunk->capacity); // Ensure the capacity is reset to the first chunk's capacity

    void *ptr4 = phase_arena_alloc(&arena, 32); // Allocate again after reset
    assert(ptr4 != NULL); // Ensure the allocation was successful

    phase_arena_destroy(&arena); // Clean up and destroy the arena

    printf("[PASS] Reset after growth test passed!\n");
}

void test_reuse_after_reset(void)
{
    printf("Testing reuse after reset...\n");

    PhaseArena arena = phase_arena_create(64); // Create an arena with an initial capacity of 64 bytes

    void *ptr1 = phase_arena_alloc(&arena, 32); // Allocate 32 bytes from the arena
    assert(ptr1 != NULL); // Ensure the allocation was successful

    void *ptr2 = phase_arena_alloc(&arena, 128); // Allocate 128 bytes from the arena
    assert(ptr2 != NULL); // Ensure the allocation was successful

    phase_arena_reset(&arena); // Reset the arena

    void *ptr3 = phase_arena_alloc(&arena, 32); // Allocate again after reset
    assert(ptr3 != NULL); // Ensure the allocation was successful

    assert(arena.current_chunk == arena.first_chunk); // Ensure the current chunk is reset to the first chunk
    assert(arena.offset > 0); // Ensure the new allocation used space in the first chunk

    phase_arena_reset(&arena); // Reset the arena again

    void *ptr4 = phase_arena_alloc(&arena, 32); // Allocate again after second reset
    assert(ptr4 != NULL); // Ensure the allocation was successful

    assert(arena.current_chunk == arena.first_chunk); // Ensure the current chunk is reset to the first chunk
    assert(arena.offset > 0); // Ensure the new allocation used space in the first chunk
    
    phase_arena_destroy(&arena); // Clean up and destroy the arena

    printf("[PASS] Reuse after reset test passed!\n");
}

void test_failure_cases(void)
{
    printf("Testing failure cases...\n");

    void *ptr1 = phase_arena_alloc(NULL, 32); // Attempt to allocate with a NULL arena
    assert(ptr1 == NULL); // Ensure the allocation failed

    PhaseArena arena = phase_arena_create(64); // Create an arena with an initial capacity of 64 bytes

    assert(arena.buffer != NULL); // Ensure the arena was created successfully

    void *ptr2 = phase_arena_alloc(&arena, 0); // Attempt to allocate 0 bytes
    assert(ptr2 == NULL); // Ensure the allocation failed

    assert(arena.offset == 0); // Ensure the offset remains unchanged
    assert(arena.current_chunk == arena.first_chunk); // Ensure the current chunk remains unchanged

    void *ptr3 = phase_arena_alloc(&arena, 32); // Attempt to allocate 32 bytes
    assert(ptr3 != NULL); // Ensure the allocation was successful

    phase_arena_destroy(&arena); // Clean up and destroy the arena

    printf("[PASS] Failure cases test passed!\n");
}

void test_stress_growth(void)
{
    printf("Testing stress growth...\n");

    PhaseArena arena = phase_arena_create(64); // Create an arena with an initial capacity of 64 bytes

    assert(arena.buffer != NULL); // Ensure the arena was created successfully

    for (int i = 0; i < 1000; i++)
    {
        size_t size = (i % 128) + 1; // Allocate sizes from 1 to 128 bytes

        unsigned char *memory = phase_arena_alloc(&arena, size); // Allocate memory from the arena
        
        assert(memory != NULL); // Ensure the allocation was successful

        for (size_t j = 0; j < size; j++)
        {
            memory[j] = (unsigned char)(i % 256); // Fill the allocated memory with some data
        }

        for (size_t j = 0; j < size; j++)
        {
            assert(memory[j] == (unsigned char)(i % 256)); // Verify that the data in the allocated memory is correct
        }
    }

    phase_arena_reset(&arena); // Reset the arena after stress testing

    for (int i = 0; i <100; i++)
    {
        void *memory = phase_arena_alloc(&arena, 32); // Allocate 32 bytes from the arena
        
        assert(memory != NULL); // Ensure the allocation was successful
    }

    phase_arena_destroy(&arena); // Clean up and destroy the arena

    printf("[PASS] Stress growth test passed!\n");
}

int main(void)
{
    printf("Running Stage 3 growth tests...\n");

    test_basic_growth(); // Run the basic growth test
    test_multiple_growths(); // Run the multiple growths test
    test_old_allocations_survive_growth(); // Run the old allocations survive growth test
    test_reset_after_growth(); // Run the reset after growth test
    test_reuse_after_reset(); // Run the reuse after reset test
    test_failure_cases(); // Run the failure cases test
    test_stress_growth(); // Run the stress growth test
    printf("\nAll growth tests passed!\n");

    return 0;
}