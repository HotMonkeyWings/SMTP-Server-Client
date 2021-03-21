//                         ____
//                        / ___|  ___ _ ____   _____ _ __
//                        \___ \ / _ \ '__\ \ / / _ \ '__|
//                         ___) |  __/ |   \ V /  __/ |
//                        |____/ \___|_|    \_/ \___|_|
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "packet.c"
#include "fields.c"

#define PORT 5000

typedef struct Acc{
    char username[50];
    char password[50];
} Acc;

typedef struct Client{
    char *username;
    char *password;
    int sockfd;

    Acc *acc;
    int total;

    struct Client *head;
    struct Client *next;
} Client;

Client *new_client(int sockfd, Client *head, Acc *acc, int total){
    Client *client = (Client *)malloc(sizeof(Client));
    client->username = NULL;
    client->password = NULL;
    client->sockfd = sockfd;

    client->acc = acc;
    client->total = total;

    client->head = head;
    client->next = NULL;
    return client;
}

Client *init_list(){
    Client *head = new_client(0, NULL, NULL, 0);
    return head;
}

void add_client(Client *head, Client *client){
    Client *curr = head;
    while (curr->next)
        curr = curr->next;
    curr->next = client;
}

int remove_client(Client *head, Client *client){
    Client *curr = head;
    
    while(curr){
        if (curr->next == client){
            curr->next = client->next;
            free(client);
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

int check_recepient(char *email, Acc *acc, int total){
    char username[50];
    int p = has_char(email, '@');
    if (p == -1)
        return -1;
    strncpy(username, email, p);
    username[p] = 0;

    for(int i = 0; i < total; i++){
        if (strcmp(acc[i].username, username) == 0)
            return i;
    }

    return -1;
}

void get_time_string(char *time_string){
    time_t current_time;
    struct tm *timeinfo;

    current_time = time(NULL);
    timeinfo = gmtime(&current_time);
    strftime(time_string, 100, "%a, %d %b %Y %I:%M:%S GMT", timeinfo);
}

void *handle_client(void *arg){
    Client *client = (Client *)arg;

    printf("[+] New client connected!\n");
    printf("Sockfd:%d\n", client->sockfd);

    char *username, *password;
    if (recv_packet(client->sockfd, &username) == -1){
        printf("[!] Data is corrupt!\n");
        printf("[-] Connected Closed\n");
        close(client->sockfd);
        remove_client(client->head, client);
        return NULL;
    }
    if (recv_packet(client->sockfd, &password) == -1){
        printf("[!] Data is corrupt!\n");
        printf("[-] Connected Closed\n");
        close(client->sockfd);
        remove_client(client->head, client);
        return NULL;
    }

    char *invalidPassword = "Invalid Password";
    char *invalidUser = "Invalid Username";
    char *success = "Success";

    int flag = 0;

    for (int i = 0; i < client->total; i++){
        if (strcmp(client->acc[i].username, username) == 0){
            if (strcmp(client->acc[i].password, password) != 0){
                send_packet(client->sockfd, invalidPassword, strlen(invalidPassword));

                close(client->sockfd);
                remove_client(client->head, client);
                return NULL;
            } else {
                flag = 1;
                client->username = username;
                client->password = password;
                break;
            }
        }
    }
    
    // Incase Username is invalid
    if (flag == 0){
        send_packet(client->sockfd, invalidUser, strlen(invalidUser));
        close(client->sockfd);
        remove_client(client->head, client);
        return NULL;
    }

    send_packet(client->sockfd, success, strlen(success));

    printf("[+] Client %s authenticated successfully.\n", client->username);

    while (1){
        char *data;
        char buffer[1024];
        int len = recv_packet(client->sockfd, &data);
        int i = 0;
        if (len == -1){
            printf("Data is corrupt!\n");
            break;
        }
        if (strcmp(data, "EXIT") == 0){
            break;
        }

        printf("[+] Received new mail\n");

        char from[50], to[50], subject[50], body[1024];

        get_field(data, "From", from);
        get_field(data, "To", to);
        get_field(data, "Subject", subject);
        get_field(data, "Body", body);

        printf("> From: %s\n", from);
        printf("> To: %s\n", to);

        if (!verify_email(from)){
            char *invalidFrom = "Invalid Sender email\n";
            printf("[X] Invalid Sender Email\n");
            send_packet(client->sockfd, invalidFrom, strlen(invalidFrom));
            continue;
        }

        if (!verify_email(to)){
            char *invalidTo = "Invalid Recepient email\n";
            printf("[X] Invalid Recepient Email\n");
            send_packet(client->sockfd, invalidTo, strlen(invalidTo));
            continue;
        }
        
        if (check_recepient(to, client->acc, client->total) == -1){
            printf("[X] Recepient %s not found\n", to);
            char *emailError = "Invalid Email";
            send_packet(client->sockfd, emailError, strlen(emailError));
            continue;
        }

        char mailbox_fname[100];
        sprintf(mailbox_fname, "%s/mymailbox.mail", client->acc[i].username);
        FILE *mailbox = fopen(mailbox_fname, "a");

        char time_str[50];
        get_time_string(time_str);

        fprintf(mailbox, "From: %s\n", from);
        fprintf(mailbox, "To: %s\n", to);
        fprintf(mailbox, "Subject: %s\n", subject);
        fprintf(mailbox, "Received: %s\n", time_str);
        fprintf(mailbox, "%s", body);

        fclose(mailbox);

        char *succ = "MAIL SENT";
        printf("[+] %s has sent mail to %s", from, to);

        send_packet(client->sockfd, succ, strlen(succ));

        free(data);

    }

    printf("[-] Client %s Disconnnected\n", client->username);

    close(client->sockfd);
    remove_client(client->head, client);
}

/*
int main(int argc, char * argv[]){
    

    int sockfd, newsockfd, portno, n;
    int opt = 1;
    char buffer[255];

    struct sockaddr_in serv_addr, cli_addr; //Gives internet address 
    socklen_t clilen; //32-bit datatype

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("Error opening Socket");
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        printf("Error setting socket options!\n");
        return -1;
    }


    bzero((char *) &serv_addr, sizeof(serv_addr)); //Clears the text

    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno); //Host to network short

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        printf("Binding Failed.");
        return -1;
    }
    
    printf("\nListening on Port %d\n\n",portno);
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    char username[50], password[50];

    FILE *acc_file = fopen("logincred.txt", "r");

    char fusername[50], fpassword[50];

    int total = 0;
    while(fscanf(acc_file, "%[^,]%*c %[^\n]%*c", fusername, fpassword) != EOF)
        total++;
    
    Acc *acc = calloc(total, sizeof(Acc));

    fseek(acc_file, 0, SEEK_SET);
   
    int i = 0;

    while(fscanf(acc_file, "%[^,]%*c %[^\n]%*c", fusername, fpassword) != EOF){
        strcpy(acc[i].username, fusername);
        strcpy(acc[i].password, fpassword);
        i++;
    }

    Client *head = init_list();

    while(1){

        if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t*)&clilen))<0){
            perror("In accept");
            exit(EXIT_FAILURE);
        }
        
        fflush(stdout);

        Client *client = new_client(newsockfd, head, acc, total);
        add_client(head, client);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client);

        close(newsockfd);
    }
    close(sockfd);
    return 0;
}
void show_usage()
{
    printf("Usage: ");
    printf("./client.o PORT\n");
    printf("PORT is the port number to use to connect to the SMTP server\n");
}
*/
int main(int argc, char *argv[])
{
    int port = PORT;
    if (argc == 2)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            show_usage();
            return 0;
        }
        if (sscanf(argv[1], "%d", &port) != 1)
        {
            printf("Invalid PORT number\n");
            show_usage();
            return -1;
        }
    }
    else if (argc == 1)
    {
        printf("PORT not specified\n");
        printf("Using the default port as %d\n", port);
    }
    else
    {
        printf("Invalid arguments\n");
        show_usage();
        return -1;
    }
    int server_fd, conn_socket, recv_len;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char message[256];
    char buffer[1024] = {0};

    printf("Starting SMTP server...\n");

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError creating socket\n");
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        printf("Error setting socket options!\n");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("Error binding to port!\n");
        return -1;
    }

    if (listen(server_fd, 3) < 0)
    {
        printf("Error while listening for connections!\n");
        return -1;
    }

    printf("Server started!\n");
    printf("Listening on 0.0.0.0:%d\n", port);

    char username[50], password[50];

    FILE *cred_file = fopen("logincred.txt", "r");

    char fusername[50], fpassword[50];

    int total = 0;
    while (fscanf(cred_file, "%[^,]%*c %[^\n]%*c", fusername, fpassword) != EOF)
        total++;

    Acc *creds = calloc(total, sizeof(Acc));

    fseek(cred_file, 0, SEEK_SET);
    int i = 0;
    while (fscanf(cred_file, "%[^,]%*c %[^\n]%*c", fusername, fpassword) != EOF)
    {
        strcpy(creds[i].username, fusername);
        strcpy(creds[i].password, fpassword);
        i++;
    }

    Client *head = init_list();

    while (1)
    {
        if ((conn_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            printf("Error accepting connection!");
            continue;
        }

        fflush(stdout);

        Client *client = new_client(conn_socket, head, creds, total);
        add_client(head, client);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client);
    }

    return 0;
}
