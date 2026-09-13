#include "../include/phasealloc.h"
#include "phasealloc_internal.h"
#include <stdint.h>
#include <stdlib.h>

struct PhaseChunk *phase_chunk_create(size_t capacity)
{
    if (capacity == 0)
    {
        return NULL; // Return NULL if the requested capacity is zero
    }

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

static void *phase_chunk_alloc(PhaseChunk *chunk, size_t size)
{
    if (!chunk || size == 0)
    {
        return NULL; // Return NULL if the chunk is invalid or size is zero
    }

    if (chunk->offset > chunk->capacity)
    {
        return NULL; // Return NULL if the offset exceeds the chunk capacity
    }

    /*
     * malloc returns memory suitably aligned for any standard C type.
     * Use the same general alignment requirement for allocations made
     * inside the chunk.
     */
    size_t alignment = _Alignof(max_align_t);

    uintptr_t current_address =
        (uintptr_t)(chunk->buffer + chunk->offset);

    size_t remainder = current_address % alignment;
    size_t padding = 0;

    if (remainder != 0)
    {
        padding = alignment - remainder; // Calculate required alignment padding
    }

    if (padding > chunk->capacity - chunk->offset)
    {
        return NULL; // Return NULL if there is not enough space for alignment
    }

    if (size > chunk->capacity - chunk->offset - padding)
    {
        return NULL; // Return NULL if there is not enough space for the allocation
    }

    chunk->offset += padding; // Apply alignment padding

    void *memory = chunk->buffer + chunk->offset; // Get allocated memory address

    chunk->offset += size; // Increase used memory

    return memory;
}

PhaseArena *phase_arena_create(size_t capacity)
{
    if (capacity == 0)
    {
        return NULL; // Return NULL if the requested capacity is zero
    }

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

    arena->current_used = 0;
    arena->peak_offset = 0;
    arena->allocation_count = 0;
    arena->chunk_count = 1;

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
        size_t old_offset = chunk->offset;

        void *memory = phase_chunk_alloc(chunk, size);

        if (memory)
        {
            size_t bytes_used = chunk->offset - old_offset;

            arena->current_used += bytes_used;

            if (arena->current_used > arena->peak_offset)
            {
                arena->peak_offset = arena->current_used;
            }

            arena->allocation_count++;
            arena->current_chunk = chunk;

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

    size_t old_offset = new_chunk->offset;

    void *memory = phase_chunk_alloc(new_chunk, size);

    if (!memory)
    {
        // Remove the unused chunk if its allocation unexpectedly fails.
        chunk->next = NULL;
        free(new_chunk->buffer);
        free(new_chunk);

        return NULL; // Return NULL if the new chunk cannot satisfy the allocation
    }

    size_t bytes_used = new_chunk->offset - old_offset;

    arena->current_used += bytes_used;

    if (arena->current_used > arena->peak_offset)
    {
        arena->peak_offset = arena->current_used;
    }

    arena->allocation_count++;
    arena->chunk_count++;
    arena->current_chunk = new_chunk;

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

    arena->current_used = 0;
    arena->current_chunk = arena->first_chunk;
}

size_t phase_arena_get_current_memory(const PhaseArena *arena)
{
    if (!arena)
    {
        return 0;
    }

    return arena->current_used;
}

size_t phase_arena_get_peak_memory(const PhaseArena *arena)
{
    if (!arena)
    {
        return 0;
    }

    return arena->peak_offset;
}

size_t phase_arena_get_allocation_count(const PhaseArena *arena)
{
    if (!arena)
    {
        return 0;
    }

    return arena->allocation_count;
}

size_t phase_arena_get_chunk_count(const PhaseArena *arena)
{
    if (!arena)
    {
        return 0;
    }

    return arena->chunk_count;
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