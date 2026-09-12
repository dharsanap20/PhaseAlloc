#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const char *skip_whitespace(const char *input)
{
    while (isspace((unsigned char)*input))
    {
        input++;
    }

    return input;
}

static JsonValue *parse_value(const char **input);

static JsonValue *parse_string(const char **input)
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

    JsonValue *value = malloc(sizeof(JsonValue));

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_STRING;

    value->data.string = malloc(length + 1);

    if (!value->data.string)
    {
        free(value);
        return NULL;
    }

    memcpy(value->data.string, start, length);
    value->data.string[length] = '\0';

    (*input)++;

    return value;
}

static JsonValue *parse_number(const char **input)
{
    char *end;

    double number = strtod(*input, &end);

    if (*input == end)
    {
        return NULL;
    }

    JsonValue *value = malloc(sizeof(JsonValue));

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_NUMBER;
    value->data.number = number;

    *input = end;

    return value;
}

static JsonValue *parse_true(const char **input)
{
    if (strncmp(*input, "true", 4) != 0)
    {
        return NULL;
    }

    JsonValue *value = malloc(sizeof(JsonValue));

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_BOOL;
    value->data.boolean = 1;

    *input += 4;

    return value;
}

static JsonValue *parse_false(const char **input)
{
    if (strncmp(*input, "false", 5) != 0)
    {
        return NULL;
    }

    JsonValue *value = malloc(sizeof(JsonValue));

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_BOOL;
    value->data.boolean = 0;

    *input += 5;

    return value;
}

static JsonValue *parse_null(const char **input)
{
    if (strncmp(*input, "null", 4) != 0)
    {
        return NULL;
    }

    JsonValue *value = malloc(sizeof(JsonValue));

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_NULL;

    *input += 4;

    return value;
}

static JsonValue *parse_array(const char **input)
{
    (*input)++;

    JsonValue *value = malloc(sizeof(JsonValue));

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_ARRAY;
    value->data.array.items = NULL;
    value->data.array.count = 0;

    *input = skip_whitespace(*input);

    if (**input == ']')
    {
        (*input)++;
        return value;
    }

    while (**input)
    {
        JsonValue *item = parse_value(input);

        if (!item)
        {
            json_free(value);
            return NULL;
        }

        JsonValue **new_items = realloc(
            value->data.array.items,
            (value->data.array.count + 1) * sizeof(JsonValue *)
        );

        if (!new_items)
        {
            json_free(item);
            json_free(value);
            return NULL;
        }

        value->data.array.items = new_items;
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
            json_free(value);
            return NULL;
        }

        (*input)++;

        *input = skip_whitespace(*input);
    }

    json_free(value);
    return NULL;
}

static JsonValue *parse_object(const char **input)
{
    (*input)++;

    JsonValue *value = malloc(sizeof(JsonValue));

    if (!value)
    {
        return NULL;
    }

    value->type = JSON_OBJECT;
    value->data.object.keys = NULL;
    value->data.object.values = NULL;
    value->data.object.count = 0;

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
            json_free(value);
            return NULL;
        }

        JsonValue *key_value = parse_string(input);

        if (!key_value)
        {
            json_free(value);
            return NULL;
        }

        char *key = key_value->data.string;
        free(key_value);

        *input = skip_whitespace(*input);

        if (**input != ':')
        {
            free(key);
            json_free(value);
            return NULL;
        }

        (*input)++;

        *input = skip_whitespace(*input);

        JsonValue *item = parse_value(input);

        if (!item)
        {
            free(key);
            json_free(value);
            return NULL;
        }

        char **new_keys = realloc(
            value->data.object.keys,
            (value->data.object.count + 1) * sizeof(char *)
        );

        if (!new_keys)
        {
            free(key);
            json_free(item);
            json_free(value);
            return NULL;
        }

        JsonValue **new_values = realloc(
            value->data.object.values,
            (value->data.object.count + 1) * sizeof(JsonValue *)
        );

        if (!new_values)
        {
            free(key);
            json_free(item);
            value->data.object.keys = new_keys;
            json_free(value);
            return NULL;
        }

        value->data.object.keys = new_keys;
        value->data.object.values = new_values;

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
            json_free(value);
            return NULL;
        }

        (*input)++;
    }

    json_free(value);
    return NULL;
}

static JsonValue *parse_value(const char **input)
{
    *input = skip_whitespace(*input);

    if (**input == '"')
    {
        return parse_string(input);
    }

    if (**input == '{')
    {
        return parse_object(input);
    }

    if (**input == '[')
    {
        return parse_array(input);
    }

    if (**input == 't')
    {
        return parse_true(input);
    }

    if (**input == 'f')
    {
        return parse_false(input);
    }

    if (**input == 'n')
    {
        return parse_null(input);
    }

    if (**input == '-' || isdigit((unsigned char)**input))
    {
        return parse_number(input);
    }

    return NULL;
}

JsonValue *json_parse(const char *input)
{
    if (!input)
    {
        return NULL;
    }

    const char *cursor = input;

    JsonValue *value = parse_value(&cursor);

    if (!value)
    {
        return NULL;
    }

    cursor = skip_whitespace(cursor);

    if (*cursor != '\0')
    {
        json_free(value);
        return NULL;
    }

    return value;
}

void json_free(JsonValue *value)
{
    if (!value)
    {
        return;
    }

    if (value->type == JSON_STRING)
    {
        free(value->data.string);
    }
    else if (value->type == JSON_ARRAY)
    {
        for (size_t i = 0; i < value->data.array.count; i++)
        {
            json_free(value->data.array.items[i]);
        }

        free(value->data.array.items);
    }
    else if (value->type == JSON_OBJECT)
    {
        for (size_t i = 0; i < value->data.object.count; i++)
        {
            free(value->data.object.keys[i]);
            json_free(value->data.object.values[i]);
        }

        free(value->data.object.keys);
        free(value->data.object.values);
    }

    free(value);
}