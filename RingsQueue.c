#include <malloc.h>
#include <pthread.h>
#include <stdatomic.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "HazardPointer.h"
#include "RingsQueue.h"

struct RingsQueueNode;
typedef struct RingsQueueNode RingsQueueNode;

typedef struct RingBuffer {
    Value data[RING_SIZE];
    int head;
    int tail;
} RingBuffer;

struct RingsQueueNode {
    _Atomic(RingsQueueNode*) next;
    RingBuffer* buff;
    atomic_ullong push_idx;
    atomic_ullong pop_idx;
};

bool node_full(RingsQueueNode* node) {
    ull push_n = atomic_load(&node->push_idx);
    ull pop_n = atomic_load(&node->pop_idx);
    return (push_n - pop_n == RING_SIZE);
}

bool node_empty(RingsQueueNode* node) {
    ull push_n = atomic_load(&node->push_idx);
    ull pop_n = atomic_load(&node->pop_idx);
    return (push_n - pop_n == 0);
}

/// this function assumes that buff is not FULL!
void node_add_item(RingsQueueNode* const node, const Value value) {
    DEBUG(assert(!node_full(node)));

    RingBuffer* const buff = node->buff;
    buff->data[buff->tail] = value;
    buff->tail = (buff->tail + 1 ) % RING_SIZE;
    ull push_n = atomic_load(&node->push_idx);
    atomic_store(&node->push_idx, push_n + 1);
}

/// this function assumes that buff is not EMPTY!
Value node_remove_item(RingsQueueNode* const node) {
    DEBUG(assert(!node_empty(node)));

    RingBuffer* const buff = node->buff;
    Value value = buff->data[buff->head];
    buff->head = (buff->head + 1) % RING_SIZE;
    ull pop_n = atomic_load(&node->pop_idx);
    atomic_store(&node->pop_idx, pop_n + 1);
    return value;
}

// TODO RingsQueueNode_new
RingsQueueNode* RingsQueueNode_new(void) {
    RingsQueueNode* node = (RingsQueueNode*)malloc(sizeof(RingsQueueNode));
    CHECK_MALLOC(node);

    atomic_init(&node->next, NULL);
    atomic_init(&node->push_idx, 0);
    atomic_init(&node->pop_idx, 0);

    RingBuffer* buff = (RingBuffer*) malloc(sizeof(RingBuffer));
    buff->head = 0;
    buff->tail = 0;
    node->buff = buff;

    return node;
}

struct RingsQueue {
    RingsQueueNode* head;
    RingsQueueNode* tail;
    pthread_mutex_t pop_mtx;
    pthread_mutex_t push_mtx;
};

RingsQueue* RingsQueue_new(void)
{
    RingsQueue* queue = (RingsQueue*)malloc(sizeof(RingsQueue));
    CHECK_MALLOC(queue);

    RingsQueueNode* dummy = RingsQueueNode_new();

    queue->tail = dummy;
    queue->head = dummy;

    pthread_mutex_init(&queue->push_mtx, NULL);
    pthread_mutex_init(&queue->pop_mtx, NULL);

    return queue;
}

void RingsQueue_delete(RingsQueue* queue)
{
    RingsQueueNode* current = queue->head;
    while (current != NULL) {
        RingsQueueNode * next = atomic_load(&current->next);
        free(current->buff);
        free(current);
        current = next;
    }

    pthread_mutex_destroy(&queue->push_mtx);
    pthread_mutex_destroy(&queue->pop_mtx);

    free(queue);
}

void RingsQueue_push(RingsQueue* queue, Value item)
{
    pthread_mutex_lock(&queue->push_mtx);

    RingsQueueNode* const tail = queue->tail;

    if (node_full(tail)) {
        // adding new node
        // and inserting value into it

        RingsQueueNode* const new_node = RingsQueueNode_new();
        node_add_item(new_node, item);

        atomic_store(&tail->next, new_node);
        queue->tail = new_node;
    } else {
        // inserting item into existing
//        printf("inserting item\n");

        node_add_item(tail, item);
    }

    pthread_mutex_unlock(&queue->push_mtx);
}

Value RingsQueue_pop(RingsQueue* queue)
{
    if (RingsQueue_is_empty(queue)) return EMPTY_VALUE;

    pthread_mutex_lock(&queue->pop_mtx);

    RingsQueueNode* const head = queue->head;

    Value ret;

    if (node_empty(head)) {
        // removing node and taking return value from next

        RingsQueueNode* new_head = atomic_load(&head->next);
        ret = node_remove_item(new_head);

        queue->head = new_head;
        free(head);
    } else {
        // removing one element from head
//        printf("removing one item from existing\n");

        ret = node_remove_item(head);
    }

    pthread_mutex_unlock(&queue->pop_mtx);

    return ret;
}

bool RingsQueue_is_empty(RingsQueue* queue)
{
    pthread_mutex_lock(&queue->pop_mtx);
    bool is_empty = queue->head == queue->tail && node_empty(queue->head);
    pthread_mutex_unlock(&queue->pop_mtx);
    return is_empty;
}
