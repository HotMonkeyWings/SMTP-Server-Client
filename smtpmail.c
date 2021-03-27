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

#define PORT 8080

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
        char send_username[50];
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

        
        // Verify Emails
        if (!verify_email(from)){
            char *invalidFrom = "Invalid";
            printf("[X] Invalid Sender Email\n");
            send_packet(client->sockfd, invalidFrom, strlen(invalidFrom));
            continue;
        }

        if (!verify_email(to)){
            char *invalidTo = "Invalid";
            printf("[X] Invalid Recepient Email\n");
            send_packet(client->sockfd, invalidTo, strlen(invalidTo));
            continue;
        }

       
        // Check Recepient
        int p = has_char(to, '@');
        int rec_flag = 0;
        strncpy(send_username, to, p);
        send_username[p] = 0;

        for(int i = 0; i < client->total; i++){
            if (strcmp(client->acc[i].username, username) == 0)
                rec_flag = 1;
        }

   
        if (!rec_flag){
            printf("[X] Recepient %s not found\n", to);
            char *emailError = "Invalid Email";
            send_packet(client->sockfd, emailError, strlen(emailError));
            continue;
        }

        char mailbox_fname[100];
        sprintf(mailbox_fname, "%s/mymailbox.mail", send_username);
        FILE *mailbox = fopen(mailbox_fname, "a");

        char time_str[50];
        get_time_string(time_str);

        fprintf(mailbox, "From: %s\n", from);
        fprintf(mailbox, "To: %s\n", to);
        fprintf(mailbox, "Subject: %s\n", subject);
        fprintf(mailbox, "Received: %s\n", time_str);
        fprintf(mailbox, "%s", body);

        fclose(mailbox);

        char *succ = "EMAIL SENT";
        printf("[+] %s has sent mail to %s.\n", from, to);

        send_packet(client->sockfd, succ, strlen(succ));

        free(data);

    }

    printf("[-] Client %s Disconnnected\n", client->username);

    close(client->sockfd);
    remove_client(client->head, client);
}
void show_usage()
{
    printf("Usage: ");
    printf("./client.o PORT\n");
}
int main(int argc, char *argv[])
{
    int my_port = PORT;
    if (argc == 2)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            show_usage();
            return 0;
        }
        if (sscanf(argv[1], "%d", &my_port) != 1)
        {
            printf("Invalid PORT number\n");
            show_usage();
            return -1;
        }
    }
    else if (argc == 1)
    {
        printf("PORT not specified\n");
        printf("Using the default my_port as %d\n", my_port);
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
    address.sin_port = htons(my_port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("Error binding to my_port!\n");
        return -1;
    }

    if (listen(server_fd, 3) < 0)
    {
        printf("Error while listening for connections!\n");
        return -1;
    }

    printf("Server started!\n");
    printf("Listening on 0.0.0.0:%d\n", my_port);

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
