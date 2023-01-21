#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>

#include "myFunctions.h"

#define BUFFER_SIZE 255

// Alarmas
#define FALLO_GENERAL 101
#define FALLO_MOTOR_PRINCIPAL 102
#define FALLO_MOTOR_ORIENTACION 103
#define ABORTAR_ALUNIZAJE 104

int sockfd, newsockfd, n;
void sig_handlerINT(int signo);

int main(int argc, char *argv[])
{
    if (signal(SIGINT, sig_handlerINT) == SIG_ERR)
        printf("\ncan't catch SIGINT\n");

    if (argc != 2)
    {
        printf("./server <port_no>\n");
        return 0;
    }

    int port_no = atoi(argv[1]);

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
    serv_addr.sin_port = htons(port_no);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Binding failed.");
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    printf("Consola del centro de mando.\n");
    while (1)
    {
        printf("Esperando conexiones...\n");
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("Error on accept");
        }
        printf("Conexion aceptada.\n");
        while (1)
        {
            while (1)
            {
                bzero(buffer, BUFFER_SIZE);
                printf("Comando: ");
                fgets(buffer, BUFFER_SIZE, stdin);
                printf("\n");
                if (FALLO_GENERAL == atoi(buffer) || FALLO_MOTOR_ORIENTACION == atoi(buffer) || FALLO_MOTOR_PRINCIPAL == atoi(buffer) || ABORTAR_ALUNIZAJE == atoi(buffer))
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
            if (ABORTAR_ALUNIZAJE == atoi(buffer))
            {
                printf("Alunizaje abortado.\n");
                break;
            }
        }
        close(newsockfd);
    }
    close(sockfd);

    return 0;
}

void sig_handlerINT(int signo)
{
    int i;
    if (signo == SIGINT)
    {
        printf("Cerrando centro de mando.\n");
    }
    close(newsockfd);
    close(sockfd);
    exit(1);
    return;
}