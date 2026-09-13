#ifndef PHASEALLOC_H
#define PHASEALLOC_H

#include <stddef.h>

typedef struct PhaseArena PhaseArena;

PhaseArena *phase_arena_create(size_t capacity);
void *phase_arena_alloc(PhaseArena *arena, size_t size);
void phase_arena_reset(PhaseArena *arena);
void phase_arena_destroy(PhaseArena *arena);
size_t phase_arena_get_peak_memory(const PhaseArena *arena);

#endif // PHASEALLOC_H