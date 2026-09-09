#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/phasealloc.h"

#define ALLOCATION_COUNT 100000 // Number of allocations to perform in the benchmark
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

    double average_allocation_time = total_allocation_time / ITERATIONS;
    double average_free_time = total_free_time / ITERATIONS;

    printf("\nResults:\n");
    printf("Average malloc allocation time: %.6f seconds\n", average_allocation_time);
    printf("Average free reclamation time: %.6f seconds\n", average_free_time);

    free(pointers); // Free the array of pointers itself

    return 0;
}
