#include "json.h"
#include "phasealloc.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Track PhaseAlloc allocation activity for benchmarking.
static size_t phase_alloc_calls = 0;

// Reset the PhaseAlloc allocation counter.
void json_reset_phase_stats(void)
{
    phase_alloc_calls = 0;
}

// Return the number of PhaseAlloc allocations.
size_t json_get_phase_alloc_calls(void)
{
    return phase_alloc_calls;
}

// Skip whitespace.
static const char *skip_whitespace(const char *input)
{
    while (isspace((unsigned char)*input))
    {
        input++;
    }

    return input;
}

// Forward declaration.
static JsonValue *parse_value(
    const char **input,
    PhaseArena *arena
);

// Allocate a JsonValue from PhaseAlloc.
static JsonValue *phase_alloc_value(PhaseArena *arena)
{
    phase_alloc_calls++;

    return phase_arena_alloc(arena, sizeof(JsonValue));
}

// Parse a JSON string.
static JsonValue *parse_string(
    const char **input,
    PhaseArena *arena
)
{
    const char *start = ++(*input);

    while (**input && **input != '"')
    {
        (*input)++;
    }

    if (**input != '"')
    {
        return NULL;
    }

    size_t length = *input - start;

    JsonValue *value = phase_alloc_value(arena);

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_STRING;

    // Allocate memory for the string itself.
    phase_alloc_calls++;
    value->data.string = phase_arena_alloc(
        arena,
        length + 1
    );

    if (!value->data.string)
    {
        return NULL;
    }

    for (size_t i = 0; i < length; i++)
    {
        value->data.string[i] = start[i];
    }

    value->data.string[length] = '\0';

    (*input)++;

    return value;
}

// Parse a JSON number.
static JsonValue *parse_number(
    const char **input,
    PhaseArena *arena
)
{
    char *end;

    double number = strtod(*input, &end);

    if (*input == end)
    {
        return NULL;
    }

    JsonValue *value = phase_alloc_value(arena);

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_NUMBER;
    value->data.number = number;

    *input = end;

    return value;
}

// Parse true.
static JsonValue *parse_true(
    const char **input,
    PhaseArena *arena
)
{
    if (strncmp(*input, "true", 4) != 0)
    {
        return NULL;
    }

    JsonValue *value = phase_alloc_value(arena);

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_BOOL;
    value->data.boolean = 1;

    *input += 4;

    return value;
}

// Parse false.
static JsonValue *parse_false(
    const char **input,
    PhaseArena *arena
)
{
    if (strncmp(*input, "false", 5) != 0)
    {
        return NULL;
    }

    JsonValue *value = phase_alloc_value(arena);

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_BOOL;
    value->data.boolean = 0;

    *input += 5;

    return value;
}

// Parse null.
static JsonValue *parse_null(
    const char **input,
    PhaseArena *arena
)
{
    if (strncmp(*input, "null", 4) != 0)
    {
        return NULL;
    }

    JsonValue *value = phase_alloc_value(arena);

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_NULL;

    *input += 4;

    return value;
}

// Grow an array inside the arena.
static void **phase_array_grow(
    PhaseArena *arena,
    void **old_items,
    size_t old_count,
    size_t *capacity
)
{
    size_t new_capacity;

    if (*capacity == 0)
    {
        new_capacity = 4;
    }
    else
    {
        new_capacity = *capacity * 2;
    }

    // Allocate new pointer storage inside the arena.
    phase_alloc_calls++;
    void **new_items = phase_arena_alloc(
        arena,
        new_capacity * sizeof(void *)
    );

    if (!new_items)
    {
        return NULL;
    }

    for (size_t i = 0; i < old_count; i++)
    {
        new_items[i] = old_items[i];
    }

    *capacity = new_capacity;

    return new_items;
}

// Parse a JSON array.
static JsonValue *parse_array(
    const char **input,
    PhaseArena *arena
)
{
    (*input)++;

    JsonValue *value = phase_alloc_value(arena);

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_ARRAY;
    value->data.array.items = NULL;
    value->data.array.count = 0;

    size_t capacity = 0;

    *input = skip_whitespace(*input);

    if (**input == ']')
    {
        (*input)++;
        return value;
    }

    while (**input)
    {
        JsonValue *item = parse_value(input, arena);

        if (!item)
        {
            return NULL;
        }

        if (value->data.array.count == capacity)
        {
            JsonValue **new_items =
                (JsonValue **)phase_array_grow(
                    arena,
                    (void **)value->data.array.items,
                    value->data.array.count,
                    &capacity
                );

            if (!new_items)
            {
                return NULL;
            }

            value->data.array.items = new_items;
        }

        value->data.array.items[value->data.array.count] = item;
        value->data.array.count++;

        *input = skip_whitespace(*input);

        if (**input == ']')
        {
            (*input)++;
            return value;
        }

        if (**input != ',')
        {
            return NULL;
        }

        (*input)++;
        *input = skip_whitespace(*input);
    }

    return NULL;
}

// Parse a JSON object.
static JsonValue *parse_object(
    const char **input,
    PhaseArena *arena
)
{
    (*input)++;

    JsonValue *value = phase_alloc_value(arena);

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_OBJECT;
    value->data.object.keys = NULL;
    value->data.object.values = NULL;
    value->data.object.count = 0;

    size_t key_capacity = 0;
    size_t value_capacity = 0;

    *input = skip_whitespace(*input);

    if (**input == '}')
    {
        (*input)++;
        return value;
    }

    while (**input)
    {
        *input = skip_whitespace(*input);

        if (**input != '"')
        {
            return NULL;
        }

        JsonValue *key_value = parse_string(input, arena);

        if (!key_value)
        {
            return NULL;
        }

        char *key = key_value->data.string;

        *input = skip_whitespace(*input);

        if (**input != ':')
        {
            return NULL;
        }

        (*input)++;
        *input = skip_whitespace(*input);

        JsonValue *item = parse_value(input, arena);

        if (!item)
        {
            return NULL;
        }

        if (value->data.object.count == key_capacity)
        {
            char **new_keys =
                (char **)phase_array_grow(
                    arena,
                    (void **)value->data.object.keys,
                    value->data.object.count,
                    &key_capacity
                );

            if (!new_keys)
            {
                return NULL;
            }

            value->data.object.keys = new_keys;
        }

        if (value->data.object.count == value_capacity)
        {
            JsonValue **new_values =
                (JsonValue **)phase_array_grow(
                    arena,
                    (void **)value->data.object.values,
                    value->data.object.count,
                    &value_capacity
                );

            if (!new_values)
            {
                return NULL;
            }

            value->data.object.values = new_values;
        }

        value->data.object.keys[value->data.object.count] = key;
        value->data.object.values[value->data.object.count] = item;

        value->data.object.count++;

        *input = skip_whitespace(*input);

        if (**input == '}')
        {
            (*input)++;
            return value;
        }

        if (**input != ',')
        {
            return NULL;
        }

        (*input)++;
        *input = skip_whitespace(*input);
    }

    return NULL;
}

// Decide which type of JSON value to parse.
static JsonValue *parse_value(
    const char **input,
    PhaseArena *arena
)
{
    *input = skip_whitespace(*input);

    if (**input == '"')
    {
        return parse_string(input, arena);
    }

    if (**input == '{')
    {
        return parse_object(input, arena);
    }

    if (**input == '[')
    {
        return parse_array(input, arena);
    }

    if (**input == 't')
    {
        return parse_true(input, arena);
    }

    if (**input == 'f')
    {
        return parse_false(input, arena);
    }

    if (**input == 'n')
    {
        return parse_null(input, arena);
    }

    if (**input == '-' ||
        isdigit((unsigned char)**input))
    {
        return parse_number(input, arena);
    }

    return NULL;
}

// Parse JSON using PhaseAlloc.
JsonValue *json_parse_phase(
    const char *input,
    PhaseArena *arena
)
{
    if (!input || !arena)
    {
        return NULL;
    }

    const char *cursor = input;

    JsonValue *value = parse_value(&cursor, arena);

    if (!value)
    {
        return NULL;
    }

    cursor = skip_whitespace(cursor);

    if (*cursor != '\0')
    {
        return NULL;
    }

    return value;
}