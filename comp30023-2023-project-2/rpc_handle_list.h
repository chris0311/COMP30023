//
// Created by Chris on 2023/5/11.
//

#ifndef COMP30023_2023_PROJECT_2_RPC_HANDLE_LIST_H
#define COMP30023_2023_PROJECT_2_RPC_HANDLE_LIST_H

#endif // COMP30023_2023_PROJECT_2_RPC_HANDLE_LIST_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


struct rpc_handle {
    /* Add variable(s) for handle */
    int id;
    char *function_name;
};
typedef struct rpc_handle rpc_handle_t;

typedef struct rpc_handle_list {
    int size;
    int capacity;
    rpc_handle_t *handles;
} rpc_handle_list_t;