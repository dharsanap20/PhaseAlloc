#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("Starting PhaseAlloc Basic Tests...\n");

    // 1. Create an arena
    PhaseArena arena = phase_arena_create(1024);

    assert(arena.buffer != NULL);
    assert(arena.capacity == 1024);
    assert(arena.offset == 0);

    printf("[PASS] Arena created successfully.\n");

    // 2. Allocate memory from the arena
    int *numbers = phase_arena_alloc(&arena, 10 * sizeof(int));

    assert(numbers != NULL);
    assert(arena.offset == 10 * sizeof(int));

    printf("[PASS] Memory allocated successfully.\n");

    // 3. Write data to the allocated memory
    for (int i = 0; i < 10; i++) 
    {
        numbers[i] = i * 10;
    }

    assert(numbers[0] == 0);
    assert(numbers[9] == 90);

    printf("[PASS] Memory written successfully.\n");

    // 4. Test dynamic growth
    void *larger_allocation = phase_arena_alloc(&arena, 2000); 

    assert(larger_allocation != NULL);

    printf("[PASS] Arena grows when allocation exceeds current capacity.\n");

    // 5. Reset the arena
    phase_arena_reset(&arena);

    assert(arena.offset == 0);

    printf("[PASS] Arena reset successfully.\n");

    // 6. Destroy the arena
    phase_arena_destroy(&arena);

    assert(arena.buffer == NULL);
    assert(arena.capacity == 0);
    assert(arena.offset == 0);

    printf("[PASS] Arena destroyed successfully.\n");

    printf("\nAll Stage 1 Basic Tests Passed!\n");

    return 0;
}