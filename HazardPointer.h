#pragma once

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_THREADS 128
static const int RETIRED_THRESHOLD = MAX_THREADS;

typedef uint64_t ull;
typedef uint8_t u8;

#ifdef DEBUG
    static bool debug = true;
#else
    static bool debug = false;
#endif

#define DEBUG(expression) do { \
    if (debug) { \
        expression; \
    } \
} while(0)

#define CHECK_MALLOC(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "Memory allocation failed at %s:%d\n", __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

//--------------------------------------------------------------------









//--------------------------------------------------------------------

struct HazardPointer {
    _Atomic(void*) pointer[MAX_THREADS];
    void* retired[MAX_THREADS][MAX_THREADS];
    u8 retired_n[MAX_THREADS];
    // TODO
};
typedef struct HazardPointer HazardPointer;

void HazardPointer_register(int thread_id, int num_threads);
void HazardPointer_initialize(HazardPointer* hp);
void HazardPointer_finalize(HazardPointer* hp);
void* HazardPointer_protect(HazardPointer* hp, const _Atomic(void*)* atom);
void HazardPointer_clear(HazardPointer* hp);
void HazardPointer_retire(HazardPointer* hp, void* ptr);
