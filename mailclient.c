//                             ____ _ _            _
//                            / ___| (_) ___ _ __ | |_
//                           | |   | | |/ _ \ '_ \| __|
//                           | |___| | |  __/ | | | |_
//                            \____|_|_|\___|_| |_|\__|
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#include "packet.c"
#include "fields.c"

#define PORT 5000
#define LINE_SIZE 81
#define MAX_LINES 51
#define MAX_BODY_SIZE LINE_SIZE *MAX_LINES + 1
#define BUFFER_SIZE 1024


void hr(){
    for (int i = 0; i < 80; i++)
        printf("-");
    printf("\n");
}


void error (const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char * argv[]){
    int sockfd, newsockfd, portno, n;
    struct sockaddr_in serv_addr; //Gives internet address 
    struct hostent *server;

    char buffer[255];

    if(argc < 3){
        fprintf(stderr, "Port Number not provided. Exitting.\n");
        exit(1);
    }
    char username[50], password[50];

    printf("Username: ");
    scanf("%s%*c", username);

    printf("Password: ");
    scanf("%s%*c", password);

 
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        error("ERROR opening socket.\n");
    }

    server = gethostbyname(argv[1]);
    if (server == NULL){
        fprintf(stderr, "Error, no such host. \n");
    }
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("Connection Failed.\n");
    }

   int length;
    char *response;

    send_packet(sockfd, username, strlen(username));
    send_packet(sockfd, password, strlen(password));

    length = recv_packet(sockfd, &response);

    if (strcmp(response, "Success") != 0){
        printf("Invalid credentials.");
        close(sockfd);
        return -1;
    }

    free(response);
    printf("Logged in!\n");
    int choice = 0;

    while(1){
        hr();
        printf("Welcome\n");
        printf("1. Send Mail\n2. Quit\n");
        printf("Option: ");
        scanf("%d%*c", &choice);
        printf("\n");

        if (choice == 2){
            printf("Goodbye\n");
            response = "EXIT";
            send_packet(sockfd, response, strlen(response));
            break;
        }

        else if(choice == 1){
            char from[50], to[50], subject[50], body[MAX_BODY_SIZE];
            char buffer[LINE_SIZE] = {0};
            int count= 0;

            printf("From: ");
            scanf("%s%*c", from);
            printf("To: ");
            scanf("%s%*c", to);
            printf("Subject: ");
            scanf("%[^\n]%*c", subject);

            if (!verify_email(to))
            {
                printf("Invalid recepient email!\nEmail should be of the format X@Y\n");
                continue;
            }
            if (!verify_email(from))
            {
                printf("Invalid sender email!\nEmail should be of the format X@Y\n");
                continue;
            }
            printf("Body:\n");
            while (strcmp(buffer, ".") != 0 && count < MAX_BODY_SIZE)
            {
                scanf("%[^\n]%*c", buffer);
                count += sprintf(body + count, "%s\n", buffer);
            }

            char data[BUFFER_SIZE];

            count = 0;
            count += sprintf(data + count, "From: %s\n", from);
            count += sprintf(data + count, "To: %s\n", to);
            count += sprintf(data + count, "Subject: %s\n", subject);
            count += sprintf(data + count, "%s\n", body);

            send_packet(sockfd, data, count);

            recv_packet(sockfd, &response);

            if (strcmp("EMAIL SENT", response) == 0)
            {
                printf("Email sent succesffully!\n");
                continue;
            }
            else
            {
                printf("Error sending email!\n");
                printf("Error: %s\n", response);
            }

            free(response);
        } 
    }
    close(sockfd);
    return 0;
}
