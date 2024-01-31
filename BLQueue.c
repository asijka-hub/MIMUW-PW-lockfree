#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
//
//void node_add_item(BLNode* node, Value item) {
//
//}

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
    for (;;) {
        BLNode* last = atomic_load(&queue->tail); // 1.
        int prev_value = atomic_fetch_add(&last->push_idx, 1); // 2

        if (prev_value < BUFFER_SIZE) {
            // 3a
        } else {
            // 3b
        }
    }


    free(queue);
}

void BLQueue_push(BLQueue* queue, Value item)
{
    // TODO
}

Value BLQueue_pop(BLQueue* queue)
{
    return EMPTY_VALUE; // TODO
}

bool BLQueue_is_empty(BLQueue* queue)
{
    return false; // TODO
}
