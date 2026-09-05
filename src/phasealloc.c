#include "../include/phasealloc.h"
#include "phasealloc_internal.h"
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
    if (!arena || !arena->buffer || size == 0) 
    {
        return NULL; // Return NULL if the arena is invalid or the requested size is zero
    }

    if (arena->offset > arena->capacity) 
    {
        return NULL; // Return NULL if the current offset exceeds the arena's capacity
    }

    uintptr_t current_address = (uintptr_t)(arena->buffer + arena->offset);

    uintptr_t remainder = current_address % 8; // Calculate the remainder when dividing the current address by 8

    size_t padding = 0;

    if (remainder != 0) 
    {
        padding = 8 - remainder; // Calculate the required padding to align to the next multiple of 8
    }


    if (padding > arena->capacity - arena->offset) 
    {
        return NULL; // Return NULL if there is not enough space for the padding
    }

    size_t available_space = arena->capacity - arena->offset - padding;

    if (size > available_space) 
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
    

