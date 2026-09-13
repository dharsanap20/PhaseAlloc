#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("Running PhaseAlloc Reset & Reuse Tests...\n");

    // 1. Test that reset allows the arena to be reused
    {
        PhaseArena *arena = phase_arena_create(1024);

        assert(arena != NULL);

        void *ptr1 = phase_arena_alloc(arena, 128);
        void *ptr2 = phase_arena_alloc(arena, 256);

        assert(ptr1 != NULL);
        assert(ptr2 != NULL);

        phase_arena_reset(arena);

        // A new allocation should succeed after reset.
        void *after_reset = phase_arena_alloc(arena, 128);

        assert(after_reset != NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Reset allows the arena to be reused.\n");
    }

    // 2. Test that memory can be reused after reset
    {
        PhaseArena *arena = phase_arena_create(512);

        assert(arena != NULL);

        void *first_pass = phase_arena_alloc(arena, 256);

        assert(first_pass != NULL);

        phase_arena_reset(arena);

        void *second_pass = phase_arena_alloc(arena, 256);

        assert(second_pass != NULL);

        assert(first_pass == second_pass);

        phase_arena_destroy(arena);

        printf("[PASS] Memory can be reused after reset.\n");
    }

    // 3. Test repeated reset and reuse cycles
    {
        PhaseArena *arena = phase_arena_create(64);

        assert(arena != NULL);

        for (int i = 0; i < 1000; ++i)
        {
            void *ptr = phase_arena_alloc(arena, 32);

            assert(ptr != NULL);

            phase_arena_reset(arena);
        }

        phase_arena_destroy(arena);

        printf("[PASS] 1000 cycles of reset and reuse completed successfully.\n");
    }

    // 4. Test that reset preserves usable arena capacity
    {
        PhaseArena *arena = phase_arena_create(1024);

        assert(arena != NULL);

        void *ptr = phase_arena_alloc(arena, 128);

        assert(ptr != NULL);

        phase_arena_reset(arena);

        // The original 1024-byte arena should still be usable.
        void *full_allocation = phase_arena_alloc(arena, 1024);

        assert(full_allocation != NULL);

        phase_arena_destroy(arena);

        printf("[PASS] Reset preserves usable arena capacity.\n");
    }

    printf("\nAll PhaseAlloc Reset & Reuse Tests Passed Successfully!\n");

    return 0;
}