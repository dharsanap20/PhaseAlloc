#include "../include/phasealloc.h"
#include "phasealloc_internal.h"
#include <stdint.h>
#include <stdlib.h>

struct PhaseChunk *phase_chunk_create(size_t capacity)
{
    struct PhaseChunk *chunk = malloc(sizeof(struct PhaseChunk)); // Allocate memory for a new PhaseChunk structure

    if (!chunk)
    {
        return NULL; // Return NULL if memory allocation for the chunk structure fails
    }

    chunk->buffer = malloc(capacity); // Allocate memory for the chunk's buffer

    if (!chunk->buffer)
    {
        free(chunk); // Free the allocated chunk structure if buffer allocation fails
        return NULL; // Return NULL if memory allocation for the buffer fails
    }

    chunk->capacity = capacity; // Set the capacity of the chunk
    chunk->offset = 0; // Initialize the offset to 0
    chunk->next = NULL; // Initialize the next pointer to NULL

    return chunk; // Return the pointer to the newly created PhaseChunk
}

struct PhaseChunk *phase_chunk_create_next(struct PhaseChunk *current_chunk, size_t capacity)
{
    if (!current_chunk)
    {
        return NULL; // Return NULL if the current chunk is NULL
    }

    struct PhaseChunk *new_chunk = phase_chunk_create(capacity); // Create a new chunk

    if (!new_chunk)
    {
        return NULL; // Return NULL if the new chunk creation fails
    }

    current_chunk->next = new_chunk; // Link the new chunk to the current chunk

    return new_chunk; // Return the new chunk
}

PhaseArena *phase_arena_create(size_t capacity)
{
    // Allocate the PhaseArena structure internally.
    PhaseArena *arena = malloc(sizeof(PhaseArena));

    if (!arena)
    {
        return NULL; // Return NULL if the arena structure cannot be allocated
    }

    *arena = (PhaseArena){0}; // Initialize all arena fields to zero

    PhaseChunk *first_chunk = phase_chunk_create(capacity); // Create the first chunk

    if (!first_chunk)
    {
        free(arena); // Free the arena if the first chunk cannot be created
        return NULL;
    }

    arena->buffer = first_chunk->buffer;
    arena->capacity = first_chunk->capacity;
    arena->offset = first_chunk->offset;
    arena->peak_offset = 0;

    arena->first_chunk = first_chunk;
    arena->current_chunk = first_chunk;

    return arena; // Return a pointer to the arena
}

void *phase_arena_alloc(PhaseArena *arena, size_t size)
{
    if (!arena || !arena->current_chunk || size == 0)
    {
        return NULL; // Return NULL if the arena is invalid or size is zero
    }

    PhaseChunk *chunk = arena->current_chunk; // Get the current chunk

    while (chunk)
    {
        if (chunk->offset > chunk->capacity)
        {
            return NULL; // Return NULL if the offset exceeds the chunk capacity
        }

        uintptr_t current_address = (uintptr_t)(chunk->buffer + chunk->offset);

        uintptr_t remainder = current_address % 8; // Calculate alignment remainder

        size_t padding = 0;

        if (remainder != 0)
        {
            padding = 8 - remainder; // Calculate required padding
        }

        if (padding <= chunk->capacity - chunk->offset &&
            size <= chunk->capacity - chunk->offset - padding)
        {
            chunk->offset += padding; // Apply alignment padding

            void *memory = chunk->buffer + chunk->offset; // Get allocated memory address

            chunk->offset += size; // Increase used memory

            arena->current_chunk = chunk;
            arena->buffer = chunk->buffer;
            arena->capacity = chunk->capacity;
            arena->offset = chunk->offset;

            // Calculate total memory currently used across all chunks.
            size_t total_used = 0;
            PhaseChunk *used_chunk = arena->first_chunk;

            while (used_chunk)
            {
                total_used += used_chunk->offset;
                used_chunk = used_chunk->next;
            }

            if (total_used > arena->peak_offset)
            {
                arena->peak_offset = total_used;
            }

            return memory;
        }

        chunk = chunk->next; // Try the next existing chunk
    }

    // No existing chunk has enough space.
    // Find the last chunk.
    chunk = arena->first_chunk;

    while (chunk->next)
    {
        chunk = chunk->next;
    }

    /*
     * The current chunks do not have enough space.
     * Create a larger chunk.
     */

    size_t new_capacity;

    if (chunk->capacity > SIZE_MAX / 2)
    {
        new_capacity = size; // Prevent capacity overflow
    }
    else
    {
        new_capacity = chunk->capacity * 2; // Double the previous capacity
    }

    if (new_capacity < size)
    {
        new_capacity = size; // Ensure the new chunk can hold the request
    }

    PhaseChunk *new_chunk =
        phase_chunk_create_next(chunk, new_capacity);

    if (!new_chunk)
    {
        return NULL; // Return NULL if the new chunk cannot be created
    }

    uintptr_t new_address = (uintptr_t)new_chunk->buffer;

    uintptr_t new_remainder = new_address % 8; // Calculate alignment remainder

    size_t new_padding = 0;

    if (new_remainder != 0)
    {
        new_padding = 8 - new_remainder;
    }

    if (new_padding > new_chunk->capacity)
    {
        return NULL;
    }

    if (size > new_chunk->capacity - new_padding)
    {
        return NULL;
    }

    new_chunk->offset += new_padding; // Apply alignment padding

    void *memory =
        new_chunk->buffer + new_chunk->offset; // Get allocated memory address

    new_chunk->offset += size; // Increase used memory

    arena->current_chunk = new_chunk;
    arena->buffer = new_chunk->buffer;
    arena->capacity = new_chunk->capacity;
    arena->offset = new_chunk->offset;

    // Calculate total memory currently used across all chunks.
    size_t total_used = 0;
    PhaseChunk *used_chunk = arena->first_chunk;

    while (used_chunk)
    {
        total_used += used_chunk->offset;
        used_chunk = used_chunk->next;
    }

    if (total_used > arena->peak_offset)
    {
        arena->peak_offset = total_used;
    }

    return memory;
}

void phase_arena_reset(PhaseArena *arena)
{
    if (!arena || !arena->first_chunk)
    {
        return; // Return if the arena is invalid
    }

    PhaseChunk *chunk = arena->first_chunk;

    while (chunk)
    {
        chunk->offset = 0; // Reset each chunk
        chunk = chunk->next;
    }

    arena->current_chunk = arena->first_chunk;
    arena->buffer = arena->first_chunk->buffer;
    arena->capacity = arena->first_chunk->capacity;
    arena->offset = 0;
}

void phase_arena_destroy(PhaseArena *arena)
{
    if (!arena)
    {
        return; // Nothing to destroy
    }

    PhaseChunk *chunk = arena->first_chunk;

    while (chunk)
    {
        PhaseChunk *next_chunk = chunk->next;

        free(chunk->buffer); // Free chunk memory
        free(chunk); // Free chunk structure

        chunk = next_chunk;
    }

    free(arena); // Free the PhaseArena structure itself
}