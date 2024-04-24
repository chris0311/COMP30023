//
// Created by Chris on 2023/3/29.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "queue.h"
#include "processQueue.h"

typedef struct process process_t;
struct process {
    int inTime;
    char* name;
    int cpuTime;
    int memReq;
};

queue_t *newList(){
    /*
     * initialize a new queue
     */
    queue_t *newQueue = malloc(sizeof(queue_t));
    newQueue->nodeCount = 0;
    newQueue->head = NULL;
    newQueue->tail = NULL;

    return newQueue;
}

void insertAtBack(queue_t *queue, node_t *newData){
    /*
     * insert a new node at the back of the queue
     */
    if (queue->nodeCount == 0){
        queue->head = newData;
        queue->tail = newData;
        newData->prev = NULL;
        newData->next = NULL;
    }
    else{
        node_t *currTail = queue->tail;
        currTail->next = newData;
        newData->prev = currTail;
        newData->next = NULL;
        queue->tail = newData;
    }
    queue->nodeCount += 1;
}

void insertAtFront(queue_t *queue, node_t *newData){
    /*
     * insert a new node at the front of the queue
     */
    if (queue->nodeCount == 0){
        queue->head = newData;
        queue->tail = newData;
    }
    else{
        node_t *currHead = queue->head;
        currHead->prev = newData;
        newData->next = currHead;
        newData->prev = NULL;
        queue->head = newData;
    }
    queue->nodeCount += 1;
}

void printProcess(process_t *process){
    /*
     * print the information of a process
     */
    printf("in time: %d, name: %s, cpu: %d, mem: %d\n", process->inTime, process->name, process->cpuTime, process->memReq);
}

void printQueue(queue_t *queue){
    /*
     * print the information of all nodes in the queue
     */
    node_t *curr_node = queue->head;
    printf("total node: %d\n", queue->nodeCount);
    while (curr_node != NULL){
        printf("address: %d, occupied space: %d, status: %c, ", curr_node->address, curr_node->occupiedSpace, curr_node->status);
        if (curr_node->process != NULL){
            printProcess(curr_node->process);
        }
        else{
            printf("process: NULL\n");
        }
        curr_node = curr_node->next;
    }
}

void deleteNode(queue_t *queue, node_t *node){
    /*
     * delete a node from the queue
     */
    if (queue->nodeCount == 1){
        queue->head = NULL;
        queue->tail = NULL;
    }
    else if (node == queue->head){
        node_t *newHead = node->next;
        newHead->prev = NULL;
        queue->head = newHead;
    }
    else if (node == queue->tail){
        node_t *newTail = node->prev;
        newTail->next = NULL;
        queue->tail = newTail;
    }
    else{
        node_t *prevNode = node->prev;
        node_t *nextNode = node->next;
        prevNode->next = nextNode;
        nextNode->prev = prevNode;
    }
    queue->nodeCount -= 1;
    free(node);
}

node_t *pop(queue_t *queue){
    /*
     * pop the first node from the queue
     */
    node_t *currHead = queue->head;
    if (queue->nodeCount == 1){
        queue->head = NULL;
        queue->tail = NULL;
    }
    else{
        node_t *newHead = currHead->next;
        newHead->prev = NULL;
        queue->head = newHead;
    }
    queue->nodeCount -= 1;
    return currHead;
}

void insertAfter(queue_t *queue, node_t *newData, node_t *target){
    /*
     * insert a new node after the target node
     */
    if (target == queue->tail){
        insertAtBack(queue, newData);
    }
    else{
        node_t *nextNode = target->next;
        target->next = newData;
        newData->prev = target;
        newData->next = nextNode;
        nextNode->prev = newData;
        queue->nodeCount += 1;
    }
}

void insertBefore(queue_t *queue, node_t *newData, node_t *target){
    /*
     * insert a new node before the target node
     */
    if (target == queue->head){
        insertAtFront(queue, newData);
    }
    else{
        node_t *prevNode = target->prev;
        target->prev = newData;
        newData->next = target;
        newData->prev = prevNode;
        prevNode->next = newData;
        queue->nodeCount += 1;
    }
}

queue_t *sortQueue(queue_t *queue){
    // use quick sort to sort the queue
    if (queue->nodeCount <= 1){
        return queue;
    }

    // transform the queue to an array
    dynamicArray_t *array = newDynamicArray();
    assert(array != NULL);

    node_t *currNode = queue->head;
    while (currNode != NULL){
        insert(array, currNode->process);
        currNode = currNode->next;
    }
    // sort the array
    sortArray(array);
    // transform the array to a queue
    queue_t *newQueue = newList();
    for (int i = 0; i < array->size; i++){
        node_t *newNode = malloc(sizeof(node_t));
        newNode->process = array->processes[i];
        insertAtBack(newQueue, newNode);
    }
    //freeArray(array);
//    freeQueue(queue);
    return newQueue;
}

void freeQueue(queue_t *queue){
    /*
     * free the memory of the queue
     */
    node_t *currNode = queue->head;
    while (currNode != NULL){
        node_t *nextNode = currNode->next;
        free(currNode->process->name);
        free(currNode->process);
        free(currNode);
        currNode = nextNode;
    }
    free(queue);
}

void mergeNode(queue_t *queue, node_t *node1, node_t *node2, char status){
    // merge node2 to node1, where node1 is the node before node2
    node1->next = node2->next;
    if (node2->next != NULL) node2->next->prev = node1;
    node1->occupiedSpace += node2->occupiedSpace;
    node1->status = status;
    if (status == 'F') node1->process = NULL;
    queue->nodeCount -= 1;
    free(node2);
}
