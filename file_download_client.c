#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_TCP_PORT 3000
#define BUF_SIZE 100

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, n, port;
    struct hostent *hp;
    struct sockaddr_in server;
    char buffer[BUF_SIZE];
    char *host, *filename;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s host [port] filename\n", argv[0]);
        exit(1);
    }else{
        host = argv[1];
        port = atoi(argv[2]);
        filename = argv[3];
    }

    /* Create a stream socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        exit(1);
    }

    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((hp = gethostbyname(host)) == NULL) {
        fprintf(stderr, "Can't get server's address\n");
        exit(1);
    }
    bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

    /* Connecting to the server */
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1){
        fprintf(stderr, "Can't connect \n");
        exit(1);
    }

    // Send filename to server
    n = write(sockfd, filename, strlen(filename));
    if (n < 0){
        fprintf(stderr, "Failed to write to socket");
    }

    memset(buffer, 0, BUF_SIZE);
    n = read(sockfd, buffer, 1); // Read indicator byte
    if (n < 0) {
        fprintf(stderr, "Failed to read from socket");
    } else {
        if (buffer[0] == 'E') {
            // Error message received
            n = read(sockfd, buffer, BUF_SIZE - 1);
            if (n < 0) {
                fprintf(stderr, "Failed to read from socket");
            } else {
                buffer[n] = '\0'; // Null terminate
                printf("Error: %s\n", buffer);
            }
        } else if (buffer[0] == 'F') {
            FILE *file = fopen(filename, "wb");
            if (file == NULL){
                fprintf(stderr, "Cant opening file to write");
            }
            // File data received
            while ((n = read(sockfd, buffer, BUF_SIZE)) > 0) {
                fwrite(buffer, 1, n, file);
            }
            fclose(file);
            printf("File \"%s\" received successfully\n", filename);
        } else {
            fprintf(stderr, "Invalid response from server\n");
        }
    }

    close(sockfd);
    return 0;
}
