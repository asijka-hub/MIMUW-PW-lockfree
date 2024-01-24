#include <malloc.h>
#include <pthread.h>
#include <stdatomic.h>

#include "SimpleQueue.h"

struct SimpleQueueNode;
typedef struct SimpleQueueNode SimpleQueueNode;

struct SimpleQueueNode {
    _Atomic(SimpleQueueNode*) next;
    Value item;
};

SimpleQueueNode* SimpleQueueNode_new(Value item)
{
    SimpleQueueNode* node = (SimpleQueueNode*)malloc(sizeof(SimpleQueueNode));
    atomic_init(&node->next, NULL);
    node->item = item;
    return node;
}

struct SimpleQueue {
    SimpleQueueNode* head;
    SimpleQueueNode* tail;
    pthread_mutex_t head_mtx;
    pthread_mutex_t tail_mtx;
};

SimpleQueue* SimpleQueue_new(void)
{
    SimpleQueue* queue = (SimpleQueue*)malloc(sizeof(SimpleQueue));
    if (queue == NULL) {
        exit(EXIT_FAILURE); //TODO czy to ma sens
    }

    SimpleQueueNode* dummy = SimpleQueueNode_new(EMPTY_VALUE);

    queue->head = dummy;
    queue->tail = dummy;

    pthread_mutex_init(&queue->head_mtx, NULL);
    pthread_mutex_init(&queue->tail_mtx, NULL);

    return queue;
}

void SimpleQueue_delete(SimpleQueue* queue)
{
    SimpleQueueNode* current = queue->head;
    while (current != NULL) {
        SimpleQueueNode* next = atomic_load(&current->next);
        free(current);
        current = next;
    }

    pthread_mutex_destroy(&queue->head_mtx);
    pthread_mutex_destroy(&queue->tail_mtx);

    free(queue);
}

void SimpleQueue_push(SimpleQueue* queue, Value item)
{
    SimpleQueueNode* node = SimpleQueueNode_new(item);

    pthread_mutex_lock(&queue->tail_mtx);

    atomic_store(&queue->tail->next, node);
    queue->tail = node;

    pthread_mutex_unlock(&queue->tail_mtx );
}

Value SimpleQueue_pop(SimpleQueue* queue)
{
    pthread_mutex_lock(&queue->head_mtx);

    SimpleQueueNode* head = queue->head;

    if (atomic_load(&head->next) == NULL) {
        // queue is empty
        pthread_mutex_unlock(&queue->head_mtx);
        return EMPTY_VALUE;
    }

    SimpleQueueNode* new_head = atomic_load(&head->next);
    Value value = new_head->item;

    queue->head = new_head;
    queue->head->item = EMPTY_VALUE;
    pthread_mutex_unlock(&queue->head_mtx);

    free(head);

    return value;
}

bool SimpleQueue_is_empty(SimpleQueue* queue)
{
//    pthread_mutex_lock(&queue->head_mtx);
//    bool is_empty = queue->head == queue->tail;
//    pthread_mutex_unlock(&queue->head_mtx);

    // TODO porownac obie wersie

    SimpleQueueNode* node = atomic_load(&queue->head->next);
    return node == NULL;
}
