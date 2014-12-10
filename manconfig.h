#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>


#define CONFIG_FILE_NAME "config.txt"
#define LENGTH_SCRIPT_NAME 31

#define DEBUG

typedef struct{
    int port;
    int n_threads;
    int policy;
    char* scripts[LENGTH_SCRIPT_NAME];
}config_struct;

int shmid;
config_struct* config;
sem_t* mutex;

int init_config();
