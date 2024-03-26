#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVER_TCP_PORT 3000
#define BUF_SIZE 100

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, n, port;
    socklen_t clilen;
    char buffer[BUF_SIZE];
    struct sockaddr_in serv_addr, cli_addr;

    switch(argc){
    case 1:
        port = SERVER_TCP_PORT;
        break;
    case 2:
        port = atoi(argv[1]);
        break;
    default:
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        exit(1);
    }

    /* Create a stream socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't create a socket\n");
        exit(1);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        fprintf(stderr, "Failed to bind");
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0){
            fprintf(stderr, "Can't accept connection");
        }

        memset(buffer, 0, BUF_SIZE);
        n = read(newsockfd, buffer, BUF_SIZE - 1);
        if (n < 0){
            fprintf(stderr, "Can't read from socket");
        }

        FILE *file = fopen(buffer, "rb");
        if (file == NULL) {
            char error_msg[] = "File not found or could not be opened.";
            buffer[0] = 'E'; // Error message indicator
            memcpy(buffer+1, error_msg, strlen(error_msg) + 1);
            write(newsockfd, buffer, strlen(error_msg) + 2); // Include null terminator and indicator
        } else {
            buffer[0] = 'F'; // File data indicator
            write(newsockfd, buffer, 1); // Send indicator
            while ((n = fread(buffer, 1, BUF_SIZE, file)) > 0) {
                if (write(newsockfd, buffer, n) < 0)
                    fprintf(stderr, "Failed to write to socket");
            }
            fclose(file);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
