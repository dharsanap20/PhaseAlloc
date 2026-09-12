#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/phasealloc.h"

#define ITERATIONS 10

typedef struct
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
} Timer;

typedef struct
{
    const char *name;
    int allocation_count;
    int allocation_size;
} Workload;

void timer_start(Timer *timer)
{
    QueryPerformanceFrequency(&timer->frequency);
    QueryPerformanceCounter(&timer->start);
}

double timer_stop(Timer *timer)
{
    QueryPerformanceCounter(&timer->end);

    return (double)(timer->end.QuadPart - timer->start.QuadPart)
         / (double)(timer->frequency.QuadPart);
}

void run_workload(Workload workload)
{
    printf("\n========================================\n");
    printf("Workload: %s\n", workload.name);
    printf("========================================\n");

    printf("Allocations: %d\n", workload.allocation_count);
    printf("Allocation size: %d bytes\n", workload.allocation_size);
    printf("Iterations: %d\n", ITERATIONS);

    Timer timer;

    void **pointers = malloc(
        workload.allocation_count * sizeof(void *)
    );

    if (pointers == NULL)
    {
        printf("Failed to allocate pointer array.\n");
        return;
    }

    double total_allocation_time = 0.0;
    double total_free_time = 0.0;

    double total_phase_allocation_time = 0.0;
    double total_phase_reset_time = 0.0;

    unsigned long long malloc_checksum = 0;
    unsigned long long phase_checksum = 0;

    /*
     * malloc/free benchmark
     */

    for (int iteration = 0; iteration < ITERATIONS; iteration++)
    {
        timer_start(&timer);

        for (int i = 0; i < workload.allocation_count; i++)
        {
            pointers[i] = malloc(workload.allocation_size);

            if (pointers[i] == NULL)
            {
                printf(
                    "malloc failed at iteration %d, allocation %d.\n",
                    iteration,
                    i
                );

                for (int j = 0; j < i; j++)
                {
                    free(pointers[j]);
                }

                free(pointers);
                return;
            }

            for (int j = 0; j < workload.allocation_size; j++)
            {
                ((unsigned char *)pointers[i])[j] =
                    (unsigned char)(i + j);
            }

            for (int j = 0; j < workload.allocation_size; j++)
            {
                malloc_checksum +=
                    ((unsigned char *)pointers[i])[j];
            }
        }

        double allocation_time = timer_stop(&timer);
        total_allocation_time += allocation_time;

        timer_start(&timer);

        for (int i = 0; i < workload.allocation_count; i++)
        {
            free(pointers[i]);
        }

        double free_time = timer_stop(&timer);
        total_free_time += free_time;
    }

    /*
     * PhaseAlloc benchmark
     */

    PhaseArena arena = phase_arena_create(
        workload.allocation_count *
        workload.allocation_size *
        2
    );

    if (arena.buffer == NULL)
    {
        printf("Failed to create PhaseAlloc arena.\n");
        free(pointers);
        return;
    }

    for (int iteration = 0; iteration < ITERATIONS; iteration++)
    {
        timer_start(&timer);

        for (int i = 0; i < workload.allocation_count; i++)
        {
            void *memory = phase_arena_alloc(
                &arena,
                workload.allocation_size
            );

            if (memory == NULL)
            {
                printf(
                    "PhaseAlloc failed at iteration %d, allocation %d.\n",
                    iteration,
                    i
                );

                phase_arena_destroy(&arena);
                free(pointers);
                return;
            }

            for (int j = 0; j < workload.allocation_size; j++)
            {
                ((unsigned char *)memory)[j] =
                    (unsigned char)(i + j);
            }

            for (int j = 0; j < workload.allocation_size; j++)
            {
                phase_checksum +=
                    ((unsigned char *)memory)[j];
            }
        }

        double phase_allocation_time = timer_stop(&timer);
        total_phase_allocation_time += phase_allocation_time;

        timer_start(&timer);

        phase_arena_reset(&arena);

        double phase_reset_time = timer_stop(&timer);
        total_phase_reset_time += phase_reset_time;
    }

    /*
     * Calculate averages
     */

    double average_allocation_time =
        total_allocation_time / ITERATIONS;

    double average_free_time =
        total_free_time / ITERATIONS;

    double average_phase_allocation_time =
        total_phase_allocation_time / ITERATIONS;

    double average_phase_reset_time =
        total_phase_reset_time / ITERATIONS;

    double malloc_total_time =
        average_allocation_time + average_free_time;

    double phase_total_time =
        average_phase_allocation_time + average_phase_reset_time;

    double phase_ratio =
        phase_total_time / malloc_total_time;

    double improvement =
        (1.0 - phase_ratio) * 100.0;

    /*
     * Results
     */

    printf("\nResults:\n");

    printf(
        "Average malloc allocation: %.6f seconds\n",
        average_allocation_time
    );

    printf(
        "Average malloc free: %.6f seconds\n",
        average_free_time
    );

    printf(
        "Average PhaseAlloc allocation: %.6f seconds\n",
        average_phase_allocation_time
    );

    printf(
        "Average PhaseAlloc reset: %.6f seconds\n",
        average_phase_reset_time
    );

    printf("\nChecksums:\n");

    printf(
        "malloc checksum: %llu\n",
        malloc_checksum
    );

    printf(
        "PhaseAlloc checksum: %llu\n",
        phase_checksum
    );

    if (malloc_checksum == phase_checksum)
    {
        printf("Checksum verification: PASS\n");
    }
    else
    {
        printf("Checksum verification: FAIL\n");
    }

    printf("\nTotal workload time:\n");

    printf(
        "malloc/free: %.6f seconds\n",
        malloc_total_time
    );

    printf(
        "PhaseAlloc: %.6f seconds\n",
        phase_total_time
    );

    printf(
        "PhaseAlloc / malloc: %.2f%%\n",
        phase_ratio * 100.0
    );

    printf(
        "Relative improvement: %.2f%%\n",
        improvement
    );

    phase_arena_destroy(&arena);
    free(pointers);
}

int main(void)
{
    printf("PhaseAlloc Benchmark Suite\n");
    printf("===========================\n");

    Workload workloads[] =
    {
        {"A - 100,000 x 32 B", 100000, 32},
        {"B - 100,000 x 256 B", 100000, 256},
        {"C - 10,000 x 4 KB", 10000, 4096},
        {"D - 1,000 x 32 B", 1000, 32}
    };

    int workload_count =
        sizeof(workloads) / sizeof(workloads[0]);

    for (int i = 0; i < workload_count; i++)
    {
        run_workload(workloads[i]);
    }

    printf("\n========================================\n");
    printf("Benchmark suite complete.\n");
    printf("========================================\n");

    return 0;
}
