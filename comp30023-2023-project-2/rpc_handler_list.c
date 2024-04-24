//
// Created by Chris on 2023/5/13.
//

#include "rpc_handler_list.h"


handler_list_t *handler_list_init() {
    handler_list_t *new_list = malloc(sizeof(handler_list_t));
    if (new_list == NULL) {
        return NULL;
    }
    new_list->size = 0;
    new_list->head = NULL;
    new_list->tail = NULL;
    return new_list;
}

/*
 * adds a handler to the end of the list
 */
void add_handler(handler_list_t *list, char *function_name, rpc_handler handler) {
    handler_node_t *new_node = malloc(sizeof(handler_node_t));
    if (new_node == NULL) {
        return;
    }
    new_node->function_name = function_name;
    new_node->handler = handler;
    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->valid = true;
    if (list->size == 0) {
        list->head = new_node;
        list->tail = new_node;
    } else {
        list->tail->next = new_node;
        new_node->prev = list->tail;
        list->tail = new_node;
    }
}

/*
 * returns the handler for the given function name
 */
handler_node_t *find_handler(handler_list_t *list, char *function_name) {
    handler_node_t *current = list->head;
    while (current != NULL) {
        if (strcmp(current->function_name, function_name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}