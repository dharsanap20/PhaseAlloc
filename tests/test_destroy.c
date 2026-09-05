#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

int main (void)
{
    printf("Running PhaseAlloc Destroy Tests...\n");

    // 1. Test that destroy clears the arena buffer
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);
        assert(arena.capacity == 1024);
        assert(arena.offset == 0);

        phase_arena_destroy(&arena);

        assert(arena.buffer == NULL);

        printf("[PASS] Destroy clears the arena buffer.\n");
    }

    // 2. Test that destroy resets capacity and offset
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);

        void *ptr = phase_arena_alloc(&arena, 128);

        assert(ptr != NULL);
        assert(arena.offset > 0);

        phase_arena_destroy(&arena);

        assert(arena.buffer == NULL);
        assert(arena.capacity == 0);
        assert(arena.offset == 0);

        printf("[PASS] Destroy resets buffer, capacity, and offset.\n");
    }

    // 3. Test that destroying an already destroyed arena is safe
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);

        phase_arena_destroy(&arena);

        assert(arena.buffer == NULL);

        // Destroy again
        phase_arena_destroy(&arena);

        assert(arena.buffer == NULL);
        assert(arena.capacity == 0);
        assert(arena.offset == 0);

        printf("[PASS] Double destroy is handled safely.\n");
    }

    // 4. test that destroying a NULL arena is safe
    {
        phase_arena_destroy(NULL); // Should not crash or cause undefined behavior

        printf("[PASS] NULL arena destroy is handled safely.\n");
    }

    // 5. Test that a new arena can be created after destruction
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);

        phase_arena_destroy(&arena);

        assert(arena.buffer == NULL);

        PhaseArena new_arena = phase_arena_create(1024);

        assert(new_arena.buffer != NULL);
        assert(new_arena.capacity == 1024);
        assert(new_arena.offset == 0);

        phase_arena_destroy(&new_arena);

        printf("[PASS] New arena can be created after destruction.\n");
    }

    printf("\nAll PhaseAlloc Destroy Tests Passed!\n");

    return 0;
}