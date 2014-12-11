#include "include.h"
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>

typedef struct{
    int port;
    int n_threads;
    int policy;
    char scripts[N_SCRIPTS][LENGTH_SCRIPT_NAME];
}config_struct;

int shmid;
sem_t* mutex;

int configuration();
