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

void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t *employees, clientstate_t *client) {
    // Cast the client buffer into a header type
    dbproto_hdr_t *hdr = (dbproto_hdr_t*)client->buffer;
    // Unpack hdr type and length from network endian to host endian


    // Add conditional logic for state being 'STATE_HELLO' or 'STATE_MSG'
    if (client->state == STATE_HELLO) {
        // if hdr type isnt a 'MSG_HELLO_REQ' or the length doesnt equal 1 then throw an error

        // initialize a pointer variable of dbproto_hello_req type to the pointer of hdr at index 1
            // cast the pointer to hdr to dbproto_hello_req type (because the data at that index is )

        // unpack the proto field of the variable just set from network to host endian
        // check to see if the variables proto field matches the PROTO_VER (defined in common.h)

        // if all those checks pass, change the clients state to STATE_MSG
    }


    if (client->state == STATE_MSG) {
        // Now that we have said hello to the client, we will expect a data request
            // employee list, add or delete

        // 
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