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
        LLNode* old_tail = atomic_load(&queue->tail); // 1.   State A1
        LLNode* next = NULL;  //      State A2

        if (atomic_compare_exchange_strong(&old_tail->next, &next, new_last)) { // 2.   swap state A -> B
            // 3a.
            // ZACHODZI -> my zmienilismy next na new_last
            // NIE ZACHODZI -> komus udalo sie ustawic next na nastepny wezel, ale nie przedluzono jeszcze head


            // store albo exchange TODO
            // TODO chyba exchange?

//            atomic_store(&queue->tail, new_last);
            atomic_compare_exchange_strong(&queue->tail, &old_tail, new_last);
            break;
        } else {
            // 3b.

            // TUTAJ GDY -> nie udalo nam sie dolozyc nowego, w next mamy snapshot co jest w rzeywisctosci
            // czyli next != NULL

            // nie my zmienilismy stan, teraz jest stan
            // C
            // albo B bo proces mogl zmienic stan ale nie zauktalizowac taila czyli my musimy to naprawic

            atomic_compare_exchange_strong(&queue->tail, &old_tail, next); // TODO check
            // ZACHODZI gdy tail == old_tail, wtedy zmieniamy tail na next
            // NIE ZACHODZI gdy tail != old_tail czyli ktos juz aktualizowal tail
        }


    }
}

Value LLQueue_pop(LLQueue* queue)
{
    LLNode* old_head;
    Value item;
    for (;;) {
        old_head = atomic_load(&queue->head);  // 1.
        Value prev_value =  atomic_exchange(&old_head->item, EMPTY_VALUE);  // 2.

        if (prev_value != EMPTY_VALUE) {
            // 3a

            // ZACHODZI GDY -> wartosc w pierwszym byla rozna od EMPTY_VALUE, czyli head nie byla na dummy
            //                 to my dokonalismy zamiany
            //                 czyli komus udalo sie 4b

//            printf("zwracamy\n");

            return prev_value;
        } else {
            // 3b

            // ZACHODZI GDY -> HEAD byla na dummy albo ktos juz popchnal head

            // TODO sprawdzamy czy kolejka jest pusta
            bool is_empty = LLQueue_is_empty(queue);

            if (is_empty) {
//                printf("pusta\n");
                return EMPTY_VALUE; // 4a
            } else {
                // wartosc z pierwszego wezle byla EMPTY_VALUE ale kolejka nie jest pusta
                //
                if(atomic_compare_exchange_strong(&queue->head, &old_head, old_head->next)) { //4b
//                    printf("przesuwamy head\n");
                }
            }
        }
    }
    HazardPointer_retire(&queue->hp, &old_head);
    return item;
}

bool LLQueue_is_empty(LLQueue* queue)
{
    LLNode* first;
    LLNode* next = NULL;

    first = atomic_load(&queue->head); // state A
    return atomic_compare_exchange_strong(&first->next, &next, NULL);
    // return true if first.next == NULL, swap doest nothing
    // return false in other case
}
