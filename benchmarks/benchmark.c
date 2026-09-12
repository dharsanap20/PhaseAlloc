#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/phasealloc.h"

#define ALLOCATION_COUNT 1000 // Number of allocations to perform in the benchmark
#define ALLOCATION_SIZE 32 // Size of each allocation in bytes
#define ITERATIONS 10 // Number of iterations to run the benchmark

typedef struct
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
} Timer;

void timer_start(Timer *timer)
{
    QueryPerformanceFrequency(&timer->frequency);
    QueryPerformanceCounter(&timer->start);
}

double timer_stop(Timer *timer)
{
    QueryPerformanceCounter(&timer->end);
    return (double)(timer->end.QuadPart - timer->start.QuadPart) / (double)(timer->frequency.QuadPart);
}

int main(void)
{
    printf("PhaseAlloc Benchmark\n");
    printf("=====================\n");

    printf("Allocations per iteration: %d\n", ALLOCATION_COUNT);
    printf("Allocation size: %d bytes\n", ALLOCATION_SIZE);
    printf("Iterations: %d\n", ITERATIONS);

    Timer timer;

    void **pointers = malloc(ALLOCATION_COUNT * sizeof(void *)); // Allocate an array to hold pointers to allocated memory

    if (pointers == NULL)
    {
        printf("Failed to allocate memory for pointers array.\n");
        return 1;
    }

    double total_allocation_time = 0.0;
    double total_free_time = 0.0;

    double total_phase_allocation_time = 0.0;
    double total_phase_reset_time = 0.0;

    unsigned long long malloc_checksum = 0;
    unsigned long long phase_checksum = 0;

    // malloc/free benchmark
    for (int iteration = 0; iteration < ITERATIONS; iteration++)
    {
        timer_start(&timer);

        for (int i = 0; i < ALLOCATION_COUNT; i++)
        {
            pointers[i] = malloc(ALLOCATION_SIZE); // Allocate memory for each pointer
            
            if (pointers[i] == NULL)
            {
                printf("Memory allocation failed at iteration %d, allocation %d.\n", iteration, i);

                for (int j = 0; j < i; j++)
                {
                    free(pointers[j]); // Free previously allocated memory in case of failure
                }

                free(pointers); // Free the array of pointers itself

                return 1;
            }

            for (int j = 0; j < ALLOCATION_SIZE; j++)
            {
                ((unsigned char *)pointers[i])[j] = (unsigned char)(i + j);
            }

            for (int j = 0; j < ALLOCATION_SIZE; j++)
            {
                malloc_checksum += ((unsigned char*)pointers[i])[j];
            }
        }

        double allocation_time = timer_stop(&timer);
        total_allocation_time += allocation_time;

        timer_start(&timer);

        for (int i = 0; i < ALLOCATION_COUNT; i++)
        {
            free(pointers[i]); // Free each allocated memory block
        }

        double free_time = timer_stop(&timer);
        total_free_time += free_time;
    }

    PhaseArena arena = phase_arena_create(
        ALLOCATION_COUNT * ALLOCATION_SIZE * 2
    );

    if (arena.buffer == NULL)
    {
        printf("Failed to create PhaseAlloc arena.\n");
        free(pointers);
        return 1;
    }

    // PhaseAlloc benchmark
    for (int iteration = 0; iteration < ITERATIONS; iteration++)
    {
        timer_start(&timer);

        for (int i = 0; i < ALLOCATION_COUNT; i++)
        {
            void *memory = phase_arena_alloc(&arena, ALLOCATION_SIZE);

            if (memory == NULL)
            {
                printf("PhaseAlloc allocation failed at iteration %d, allocation %d.\n", iteration, i);

                phase_arena_destroy(&arena);
                free(pointers);

                return 1;
            }

            for (int j = 0; j < ALLOCATION_SIZE; j++)
            {
                ((unsigned char *)memory)[j] = (unsigned char)(i + j);
            }

            for (int j = 0; j < ALLOCATION_SIZE; j++)
            {
                phase_checksum += ((unsigned char *)memory)[j];
            }
        }

        double phase_allocation_time = timer_stop(&timer);
        total_phase_allocation_time += phase_allocation_time;

        timer_start(&timer);

        phase_arena_reset(&arena);

        double phase_reset_time = timer_stop(&timer);
        total_phase_reset_time += phase_reset_time;
    }

    double average_allocation_time = total_allocation_time / ITERATIONS;
    double average_free_time = total_free_time / ITERATIONS;

    double average_phase_allocation_time = total_phase_allocation_time / ITERATIONS;
    double average_phase_reset_time = total_phase_reset_time / ITERATIONS;

    double malloc_total_time = average_allocation_time + average_free_time;

    double phase_total_time = average_phase_allocation_time + average_phase_reset_time;

    printf("\nResults:\n");
    printf("Average malloc allocation time: %.6f seconds\n", average_allocation_time);
    printf("Average free reclamation time: %.6f seconds\n", average_free_time);

    printf("Malloc checksum: %llu\n", malloc_checksum);
    printf("\nPhaseAlloc:\n");
    printf("Average PhaseAlloc allocation time: %.6f seconds\n", average_phase_allocation_time);
    printf("Average PhaseAlloc reset time: %.6f seconds\n", average_phase_reset_time);
    
    printf("PhaseAlloc checksum: %llu\n", phase_checksum);
    printf("\nTotal workload time:\n");
    printf("malloc/free: %.6f seconds\n", malloc_total_time);
    printf("PhaseAlloc: %.6f seconds\n", phase_total_time);

    phase_arena_destroy(&arena);

    free(pointers); // Free the array of pointers itself

    return 0;
}
