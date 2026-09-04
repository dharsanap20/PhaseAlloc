#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("Running PhaseAlloc Reset & Reuse Tests...\n");

    // 1. Test that reset returns the offset to zero
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);
        assert(arena.offset == 0);

        void *ptr1 = phase_arena_alloc(&arena, 128);
        void *ptr2 = phase_arena_alloc(&arena, 256);

        assert(ptr1 != NULL);
        assert(ptr2 != NULL);
        assert(arena.offset > 0);

        phase_arena_reset(&arena);

        assert(arena.offset == 0); // After reset, offset should be zero

        phase_arena_destroy(&arena);

        printf("[PASS] Reset returns offset to zero.\n");
    }

    // 2. Test that memory can be reused after reset
    {
        PhaseArena arena = phase_arena_create(512);

        assert(arena.buffer != NULL);

        void *first_pass = phase_arena_alloc(&arena, 256);

        assert(first_pass != NULL);

        phase_arena_reset(&arena);

        assert(arena.offset == 0); // After reset, offset should be zero

        void *second_pass = phase_arena_alloc(&arena, 256);

        assert(second_pass != NULL);

        assert(first_pass == second_pass); // After reset, the same memory should be reused

        phase_arena_destroy(&arena);

        printf("[PASS] Memory can be reused after reset.\n");      
    }

    // 3. Test repeated reset and reuse cycles
    {
        PhaseArena arena = phase_arena_create(64);

        assert(arena.buffer != NULL);

        for (int i = 0; i < 1000; ++i) 
        {
            void *ptr = phase_arena_alloc(&arena, 32);

            assert(ptr != NULL);
            assert(arena.offset > 0);

            phase_arena_reset(&arena);

            assert(arena.offset == 0); // After reset, offset should be zero
        }

        phase_arena_destroy(&arena);

        printf("[PASS] 1000 cycles of reset and reuse completed successfully.\n");
    }

    // 4. Test that reset preserves the arena buffer and capacity
    {
        PhaseArena arena = phase_arena_create(1024);

        assert(arena.buffer != NULL);

        unsigned char *original_buffer = arena.buffer;

        void *ptr = phase_arena_alloc(&arena, 128);

        assert(ptr != NULL);

        phase_arena_reset(&arena);

        assert(arena.offset == 0); // After reset, offset should be zero
        assert(arena.buffer == original_buffer); // Buffer should remain the same
        assert(arena.capacity == 1024); // Capacity should remain the same

        phase_arena_destroy(&arena);

        printf("[PASS] Reset preserves arena buffer and capacity.\n");
    }

    printf("\nAll PhaseAlloc Reset & Reuse Tests Passed Successfully!\n");

    return 0;
}