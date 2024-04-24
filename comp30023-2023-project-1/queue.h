//
// Created by Chris on 2023/3/29.
//

#ifndef COMP30023_2023_PROJECT_1_QUEUE_H
#define COMP30023_2023_PROJECT_1_QUEUE_H

#endif //COMP30023_2023_PROJECT_1_QUEUE_H

typedef struct process process_t;
typedef struct node node_t;
struct node {
    int address;
    int occupiedSpace;
    char status;
    node_t *next;
    node_t *prev;
    process_t *process;
};

typedef struct queue queue_t;
struct queue{
    node_t *head;
    node_t *tail;
    int nodeCount;
};

queue_t *newList();
void insertAtBack(queue_t *queue, node_t *newData);
void insertAtFront(queue_t *queue, node_t *newData);
void printQueue(queue_t *queue);
void deleteNode(queue_t *queue, node_t *node);
void insertAfter(queue_t *queue, node_t *newData, node_t *target);
void insertBefore(queue_t *queue, node_t *newData, node_t *target);
node_t *pop(queue_t *queue);
queue_t *sortQueue(queue_t *queue);
void freeQueue(queue_t *queue);
void mergeNode(queue_t *queue, node_t *node1, node_t *node2, char status);
void printProcess(process_t *process);