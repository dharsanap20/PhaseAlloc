#include <stdio.h>
#include <time.h>
#include "../src/json.h"
#include "../include/phasealloc.h"

#define ITERATIONS 10000

typedef struct
{
    const char *name;
    const char *json;
} JsonWorkload;

// Run one JSON workload through both memory strategies.
static void run_workload(const JsonWorkload *workload)
{
    clock_t malloc_start;
    clock_t malloc_end;
    clock_t phase_start;
    clock_t phase_end;

    long long malloc_checksum = 0;
    long long phase_checksum = 0;

    // Benchmark the regular malloc/free parser.
    malloc_start = clock();

    for (int i = 0; i < ITERATIONS; i++)
    {
        JsonValue *root = json_parse(workload->json);

        if (!root)
        {
            printf("malloc/free parser failed.\n");
            return;
        }

        // Read values from the parsed structure.
        malloc_checksum += root->data.object.count;

        // Free the JSON structure.
        json_free(root);
    }

    malloc_end = clock();

    // Benchmark the PhaseAlloc parser.
    phase_start = clock();

    for (int i = 0; i < ITERATIONS; i++)
    {
        // Create a fresh arena for each JSON document.
        PhaseArena arena = phase_arena_create(16384);

        if (!arena.buffer)
        {
            printf("PhaseAlloc arena creation failed.\n");
            return;
        }

        JsonValue *root = json_parse_phase(workload->json, &arena);

        if (!root)
        {
            printf("PhaseAlloc parser failed.\n");
            phase_arena_destroy(&arena);
            return;
        }

        // Read the same information from the PhaseAlloc structure.
        phase_checksum += root->data.object.count;

        // Destroy the arena and release all its memory.
        phase_arena_destroy(&arena);
    }

    phase_end = clock();

    double malloc_time =
        (double)(malloc_end - malloc_start) / CLOCKS_PER_SEC;

    double phase_time =
        (double)(phase_end - phase_start) / CLOCKS_PER_SEC;

    double ratio = phase_time / malloc_time;
    double improvement = (1.0 - ratio) * 100.0;

    printf("\n%s\n", workload->name);
    printf("------------------------------\n");
    printf("malloc/free time: %.6f seconds\n", malloc_time);
    printf("PhaseAlloc time:  %.6f seconds\n", phase_time);
    printf("PhaseAlloc / malloc ratio: %.2f%%\n", ratio * 100.0);
    printf("Relative improvement:      %.2f%%\n", improvement);

    // Verify that both parsers produced the same result.
    if (malloc_checksum == phase_checksum)
    {
        printf("Checksum: %lld\n", malloc_checksum);
        printf("PASS: Results match.\n");
    }
    else
    {
        printf("malloc checksum: %lld\n", malloc_checksum);
        printf("PhaseAlloc checksum: %lld\n", phase_checksum);
        printf("FAIL: Results do not match.\n");
    }
}

int main(void)
{
    // Small object with only a few fields.
    const char *small_json =
        "{"
        "\"name\":\"PhaseAlloc\","
        "\"version\":3,"
        "\"working\":true,"
        "\"value\":10"
        "}";

    // Medium object containing several arrays and values.
    const char *medium_json =
        "{"
        "\"name\":\"PhaseAlloc\","
        "\"version\":3,"
        "\"working\":true,"
        "\"numbers\":[10,20,30,40,50,60,70,80,90,100],"
        "\"scores\":[1,2,3,4,5,6,7,8,9,10],"
        "\"status\":\"active\""
        "}";

    // Larger nested JSON structure.
    const char *large_json =
        "{"
        "\"name\":\"PhaseAlloc\","
        "\"version\":3,"
        "\"working\":true,"
        "\"data\":{"
            "\"numbers\":[10,20,30,40,50,60,70,80,90,100],"
            "\"values\":[1,2,3,4,5,6,7,8,9,10],"
            "\"info\":{"
                "\"enabled\":true,"
                "\"count\":100,"
                "\"type\":\"test\""
            "}"
        "},"
        "\"status\":\"active\""
        "}";

    // Generate a JSON document containing many small objects.
    static char allocation_heavy_json[65536];

    int position = 0;

    position += snprintf(
        allocation_heavy_json + position,
        sizeof(allocation_heavy_json) - position,
        "{\"items\":["
    );

    for (int i = 0; i < 500; i++)
    {
        position += snprintf(
            allocation_heavy_json + position,
            sizeof(allocation_heavy_json) - position,
            "{\"id\":%d,\"value\":%d}%s",
            i,
            i * 10,
            (i == 499) ? "" : ","
        );
    }

    snprintf(
        allocation_heavy_json + position,
        sizeof(allocation_heavy_json) - position,
        "]}"
    );

    // Workloads to test.
    JsonWorkload workloads[] =
    {
        {"Small JSON", small_json},
        {"Medium JSON", medium_json},
        {"Large Nested JSON", large_json},
        {"Allocation-Heavy JSON", allocation_heavy_json}
    };

    size_t workload_count =
        sizeof(workloads) / sizeof(workloads[0]);

    printf("JSON Workload Benchmark\n");
    printf("=======================\n");
    printf("Iterations per workload: %d\n", ITERATIONS);

    // Run every workload through both parsers.
    for (size_t i = 0; i < workload_count; i++)
    {
        run_workload(&workloads[i]);
    }

    return 0;
}