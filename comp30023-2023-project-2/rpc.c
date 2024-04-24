#include "rpc.h"
#include "rpc_handler_list.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct rpc_server {
    /* Add variable(s) for server state */
    int port;
    handler_list_t *handler_list;
};

rpc_server *rpc_init_server(int port) {
    rpc_server *new_server = malloc(sizeof(rpc_server));
    if (new_server == NULL) {
        return NULL;
    }
    // init handler list
    new_server->handler_list = handler_list_init();
    if (new_server->handler_list == NULL) {
        free(new_server);
        return NULL;
    }
    new_server->port = port;
    return new_server;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    // check if arguments are null
    if (srv == NULL || name == NULL || handler == NULL) {
        return -1;
    }
    // check if the name is already registered
    handler_node_t *handler_node = find_handler(srv->handler_list, name);
    if (handler_node != NULL) {
        // replace the handler
        handler_node->handler = handler;
        return 200;
    } else {
        // add the handler
        add_handler(srv->handler_list, name, handler);
        return 200;
    }
}

int create_listening_socket(int port) {
    int re, s, socket_fd;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;      // IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // use my IP

    // change port to string
    char port_str[6];
    sprintf(port_str, "%d", port);
    // get address info of the server
    s = getaddrinfo(NULL, port_str, &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    // create socket
    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socket_fd == -1) {
        perror("socket");
        return -1;
    }

    // Reuse port if possible
    re = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
        perror("setsockopt");
        return -1;
    }

    // Bind address to the socket
    if (bind(socket_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        return -1;
    }
    freeaddrinfo(res);

    return socket_fd;
}

void rpc_serve_all(rpc_server *srv) {
    int socket_fd, newsockfd, n, i;
    char buffer[256];
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size;
    // create listening socket
    socket_fd = create_listening_socket(srv->port);
    if (socket_fd < 0) {
        perror("create_listening_socket");
        return;
    }
    // Listen on the socket
    if (listen(socket_fd, 10) < 0) {
        perror("listen");
        return;
    }
    // accept connections
    client_addr_size = sizeof client_addr;
    newsockfd =
        accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_size);
    if (newsockfd < 0) {
        perror("accept");
        return;
    }

    // Read characters from the connection, then process
    while (1) {
        n = read(newsockfd, buffer, 255); // n is number of characters read
        if (n < 0) {
            perror("ERROR reading from socket");
            exit(EXIT_FAILURE);
        }
        // Null-terminate string
        buffer[n] = '\0';

        // Process find
        if (strncmp(buffer, "FIND\n", 5) == 0) {
            // get the function name
            char *function_name = buffer + 5;
            // find the handler
            handler_node_t *handler_node =
                find_handler(srv->handler_list, function_name);
            if (handler_node == NULL) {
                // send 404 to client, indicating not found
                n = write(newsockfd, "404", 3);
                if (n < 0) {
                    perror("ERROR writing to socket");
                    exit(EXIT_FAILURE);
                }
            } else {
                // send 200 to client, indicating found
                n = write(newsockfd, "200", 3);
                if (n < 0) {
                    perror("ERROR writing to socket");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // TODO: Process call
        else if (strncmp(buffer, "CALL\n", 5) == 0) {
            // get the function name
            char *function_name = buffer + 5;
            // find the handler
            handler_node_t *handler_node =
                find_handler(srv->handler_list, function_name);
            if (handler_node == NULL) {
                // send 404 to client, indicating not found
                n = write(newsockfd, "404", 3);
                if (n < 0) {
                    perror("ERROR writing err code to socket");
                    exit(EXIT_FAILURE);
                }
            }
            // send success code to client
            n = write(newsockfd, "200", 3);
            if (n < 0) {
                perror("ERROR writing success code to socket");
                exit(EXIT_FAILURE);
            }
            rpc_data *data = malloc(sizeof(rpc_data));
            // read data1 from client
            uint32_t data1;
            n = read(newsockfd, &data1, sizeof(uint32_t));
            if (n < 0) {
                fprintf(stderr, "ERROR reading from socket\n");
                exit(EXIT_FAILURE);
            }
            data->data1 = ntohl(data1);
            // send 200 to client
            n = write(newsockfd, "200", 3);
            if (n < 0) {
                fprintf(stderr, "ERROR writing to socket\n");
                exit(EXIT_FAILURE);
            }
            // read data2_len from client
            uint32_t data2_len;
            n = read(newsockfd, &data2_len, sizeof(uint32_t));
            if (n < 0) {
                fprintf(stderr, "ERROR reading from socket\n");
                exit(EXIT_FAILURE);
            }
            data->data2_len = ntohl(data2_len);
            n = write(newsockfd, "200", 3);
            if (n < 0) {
                fprintf(stderr, "ERROR writing to socket\n");
                exit(EXIT_FAILURE);
            }
            // read data2 from client if data2_len > 0
            if (data->data2_len > 0) {
                data->data2 = malloc(data->data2_len);
                n = read(newsockfd, data->data2, data->data2_len);
                if (n < 0) {
                    fprintf(stderr, "ERROR reading from socket\n");
                    exit(EXIT_FAILURE);
                }
            } else{
                data->data2 = NULL;
            }

            // execute the function
            rpc_data *result = handler_node->handler(data);
            // send data1 to client
            uint32_t result_data1 = htonl(result->data1);
            n = write(newsockfd, &result_data1, sizeof(uint32_t));
            if (n < 0) {
                fprintf(stderr, "ERROR writing to socket\n");
                exit(EXIT_FAILURE);
            }
            n = read(newsockfd, buffer, 255);
            if (n < 0) {
                fprintf(stderr, "ERROR reading from socket\n");
                exit(EXIT_FAILURE);
            }
            if (strncmp(buffer, "200", 3) != 0) {
                fprintf(stderr, "ERROR respond from client\n");
                exit(EXIT_FAILURE);
            }
            memset(buffer, 0, 256);

            // send data2_len
            uint32_t result_data2_len = htonl(result->data2_len);
            n = write(newsockfd, &result_data2_len, sizeof(uint32_t));
            if (n < 0) {
                fprintf(stderr, "ERROR writing from socket\n");
                exit(EXIT_FAILURE);
            }
            n = read(newsockfd, buffer, 255);
            if (n < 0) {
                fprintf(stderr, "ERROR reading from socket\n");
                exit(EXIT_FAILURE);
            }
            if (strncmp(buffer, "200", 3) != 0) {
                fprintf(stderr, "ERROR respond from client\n");
                exit(EXIT_FAILURE);
            }
            memset(buffer, 0, 256);

            // send data2 if data2_len > 0
            if (result->data2_len > 0){
                n = write(newsockfd, result->data2, result->data2_len);
                if (n < 0) {
                    fprintf(stderr, "ERROR writing data2 from socket\n");
                    exit(EXIT_FAILURE);
                }
            }
            free(result);
            free(data);
        }
        memset(buffer, 0, 256);
    }
}

struct rpc_client {
    /* Add variable(s) for client state */
    char *addr;
    int port;
    // TODO: Add rpc handle list
    int socket_fd;
};

struct rpc_handle {
    /* Add variable(s) for handle */
    int id;
    char *function_name;
};

rpc_client *rpc_init_client(char *addr, int port) {
    rpc_client *new_client = malloc(sizeof(rpc_client));
    if (new_client == NULL) {
        return NULL;
    }
    new_client->addr = addr;
    new_client->port = port;

    int s;
    struct addrinfo hints, *server_info, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;      // IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    // change port to string
    char port_str[6];
    sprintf(port_str, "%d", port);
    // get address info of the server
    s = getaddrinfo(addr, port_str, &hints, &server_info);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        free(new_client);
        return NULL;
    }

    // connect to the server
    for (rp = server_info; rp != NULL; rp = rp->ai_next) {
        new_client->socket_fd =
            socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (new_client->socket_fd == -1) {
            continue;
        }
        if (connect(new_client->socket_fd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break; // success
        }
        close(new_client->socket_fd);
    }

    if (rp == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        free(new_client);
        return NULL;
    }

    return new_client;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    int n;
    // concat FIND and function name, delimited by new line
    char *find = "FIND\n";
    char *find_name = malloc(strlen(find) + strlen(name) + 1);
    strcpy(find_name, find);
    strcat(find_name, name);
    // send FIND to server
    n = write(cl->socket_fd, find_name, strlen(find_name));
    if (n < 0) {
        fprintf(stderr, "ERROR writing to socket\n");
        return NULL;
    }
    // read response from server
    char buffer[256];
    n = read(cl->socket_fd, buffer, 255);
    if (n < 0) {
        fprintf(stderr, "ERROR reading response from socket\n");
        return NULL;
    }
    // check if the function exists
    if (strcmp(buffer, "200") == 0) {
        // function exists
        rpc_handle *new_handle = malloc(sizeof(rpc_handle));
        if (new_handle == NULL) {
            return NULL;
        }
        new_handle->function_name = name;
        return new_handle;
    } else {
        // function does not exist
        return NULL;
    }
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    int n;
    char *call = "CALL\n";
    char *call_name = malloc(strlen(call) + strlen(h->function_name) + 1);
    strcpy(call_name, call);
    strcat(call_name, h->function_name);
    // send CALL to server
    n = write(cl->socket_fd, call_name, strlen(call_name));
    if (n < 0) {
        fprintf(stderr, "ERROR writing to socket\n");
        return NULL;
    }
    // read response from server
    char buffer[256];
    n = read(cl->socket_fd, buffer, 255);
    if (n < 0) {
        fprintf(stderr, "ERROR reading response from socket\n");
        return NULL;
    }
    // check if server is ready to receive payload
    if (strncmp(buffer, "200", 3) != 0) {
        fprintf(stderr, "ERROR server reject connection\n");
        return NULL;
    }
    // clear buffer
    memset(buffer, 0, 256);
    // send payload to server
    // change int to network byte order
    uint32_t data1 = htonl(payload->data1);
    n = write(cl->socket_fd, &data1, sizeof(data1));
    if (n < 0) {
        fprintf(stderr, "ERROR writing data1 to socket\n");
        return NULL;
    }
    // read response from server
    n = read(cl->socket_fd, buffer, 255);
    if (n < 0) {
        fprintf(stderr, "ERROR reading response from socket\n");
        return NULL;
    }
    if (strncmp(buffer, "200", 3) != 0) {
        fprintf(stderr, "ERROR respond from server\n");
        return NULL;
    }
    memset(buffer, 0, 256);
    // send data2_len to server
    uint32_t data2_len = htonl(payload->data2_len);
    n = write(cl->socket_fd, &data2_len, sizeof(data2_len));
    if (n < 0) {
        fprintf(stderr, "ERROR writing data2_len to socket\n");
        return NULL;
    }
    n = read(cl->socket_fd, buffer, 255);
    if (n < 0) {
        fprintf(stderr, "ERROR reading response from socket\n");
        return NULL;
    }
    if (strncmp(buffer, "200", 3) != 0) {
        fprintf(stderr, "ERROR respond from server\n");
        return NULL;
    }
    memset(buffer, 0, 256);
    // send data2 to server if data2_len > 0
    if (payload->data2_len > 0) {
        n = write(cl->socket_fd, payload->data2, payload->data2_len);
        if (n < 0) {
            fprintf(stderr, "ERROR writing data2 to socket\n");
            return NULL;
        }
        n = read(cl->socket_fd, buffer, 255);
        if (n < 0) {
            fprintf(stderr, "ERROR reading response from socket\n");
            return NULL;
        }
        if (strncmp(buffer, "200", 3) != 0) {
            fprintf(stderr, "ERROR respond from server\n");
            return NULL;
        }
        memset(buffer, 0, 256);
    }

    // read function result from server
    rpc_data *result = malloc(sizeof(rpc_data));
    // read data1
    uint32_t result_data1;
    n = read(cl->socket_fd, &result_data1, sizeof(uint32_t));
    if (n < 0) {
        fprintf(stderr, "ERROR reading data1 from socket\n");
        return NULL;
    }
    result->data1 = ntohl(result_data1);
    n = write(cl->socket_fd, "200", 3);
    if (n < 0) {
        fprintf(stderr, "ERROR writing success code to socket\n");
        return NULL;
    }

    // read data2_len
    uint32_t result_data2_len;
    n = read(cl->socket_fd, &result_data2_len, sizeof(uint32_t));
    if (n < 0) {
        fprintf(stderr, "ERROR reading data2_len from socket\n");
        return NULL;
    }
    result->data2_len = ntohl(result_data2_len);
    n = write(cl->socket_fd, "200", 3);
    if (n < 0) {
        fprintf(stderr, "ERROR writing success code to socket\n");
        return NULL;
    }

    // read data2 if data2_len > 0
    if (result->data2_len > 0) {
        // FIXME: allocate memory for data2
        result->data2 = malloc(result->data2_len);
        n = read(cl->socket_fd, result->data2, result->data2_len);
        if (n < 0) {
            fprintf(stderr, "ERROR reading data2 from socket\n");
            return NULL;
        }
    } else {
        result->data2 = NULL;
    }

    return result;
}

void rpc_close_client(rpc_client *cl) {
    if (cl == NULL){
        return ;
    }
    close(cl->socket_fd);
    // free client
    free(cl);
    cl = NULL;
}

void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
