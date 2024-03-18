#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>
#include <poll.h>

#include "srvpoll.h"
#include "common.h"
#include "parse.h"

void fsm_reply_hello(clientstate_t *client, dbproto_hdr_t *hdr) {
    hdr->type = htonl(MSG_HELLO_RESP);
    hdr->len = htons(1);
    dbproto_hello_resp* hello = (dbproto_hello_resp*)&hdr[1];
    hello->proto = htons(PROTO_VER);

    write(client->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req));
}

void fsm_reply_hello_err(clientstate_t *client, dbproto_hdr_t *hdr) {
    printf("in fsm_reply_hello\n");
    hdr->type = htonl(MSG_ERROR);
    hdr->len = htons(0);

    write(client->fd, hdr, sizeof(dbproto_hdr_t));
}

void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t *employees, clientstate_t *client) {
    dbproto_hdr_t *hdr = (dbproto_hdr_t*)client->buffer;
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (client->state == STATE_CONNECTED) {
        client->state = STATE_HELLO; 
    }
    
    struct employee_t *employeePtr = NULL;

    if (client->state == STATE_HELLO && hdr->type == MSG_HELLO_REQ) {
        printf("Header type: %d\n", hdr->type);
        printf("Header len: %d\n", hdr->len);
        if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
            printf("Didn't get MSG_HELLO in HELLO state...\n");
            return;
        }

        dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
        hello->proto = ntohs(hello->proto);
        if (hello->proto != PROTO_VER) {
            printf("Protocol mismatch...\n");
            fsm_reply_hello_err(client, hdr);
            return;
        }
        fsm_reply_hello(client, hdr);
        client->state = STATE_MSG;
    }


    if (client->state == STATE_MSG) {
        printf("Inside if STATE_MSG\n");
        printf("Header type inside STATE_MSG: %d\n", ntohl(hdr->type));
        if (hdr->type == MSG_EMPLOYEE_ADD_REQ) {
            dbproto_employee_add_req* employee = (dbproto_employee_add_req*)&hdr[1];
            printf("Adding employee: %s\n", employee->data);
            // if (add_employee(dbhdr, employeePtr, employee->data) != STATUS_SUCCESS) {
            //     fsm_reply_add_err(client, hdr);
            //     return;
            // } 
            // else {
            //     fsm_reply_add(client, hdr);
            //     output_file(dbfd, dbhdr, *employeePtr);
            // }
        }
    }
};

void init_clients(clientstate_t* states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        states[i].fd = -1;
        states[i].state = STATE_NEW;
        memset(&states[i].buffer, '\0', BUFFER_SIZE);
    }
};

int find_free_slot(clientstate_t* states) {
     for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

int find_slot_by_fd(clientstate_t* states, int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == fd) {
             return i;
        }
    }
    return -1;
}