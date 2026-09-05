#ifndef PHASEALLOC_H
#define PHASEALLOC_H

#include <stddef.h>

typedef struct PhaseChunk PhaseChunk; // Forward declaration

typedef struct 
{
    unsigned char *buffer; // Pointer that will hold the address of the arena's memory
    size_t capacity; // Will store the total size of the arena's memory in bytes
    size_t offset; // Will store how much of the arena's memory has been used

    PhaseChunk *first_chunk; // Pointer to the first chunk in the arena
    PhaseChunk *current_chunk; // Pointer to the current chunk in the arena
} PhaseArena;

PhaseArena phase_arena_create(size_t capacity); // Function to create a PhaseArena with a requested capacity
void *phase_arena_alloc(PhaseArena *arena, size_t size); // Function that receives a pointer (address) to a PhaseArena and a requested size, then will return the pointer (address) to the allocated memory
void phase_arena_reset(PhaseArena *arena); // Function that resets the arena by accessing its PhaseArena data
void phase_arena_destroy(PhaseArena *arena); // Function that destroys the arena by accessing its PhaseArena data
#endif // PHASEALLOC_H  