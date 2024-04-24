//
// Created by Chris on 2023/5/13.
//

# include "rpc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef COMP30023_2023_PROJECT_2_RPC_HANDLER_LIST_H
#define COMP30023_2023_PROJECT_2_RPC_HANDLER_LIST_H

#endif // COMP30023_2023_PROJECT_2_RPC_HANDLER_LIST_H

typedef struct handler_node handler_node_t;
struct handler_node {
    char *function_name;
    rpc_handler handler;
    handler_node_t *next;
    handler_node_t *prev;
    bool valid;
};

typedef struct handler_list {
    int size;
    handler_node_t *head;
    handler_node_t *tail;
} handler_list_t;

handler_list_t *handler_list_init();

void add_handler(handler_list_t *list, char *function_name, rpc_handler handler);

handler_node_t *find_handler(handler_list_t *list, char *function_name);

void free_handler_list(handler_list_t *list);