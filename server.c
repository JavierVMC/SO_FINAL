#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 255
#define PORT_NO 9999

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, n;
    char buffer[BUFFER_SIZE];

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Error opening socket.");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_NO);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Binding failed.");
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    if (newsockfd < 0)
    {
        error("Error on accept");
    }

    while (1)
    {
        bzero(buffer, BUFFER_SIZE);
        n = read(newsockfd, buffer, BUFFER_SIZE);
        if (n < 0)
        {
            error("Error on reading.");
        }
        printf("Centro de control: %s", buffer);
        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);

        n = write(newsockfd, buffer, strlen(buffer));
        if (n < 0)
        {
            error("Error on writing.");
        }

        int i = strncmp("quit", buffer, 4);
        if (i == 0)
        {
            break;
        }
    }
    close(newsockfd);
    close(sockfd);
    return 0;
}