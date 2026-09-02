#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("Running PhaseAlloc Overflow Tests...\n");

    // 1. Test an extremely large allocation
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);

        size_t original_offset = arena.offset;

        void *ptr = phase_arena_alloc(&arena, SIZE_MAX); // Request an extremely large allocation

        assert(ptr == NULL); // Should return NULL due to overflow
        assert(arena.offset == original_offset); // Offset should remain unchanged

        phase_arena_destroy(&arena);

        printf("[PASS] SIZE_MAX allocation rejected safely.\n");
    }

    // 2. Test a huge allocation after using some arena memory
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);

        void *first = phase_arena_alloc(&arena, 16); // Allocate some memory first

        assert(first != NULL);

        size_t original_offset = arena.offset;

        void *ptr = phase_arena_alloc(&arena, SIZE_MAX); // Request an extremely large allocation

        assert(ptr == NULL); // Should return NULL due to overflow
        assert(arena.offset == original_offset); // Offset should remain unchanged

        phase_arena_destroy(&arena);

        printf("[PASS] Huge allocation after existing allocations rejected safely.\n");
    }

    // 3. Make sure the arena still works after a failed huge allocation
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);

        void *failed = phase_arena_alloc(&arena, SIZE_MAX); // Request an extremely large allocation

        assert(failed == NULL); // Should return NULL due to overflow
        assert(arena.offset == 0); // Offset should remain unchanged

        void *valid = phase_arena_alloc(&arena, sizeof(int)); // Allocate a small valid size

        assert(valid != NULL);

        phase_arena_destroy(&arena); 

        printf("[PASS] Arena remains functional after failed huge allocation.\n");
    }

    printf("\nAll PhaseAlloc Overflow Tests Passed!\n");

    return 0;
}