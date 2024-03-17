#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>
#include <poll.h>

#include "parse.h"
#include "file.h"
#include "common.h"
#include "srvpoll.h"

#define MAX_CLIENTS 256

clientstate_t clientStates[MAX_CLIENTS] = {0};

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file>\n", argv[0]);
    printf("\t -n - create new database file\n");
    printf("\t -f - (required) path to database file\n");
    return;
}

// poll functionality from previous lesson
void poll_loop(unsigned short port, struct dbheader_t *dbhdr, struct employee_t *employees) {
    int listen_fd, conn_fd, nfds, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS + 1];
    nfds = 1;
    int opt = 1;

    init_clients(&clientStates);

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return;
    }

    // make socket non waiting, short for 'set socket option'
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = 0;
    server_addr.sin_port = htons(8080);

    // bind the socket
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return;
    }

    // listen to the socket
    if (listen(listen_fd, 0) == -1) {
        perror("listen");
        close(listen_fd);
        return;
    }

    printf("Server listening on port %d\n", PORT);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {
        // First, rebuild the fds array for poll() on each iteration
        nfds = 1; // Start with one for listen_fd
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1) {
                fds[nfds].fd = clientStates[i].fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        // Wait for an event on one of the sockets
        int n_events = poll(fds, nfds, -1); // -1 for no timeout
        if (n_events == -1) {
            perror("poll");
            break; // Exit or handle error appropriately
        }

        // Check if there's an incoming connection on the listen_fd
        if (fds[0].revents & POLLIN) {
            conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (conn_fd == -1) {
                perror("accept");
                continue; // Continue to next iteration of the loop
            }

            printf("New Connection: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            int freeSlot = find_free_slot(&clientStates);
            if (freeSlot == -1) {
                printf("Server is full: closing new connection\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_CONNECTED;
                // No need to increment nfds here, it's done at the beginning of the loop
                printf("Slot %d has fd %d\n", freeSlot, clientStates[freeSlot].fd);
            }
        }

        // Handle IO on other sockets
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int slot = find_slot_by_fd(&clientStates, fds[i].fd);
                if (slot != -1) { // Valid slot found
                    ssize_t bytes_read = read(fds[i].fd, clientStates[slot].buffer, BUFFER_SIZE - 1);
                    if (bytes_read <= 0) {
                        // Connection closed or error
                        // printf("Connection closed or error on fd %d\n", fds[i].fd);
                        close(fds[i].fd);
                        clientStates[slot].fd = -1; // Mark slot as free
                        clientStates[slot].state = STATE_DISCONNECTED;
                        // No need to decrement nfds here, it's recalculated at the start of the loop
                    } else {
                        // Data received, process here
                        // Ensure to null-terminate the received string if expecting C-strings
                        clientStates[slot].buffer[bytes_read] = '\0';
                        // printf("Received data from client: %s\n", clientStates[slot].buffer);
                        // Respond to client or process data as needed

                        // Now we will call functionality for handling the client fsm
                        clientStates[freeSlot].state = STATE_HELLO;
                        handle_client_fsm(dbhdr, employees, &clientStates[slot]);
                    }
                }
            }
        }
    }
}


int main(int argc, char *argv[]) {
    char *filepath = NULL;
    char *portarg = NULL;
    unsigned short port = 0;
    bool newfile = false;
    bool list = false;
    int c;
    int dbfd = -1;

    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    while ((c = getopt(argc, argv, "nf:p:")) != -1) {
        switch (c) {
            case 'n':
                newfile = true;
                break;
            case 'f':
                filepath = optarg;
                break;
            case 'p':
                portarg = optarg;
                port = atoi(portarg);
                if (port == 0) {
                    printf("Bad port: %s\n", portarg);
                }
                break;
            case '?':
                printf("Unknown option -%c\n", c);
                break;
            default:
                return -1;
        }
    }

    if (filepath == NULL) {
        printf("Filepath is a required agrument\n");
        print_usage(argv);

        return 0;
    }

    if (newfile) {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to create database file\n");
            return -1;
        }

       if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to create database header\n");
            return -1;
       }
    } else {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR) {
            printf("Unable to open database file\n");
            return -1;
        }
        if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
            printf("Failed to validate database header\n");
            return -1;
        }
    }
    
    if (read_employees(dbfd, dbhdr, &employees) != STATUS_SUCCESS) {
        printf("Failed to read employees\n");
        return 0;
    } 

    if (list) {
        list_employees(dbhdr, employees);
    }

    // will add functionality for calling our poll functionality
    poll_loop(port, dbhdr, employees);

    output_file(dbfd, dbhdr, employees);
    
    return 0;
}