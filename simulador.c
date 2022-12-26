// C program rocket simulation
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define SHMSZ 4
#define NUM 5
#define SHM_ADDR 233
#define ALTURA_SEGURA 5

int *param[NUM], *distancia, *nivel, *giro1, *giro2, *alarma;
int intervalo, shmid[NUM];
pthread_t tid;

pthread_t tid_p0;
pthread_t tid_p1;
pthread_t tid_p2;
pthread_t tid_p3;
pthread_t tid_p4;

int inicializar_memoria_compartida(void);
void sig_handlerINT(int signo);
void *descender(void *param);
void *propulsor1(void *param);
void *propulsor2(void *param);
void *propulsor3(void *param);
void *propulsor4(void *param);

int main(int argc, char *argv[])
{
  pthread_attr_t attr;

  if (signal(SIGINT, sig_handlerINT) == SIG_ERR)
    printf("\ncan't catch SIGINT\n");

  if (argc != 4)
  {
    printf("enter 1 arguments only eg.\" distancia porcentaje_combustible intervalo_descenso !\"");
    return 0;
  }

  if (inicializar_memoria_compartida() == -1)
    exit(-1);
  *distancia = atoi(argv[1]);
  *nivel = atoi(argv[2]);
  intervalo = atoi(argv[3]);

  *giro1 = 0;
  *giro2 = 0;
  *alarma = 0;

  pthread_attr_init(&attr);
  pthread_create(&tid, &attr, descender, NULL);
  pthread_create(&tid_p1, NULL, propulsor1, NULL);
  pthread_create(&tid_p2, NULL, propulsor2, NULL);
  pthread_create(&tid_p3, NULL, propulsor3, NULL);
  pthread_create(&tid_p4, NULL, propulsor4, NULL);

  printf("Valor actual giro1 giro2 alarma= %d %d %d \n", *giro1, *giro2, *alarma);
  printf("Ingrese giro1 giro2 alarma= %d %d %d \n", *giro1, *giro2, *alarma);
  scanf("%d %d %d", giro1, giro2, alarma);

  while (1)
  {
    if(*alarma == 100){
      printf("El cohete llego al suelo!\n");
      exit(0);
    }
    else if (*alarma == 104){
      printf("Abortando alunisaje!\n");
      exit(0);
    }
    printf("Valor actual distancia %d, combustible %d, giro1 %d, giro2 %d\n", *distancia, *nivel, *giro1, *giro2);
    sleep(1);
  }
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
  giro1 = param[2];
  giro2 = param[3];
  alarma = param[4];

  return (1);
}

void sig_handlerINT(int signo)
{
  int i;
  if (signo == SIGINT)
  {
    printf("abortar alunizaje");
    *alarma = 104;
    sleep(1);
    for (i = 0; i < NUM; i++)
      close(shmid[i]);
  }
  pthread_cancel(tid);
  exit(1);
  return;
}

void *descender(void *param)
{
  while (1)
  {
    sleep(intervalo);
    *distancia = *distancia - 1; // desciende 1 metro por intervalo
    *nivel = *nivel - 1;

    if (*distancia < 0 && *giro1 == 0 && *giro2 == 0)
    {
      *alarma = 100; // cohete llegó al suelo
      pthread_exit(0);
    }

    if (*distancia < ALTURA_SEGURA && (*giro1 != 0 || *giro2 != 0))
    {
      *alarma = 104; // abortar alunizaje
      pthread_exit(0);
    }
  }
}

void *propulsor1(void *param)
{

  while (1)
  {
    sleep(intervalo);
    if (*distancia < ALTURA_SEGURA)
    {
      pthread_exit(0);
    }
    if (*nivel <= 0)
    {
      *alarma = 104;
      pthread_exit(0);
    }
    if(*giro1 > 0){
      *giro1 = *giro1 - 1;
      *nivel = *nivel - 1;
    }
  }
}

void *propulsor3(void *param)
{

  while (1)
  {
    sleep(intervalo);
    if (*distancia < ALTURA_SEGURA)
    {
      pthread_exit(0);
    }
    if (*nivel <= 0)
    {
      *alarma = 104;
      pthread_exit(0);
    }
    if(giro1 < 0){
      *giro1 = *giro1 + 1;
      *nivel = *nivel - 1;
    }
  }
}

void *propulsor2(void *param)
{

  while (1)
  {
    sleep(intervalo);
    if (*distancia < ALTURA_SEGURA)
    {
      pthread_exit(0);
    }
    if (*nivel <= 0)
    {
      *alarma = 104;
      pthread_exit(0);
    }
    if(giro2 > 0){
      *giro2 = *giro2 - 1;
      *nivel = *nivel - 1;
    }
  }
}

void *propulsor4(void *param)
{

  while (1)
  {
    sleep(intervalo);
    if (*distancia < ALTURA_SEGURA)
    {
      pthread_exit(0);
    }
    if (*nivel <= 0)
    {
      *alarma = 104;
      pthread_exit(0);
    }
    if(giro2 < 0){
      *giro2 = *giro2 + 1;
      *nivel = *nivel - 1;
    }
  }
}