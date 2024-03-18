#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"

int send_employee(int fd, char *addstr) {
    printf("The add string: %s\n", addstr);
    // Now that I am here, lets focus on sending an employee
    // lets use the send_hello as a reference
    char buff[4096] = {0};

    // create the hdr
    dbproto_hdr_t *hdr = buff;
    hdr->type = MSG_EMPLOYEE_ADD_REQ;
    hdr->len = 1;

    dbproto_employee_add_req* employee = (dbproto_hello_req*)&hdr[1];
    strncpy(&employee->data, addstr, sizeof(employee->data));

    hdr->type = htonl(hdr->type);
    hdr->len =  htons(hdr->len);

    write(fd, buff, sizeof(dbproto_hdr_t) + sizeof(dbproto_employee_add_req));

    read(fd, buff, sizeof(buff));

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (hdr->type == MSG_ERROR) {
        printf("Improper format for the add employee string.\n");
        close(fd);
        return STATUS_ERROR;
    }

    if (hdr->type == MSG_EMPLOYEE_ADD_RESP) {
        printf("Employee successfully added\n");
    }

    return STATUS_SUCCESS;
}

// implement a function for sending a message to the client
int send_hello(int fd) {
    char buff[4096] = {0};

    // create the hdr
    dbproto_hdr_t *hdr = buff;
    hdr->type = MSG_HELLO_REQ;
    hdr->len = 1;
    

    // send the hello request with the version we speak
    dbproto_hello_req* hello = (dbproto_hello_req*)&hdr[1];
    hello->proto = PROTO_VER;

    // convert the hdr to network endian
    hdr->type = htonl(hdr->type);
    hdr->len =  htons(hdr->len);
    hello->proto = htons(hello->proto);

    // write the fd to the buffer
    printf("in send_hello before write: %d\n", hdr->type);
    write(fd, buff, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req));

    // receive the response
    read(fd, buff, sizeof(buff));

    // convert hdr to network endian
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);
    printf("Received response: hdr->type after ntohl: %d\n", hdr->type);

    // handle error response
    if (hdr->type == MSG_ERROR) {
        printf("Protocol mismatch.\n");
        close(fd);
        return STATUS_ERROR;
    }

    // return success
    printf("Server connected, protocol v1.\n");
    return STATUS_SUCCESS;
    // I believe I will need to call send employee instead of returning STATUS_SUCCESS
}

int main(int argc, char *argv[]) {
    char *addarg = NULL;
    char *portarg = NULL;
    char *hostarg = NULL;
    unsigned short port = 0;

    // check to see if args were handed in from the command line
    if (argc < 2) {
        printf("Usage: %s <ip of the host>\n", argv[0]);
        return 0;
    }

    int c;
    while ((c = getopt(argc, argv, "p:h:a:")) != -1) {
        switch (c) {
            case 'a':
                addarg = optarg;
                break;
            case 'h':
                hostarg = optarg;
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

    if (port == 0) {
        printf("Bad Port: %s\n", portarg);
        return -1;
    }

    if (hostarg == NULL) {
        printf("Must specify host with -h\n");
        return -1;
    }


    struct sockaddr_in serverInfo = {0};

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = inet_addr(hostarg);
    serverInfo.sin_port = htons(port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    } 

    if (connect(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
        perror("connect test");
        close(fd);
        return -1;
    }

    if(send_hello(fd) != STATUS_SUCCESS) {
        return -1;
    }

    // This will be for adding employees to the db from the client
    if (addarg) {
        send_employee(fd, addarg);
    }

    close(fd);
} 