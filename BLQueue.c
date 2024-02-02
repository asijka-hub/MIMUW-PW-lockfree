#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <assert.h>

#include "BLQueue.h"
#include "HazardPointer.h"

struct BLNode;
typedef struct BLNode BLNode;
typedef _Atomic(BLNode*) AtomicBLNodePtr;

typedef struct BLBuffer {
    Value data[BUFFER_SIZE];
} BLBuffer;

struct BLNode {
    AtomicBLNodePtr next;
    BLBuffer* buffer;
    atomic_int push_idx;
    atomic_int pop_idx;
    // TODO
};

// TODO BLNode_new

struct BLQueue {
    AtomicBLNodePtr head;
    AtomicBLNodePtr tail;
    HazardPointer hp; // TODO to chyba trzeba zaalocowac
};

BLNode* BLNode_new(void) {
    BLNode* node = (BLNode*)malloc(sizeof(BLNode));
    CHECK_MALLOC(node);

    BLBuffer* buff = (BLBuffer*) calloc(1, sizeof(BLBuffer)); // TODO sprawdzic to
    CHECK_MALLOC(node);


    node->buffer = buff;

    atomic_init(&node->next, NULL);
    atomic_init(&node->push_idx, 0);
    atomic_init(&node->pop_idx, 0);

    return node;
}

BLNode* BLNode_with_item(Value item) {
    BLNode* node = (BLNode*)malloc(sizeof(BLNode));
    CHECK_MALLOC(node);

    BLBuffer* buff = (BLBuffer*) calloc(1, sizeof(BLBuffer)); // TODO sprawdzic to
    CHECK_MALLOC(node);

    node->buffer = buff;
    node->buffer->data[0] = item;

    atomic_init(&node->next, NULL);
    atomic_init(&node->push_idx, 1);
    atomic_init(&node->pop_idx, 0);

    return node;
}

BLQueue* BLQueue_new(void)
{
    BLQueue* queue = (BLQueue*)malloc(sizeof(BLQueue));
    CHECK_MALLOC(queue);

    BLNode* dummy = BLNode_new();

    atomic_init(&queue->head, dummy);
    atomic_init(&queue->tail, dummy);

    return queue;
}


void BLQueue_delete(BLQueue* queue)
{
    BLNode * current = queue->head;
    while (current) {
        BLNode * next = atomic_load(&current->next);
        free(current->buffer);
        free(current);
        current = next;
    }

    free(queue);
}

void BLQueue_push(BLQueue* queue, Value item)
{
    Value empty_value = EMPTY_VALUE;
    BLNode* next = NULL;

    for (;;) {
        next = NULL;
        empty_value = EMPTY_VALUE;
        BLNode* old_tail = atomic_load(&queue->tail); // 1.
        int prev_idx = atomic_fetch_add(&old_tail->push_idx, 1); // 2
//        printf("prev_idx: %d\n", prev_idx);

        if (prev_idx < BUFFER_SIZE) {
            // 3a

            if (atomic_compare_exchange_strong(&(old_tail->buffer->data[prev_idx]), &empty_value, item)) {
//                printf("wtawilismy1: %d\n", item);
                return;
            }
        } else {
            // 3b

            // czy zostal utworzony wezel

            if (atomic_compare_exchange_strong(&old_tail->next, &next, NULL)) {
                // 4b bo next = NULL

                BLNode* new_node = BLNode_with_item(item);

                assert(next == NULL);

                if (atomic_compare_exchange_strong(&old_tail->next, &next, new_node)) {
                    // przedluzanie sie udalo

//                    printf("wtawilismy2: %d\n", item);
//                    atomic_store(&queue->tail, new_node);
                    atomic_compare_exchange_strong(&queue->tail, &old_tail, new_node);
                    return;
                } else {
                    // przedluzanie sie nie udalo

                    free(new_node->buffer);
                    free(new_node);
                }
            } else {
                // 4a
                // jesli fail to w next nowy

                atomic_compare_exchange_strong(&queue->tail, &old_tail, next);
            }
        }
    }
}

Value BLQueue_pop(BLQueue* queue)
{
    BLNode* old_head;
    BLNode* next = NULL;
    Value old_val;


    for (;;) {
        old_head = atomic_load(&queue->head); // 1.
        next = NULL;
        int prev_idx = atomic_fetch_add(&old_head->pop_idx, 1); // 2

        if (prev_idx < BUFFER_SIZE) { // 3a.
            old_val = atomic_exchange(&old_head->buffer->data[prev_idx], TAKEN_VALUE);

            if (old_val == EMPTY_VALUE) {
                continue;
            } else {
                return old_val;
            }
        } else {  // 3b.

            if (atomic_compare_exchange_strong(&old_head->next, &next, NULL)) {
                // 4b bo next = NULL
                return EMPTY_VALUE;

            } else {
                // 4a w next mamy nastepny
                atomic_compare_exchange_strong(&queue->head, &old_head, next);

            }
        }
    }
}

bool BLQueue_is_empty(BLQueue* queue)
{

    BLNode* first;
    BLNode* next = NULL;

    first = atomic_load(&queue->head); // state A
    return atomic_compare_exchange_strong(&first->next, &next, NULL)
        && (atomic_load(&first->push_idx) == atomic_load(&first->pop_idx));
}
