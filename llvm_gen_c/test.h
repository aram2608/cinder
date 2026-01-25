#ifndef TEST_H_
#define TEST_H_

#include <stdlib.h>
#include <stdio.h>

#define BUFF_SIZE 30

typedef struct {
    size_t size;
    int* data;
} Thing;

Thing* NewThing() {
    Thing* thing = (Thing*)malloc(sizeof(Thing));
    thing->data = (int*)malloc(sizeof(int) * BUFF_SIZE);
    thing->size = 0;
    return thing;
}

void AddData(Thing* thing, int data) {
    thing->data[thing->size++] = data;
}

void PrintContents(Thing* thing) {
    printf("----------------------------------\n");
    for (size_t it = 0; it < thing->size; it++) {
        printf("Data: %10d\nIndex: %10zu\n", thing->data[it], it);
    }
    printf("----------------------------------\n");
}

#endif