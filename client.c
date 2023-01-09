#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 255
#define PORT_NO 9999
#define SERVER_IP_ADDR "127.0.0.1"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        error("Error opening socket.");
    }

    server = gethostbyname(SERVER_IP_ADDR);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host.");
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(PORT_NO);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Connection failed.");
    }
    while (1)
    {
        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
        {
            error("Error on writting.");
        }
        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE);
        if (n < 0)
        {
            error("Error on reading.");
        }
        printf("Cohete: %s", buffer);

        int i = strncmp("quit", buffer, 4);
        if (i == 0)
        {
            break;
        }
    }
    close(sockfd);
    return 0;
}