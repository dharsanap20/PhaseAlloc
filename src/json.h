#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include "phasealloc.h"

typedef enum
{
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonValue JsonValue;

struct JsonValue
{
    JsonType type;

    union
    {
        int boolean;
        double number;
        char *string;

        struct
        {
            JsonValue **items;
            size_t count;
        } array;

        struct
        {
            char **keys;
            JsonValue **values;
            size_t count;
        } object;
    } data;
};

// Parse JSON using malloc/free.
JsonValue *json_parse(const char *input);

// Parse JSON using PhaseAlloc.
JsonValue *json_parse_phase(
    const char *input,
    PhaseArena *arena
);

// Free a JSON structure created with malloc/free.
void json_free(JsonValue *value);

// Reset malloc/realloc allocation counters.
void json_reset_malloc_stats(void);

// Get malloc/realloc allocation counters.
void json_get_malloc_stats(
    size_t *malloc_count,
    size_t *realloc_count
);

// Reset the PhaseAlloc allocation counter.
void json_reset_phase_stats(void);

// Get the number of PhaseAlloc allocations.
size_t json_get_phase_alloc_calls(void);

#endif // JSON_H