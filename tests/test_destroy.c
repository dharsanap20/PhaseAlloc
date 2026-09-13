#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("Running PhaseAlloc Destroy Tests...\n");

    // 1. Test that destroy safely frees an arena
    {
        PhaseArena *arena = phase_arena_create(1024);

        assert(arena != NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Arena destroyed successfully.\n");
    }

    // 2. Test that destroy works after allocations
    {
        PhaseArena *arena = phase_arena_create(1024);

        assert(arena != NULL);

        void *ptr = phase_arena_alloc(arena, 128);

        assert(ptr != NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Arena with allocated memory destroyed successfully.\n");
    }

    // 3. Test that destroying a NULL arena is safe
    {
        phase_arena_destroy(NULL);

        printf("[PASS] NULL arena destroy is handled safely.\n");
    }

    // 4. Test that a new arena can be created after destruction
    {
        PhaseArena *arena = phase_arena_create(1024);

        assert(arena != NULL);

        phase_arena_destroy(arena);

        PhaseArena *new_arena = phase_arena_create(1024);

        assert(new_arena != NULL);

        phase_arena_destroy(new_arena);

        printf("[PASS] New arena can be created after destruction.\n");
    }

    printf("\nAll PhaseAlloc Destroy Tests Passed!\n");

    return 0;
}