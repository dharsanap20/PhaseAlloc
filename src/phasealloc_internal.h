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

#endif // PHASEALLOC_INTERNAL_H