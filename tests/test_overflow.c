#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("Running PhaseAlloc Overflow Tests...\n");

    // 1. Test an extremely large allocation
    {
        PhaseArena *arena = phase_arena_create(1024);

        assert(arena != NULL);

        void *ptr = phase_arena_alloc(arena, SIZE_MAX);

        assert(ptr == NULL);

        phase_arena_destroy(arena);

        printf("[PASS] SIZE_MAX allocation rejected safely.\n");
    }

    // 2. Test a huge allocation after using some arena memory
    {
        PhaseArena *arena = phase_arena_create(1024);

        assert(arena != NULL);

        void *first = phase_arena_alloc(arena, 16);

        assert(first != NULL);

        void *ptr = phase_arena_alloc(arena, SIZE_MAX);

        assert(ptr == NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Huge allocation after existing allocations rejected safely.\n");
    }

    // 3. Make sure the arena still works after a failed huge allocation
    {
        PhaseArena *arena = phase_arena_create(1024);

        assert(arena != NULL);

        void *failed = phase_arena_alloc(arena, SIZE_MAX);

        assert(failed == NULL);

        void *valid = phase_arena_alloc(arena, sizeof(int));

        assert(valid != NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Arena remains functional after failed huge allocation.\n");
    }

    printf("\nAll PhaseAlloc Overflow Tests Passed!\n");

    return 0;
}