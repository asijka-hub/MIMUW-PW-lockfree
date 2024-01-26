#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include "HazardPointer.h"

thread_local int _thread_id = -1;
int _num_threads = -1;

void HazardPointer_register(int thread_id, int num_threads)
{
    _thread_id = thread_id;
    _num_threads = num_threads;
}

void HazardPointer_initialize(HazardPointer* hp)
{
    for (int i = 0; i < MAX_THREADS; ++i) {
        atomic_init(&hp->pointer[i], NULL);
    }

    for (int i = 0; i < MAX_THREADS; ++i) {
        for (int j = 0; j < RETIRED_THRESHOLD; ++j) {
            hp->retired[i][j] = NULL;
        }
    }

    for (int i = 0; i < RETIRED_THRESHOLD; ++i) {
        hp->retired_n[i] = 0;
    }
}

void HazardPointer_finalize(HazardPointer* hp)
{
    for (int i = 0; i < MAX_THREADS; ++i) {
        atomic_store(&hp->pointer[i], NULL);
    }


    for (int i = 0; i < RETIRED_THRESHOLD; ++i) {
        for (int j = 0; j < RETIRED_THRESHOLD; ++j) {
            void* ptr = atomic_load(&hp->retired[i][j]);
            if (ptr != NULL) {
                free(ptr);
            }
        }
    }
}

void* HazardPointer_protect(HazardPointer* hp, const _Atomic(void*)* atom)
{
    void* ptr = atomic_load(atom);
    atomic_store(&hp->pointer[_thread_id], ptr);
    return ptr;
}

void HazardPointer_clear(HazardPointer* hp)
{
    atomic_store(&hp->pointer[_thread_id], NULL);
}

void HazardPointer_retire(HazardPointer* hp, void* ptr)
{
//    atomic_store(&hp->retired[_thread_id], ptr);

    // Check if the retired array exceeds the threshold
    if (_thread_id == _num_threads - 1) {
        for (int i = 0; i < RETIRED_THRESHOLD; ++i) {
//            void* retired_ptr = atomic_load(&hp->retired[i]);
//            if (retired_ptr != NULL) {
//                bool in_use = false;
//
//                // Check if the retired pointer is still in use by any thread
//                for (int j = 0; j < _num_threads; ++j) {
//                    if (atomic_load(&hp->pointer[j]) == retired_ptr) {
//                        in_use = true;
//                        break;
//                    }
//                }
//
//                if (!in_use) {
//                    // If not in use, free the retired pointer
//                    free(retired_ptr);
//                    atomic_store(&hp->retired[i], NULL);
//                }
//            }
        }
    }
}
