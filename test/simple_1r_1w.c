#include <stdio.h>
#include <assert.h>
#include "../SimpleQueue.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void* reader_main(void* data)
{
    printf("reader\n");
    sleep(1);

    SimpleQueue* queue = data;

    for (int i = 0; i < 10; ++i) {
        SimpleQueue_push(queue, i);
        printf("pushed: %d\n", i);
    }

    printf("reader END\n");
}

void* writer_main(void* data)
{
    printf("writer\n");
    sleep(1);


//    Value arr[10];
//    SimpleQueue* queue = data;
//
//    for (int i = 0; i < 10; ++i) {
//        arr[i] = SimpleQueue_pop(queue);
//        printf("poped: %ld\n", arr[i]);
//    }
//
//    sleep(1);
//
//    printf("recieved: ");
//    for (int i = 0; i < 10; ++i) {
//        printf("%ld ", arr[i]);
//    }
//    printf("\n");

    printf("writer END\n");
}

int main() {
    printf("Hello from test simple_1r_1w\n");

    SimpleQueue* simpleQueue = SimpleQueue_new();

    sleep(1);

    pthread_t threads[2];

    pthread_create(&threads[0], NULL, reader_main, &simpleQueue);
    pthread_create(&threads[1], NULL, writer_main, &simpleQueue);

    for (int i = 0; i < 2; i++)
        pthread_join(threads[i], NULL);

    printf("main ended\n");
}