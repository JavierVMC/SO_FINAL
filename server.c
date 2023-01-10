#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 255
#define PORT_NO 9999

// Alarmas
#define FALLO_GENERAL 101
#define FALLO_MOTOR_PRINCIPAL 102
#define FALLO_MOTOR_ORIENTACION 103
#define ABORTAR_ALUNIZAJE 104

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
        while (1)
        {
            bzero(buffer, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            if (FALLO_GENERAL == atoi(buffer)
             || FALLO_MOTOR_ORIENTACION == atoi(buffer)
             || FALLO_MOTOR_PRINCIPAL == atoi(buffer)
             || ABORTAR_ALUNIZAJE == atoi(buffer))
            {
                break;
            }
            if (strncmp("quit", buffer, 4) == 0)
            {
                break;
            }
            printf("Comando no reconocido.\n");
        }

        n = write(newsockfd, buffer, strlen(buffer));
        if (n < 0)
        {
            error("Error on writing.");
        }
    }
    close(newsockfd);
    close(sockfd);
    return 0;
}