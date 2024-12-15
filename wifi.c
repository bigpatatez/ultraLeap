#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "wifi.h"

#define PORT 1212


// Function to get the local IP address
char* get_local_ip() {
    static char local_ip[INET_ADDRSTRLEN];
    int sock;
    struct sockaddr_in addr;

    // Create a UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return "127.0.0.1";
    }

    // Specify an external address to connect to
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connect failed");
        close(sock);
        return "127.0.0.1";
    }

    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(sock, (struct sockaddr*)&local_addr, &addr_len) == -1) {
        perror("getsockname failed");
        close(sock);
        return "127.0.0.1";
    }

    // Convert the address to a readable format
    inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, INET_ADDRSTRLEN);
    close(sock);
    printf("local ip: %s\n",local_ip);
    return local_ip;
}

void* wifiRoutine(void* arg) {
    char* HOST = get_local_ip();//"192.168.243.66";
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    //char buffer[BUFFER_SIZE]; // this should be common to both threads
    ThreadArgs* args = (ThreadArgs*)arg;
    char* buffer = args->buffer;
    pthread_mutex_t* mutex = args->mutex;
    int BUFFER_SIZE = args->size;
    BRAMReader* reader = args->reader;

    // Create the server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &server_addr.sin_addr);

    // Bind the server socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 1) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s:%d\n", HOST, PORT);

    while (1) {
        printf("Waiting for a connection...\n");

        // Accept a new client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Connected to client\n");

        // Receive data from the client
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

            if (bytes_received <= 0) {
                printf("Client disconnected\n");
                break;
            }
            int received_value = atoi(buffer);
            printf("Received message: %i\n", received_value);

            if (received_value == 0){
                pthread_mutex_lock(mutex);
                uint32_t current_value;
                readBRAMData(reader, 0, &current_value); // get the current value at the first word
	            modifyBRAMBits(reader, 0, 0x1, current_value ^ 0x1);
	            pthread_mutex_unlock(mutex);
            } else {
            	received_value <<= 1;
                pthread_mutex_lock(mutex);
	            modifyBRAMBits(reader, 0, 0x6, received_value);
	            pthread_mutex_unlock(mutex);
                if (received_value == 0x6){// listen to new button to determine filter type, then set it into memory
                    printf("Selecting filter type, press another button\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    ssize_t filter_type_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

                    if (filter_type_received <= 0) {
                        printf("Client disconnected\n");
                        break;
                    }
					int filter_type_to_mem = atoi(buffer);
                    printf("Received message: %d\n", filter_type_to_mem);
           
                    filter_type_to_mem <<= 12;
                    pthread_mutex_lock(mutex);
	                modifyBRAMBits(reader, 0, 0x7000, filter_type_to_mem);
	                pthread_mutex_unlock(mutex);
                }
            }
            
        }
        // Close the client socket
        close(client_socket);
    }

    // Close the server socket
    close(server_socket);
    return 0;
}
