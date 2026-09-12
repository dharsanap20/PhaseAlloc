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

JsonValue *json_parse(const char *input);

JsonValue *json_parse_phase(
    const char *input,
    PhaseArena *arena
);

void json_free(JsonValue *value);

#endif