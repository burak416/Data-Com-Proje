#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")  

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock could not be initialized. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket could not be created. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Connection error. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    
    while (1) {
        printf("Enter product (type 'finish' to total price): ");
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = 0;

        send(sock, message, strlen(message), 0);

        memset(buffer, 0, BUFFER_SIZE);
        recv(sock, buffer, BUFFER_SIZE, 0);
        printf("Server: %s\n", buffer);

        if (strcmp(message, "finish") == 0) {
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

