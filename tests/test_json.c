#include <assert.h>
#include <stdio.h>
#include "../src/json.h"
#include "../include/phasealloc.h"

int main(void)
{
    // Use the same JSON input as the PhaseAlloc test.
    const char *json =
        "{"
        "\"name\":\"PhaseAlloc\","
        "\"version\":3,"
        "\"working\":true,"
        "\"numbers\":[10,20,30]"
        "}";

    // Parse the JSON using the regular malloc/free parser.
    JsonValue *root = json_parse(json);

    // Make sure parsing succeeded and produced an object.
    assert(root != NULL);
    assert(root->type == JSON_OBJECT);
    assert(root->data.object.count == 4);

    // Check that each field has the correct JSON type.
    assert(root->data.object.values[0]->type == JSON_STRING);
    assert(root->data.object.values[1]->type == JSON_NUMBER);
    assert(root->data.object.values[2]->type == JSON_BOOL);
    assert(root->data.object.values[3]->type == JSON_ARRAY);

    // Check the actual values.
    assert(root->data.object.values[1]->data.number == 3);
    assert(root->data.object.values[2]->data.boolean == 1);

    // Check that the array contains the expected numbers.
    assert(root->data.object.values[3]->data.array.count == 3);
    assert(root->data.object.values[3]->data.array.items[0]->data.number == 10);
    assert(root->data.object.values[3]->data.array.items[1]->data.number == 20);
    assert(root->data.object.values[3]->data.array.items[2]->data.number == 30);

    // All checks passed.
    printf("malloc/free JSON test passed.\n");

    // Free the JSON structure using the regular recursive free function.
    json_free(root);

    return 0;
}