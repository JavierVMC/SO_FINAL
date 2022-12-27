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
#include <errno.h>


#define SHMSZ 4
#define NUM 3
#define SHM_ADDR 233

#define SHM_ADDR_G 266
#define NUMG 2

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

int *param[NUM], *distancia, *nivel, *alarma;
float *giros[NUMG], *giro1, *giro2;
int intervalo, shmid[NUM], shmid_g[NUMG], port;
pthread_t tid;

pthread_t tid_p0;
pthread_t tid_p1;
pthread_t tid_p2;
pthread_t tid_p3;
pthread_t tid_p4;

int msleep(long msec);
int inicializar_memoria_compartida(void);
void sig_handlerINT(int signo);
void *descender(void *param);
void *propulsor0(void *param);
void *propulsor1(void *param);
void *propulsor2(void *param);
void *propulsor3(void *param);
void *propulsor4(void *param);

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
  port = atoi(argv[6]);
  *alarma = 0;

  pthread_attr_init(&attr);
  pthread_create(&tid, &attr, descender, NULL);
  pthread_create(&tid_p0, NULL, propulsor0, NULL);
  pthread_create(&tid_p1, NULL, propulsor1, NULL);
  pthread_create(&tid_p2, NULL, propulsor2, NULL);
  pthread_create(&tid_p3, NULL, propulsor3, NULL);
  pthread_create(&tid_p4, NULL, propulsor4, NULL);

  while (1)
  {
    if (*alarma == ATERRIZAJE_EXITOSO)
    {
      printf("El cohete llego al suelo!\n");
      exit(0);
    }
    else if (*alarma == ABORTAR_ALUNIZAJE && *nivel < NIVEL_MINIMO)
    {
      printf("Abortando alunisaje!\n");
      exit(0);
    }
    printf("Valor actual distancia %d, combustible %d, giro1 %.2f, giro2 %.2f\n", *distancia, *nivel, *giro1, *giro2);
    msleep(intervalo);
  }
}

void *descender(void *param)
{
  while (1)
  {
    msleep(intervalo);
    *distancia = *distancia - 1; // desciende 1 metro por intervalo
    if (*distancia < 0 && *giro1 == 0 && *giro2 == 0)
    {
      *alarma = ATERRIZAJE_EXITOSO; // cohete llegó al suelo a salvo
      pthread_exit(0);
    }
    if (*distancia < ALTURA_SEGURA && (*giro1 != 0 || *giro2 != 0))
    {
      *alarma = ABORTAR_ALUNIZAJE; // cohete no esta recto aun
    }
    if (*nivel < NIVEL_MINIMO)
    {
      *alarma = ABORTAR_ALUNIZAJE; // muy poco combustible restante
      pthread_exit(0);
    }
  }
}

// Propulsor principal P0
void *propulsor0(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if (*distancia < ALTURA_ATERRIZAJE)
    {
      *nivel = *nivel - 1;
      if (*alarma == ABORTAR_ALUNIZAJE && *nivel > NIVEL_MINIMO)
      {
        *alarma = 0;
        *distancia = *distancia + 30;
        *nivel = *nivel - 5;
      }else if(*alarma == ABORTAR_ALUNIZAJE && *nivel < NIVEL_MINIMO){
        pthread_exit(0);
      }
      if(*distancia == 1){
        printf("Apagando propulsor principal\n");
        pthread_exit(0);
      }
    }
  }
}

// Propulsor P1
void *propulsor1(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if (*distancia == 1 || *nivel < NIVEL_MINIMO)
    {
      printf("Apagando propulsor 1\n");
      pthread_exit(0);
    }
    if (*giro1 > 0)
    {
      *giro1 = *giro1 - TAZA_INCLINACION;
      *nivel = *nivel - 1;
    }
  }
}

// Propulsor P3
void *propulsor3(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if (*distancia == 1 || *nivel < NIVEL_MINIMO)
    {
      printf("Apagando propulsor 3\n");
      pthread_exit(0);
    }
    if (giro1 < 0)
    {
      *giro1 = *giro1 + TAZA_INCLINACION;
      *nivel = *nivel - 1;
    }
  }
}

// Propulsor P2
void *propulsor2(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if (*distancia == 1 || *nivel < NIVEL_MINIMO)
    {
      printf("Apagando propulsor 2\n");
      pthread_exit(0);
    }
    if (*giro2 > 0.0)
    {
      *giro2 = *giro2 - TAZA_INCLINACION;
      *nivel = *nivel - 1;
    }
  }
}

// Propulsor P4
void *propulsor4(void *param)
{
  while (1)
  {
    msleep(intervalo);
    if (*distancia == 1 || *nivel < NIVEL_MINIMO)
    {
      printf("Apagando propulsor 4\n");
      pthread_exit(0);
    }
    if (*giro2 < 0.0)
    {
      *giro2 = *giro2 + TAZA_INCLINACION;
      *nivel = *nivel - 1;
    }
  }
}

/* Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do
    {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
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
    printf("abortar alunizaje\n");
    *alarma = ABORTAR_ALUNIZAJE;
    sleep(1);
    for (i = 0; i < NUM; i++)
      close(shmid[i]);
  }
  pthread_cancel(tid);
  exit(1);
  return;
}