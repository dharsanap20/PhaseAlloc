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
    chunk->offset = 0; // Initialize the offset to 0, indicating no memory has been used yet
    chunk->next = NULL; // Initialize the next pointer to NULL, indicating no next chunk

    return chunk; // Return the pointer to the newly created PhaseChunk
}

struct PhaseChunk *phase_chunk_create_next (struct PhaseChunk *current_chunk, size_t capacity) 
{
    if (!current_chunk) 
    {
        return NULL; // Return NULL if the current chunk is NULL
    }

    struct PhaseChunk *new_chunk = phase_chunk_create(capacity); // Create a new chunk with the specified capacity

    if (!new_chunk) 
    {
        return NULL; // Return NULL if the new chunk creation fails
    }

    current_chunk->next = new_chunk; // Link the new chunk to the current chunk's next pointer

    return new_chunk; // Return the pointer to the newly created next chunk
}

PhaseArena phase_arena_create(size_t capacity) 
{
    PhaseArena arena = {0}; // Creates a arena of "PhaseArena" type and initializes it to zero
    
    PhaseChunk *first_chunk = phase_chunk_create(capacity); // Create the first chunk with the requested capacity

    if (!first_chunk) 
    {
        return arena; // Return the empty arena if chunk creation fails
    }

    arena.buffer = first_chunk->buffer; // Set the arena's buffer to the first chunk's buffer
    arena.capacity = first_chunk->capacity; // Set the arena's capacity to the first chunk's capacity
    arena.offset = first_chunk->offset; // Set the arena's offset to the first chunk's offset
    arena.peak_offset = 0; // Initialize peak memory usage to 0

    arena.first_chunk = first_chunk; // Set the first chunk of the arena
    arena.current_chunk = first_chunk; // Set the current chunk of the arena to the first chunk

    return arena;
}

void *phase_arena_alloc(PhaseArena *arena, size_t size) 
{
    if (!arena || !arena->current_chunk || size == 0) 
    {
        return NULL; // Return NULL if the arena is invalid or the requested size is zero
    }

    PhaseChunk *chunk = arena->current_chunk; // Get the current chunk from the arena

    while(chunk)
    {
        if (chunk->offset > chunk->capacity) 
        {
        return NULL; // Return NULL if the current offset exceeds the arena's capacity
        }

        uintptr_t current_address = (uintptr_t)(chunk->buffer + chunk->offset);

        uintptr_t remainder = current_address % 8; // Calculate the remainder when dividing the current address by 8

        size_t padding = 0;

        if (remainder != 0) 
        {
            padding = 8 - remainder; // Calculate the required padding to align to the next multiple of 8
        }
        
        if (padding <= chunk->capacity - chunk->offset && size <= chunk->capacity - chunk->offset -padding) 
        {
            chunk->offset += padding; // Apply the padding to the chunk's offset

            void *memory = chunk->buffer + chunk->offset; // Calculate the address of the allocated memory

            chunk->offset += size; // Update the chunk's offset to reflect the allocated memory

            arena->current_chunk = chunk; // Update the arena's current chunk to the current chunk
            arena->buffer = chunk->buffer; // Update the arena's buffer to the chunk's buffer
            arena->capacity = chunk->capacity; // Update the arena's capacity to the chunk's capacity
            arena->offset = chunk->offset; // Update the arena's offset to the chunk's offset

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
                arena->peak_offset = total_used; // Update peak total memory usage
            }

            return memory; // Return the pointer to the allocated memory
        }
        
        chunk = chunk->next; // Move to the next chunk if the current chunk does not have enough space
    }

    chunk = arena->first_chunk; // Reset to the first chunk if no suitable chunk was found

    while (chunk->next) 
    {
        chunk = chunk->next; // Traverse to the last chunk in the linked list
    }

    /*
    * The current does not have enough space.
    * Create a larger chunk
    */

    size_t new_capacity;

    if (chunk->capacity > SIZE_MAX / 2) 
    {
        new_capacity = size; // If the current capacity is greater than half of SIZE_MAX, set the new capacity to the requested size
    } 
    else 
    {
        new_capacity = chunk->capacity * 2; // Otherwise, double the current capacity for the new chunk
    }

    if (new_capacity < size) 
    {
        new_capacity = size; // Ensure the new capacity is at least as large as the requested size
    }

    PhaseChunk *new_chunk = phase_chunk_create_next(chunk, new_capacity); // Create a new chunk with the new capacity

    if (!new_chunk) 
    {
        return NULL; // Return NULL if the new chunk creation fails
    }

    uintptr_t new_address = (uintptr_t)new_chunk->buffer;

    uintptr_t new_remainder = new_address % 8; // Calculate the remainder for the new chunk's address
    
    size_t new_padding = 0;

    if (new_remainder != 0) 
    {
        new_padding = 8 - new_remainder; // Calculate the required padding for the new chunk
    }

    if (new_padding > new_chunk->capacity)
    {
        return NULL; // Return NULL if the required padding exceeds the new chunk's capacity
    }

    if (size > new_chunk->capacity - new_padding) 
    {
        return NULL; // Return NULL if the requested size exceeds the available space in the new chunk after padding
    }

    new_chunk->offset += new_padding; // Apply the padding to the new chunk's offset

    void *memory = new_chunk->buffer + new_chunk->offset; // Calculate the address of the allocated memory in the new chunk

    new_chunk->offset += size; // Update the new chunk's offset to reflect the allocated memory

    arena->current_chunk = new_chunk; // Update the arena's current chunk to the new chunk
    arena->buffer = new_chunk->buffer; // Update the arena's buffer to the new chunk's buffer
    arena->capacity = new_chunk->capacity; // Update the arena's capacity to the new chunk's capacity
    arena->offset = new_chunk->offset; // Update the arena's offset to the new chunk's offset

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
        arena->peak_offset = total_used; // Update peak total memory usage
    }

    return memory; // Return the pointer to the allocated memory in the new chunk
}

void phase_arena_reset(PhaseArena *arena) 
{
    if (!arena || !arena->first_chunk) 
    {
        return; // Return if the arena is invalid or has no first chunk
    }
    
    PhaseChunk *chunk = arena->first_chunk; // Start with the first chunk

    while (chunk) 
    {
        chunk->offset = 0; // Reset the offset of each chunk to 0
        chunk = chunk->next; // Move to the next chunk
    }

    arena->current_chunk = arena->first_chunk; // Reset the current chunk to the first chunk
    arena->buffer = arena->first_chunk->buffer; // Reset the arena's buffer to the first chunk's buffer
    arena->capacity = arena->first_chunk->capacity; // Reset the arena's capacity to the first chunk's capacity
    arena->offset = 0; // Reset the arena's offset to 0
}

void phase_arena_destroy(PhaseArena *arena) 
{
    if (!arena || !arena->first_chunk) 
    {
        return; // Return if the arena is invalid or has no first chunk
    }

    PhaseChunk *chunk = arena->first_chunk; // Start with the first chunk

    while (chunk) 
    {
        PhaseChunk *next_chunk = chunk->next; // Store the next chunk before freeing the current one
        free(chunk->buffer); // Free the memory buffer of the current chunk
        free(chunk); // Free the current chunk structure
        chunk = next_chunk; // Move to the next chunk
    }

    arena->buffer = NULL; // Reset the arena's buffer to NULL
    arena->capacity = 0; // Reset the arena's capacity to 0
    arena->offset = 0; // Reset the arena's offset to 0
    arena->peak_offset = 0; // Reset peak memory usage
    arena->first_chunk = NULL; // Reset the arena's first chunk to NULL
    arena->current_chunk = NULL; // Reset the arena's current chunk to NULL
}