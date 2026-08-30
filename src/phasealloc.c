#include "../include/phasealloc.h"
#include <stdlib.h>

static size_t align_up(size_t size, size_t alignment) 
{
    return (size + alignment - 1) & ~(alignment - 1);
}

PhaseArena phase_arena_create(size_t capacity) 
{
    PhaseArena arena = {0}; // Creates a arena of "PhaseArena" type and initializes it to zero
    arena.buffer = malloc(capacity);
    if (arena.buffer)
    {
            arena.capacity = capacity;
            arena.offset = 0;
    }

    return arena;
}

void *phase_arena_alloc(PhaseArena *arena, size_t size) 
{
    if (!arena || !arena->buffer || size == 0) 
    {
        return NULL; // Return NULL if the arena is invalid or the requested size is zero
    }

    uintptr_t current_address = (uintptr_t)(arena->buffer + arena->offset);

    uintptr_t aligned_address = align_up(current_address, 8); // Align the address to the next multiple of 8
    
    size_t padding = aligned_address - current_address; // Calculate the padding needed for alignment

    if (padding > arena->capacity - arena->offset) 
    {
        return NULL; // Return NULL if there is not enough space for the padding
    }

    if (size > arena->capacity - arena->offset - padding) 
    {
        return NULL; // Return NULL if there is not enough space for the requested size after padding
    }

    arena->offset += padding; // Update the offset to account for the padding

    void *memory = arena->buffer + arena->offset; // Calculate the address of the allocated memory

    arena->offset += size; // Update the offset to account for the allocated size

    return memory; // Return the pointer to the allocated memory
}

void phase_arena_reset(PhaseArena *arena) 
{
    if (arena) 
    {
        arena->offset = 0; // Reset the offset to 0, effectively "freeing" all allocated memory
    }
}

void phase_arena_destroy(PhaseArena *arena) 
{
    if (arena && arena->buffer) 
    {
        free(arena->buffer); // Free the allocated memory for the arena
        
        arena->buffer = NULL; // Set the buffer pointer to NULL to avoid dangling pointer
        arena->capacity = 0; // Reset capacity to 0
        arena->offset = 0; // Reset offset to 0
    }
}
    

