#ifndef PHASEALLOC_INTERNAL_H
#define PHASEALLOC_INTERNAL_H

#include <stddef.h>

struct PhaseChunk 
{
    unsigned char *buffer; // Pointer to the chunk's memory buffer
    size_t capacity; // Total size of the chunk's memory in bytes
    size_t offset; // Amount of memory used in the chunk
    
    struct PhaseChunk *next; 
};

struct PhaseChunk *phase_chunk_create(size_t capacity); // Function to create a PhaseChunk with a requested capacity

#endif // PHASEALLOC_INTERNAL_H