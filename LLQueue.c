#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "HazardPointer.h"
#include "LLQueue.h"

struct LLNode;
typedef struct LLNode LLNode;
typedef _Atomic(LLNode*) AtomicLLNodePtr;

struct LLNode {
    AtomicLLNodePtr next;
    Value item;
    // TODO
};

LLNode* LLNode_new(Value item)
{
    LLNode* node = (LLNode*)malloc(sizeof(LLNode));
    atomic_store(&node->next, NULL);
    node->item = item;
    return node;
}

struct LLQueue {
    AtomicLLNodePtr head;
    AtomicLLNodePtr tail;
    HazardPointer hp;
    // TODO zakladamy ze register jest juz wywolane?
};

LLQueue* LLQueue_new(void)
{
    LLQueue* queue = (LLQueue*)malloc(sizeof(LLQueue));
    CHECK_MALLOC(queue);

    LLNode* dummy = LLNode_new(EMPTY_VALUE);

    atomic_store(&queue->head, dummy);
    atomic_store(&queue->tail, dummy);

    return queue;
}

void LLQueue_delete(LLQueue* queue)
{
    // TODO
    free(queue);
}

void LLQueue_push(LLQueue* queue, Value item)
{
    LLNode* node = LLNode_new(item);

    for (;;) {
        LLNode* tail = atomic_load(&queue->tail);
        LLNode* next = atomic_load(&tail->next);

        if (tail == atomic_load(&queue->tail)) {
            if (next == NULL) {
                if (atomic_compare_exchange_weak(&tail->next, &next, node)) // TODO check!!!
                    break;
            }
        } else {
            atomic_compare_exchange_weak(&tail, &queue->tail, queue->tail); // TODO check !!!!
        }
    }
}

Value LLQueue_pop(LLQueue* queue)
{
    return EMPTY_VALUE; // TODO
}

bool LLQueue_is_empty(LLQueue* queue)
{
    return false; // TODO
}
