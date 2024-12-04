#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <pthread.h>

#pragma comment(lib, "ws2_32.lib") 

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    char product_name[BUFFER_SIZE];
    int stock;
    float price;
} Product;


Product inventory[] = {
    {"3060", 5, 5000.0},
    {"3050", 7, 4000.0},
    {"4050", 9, 7000.0},
    {"4060", 12, 8000.0},
    {"i7 14th 14700k" , 12, 8000.0},
    {"i7 12th 12700k" , 6, 6000.0},
    {"i5 13th 13600k" , 7, 5000.0},
    {"i5 14th 14700k" , 14, 7000.0},
    {"Cooler", 8, 5500.0}	
};

typedef struct {
    SOCKET socket;
    struct sockaddr_in address;
} Client;

int client_count = 0;
pthread_mutex_t lock;

void* handle_client(void* client_socket);

int main() {
    WSADATA wsa;
    SOCKET server_fd;
    struct sockaddr_in address;

    pthread_mutex_init(&lock, NULL);

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        printf("Socket could not be created. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        printf("Bind error. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        printf("Listen error. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Server started. Waiting for clients...\n");

    while (1) {
        SOCKET new_socket;
        struct sockaddr_in client_address;
        int addrlen = sizeof(client_address);

        new_socket = accept(server_fd, (struct sockaddr*)&client_address, &addrlen);
        if (new_socket == INVALID_SOCKET) {
            printf("Connection could not be accepted. Error Code: %d\n", WSAGetLastError());
            continue;
        }

        pthread_mutex_lock(&lock);
        client_count++;
        printf("A new client connected. Total clients: %d\n", client_count);
        pthread_mutex_unlock(&lock);

        pthread_t thread_id;
        Client* client = malloc(sizeof(Client));
        client->socket = new_socket;
        client->address = client_address;

        pthread_create(&thread_id, NULL, handle_client, client);
        pthread_detach(thread_id);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}

void* handle_client(void* arg) {
    Client* client = (Client*)arg;
    SOCKET socket = client->socket;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    float total_price = 0.0;
    char cart[BUFFER_SIZE * 2] = "";

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = recv(socket, buffer, BUFFER_SIZE, 0);
        if (valread <= 0) {
            printf("A client disconnected.\n");
            pthread_mutex_lock(&lock);
            client_count--;
            printf("Remaining clients: %d\n", client_count);
            pthread_mutex_unlock(&lock);
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "finish") == 0) {
            sprintf(response, "Products in your cart:\n%sTotal Price: %.2f TL\n", cart, total_price);
            send(socket, response, strlen(response), 0);
            continue;
        }

        int i, found = 0;
        for (i = 0; i < sizeof(inventory) / sizeof(inventory[0]); i++) {
            if (strcmp(buffer, inventory[i].product_name) == 0) {
                found = 1;
                if (inventory[i].stock > 0) {
                    inventory[i].stock--;
                    total_price += inventory[i].price;
                    sprintf(response, "%s added to cart. Remaining stock: %d. Price: %.2f TL\n", 
                            inventory[i].product_name, inventory[i].stock, inventory[i].price);
                    strcat(cart, inventory[i].product_name);
                    strcat(cart, "\n");
                } else {
                    sprintf(response, "%s is out of stock.\n", inventory[i].product_name);
                }
                break;
            }
        }
        if (!found) {
            sprintf(response, "%s not found.\n", buffer);
        }

        send(socket, response, strlen(response), 0);
    }

    closesocket(socket);
    free(client);
    return NULL;
}

