#include "../include/phasealloc.h"
#include <stdlib.h>

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
    if (!arena || !arena->buffer) 
    {
        return NULL; // Invalid arena or uninitialized buffer
    }
    
    if (arena->offset + size > arena->capacity) 
    {
        return NULL; // Not enough space in the arena
    }

    void *memory = arena->buffer + arena->offset;
    arena->offset += size;

    return memory;
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
    

