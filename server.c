#include "server.h"

int main (int argc, char const *argv[])
{
    int i;
    config_struct* config;

    if(init() == 1){
        perror("Init\n");
        return 1;
    }

    config = (config_struct*)shmat(shmid, NULL, 0);

    for(i = 0; i < N_PROC; i++) {
        if(fork() == 0) {
            id_proc[i] = getpid();
            printf("Child: %d\tFather: %d\n", getpid(), getppid());
            switch(i){
                case 0:
                    //Config Manager
                    configuration();
                    break;
                case 1:{
                    //Stat Manager

                    break;
                }
                case 2:
                    //Main
                    break;
                default:
                    perror("While creating process\n");
            }
            exit(0);
        }
    }

    for(i = 0; i < N_PROC; i++){
        wait(NULL);
    }

    sem_wait(mutex);

    printf("Shared memory info\n");
    printf("p: %d\nn: %d\npl:%d\nns:%d\n", config->port, config->n_threads, config->policy, N_SCRIPTS);



    for(i = 0; i < N_SCRIPTS; i++){
        printf("script: %s", config->scripts[i]);
    }
    
    sem_post(mutex);

    //destroy shared memory
    shmdt(config);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}

int init()
{
    config_struct* config;

    //Shared memory
    #ifdef DEBUG
        printf("Creating shared memory\n");
    #endif

    if( (shmid = shmget(IPC_PRIVATE, sizeof(config_struct*), IPC_CREAT|0700)) < 0) {
        perror("While creating shared memory\n");
        return 1;
    }
    config = (config_struct*)shmat(shmid, NULL, 0);

    //Semaphore
    #ifdef DEBUG
        printf("Creating semaphore\n");
    #endif

    sem_unlink("MUTEX");
    mutex = sem_open("MUTEX", O_CREAT|O_EXCL, 0700, 1);

    return 0;
}
