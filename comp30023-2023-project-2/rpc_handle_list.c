//
// Created by Chris on 2023/5/11.
//

#include "rpc_handle_list.h"

rpc_handle_list_t *rpc_handle_list_init() {
    rpc_handle_list_t *new_list = malloc(sizeof(rpc_handle_list_t));
    if (new_list == NULL) {
        return NULL;
    }
    new_list->size = 0;
    new_list->capacity = 10;
    new_list->handles = malloc(sizeof(rpc_handle_t) * new_list->capacity);
    if (new_list->handles == NULL) {
        free(new_list);
        return NULL;
    }
    return new_list;
}

void add_handle(rpc_handle_list_t *list, rpc_handle_t *handle) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->handles = realloc(list->handles, sizeof(rpc_handle_t) * list->capacity);
    }
    list->handles[list->size] = *handle;
    list->size++;
}

rpc_handle_t *find_handle(rpc_handle_list_t *list, int id) {
    if (list == NULL) {
        return NULL;
    }
    if (id > list->size) {
        return NULL;
    }
    if (id < 1) {
        return NULL;
    }
    return &list->handles[id - 1];
}