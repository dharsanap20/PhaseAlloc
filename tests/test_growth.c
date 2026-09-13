#include <stdio.h>
#include <assert.h>
#include "../include/phasealloc.h"

void test_basic_growth(void)
{
    printf("Testing basic growth tests...\n");

    PhaseArena *arena = phase_arena_create(64);

    assert(arena != NULL);

    void *ptr1 = phase_arena_alloc(arena, 32);
    assert(ptr1 != NULL);

    void *ptr2 = phase_arena_alloc(arena, 128);
    assert(ptr2 != NULL);

    assert(ptr1 != ptr2);

    phase_arena_destroy(arena);

    printf("[PASS] Basic growth test passed!\n");
}

void test_multiple_growths(void)
{
    printf("Testing multiple growth events...\n");

    PhaseArena *arena = phase_arena_create(64);

    assert(arena != NULL);

    void *ptr1 = phase_arena_alloc(arena, 32);
    assert(ptr1 != NULL);

    void *ptr2 = phase_arena_alloc(arena, 128);
    assert(ptr2 != NULL);

    void *ptr3 = phase_arena_alloc(arena, 256);
    assert(ptr3 != NULL);

    void *ptr4 = phase_arena_alloc(arena, 512);
    assert(ptr4 != NULL);

    assert(ptr1 != ptr2);
    assert(ptr2 != ptr3);
    assert(ptr3 != ptr4);

    phase_arena_destroy(arena);

    printf("[PASS] Multiple growths test passed!\n");
}

void test_old_allocations_survive_growth(void)
{
    printf("Testing old allocations after growth...\n");

    PhaseArena *arena = phase_arena_create(64);

    int *value = phase_arena_alloc(arena, sizeof(int));
    assert(value != NULL);

    *value = 42;

    void *ptr2 = phase_arena_alloc(arena, 128);
    assert(ptr2 != NULL);

    void *ptr3 = phase_arena_alloc(arena, 256);
    assert(ptr3 != NULL);

    assert(*value == 42);

    phase_arena_destroy(arena);

    printf("[PASS] Old allocations survive growth test passed!\n");
}

void test_reset_after_growth(void)
{
    printf("Testing reset after growth...\n");

    PhaseArena *arena = phase_arena_create(64);

    void *ptr1 = phase_arena_alloc(arena, 32);
    assert(ptr1 != NULL);

    void *ptr2 = phase_arena_alloc(arena, 128);
    assert(ptr2 != NULL);

    void *ptr3 = phase_arena_alloc(arena, 256);
    assert(ptr3 != NULL);

    phase_arena_reset(arena);

    // After reset, the arena should be usable again.
    void *ptr4 = phase_arena_alloc(arena, 32);
    assert(ptr4 != NULL);

    phase_arena_destroy(arena);

    printf("[PASS] Reset after growth test passed!\n");
}

void test_reuse_after_reset(void)
{
    printf("Testing reuse after reset...\n");

    PhaseArena *arena = phase_arena_create(64);

    void *ptr1 = phase_arena_alloc(arena, 32);
    assert(ptr1 != NULL);

    void *ptr2 = phase_arena_alloc(arena, 128);
    assert(ptr2 != NULL);

    phase_arena_reset(arena);

    void *ptr3 = phase_arena_alloc(arena, 32);
    assert(ptr3 != NULL);

    assert(ptr3 == ptr1);

    phase_arena_reset(arena);

    void *ptr4 = phase_arena_alloc(arena, 32);
    assert(ptr4 != NULL);

    assert(ptr4 == ptr1);

    phase_arena_destroy(arena);

    printf("[PASS] Reuse after reset test passed!\n");
}

void test_failure_cases(void)
{
    printf("Testing failure cases...\n");

    void *ptr1 = phase_arena_alloc(NULL, 32);

    assert(ptr1 == NULL);

    PhaseArena *arena = phase_arena_create(64);

    assert(arena != NULL);

    void *ptr2 = phase_arena_alloc(arena, 0);

    assert(ptr2 == NULL);

    void *ptr3 = phase_arena_alloc(arena, 32);
    assert(ptr3 != NULL);

    phase_arena_destroy(arena);

    printf("[PASS] Failure cases test passed!\n");
}

void test_stress_growth(void)
{
    printf("Testing stress growth...\n");

    PhaseArena *arena = phase_arena_create(64);

    assert(arena != NULL);

    for (int i = 0; i < 1000; i++)
    {
        size_t size = (i % 128) + 1;

        unsigned char *memory = phase_arena_alloc(arena, size);

        assert(memory != NULL);

        for (size_t j = 0; j < size; j++)
        {
            memory[j] = (unsigned char)(i % 256);
        }

        for (size_t j = 0; j < size; j++)
        {
            assert(memory[j] == (unsigned char)(i % 256));
        }
    }

    phase_arena_reset(arena);

    for (int i = 0; i < 100; i++)
    {
        void *memory = phase_arena_alloc(arena, 32);

        assert(memory != NULL);
    }

    phase_arena_destroy(arena);

    printf("[PASS] Stress growth test passed!\n");
}

int main(void)
{
    printf("Running Stage 3 growth tests...\n");

    test_basic_growth();
    test_multiple_growths();
    test_old_allocations_survive_growth();
    test_reset_after_growth();
    test_reuse_after_reset();
    test_failure_cases();
    test_stress_growth();

    printf("\nAll growth tests passed!\n");

    return 0;
}