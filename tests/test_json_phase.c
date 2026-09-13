#include <assert.h>
#include <stdio.h>
#include "../src/json.h"
#include "../include/phasealloc.h"

int main(void)
{
    // JSON input used to test the PhaseAlloc parser.
    const char *json =
        "{"
        "\"name\":\"PhaseAlloc\","
        "\"version\":3,"
        "\"working\":true,"
        "\"numbers\":[10,20,30]"
        "}";

    // Create an arena that the JSON parser will use for its allocations.
    PhaseArena *arena = phase_arena_create(4096);

    // Make sure the arena was created successfully.
    assert(arena != NULL);

    // Parse the JSON using PhaseAlloc.
    JsonValue *root = json_parse_phase(json, arena);

    // Make sure parsing succeeded and produced a JSON object.
    assert(root != NULL);
    assert(root->type == JSON_OBJECT);
    assert(root->data.object.count == 4);

    // Check that each field has the correct JSON type.
    assert(root->data.object.values[0]->type == JSON_STRING);
    assert(root->data.object.values[1]->type == JSON_NUMBER);
    assert(root->data.object.values[2]->type == JSON_BOOL);
    assert(root->data.object.values[3]->type == JSON_ARRAY);

    // Check the actual values inside the JSON object.
    assert(root->data.object.values[1]->data.number == 3);
    assert(root->data.object.values[2]->data.boolean == 1);

    // Check that the array contains the expected numbers.
    assert(root->data.object.values[3]->data.array.count == 3);
    assert(root->data.object.values[3]->data.array.items[0]->data.number == 10);
    assert(root->data.object.values[3]->data.array.items[1]->data.number == 20);
    assert(root->data.object.values[3]->data.array.items[2]->data.number == 30);

    // If all assertions passed, the parser produced the expected structure.
    printf("PhaseAlloc JSON test passed.\n");

    // Free the entire arena and everything allocated inside it.
    phase_arena_destroy(arena);

    return 0;
}