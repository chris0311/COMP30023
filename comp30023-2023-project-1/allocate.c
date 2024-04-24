#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <signal.h>

#include "processQueue.h"
#include "queue.h"
#include "utils.h"

#define IMPLEMENTS_REAL_PROCESS
#define MAX_MEMORY 2048
#define INF 9999999


void updateInputQueue(queue_t *inputQueue, queue_t *inputFile);
bool assignMemory(queue_t *inputQueue, queue_t *readyQueue, queue_t *memory, char mode);
queue_t *initMemory();
void freeAdjacentMem(queue_t *memory, const node_t *runningProcess);
void readInput(char *fileName, queue_t *inputFile);

typedef struct process process_t;
struct process {
    int inTime;
    char *name;
    int cpuTime;
    int memReq;
    int totalCpuTime;
    bool initialized;
    pid_t pid;
    int parentFd[2];
    int childFd[2];
};


int systemTime = 0;

int main(int argc, char *argv[]) {
    // read command line args
    char *fileName;
    char *strategy;
    char *mode;
    int quantum;
    char *input1;
    char *input2;
    char *tmp;
    for (int i = 1; i < argc; i += 2) {
        input1 = argv[i];
        input2 = argv[i + 1];
        if (strcmp(input1, "-f") == 0)
            fileName = input2;
        else if (strcmp(input1, "-s") == 0)
            strategy = input2;
        else if (strcmp(input1, "-m") == 0)
            mode = input2;
        else if (strcmp(input1, "-q") == 0)
            quantum = strtol(input2, &tmp, 10);
    }

    queue_t *inputFile = newList();
    assert(inputFile != NULL);

    //  read input file
    readInput(fileName, inputFile);

    int totalProcesses = inputFile->nodeCount;
    double avgOverHead = 0, maxOverHead = 0;
    int totalCpuTime = 0;

    uint8_t bigEndianBuffer[4];
    uint8_t validate;
    int wstatus;

    // initialize required queues
    queue_t *inputQueue = newList();
    assert(inputQueue != NULL);
    queue_t *readyQueue = newList();
    assert(readyQueue != NULL);
    queue_t *memory = initMemory();

    bool processSwitched = true;
    bool processFinishedInQuantum = false;
    // update input queue
    updateInputQueue(inputQueue, inputFile);
    // assign memory
    bool newProcessArrived = assignMemory(inputQueue, readyQueue, memory, mode[0]);

    // start process scheduling
    while (inputFile->nodeCount != 0 || readyQueue->nodeCount != 0) {
        if (readyQueue->nodeCount == 0) {
            // no process in ready queue
            systemTime += quantum;
            // update input queue
            updateInputQueue(inputQueue, inputFile);
            // assign memory
            newProcessArrived = assignMemory(inputQueue, readyQueue, memory, mode[0]);
            continue;
        }

        if (strategy[0] == 'S' && newProcessArrived) {
            // need to sort ready queue if new process arrived in SJF mode
            if (processFinishedInQuantum) {
                // no current running process
                readyQueue = sortQueue(readyQueue);
            } else {
                // sort ready queue but keep running process at front
                node_t *readyHead = pop(readyQueue);
                readyQueue = sortQueue(readyQueue);
                insertAtFront(readyQueue, readyHead);
            }
        }

        // run the first process in ready queue
        node_t *runningProcess = pop(readyQueue);
        if (!runningProcess->process->initialized) {
            // initialize child process
            runningProcess->process->initialized = true;
            pipe(runningProcess->process->parentFd);
            pipe(runningProcess->process->childFd);
            runningProcess->process->pid = fork();
            if (runningProcess->process->pid == 0) {
                // child process
                close(runningProcess->process->parentFd[1]);
                close(runningProcess->process->childFd[0]);
                dup2(runningProcess->process->childFd[1], STDOUT_FILENO);
                dup2(runningProcess->process->parentFd[0], STDIN_FILENO);
                char *args[] = {"./process", runningProcess->process->name, NULL};
                execv("./process", args);
            } else {
                // parent process
                close(runningProcess->process->parentFd[0]);
                close(runningProcess->process->childFd[1]);
                getBigEndian(systemTime, bigEndianBuffer);
                write(runningProcess->process->parentFd[1], bigEndianBuffer, 4);
                read(runningProcess->process->childFd[0], &validate, 1);
                assert(validate == bigEndianBuffer[3]);
            }
        } else {
            // process already initialized
            getBigEndian(systemTime, bigEndianBuffer);
            write(runningProcess->process->parentFd[1], bigEndianBuffer, 4);

            // need to continue process
            kill(runningProcess->process->pid, SIGCONT);
            read(runningProcess->process->childFd[0], &validate, 1);
            if (validate != bigEndianBuffer[3]) printf("Validate failed\n");
        }

        if (processSwitched) {
            printf("%d,RUNNING,process_name=%s,remaining_time=%d\n", systemTime, runningProcess->process->name, runningProcess->process->cpuTime);
        }

        runningProcess->process->cpuTime -= quantum;
        systemTime += quantum;

        if (runningProcess->process->cpuTime <= 0) {
            // process finished
            // calculate performance
            int currentTime = systemTime;
            int turnAroundTime = currentTime - runningProcess->process->inTime;
            totalCpuTime += turnAroundTime;
            double overHead = (double) turnAroundTime / runningProcess->process->totalCpuTime;
            avgOverHead += overHead;
            if (overHead > maxOverHead)
                maxOverHead = overHead;

            // terminate process
            getBigEndian(systemTime, bigEndianBuffer);
            write(runningProcess->process->parentFd[1], bigEndianBuffer, 4);

            kill(runningProcess->process->pid, SIGTERM);
            char sha[65];
            read(runningProcess->process->childFd[0], sha, sizeof(sha));
            sha[64] = '\0';

            printf("%d,FINISHED,process_name=%s,proc_remaining=%d\n", systemTime, runningProcess->process->name, readyQueue->nodeCount + inputQueue->nodeCount);
            printf("%d,FINISHED-PROCESS,process_name=%s,sha=%s\n", systemTime, runningProcess->process->name, sha);

            // need to free memory in best fit mode
            if (mode[0] == 'b') {
                freeAdjacentMem(memory, runningProcess);
            }

            free(runningProcess->process->name);
            runningProcess->process->name = NULL;
            free(runningProcess->process);
            runningProcess->process = NULL;
            free(runningProcess);
            runningProcess = NULL;
            processFinishedInQuantum = true;
            processSwitched = true;

            // update input queue
            updateInputQueue(inputQueue, inputFile);
            // assign memory
            newProcessArrived = assignMemory(inputQueue, readyQueue, memory, mode[0]);
        } else {
            // update input queue
            updateInputQueue(inputQueue, inputFile);
            // assign memory
            newProcessArrived = assignMemory(inputQueue, readyQueue, memory, mode[0]);
            if (strategy[0] == 'R') {
                // round-robin, need to place the current running process at the end of ready queue
                insertAtBack(readyQueue, runningProcess);
                processFinishedInQuantum = false;
                if (readyQueue->nodeCount == 1) {
                    processSwitched = false;
                } else{
                    processSwitched = true;
                    // suspend process
                    getBigEndian(systemTime, bigEndianBuffer);
                    write(runningProcess->process->parentFd[1], bigEndianBuffer, 4);

                    kill(runningProcess->process->pid, SIGTSTP);
                    do {
                        pid_t w = waitpid(runningProcess->process->pid, &wstatus, WUNTRACED);

                        if (w == -1) {
                            perror("waitpid");
                            exit(EXIT_FAILURE);
                        }
                        if (WIFEXITED(wstatus)) {
                            printf("exited");
                        }

                    } while (!WIFSTOPPED(wstatus));

                }
            } else if (strategy[0] == 'S') {
                // put the process back to the front of ready queue so that it will be run next
                insertAtFront(readyQueue, runningProcess);
                processFinishedInQuantum = false;
                processSwitched = false;
            }
        }
    }

    // print performance stats
    printf("Turnaround time %d\n", (int) ceil((double) totalCpuTime / totalProcesses));
    printf("Time overhead %.2f %.2f\n", maxOverHead, avgOverHead / totalProcesses);
    printf("Makespan %d\n", systemTime);

    free(inputFile);
    inputFile = NULL;
    free(readyQueue);
    readyQueue = NULL;
    free(inputQueue);
    inputQueue = NULL;
    free(memory);
    memory = NULL;
    return 0;
}

void readInput(char *fileName, queue_t *inputFile) {
    /*
     * Read input file and put processes in input queue
     */
    FILE *fp = fopen(fileName, "r");
    assert(fp != NULL);
    int inputTime, cpuTime, reqMem;
    char processName[8];
    while (fscanf(fp, "%d %s %d %d", &inputTime, processName, &cpuTime, &reqMem) == 4) {
        process_t *newProcess = malloc(sizeof(process_t));
        newProcess->inTime = inputTime;
        newProcess->cpuTime = cpuTime;
        newProcess->totalCpuTime = cpuTime;
        newProcess->memReq = reqMem;
        newProcess->initialized = false;
        newProcess->name = malloc(8 * sizeof(char));
        strcpy(newProcess->name, processName);

        // put process in input queue
        node_t *newNode = malloc(sizeof(node_t));
        newNode->process = newProcess;
        insertAtBack(inputFile, newNode);
    }
    fclose(fp);
}

void freeAdjacentMem(queue_t *memory, const node_t *runningProcess) {
    /*
     * Free memory occupied by a process, and merge adjacent free blocks
     */
    node_t *currMemNode = memory->head;
    while (currMemNode != NULL) {
        if (currMemNode->process != NULL && strcmp(currMemNode->process->name, runningProcess->process->name) == 0) {
            // merge with adjacent free blocks
            if (currMemNode->prev != NULL && currMemNode->prev->status == 'F') {
                node_t *temp = currMemNode->prev;
                mergeNode(memory, currMemNode->prev, currMemNode, 'F');
                if (temp->next != NULL && temp->next->status == 'F') {
                    mergeNode(memory, temp, temp->next, 'F');
                }
                break;
            } else if (currMemNode->next != NULL && currMemNode->next->status == 'F') { // only the next node is free
                mergeNode(memory, currMemNode, currMemNode->next, 'F');
                break;
            } else {
                // adjacent nodes are all allocated
                currMemNode->status = 'F';
                currMemNode->process = NULL;
                break;
            }
        }
        currMemNode = currMemNode->next;
    }
}

void updateInputQueue(queue_t *inputQueue, queue_t *inputFile) {
    // put all processes that have arrived into the input queue
    node_t *currNode = NULL;
    while (inputFile->head != NULL && inputFile->head->process->inTime <= systemTime) {
        currNode = pop(inputFile);
        node_t *newInputNode = malloc(sizeof(node_t));
        newInputNode->process = currNode->process;
        insertAtBack(inputQueue, newInputNode);
        free(currNode);
        currNode = NULL;
    }
}

queue_t *initMemory() {
    /*
     * Initialize memory with one free block
     */
    queue_t *memory = newList();
    assert(memory != NULL);
    node_t *initMemory = malloc(sizeof(node_t));
    initMemory->process = NULL;
    initMemory->address = 0;
    initMemory->occupiedSpace = MAX_MEMORY;
    initMemory->status = 'F';
    insertAtBack(memory, initMemory);

    return memory;
}

bool assignMemory(queue_t *inputQueue, queue_t *readyQueue, queue_t *memory, char mode) {
    /*
     * Assign memory to processes in input queue and move them to ready queue if memory is available
     * mode: 'i' for infinite memory, 'b' for best-fit
     */
    if (inputQueue->head == NULL) return false; // no process to assign memory (input queue is empty

    if (mode == 'i') {
        // assign assuming infinite memory, move directly to ready queue
        while (inputQueue->head != NULL) {
            insertAtBack(readyQueue, pop(inputQueue));
            readyQueue->tail->address = 0; // assume inf memory, no need to assign address
            readyQueue->tail->occupiedSpace = readyQueue->tail->process->memReq;
            readyQueue->tail->status = 'R'; // process in ready state
        }
        return true;
    } else if (mode == 'b') {
        // assign using best-fit
        queue_t *notFit = newList();

        while (inputQueue->head != NULL) {
            node_t *currNode = inputQueue->head;
            // assign memory to the process
            int minSpace = INF;
            node_t *minNode = NULL;

            node_t *memoryNode = memory->head;
            // find the smallest hole that can fit the process
            while (memoryNode != NULL) {
                if (memoryNode->status == 'F' && memoryNode->occupiedSpace >= currNode->process->memReq && memoryNode->occupiedSpace < minSpace) {
                    minSpace = memoryNode->occupiedSpace;
                    minNode = memoryNode;
                }
                memoryNode = memoryNode->next;
            }

            if (minNode == NULL) {
                // no hole can fit the process
                insertAtBack(notFit, pop(inputQueue));
            } else {
                // insert process in memory
                node_t *newMemNode = malloc(sizeof(node_t));
                newMemNode->address = minNode->address;
                newMemNode->occupiedSpace = currNode->process->memReq;
                newMemNode->status = 'O';
                newMemNode->process = currNode->process;
                insertBefore(memory, newMemNode, minNode);
                // adjust free memory space
                minNode->address += currNode->process->memReq;
                minNode->occupiedSpace -= currNode->process->memReq;

                // insert process in ready queue
                insertAtBack(readyQueue, pop(inputQueue));

                // print result to stdout
                printf("%d,READY,process_name=%s,assigned_at=%d\n", systemTime, newMemNode->process->name, newMemNode->address);
            }
        }

        // reassign input queue as the temp queue
        inputQueue->head = notFit->head;
        inputQueue->tail = notFit->tail;
        inputQueue->nodeCount = notFit->nodeCount;

        free(notFit);
        notFit = NULL;
    }
    return true;
}

