#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_FILENAME_LEN 100
#define MAX_DATA_LEN 100

struct pdu {
    char type;
    char data[MAX_DATA_LEN];
};

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    int server_socket, client_socket, port=3000;
    socklen_t client_addr_len;

    switch(argc){
		case 1:
		break;
		case 2:
			port = atoi(argv[1]);
		break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    /* Allocate a socket */
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0){
        fprintf(stderr, "Can't create socket \n");
    }

    // Bind socket to port
    if (bind(server_socket, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        struct pdu rpdu;
        memset(&rpdu, 0, sizeof(rpdu));

        printf("\nServer waiting for client request...\n");

        // Receive filename from client
        ssize_t bytes_received = recvfrom(server_socket, &rpdu, sizeof(rpdu), 0, (struct sockaddr*)&client_addr, &client_addr_len);
        if (bytes_received < 0) {
            fprintf(stderr, "Error receiving data from client\n");
            continue;
        }

        if (rpdu.type != 'C') {
            fprintf(stderr, "Invalid request from client\n");
            continue;
        }

        // Open requested file
        FILE *fp = fopen(rpdu.data, "rb");
        if (fp == NULL) {
            struct pdu spdu;
            spdu.type = 'E';
            strcpy(spdu.data, "File not found");
            sendto(server_socket, &spdu, strlen(spdu.data)+1, 0, (struct sockaddr*)&client_addr, client_addr_len);
            fprintf(stderr, "File \"%s\" not found\n", rpdu.data);
            continue;
        }
        printf("Sending file \"%s\"\n", rpdu.data);

        // Send file data to client
        while (1) {
            ssize_t bytes_read = fread(rpdu.data, 1, MAX_DATA_LEN, fp);

            if (bytes_read > 0) {
                if (bytes_read < MAX_DATA_LEN) {
                    rpdu.type = 'F'; // Final PDU
                } else {
                    rpdu.type = 'D'; // Data PDU
                }

                sendto(server_socket, &rpdu, bytes_read + 1, 0, (struct sockaddr*)&client_addr, client_addr_len);
                printf("Sending %zu bytes\n", bytes_read);
            }

            if (bytes_read < MAX_DATA_LEN) {
                break;
            }
        }

        fclose(fp);
    }

    close(server_socket);
    return 0;
}
