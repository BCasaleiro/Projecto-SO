#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>


#define CONFIG_FILE_NAME "config.txt"
#define LENGTH_SCRIPT_NAME 31
#define N_SCRIPTS 3

#define DEBUG

typedef struct{
    int port;
    int n_threads;
    int policy;
    char scripts[N_SCRIPTS][LENGTH_SCRIPT_NAME];
}config_struct;

int shmid;
sem_t* mutex;

int configuration();
