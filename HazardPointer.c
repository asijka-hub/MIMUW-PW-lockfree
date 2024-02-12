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
        for (int j = 0; j < MAX_THREADS + 1; ++j) {
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
            void* ptr = hp->retired[i][j];
            if (ptr != NULL) {
                free(ptr);
            }
        }
    }
}

void* HazardPointer_protect(HazardPointer* hp, const _Atomic(void*)* atom)
{

    void* expected_atom;

    do {
        expected_atom = atomic_load(atom);                      //1. A
        atomic_store(&hp->pointer[_thread_id], expected_atom);  //2. A

        // 1. i 2. nie sa atomowe wiec pomiedzy ktos mogl sie wkrasc
        // wiemy ze tak sie stalo jesli atom sie zmienil
    } while (atomic_load(atom) != expected_atom);

    return expected_atom;
}

void HazardPointer_clear(HazardPointer* hp)
{
    atomic_store(&hp->pointer[_thread_id], NULL);
}

void HazardPointer_retire(HazardPointer* hp, void* ptr)
{
    int idx = 0;
    for (int i = 0; i < RETIRED_THRESHOLD; ++i) {
        if (hp->retired[_thread_id][i] != NULL) idx++;
        else break;
    }

    atomic_store(&hp->retired[_thread_id][idx], ptr);
    hp->retired_n[_thread_id]++;

    if (hp->retired_n[_thread_id] >= RETIRED_THRESHOLD) {
        for (int i = 0; i <= RETIRED_THRESHOLD; ++i) {
            void* retired_ptr = atomic_load(&hp->retired[_thread_id][i]);
            if (retired_ptr != NULL) {
                bool in_use = false;

                for (int j = 0; j < _num_threads; ++j) {
                    if (atomic_load(&hp->pointer[j]) == retired_ptr) {
                        in_use = true;
                        break;
                    }
                }

                if (!in_use) {
                    free(retired_ptr);
                    hp->retired_n[_thread_id]--;
                    atomic_store(&hp->retired[_thread_id][i], NULL);
                }
            }
        }
    }
}
