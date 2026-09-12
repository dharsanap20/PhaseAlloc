#include <stdio.h>
#include "../include/phasealloc.h"
#include "../src/json.h"

int main(void)
{
    const char *json =
        "{"
        "\"name\":\"PhaseAlloc\","
        "\"version\":3,"
        "\"working\":true,"
        "\"numbers\":[10,20,30]"
        "}";

    PhaseArena arena = phase_arena_create(4096);

    if (!arena.buffer)
    {
        printf("Failed to create arena.\n");
        return 1;
    }

    JsonValue *value = json_parse_phase(json, &arena);

    if (!value)
    {
        printf("JSON parsing failed.\n");
        phase_arena_destroy(&arena);
        return 1;
    }

    printf("JSON parsed successfully.\n");
    printf("Root type: %d\n", value->type);
    printf("Object fields: %zu\n", value->data.object.count);

    phase_arena_destroy(&arena);

    return 0;
}