//
// Created by Chris on 2023/3/29.
//

#include "processQueue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"

typedef struct process process_t;
struct process {
    int inTime;
    char* name;
    int cpuTime;
    int memReq;
};
typedef struct dynamicArray dynamicArray_t;

dynamicArray_t *newDynamicArray(){
    /*
     * initialize a new dynamic array
     */
    dynamicArray_t *newArray = malloc(sizeof(dynamicArray_t));
    newArray->size = 0;
    newArray->capacity = 10;
    newArray->processes = malloc(sizeof(process_t*) * newArray->capacity);

    return newArray;
}

void insert(dynamicArray_t *array, process_t *newProcess){
    /*
     * insert a new process into the array
     */
    if (array->size == array->capacity){
        array->capacity *= 2;
        array->processes = realloc(array->processes, sizeof(process_t*) * array->capacity);
    }
    array->processes[array->size] = newProcess;
    array->size += 1;
}

int compare(const void *a, const void *b){
    /*
     * compare two processes, used in qsort
     */

    process_t *processA = *(process_t **)a;
    process_t *processB = *(process_t **)b;

    int result = processA->cpuTime - processB->cpuTime;
    if (result == 0){
        // compare in time
        result = processA->inTime - processB->inTime;
        if (result == 0){
            // compare name
            result = strcmp(processA->name, processB->name);
        }
    }
    return result;
}

void printArray(dynamicArray_t *array) {
    /*
     * print the array
     */
    printf("size: %d, capacity: %d\n", array->size, array->capacity);
    for (int i = 0; i < array->size; i++) {
        printf("process name: %s, inTime: %d, cpuTime: %d, memReq: %d\n", array->processes[i]->name, array->processes[i]->inTime, array->processes[i]->cpuTime, array->processes[i]->memReq);
    }
}

void sortArray(dynamicArray_t *array){
    // use qsort to sort the array
    qsort(array->processes, array->size, sizeof(process_t *), compare);
}

void freeArray(dynamicArray_t *array){
    /*
     * frees the array
     */
    for (int i = 0; i < array->size; i++){
        free(array->processes[i]->name);
        free(array->processes[i]);
    }
    free(array->processes);
    free(array);
}