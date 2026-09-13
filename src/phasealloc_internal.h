#ifndef PHASEALLOC_INTERNAL_H
#define PHASEALLOC_INTERNAL_H

#include <stddef.h>

typedef struct PhaseChunk PhaseChunk;

struct PhaseChunk
{
    unsigned char *buffer;
    size_t capacity;
    size_t offset;

    PhaseChunk *next;
};

struct PhaseArena
{
    size_t current_used;
    size_t peak_offset;

    PhaseChunk *first_chunk;
    PhaseChunk *current_chunk;
};

PhaseChunk *phase_chunk_create(size_t capacity);
PhaseChunk *phase_chunk_create_next(PhaseChunk *current_chunk, size_t capacity);

#endif // PHASEALLOC_INTERNAL_H