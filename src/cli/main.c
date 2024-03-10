#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>

#include "common.h"

// implement a function for sending a message to the client
void send_hello(int fd) {
    char buff[4096] = {0};

    // create the hdr
    dbproto_hdr_t *hdr = buff;
    hdr->type = MSG_HELLO_REQ;
    hdr->len = 1;

    // convert the hdr to network endian
    hdr->type = htonl(hdr->type);
    hdr->len =  htons(hdr->len);

    // write the fd to the buffer
    write(fd, buff, sizeof(dbproto_hdr_t));

    printf("Server connected, protocol v1.\n");
}

int main(int argc, char *argv[]) {
    // check to see if args were handed in from the command line
    if (argc != 2) {
        printf("Usage: %s <ip of the host>\n", argv[0]);
        return 0;
    }

    struct sockaddr_in serverInfo = {0};

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);
    serverInfo.sin_port = htons(9128);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    } 

    if  (connect(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
        perror("connect");
        close(fd);
        return -1;
    }

    send_hello(fd);

    close(fd);
} 