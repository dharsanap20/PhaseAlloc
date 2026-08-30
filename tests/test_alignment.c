#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "../include/phasealloc.h"

int main(void)
{
    PhaseArena arena = phase_arena_create(1024); // Create an arena with 1024 bytes

    assert(arena.buffer != NULL);

    void *memory = phase_arena_alloc(&arena, 1); // Allocate 1 byte of memory

    assert(memory != NULL);

    assert(((uintptr_t)memory % 8) == 0); // Check if the allocated memory is aligned to 8 bytes

    printf("[PASS] Memory allocation is aligned to 8 bytes.\n");

    phase_arena_destroy(&arena); // Clean up the arena

    printf("All alignment tests passed successfully!\n");

    return 0;
}