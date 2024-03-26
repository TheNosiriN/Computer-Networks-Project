#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_FILENAME_LEN 100
#define MAX_DATA_LEN 100

struct pdu {
    char type;
    char data[MAX_DATA_LEN];
};

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    struct hostent *phe;
    int client_socket, port;
    char *host = "localhost";

    switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: %s [host [port]]\n", argv[0]);
		exit(1);
	}

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    // inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    /* Map host name to IP address, allowing for dotted decimal */
    if ( phe = gethostbyname(host) ){
        memcpy(&server_addr.sin_addr, phe->h_addr, phe->h_length);
    }else if ( (server_addr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE ){
        fprintf(stderr, "Can't get host entry \n");
    }

    /* Allocate a socket */
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0){
        fprintf(stderr, "Can't create socket \n");
    }

    /* Connect the socket */
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        fprintf(stderr, "Can't connect to socket\n");
    }


    while (1) {
        struct pdu spdu;
        char filename[MAX_FILENAME_LEN];
        memset(&spdu, 0, sizeof(spdu));
        memset(&filename, 0, sizeof(filename));

        printf("\nEnter filename to download (or type 'quit' to exit): ");
        scanf("%s", filename);
        if (strcmp(filename, "quit") == 0) break;

        // Send filename to server
        spdu.type = 'C';
        strcpy(spdu.data, filename);
        sendto(client_socket, &spdu, strlen(filename)+1, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        FILE *fp = NULL;
        while (1) {
            ssize_t bytes_received = recvfrom(client_socket, &spdu, sizeof(spdu), 0, (struct sockaddr*)&server_addr, &server_addr_len);
            printf("bytes received: %zd\n", bytes_received);
            if (bytes_received < 0) {
                fprintf(stderr, "Error receiving data from server\n");
                break;
            }

            if (spdu.type == 'E') {
                fprintf(stderr, "Server error: %s\n", spdu.data);
                break;
            }

            if (fp == NULL){
                fp = fopen(filename, "wb");
                if (fp == NULL) {
                    fprintf(stderr, "Error creating file\n");
                    continue;
                }
            }

            if (spdu.type == 'F') {
                fwrite(spdu.data, 1, bytes_received - 1, fp);
                printf("File '%s' downloaded successfully\n", filename);
                break;
            }

            if (spdu.type == 'D') {
                fwrite(spdu.data, 1, bytes_received - 1, fp);
            }
        }

        if (fp){
            fclose(fp);
        }
    }

    close(client_socket);
    return 0;
}
