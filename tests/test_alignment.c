#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "../include/phasealloc.h"

int main(void)
{
    PhaseArena *arena = phase_arena_create(1024);

    assert(arena != NULL);

    size_t alignment = _Alignof(max_align_t);

    // Test allocations of different sizes.
    for (size_t size = 1; size <= 64; size++)
    {
        void *memory = phase_arena_alloc(arena, size);

        assert(memory != NULL);
        assert((uintptr_t)memory % alignment == 0);
    }

    // Test many allocations to exercise alignment across the arena.
    for (int i = 0; i < 100; i++)
    {
        void *memory = phase_arena_alloc(arena, sizeof(double));

        assert(memory != NULL);
        assert((uintptr_t)memory % alignment == 0);
    }

    phase_arena_destroy(arena);

    printf("[PASS] All allocations are aligned to max_align_t.\n");
    printf("Alignment requirement: %zu bytes\n", alignment);

    printf("\nAll Alignment tests passed!\n");

    return 0;
}