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
    atomic_init(&node->next, NULL); //TODO moze atomic init?
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
    LLNode* new_last = LLNode_new(item);          //      State B

    for (;;) {
//        printf("push\n");
        LLNode* last = atomic_load(&queue->tail); // 1.   State A1
        LLNode* next = atomic_load(&last->next);  //      State A2

        if (atomic_compare_exchange_strong(&last->next, &next, new_last)) { // 2.   swap state A -> B
            // 3a.
            // ZACHODZI -> my zmienilismy next na new_last
            // NIE ZACHODZI -> komus udalo sie ustawic next na nastepny wezel


            // store albo exchange TODO
            atomic_store(&queue->tail, new_last);
            break;
        } else {
            // 3b.

            // nie my zmienilismy stan, teraz jest stan
            // C
            // albo B bo proces mogl zmienic stan ale nie zauktalizowac taila czyli my musimy to naprawic

            atomic_compare_exchange_strong(&queue->tail, &last, last->next); // TODO check
        }


    }
}

Value LLQueue_pop(LLQueue* queue)
{
    LLNode* first;
    Value item;
    for (;;) {
        first = atomic_load(&queue->head);  // 1.
        Value prev_value =  atomic_exchange(&first->item, EMPTY_VALUE);  // 2.

        if (prev_value != EMPTY_VALUE) {
            // 3a

            // wartosc w pierwszym byla rozna od EMPTY_VALUE

//            printf("zwracamy\n");


            if (atomic_load(&queue->head) != first) {
//                atomic_store(&queue->head, first->next);
//                printf("hmm\n");
            }

            return prev_value;
        } else {
            // 3b

            // TODO sprawdzamy czy kolejka jest pusta
            bool is_empty = LLQueue_is_empty(queue);

            if (is_empty) {
//                printf("pusta\n");
                return EMPTY_VALUE;
            } else {
                // wartosc z pierwszego wezle byla EMPTY_VALUE ale kolejka nie jest pusta
                if(atomic_compare_exchange_strong(&queue->head, &first, first->next)) {
//                    printf("przesuwamy head\n");
                }
            }
        }
    }
    HazardPointer_retire(&queue->hp, &first);
    return item;
}

bool LLQueue_is_empty(LLQueue* queue)
{
    LLNode* first;
    LLNode* next = NULL;

//    for (;;) {
//        first = atomic_load(&queue->head);
//        if (first == atomic_load(&queue->head)) {
//            return first->next == NULL;
//        } else {
//            continue;
//        }
//    }

//    for (;;) {
//        first = atomic_load(&queue->head);  // State A
//        next = atomic_load(&first->next);   // State B
//
//        if (first->next == next) { // State A == B ?
//            return next == NULL;
//        } else {
//            // state are not the same, something happen between loadings
//            continue;
//        }
//    }

    first = atomic_load(&queue->head); // state A
    return atomic_compare_exchange_strong(&first->next, &next, NULL);
    // return true if first.next == NULL, swap doest nothing
    // return false in other case
}
