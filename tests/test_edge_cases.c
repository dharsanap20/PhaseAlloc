#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("Running PhaseAlloc Edge Case Tests...\n");

    // 1. Zero-byte allocation
    {
        PhaseArena *arena = phase_arena_create(1024);

        void *ptr = phase_arena_alloc(arena, 0);

        assert(ptr == NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Zero-byte allocation handled correctly.\n");
    }

    // 2. Out-of-memory allocation
    {
        PhaseArena *arena = phase_arena_create(64);

        void *ptr = phase_arena_alloc(arena, 128);

        assert(ptr != NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Allocation larger than current chunk handled correctly.\n");
    }

    // 3. Exact-capacity allocation
    {
        PhaseArena *arena = phase_arena_create(64);

        void *ptr = phase_arena_alloc(arena, 64);

        assert(ptr != NULL);

        void *next_ptr = phase_arena_alloc(arena, 1);

        assert(next_ptr != NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Exact-capacity allocation followed by growth handled correctly.\n");
    }

    // 4. Multiple allocations
    {
        PhaseArena *arena = phase_arena_create(128);

        void *first = phase_arena_alloc(arena, 16);
        void *second = phase_arena_alloc(arena, 32);
        void *third = phase_arena_alloc(arena, 64);

        assert(first != NULL);
        assert(second != NULL);
        assert(third != NULL);

        assert(first != second);
        assert(second != third);
        assert(first != third);

        phase_arena_destroy(arena);

        printf("[PASS] Multiple allocations handled correctly.\n");
    }

    // 5. NULL arena safety
    {
        void *ptr = phase_arena_alloc(NULL, 10);

        assert(ptr == NULL);

        phase_arena_reset(NULL); // Should not crash
        phase_arena_destroy(NULL); // Should not crash

        printf("[PASS] NULL arena safety handled correctly.\n");
    }

    printf("\nAll Edge Case Tests Passed Successfully!\n");

    return 0;
}