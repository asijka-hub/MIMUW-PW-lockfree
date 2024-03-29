#pragma once

#include <stdbool.h>

#include "common.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


struct SimpleQueue;
typedef struct SimpleQueue SimpleQueue;

SimpleQueue* SimpleQueue_new(void);
void SimpleQueue_delete(SimpleQueue* queue);
void SimpleQueue_push(SimpleQueue* queue, Value item);
Value SimpleQueue_pop(SimpleQueue* queue);
bool SimpleQueue_is_empty(SimpleQueue* queue);
