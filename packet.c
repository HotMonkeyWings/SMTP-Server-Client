#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define HEADER_SIZE 8

int send_packet(int sockfd, char *data, int length){
    char header[HEADER_SIZE];
    char format[20];

    sprintf(format, "%%0%dd", HEADER_SIZE - 1);
    sprintf(header, format, length);

    header[HEADER_SIZE - 1] = 0;

    send(sockfd, header, HEADER_SIZE, 0);

    return send(sockfd, data, length, 0);
}

int recv_packet(int sockfd, char **data){
    char header[HEADER_SIZE];

    recv(sockfd, header, HEADER_SIZE, 0);
    int length = -1;
    sscanf(header, "%d", &length);

    if (length <= 0){
        printf("COMM_ERROR : Invalid length/header format\n");
        return -1;
    }

    char *buffer = calloc(length, sizeof(char));
    *data = buffer;

    int recv_len = recv(sockfd, buffer, length, 0);
    buffer[recv_len] = '\0';
    return recv_len + 1;
}
