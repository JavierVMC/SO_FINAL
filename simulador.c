#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "myFunctions.h"

#define SHMSZ 4
#define NUM 3
#define SHM_ADDR 233

#define SHM_ADDR_G 266
#define NUMG 2

#define BUFFER_SIZE 255
#define SERVER_IP_ADDR "127.0.0.1"

// Puntos criticos
#define ALTURA_SEGURA 5       // altura en metros
#define ALTURA_ATERRIZAJE 100 // altura en metros
#define NIVEL_MINIMO 10       // % de combustible
#define TAZA_INCLINACION 0.5  // inclinacion en grados por segundo

// Alarmas
#define ATERRIZAJE_EXITOSO 100
#define FALLO_GENERAL 101
#define FALLO_MOTOR_PRINCIPAL 102
#define FALLO_MOTOR_ORIENTACION 103
#define ABORTAR_ALUNIZAJE 104
#define INTENTAR_DENUEVO 105

pthread_mutex_t mutex;
pthread_cond_t cond0, cond1, cond2, cond3, cond4 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int *param[NUM], *distancia, *nivel, *alarma;
float *giros[NUMG], *giro1, *giro2;
int intervalo, port_no, shmid[NUM], shmid_g[NUMG];
int p0_s, p1_s, p2_s, p3_s, p4_s;
pthread_t tid, tid_p0, tid_p1, tid_p2, tid_p3, tid_p4, tid_cdc;


int inicializar_memoria_compartida(void);
void sig_handlerINT(int signo);
void *descender(void *param);
void *propulsor0(void *param);
void *propulsor1(void *param);
void *propulsor2(void *param);
void *propulsor3(void *param);
void *propulsor4(void *param);
void *centro_de_control(void *param);

int main(int argc, char *argv[])
{
  pthread_attr_t attr;

  if (signal(SIGINT, sig_handlerINT) == SIG_ERR)
    printf("\ncan't catch SIGINT\n");

  if (argc != 7)
  {
    printf("./simulador <Intervalo de simulacion en milisegundos> <angulo giroscipio 1 en grados > <angulo giroscopio 2 en grados > < nivel de combustible numero entre 0 a 100> <Distancia inicial><puerto communicaciones>\n");
    return 0;
  }

  if (inicializar_memoria_compartida() == -1)
    exit(-1);
  
  intervalo = atoi(argv[1]);
  *giro1 = atof(argv[2]);
  *giro2 = atof(argv[3]);
  *nivel = atoi(argv[4]);
  *distancia = atoi(argv[5]);
  port_no = atoi(argv[6]);
  *alarma = 0;

  pthread_mutex_init(&mutex, NULL);
  pthread_attr_init(&attr);
  pthread_create(&tid_cdc, NULL, centro_de_control, NULL);
  pthread_create(&tid, &attr, descender, NULL);
  pthread_create(&tid_p0, NULL, propulsor0, NULL);
  pthread_create(&tid_p1, NULL, propulsor1, NULL);
  pthread_create(&tid_p2, NULL, propulsor2, NULL);
  pthread_create(&tid_p3, NULL, propulsor3, NULL);
  pthread_create(&tid_p4, NULL, propulsor4, NULL);

  while (1)
  {
    pthread_mutex_lock(&mutex);
    if (*alarma == ATERRIZAJE_EXITOSO || *alarma == ABORTAR_ALUNIZAJE)
    {
      break;
    }
    if(*alarma == FALLO_MOTOR_ORIENTACION && p1_s == 0 && p2_s == 0 && p3_s == 0 && p4_s == 0){
      *alarma = 0;
      pthread_cond_signal(&cond1);
      pthread_cond_signal(&cond2);
      pthread_cond_signal(&cond3);
      pthread_cond_signal(&cond4);
    }
    if(*alarma == FALLO_GENERAL && p0_s == 0 && p1_s == 0 && p2_s == 0 && p3_s == 0 && p4_s == 0){
      *alarma = 0;
      pthread_cond_signal(&cond0);
      pthread_cond_signal(&cond1);
      pthread_cond_signal(&cond2);
      pthread_cond_signal(&cond3);
      pthread_cond_signal(&cond4);
    }
    pthread_mutex_unlock(&mutex);
    printf("Distancia %d | Combustible %d | Giro1 %.2f, Giro2 %.2f | P0 %d, P1 %d, P2 %d, P3 %d, P4 %d\n", *distancia, *nivel, *giro1, *giro2, p0_s, p1_s, p2_s, p3_s, p4_s);
    msleep(intervalo);
  }

  // Esperando a que se apagen los propulsores de orientacion
  pthread_join(tid_p1, NULL);
  pthread_join(tid_p2, NULL);
  pthread_join(tid_p3, NULL);
  pthread_join(tid_p4, NULL);
  pthread_join(tid_p0, NULL);

  // Revisando estado final de la mision
  if (*alarma == ATERRIZAJE_EXITOSO)
  {
    printf("El cohete llego al suelo!\n");
  }
  else if (*alarma == ABORTAR_ALUNIZAJE)
  {
    printf("Alunizaje abortado.\n");
  }
  if (*nivel < NIVEL_MINIMO)
  {
    printf("Sin suficiente combustible.\n");
  }
  pthread_mutex_destroy(&mutex);
  return 0;
}

void *descender(void *param)
{
  while (1)
  {
    msleep(intervalo);
    pthread_mutex_lock(&mutex);
    *distancia = *distancia - 1; // desciende 1 metro por intervalo
    pthread_mutex_unlock(&mutex);
    if (*distancia < 0 && *giro1 == 0 && *giro2 == 0)
    {
      pthread_mutex_lock(&mutex);
      *alarma = ATERRIZAJE_EXITOSO; // cohete llegó al suelo a salvo
      pthread_mutex_unlock(&mutex);
      pthread_exit(0);
    }
    if (*distancia < ALTURA_SEGURA && (*giro1 != 0 || *giro2 != 0) && *nivel < NIVEL_MINIMO)
    {
      pthread_mutex_lock(&mutex);
      *alarma = ABORTAR_ALUNIZAJE; // cohete no logro ponerse recto
      pthread_mutex_unlock(&mutex);
      pthread_exit(0);
    }
    if (*nivel < NIVEL_MINIMO)
    {
      pthread_mutex_lock(&mutex);
      *alarma = ABORTAR_ALUNIZAJE; // cohete se quedo sin combustible suficiente
      pthread_mutex_unlock(&mutex);
      pthread_exit(0);
    }
    if (*distancia < ALTURA_SEGURA && *distancia != 0 && (*giro1 != 0 || *giro2 != 0) && *nivel > NIVEL_MINIMO)
    {
      pthread_mutex_lock(&mutex);
      *alarma = INTENTAR_DENUEVO; // el cohete no esta recto aun
      pthread_mutex_unlock(&mutex);
    }
  }
}

// Propulsor principal P0
void *propulsor0(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if(*alarma == FALLO_GENERAL){
      printf("Reiniciando propulsor principal...\n");
      p0_s = 0;
      pthread_mutex_lock(&mutex);
      pthread_cond_wait(&cond0, &mutex);
      pthread_mutex_unlock(&mutex);
      msleep(intervalo * 2);
      printf("Propulsor principal listo.\n");
    }
    if(*alarma == FALLO_MOTOR_PRINCIPAL){
      printf("Reiniciando propulsor principal...\n");
      p0_s = 0;
      msleep(intervalo * 2);
      pthread_mutex_lock(&mutex);
      *alarma = 0;
      pthread_mutex_unlock(&mutex);
      printf("Propulsor principal listo.\n");
    }
    if (*alarma == ABORTAR_ALUNIZAJE)
    {
      p0_s = 1;
      printf("Encenciendo propulsor principal para regresar a la Tierra.\n");
      pthread_exit(0);
    }
    if (*distancia < ALTURA_ATERRIZAJE)
    {
      p0_s = 1;
      pthread_mutex_lock(&mutex);
      *nivel = *nivel - 1;
      if (*alarma == INTENTAR_DENUEVO)
      {
        *distancia = *distancia + 30;
        *nivel = *nivel - 5;
        *alarma = 0;
      }
      pthread_mutex_unlock(&mutex);
      if (*distancia <= 1)
      {
        p0_s = 0;
        printf("Apagando propulsor principal.\n");
        pthread_exit(0);
      }
    }
    else
    {
      p0_s = 0;
    }
  }
}

// Propulsor P1
void *propulsor1(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if(*alarma == FALLO_MOTOR_ORIENTACION || *alarma == FALLO_GENERAL){
      printf("Reiniciando propulsor 1...\n");
      p1_s = 0;
      pthread_mutex_lock(&mutex);
      pthread_cond_wait(&cond1, &mutex);
      pthread_mutex_unlock(&mutex);
      msleep(intervalo * 2);
      printf("Propulsor 1 listo.\n");
    }
    if (*distancia <= 1 || *nivel < NIVEL_MINIMO || *alarma == ABORTAR_ALUNIZAJE)
    {
      p1_s = 0;
      printf("Apagando propulsor 1\n");
      pthread_exit(0);
    }
    if (*giro1 > 0.0)
    {
      p1_s = 1;
      pthread_mutex_lock(&mutex);
      *giro1 = *giro1 - TAZA_INCLINACION;
      *nivel = *nivel - 1;
      pthread_mutex_unlock(&mutex);
    }
    else
    {
      p1_s = 0;
    }
  }
}

// Propulsor P3
void *propulsor3(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if(*alarma == FALLO_MOTOR_ORIENTACION || *alarma == FALLO_GENERAL){
      printf("Reiniciando propulsor 3...\n");
      p3_s = 0;
      pthread_mutex_lock(&mutex);
      pthread_cond_wait(&cond3, &mutex);
      pthread_mutex_unlock(&mutex);
      msleep(intervalo * 2);
      printf("Propulsor 3 listo.\n");
    }
    if (*distancia <= 1 || *nivel < NIVEL_MINIMO || *alarma == ABORTAR_ALUNIZAJE)
    {
      p3_s = 0;
      printf("Apagando propulsor 3\n");
      pthread_exit(0);
    }
    if (*giro1 < 0.0)
    {
      p3_s = 1;
      pthread_mutex_lock(&mutex);
      *giro1 = *giro1 + TAZA_INCLINACION;
      *nivel = *nivel - 1;
      pthread_mutex_unlock(&mutex);
    }
    else
    {
      p3_s = 0;
    }
  }
}

// Propulsor P2
void *propulsor2(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if(*alarma == FALLO_MOTOR_ORIENTACION || *alarma == FALLO_GENERAL){
      printf("Reiniciando propulsor 2...\n");
      p2_s = 0;
      pthread_mutex_lock(&mutex);
      pthread_cond_wait(&cond2, &mutex);
      pthread_mutex_unlock(&mutex);
      msleep(intervalo * 2);
      printf("Propulsor 2 listo.\n");
    }
    if (*distancia <= 1 || *nivel < NIVEL_MINIMO || *alarma == ABORTAR_ALUNIZAJE)
    {
      p2_s = 0;
      printf("Apagando propulsor 2\n");
      pthread_exit(0);
    }
    if (*giro2 > 0.0)
    {
      p2_s = 1;
      pthread_mutex_lock(&mutex);
      *giro2 = *giro2 - TAZA_INCLINACION;
      *nivel = *nivel - 1;
      pthread_mutex_unlock(&mutex);
    }
    else
    {
      p2_s = 0;
    }
  }
}

// Propulsor P4
void *propulsor4(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if(*alarma == FALLO_MOTOR_ORIENTACION || *alarma == FALLO_GENERAL){
      printf("Reiniciando propulsor 4...\n");
      p4_s = 0;
      pthread_mutex_lock(&mutex);
      pthread_cond_wait(&cond4, &mutex);
      pthread_mutex_unlock(&mutex);
      msleep(intervalo * 2);
      printf("Propulsor 4 listo.\n");
    }
    if (*distancia <= 1 || *nivel < NIVEL_MINIMO || *alarma == ABORTAR_ALUNIZAJE)
    {
      p4_s = 0;
      printf("Apagando propulsor 4\n");
      pthread_exit(0);
    }
    if (*giro2 < 0.0)
    {
      p4_s = 1;
      pthread_mutex_lock(&mutex);
      *giro2 = *giro2 + TAZA_INCLINACION;
      *nivel = *nivel - 1;
      pthread_mutex_unlock(&mutex);
    }
    else
    {
      p4_s = 0;
    }
  }
}

void *centro_de_control(void *param)
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
  serv_addr.sin_port = htons(port_no);
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    error("Connection failed.");
  }

  while (1)
  {
    bzero(buffer, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE);
    if (n < 0)
    {
      error("Error on reading.");
      pthread_exit(0);
    }
    if(n == 0){
      break;
    }
    printf("Centro de control: %s", buffer);
    pthread_mutex_lock(&mutex);
    *alarma = atoi(buffer);
    pthread_mutex_unlock(&mutex);
  }

  close(sockfd);
}

int inicializar_memoria_compartida(void)
{
  int i;

  for (i = 0; i < NUM; i++)
  {
    if ((shmid[i] = shmget(SHM_ADDR + i, SHMSZ, IPC_CREAT | 0666)) < 0)
    {
      perror("shmget");
      return (-1);
    }
    if ((param[i] = shmat(shmid[i], NULL, 0)) == (int *)-1)
    {
      perror("shmat");
      return (-1);
    }
  }

  distancia = param[0];
  nivel = param[1];
  alarma = param[2];

  for (i = 0; i < NUMG; i++)
  {
    if ((shmid_g[i] = shmget(SHM_ADDR_G + i, SHMSZ, IPC_CREAT | 0666)) < 0)
    {
      perror("shmget");
      return (-1);
    }
    if ((giros[i] = shmat(shmid_g[i], NULL, 0)) == (float *)-1)
    {
      perror("shmat");
      return (-1);
    }
  }

  giro1 = giros[0];
  giro2 = giros[1];

  return (1);
}

void sig_handlerINT(int signo)
{
  int i;
  if (signo == SIGINT)
  {
    printf("Abortar alunizaje en sig\n");
    *alarma = ABORTAR_ALUNIZAJE;
    sleep(1);
    for (i = 0; i < NUM; i++)
      close(shmid[i]);
    for (i = 0; i < NUMG; i++)
      close(shmid_g[i]);
  }
  pthread_cancel(tid);
  pthread_cancel(tid_cdc);
  exit(1);
  return;
}