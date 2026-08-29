#include <stdio.h>
#include "../include/phasealloc.h"

int main(void)
{
    printf("PhaseAlloc Basic Example:\n");

    PhaseArena arena = phase_arena_create(1024); // 1. Create an arena with 1024 bytes

    if (arena.buffer == NULL)
    {
        printf("Failed to create arena buffer.\n");
        return 1;
    }

    printf("Arena created successfully with %zu bytes\n", arena.capacity);

    int *numbers = phase_arena_alloc(&arena, 5 * sizeof(int)); // 2. Allocate space for 5 integers

    if (numbers == NULL)
    {
        printf("Allocation failed!\n");
        phase_arena_destroy(&arena);
        return 1;
    }

    for (int i = 0; i < 5; i++) // Store values in the allocated memory
    {
        numbers[i] = (i + 1) * 10; 
        printf("numbers[%d] = %d\n", i, numbers[i]); // Print the values stored in the allocated memory
    }

    printf("Current arena offset: %zu\n", arena.offset); // Print the current offset of the arena
    
    printf("\n");

    phase_arena_reset(&arena); // 3. Reset the arena so its memory can be reused
    
    printf("Arena reset. Current offset: %zu bytes.\n", arena.offset);

    phase_arena_destroy(&arena); // 4. Destroy the arena and free its memory

    printf("Arena destroyed successfully.\n");

    return 0;

}
