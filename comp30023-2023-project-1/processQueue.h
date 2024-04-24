//
// Created by Chris on 2023/3/29.
//

#ifndef COMP30023_2023_PROJECT_1_PROCESSQUEUE_H
#define COMP30023_2023_PROJECT_1_PROCESSQUEUE_H

#endif //COMP30023_2023_PROJECT_1_PROCESSQUEUE_H

typedef struct process process_t;
typedef struct dynamicArray dynamicArray_t;
struct dynamicArray {
    int size;
    int capacity;
    process_t **processes;
};

dynamicArray_t *newDynamicArray();
void insert(dynamicArray_t *array, process_t *newProcess);
void printArray(dynamicArray_t *array);
void deleteProcess(dynamicArray_t *array, process_t *process);
void sortArray(dynamicArray_t *array);
void freeArray(dynamicArray_t *array);