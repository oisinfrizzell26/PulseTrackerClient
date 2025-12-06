#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5001

// Helper function to create a socket and connect to the server
int connect_to_server() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed. Is the server running?\n");
        return -1;
    }
    return sock;
}

// Function to get the current mode from the server
int get_mode() {
    int sock = connect_to_server();
    if (sock < 0) return -1; // Return -1 on connection error

    char *request = "GET /get-mode HTTP/1.1\r\nHost: " SERVER_IP "\r\nConnection: close\r\n\r\n";
    send(sock, request, strlen(request), 0);

    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    close(sock);

    // Simple parsing of JSON: {"mode": 1}
    // Look for "mode": and take the number after it
    char *mode_ptr = strstr(buffer, "\"mode\":");
    if (mode_ptr) {
        // Move pointer past "mode":
        // "mode": is 7 chars. 
        // It might be "mode": 1 or "mode":1
        char *num_start = mode_ptr + 7; 
        return atoi(num_start);
    }
    return -1; // Return -1 if parsing fails
}



int main() {
    printf("--- Starting C Client (Mock ESP32) ---\n");
    printf("Target: %s:%d\n\n", SERVER_IP, SERVER_PORT);

    int current_mode = 0;

    while (1) {
        // 1. Poll for mode
        int server_mode = get_mode();
        
        if (server_mode != -1) {
            current_mode = server_mode;
            printf("[CLIENT] Server says Mode is: %d\n", current_mode);
        } else {
            printf("[CLIENT] Connection failed. Using cached Mode: %d\n", current_mode);
        }

        // 2. Wait 1 second
        sleep(1);
    }
    return 0;
}
