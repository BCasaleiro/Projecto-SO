#include "server.h"

int main (int argc, char const *argv[])
{
    int i;
    config_struct* config;
    stat_struct info;

    if(init() == 1){
        perror("In Init\n");
        return 1;
    }

    config = (config_struct*)shmat(shmid, NULL, 0);

    for(i = 0; i < N_PROC; i++) {
        if(fork() == 0) {
            id_proc[i] = getpid();
            //printf("Child: %d\tFather: %d\n", getpid(), getppid());
            switch(i){
                case 0:
                    //Config Manager
                    configuration();
                    break;
                case 1:{
                    //Stat Manager
                    get_message();
                    break;
                }
                case 2:
                    //Main
                    strcpy(info.file, "o godinho e gay");
                    strcpy(info.request_arrival, "agora");
                    strcpy(info.request_handled, "daqui a pouco");
                    info.thread_number = 5;
                    info.request_type = 0;

                    msgsnd(mqid, &info, sizeof(info), 0);
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

    writeHourPresent(sv_start);

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

    #ifdef DEBUG
        printf("Creating message queue\n");
    #endif

    if ((mqid = msgget(IPC_PRIVATE, IPC_CREAT|0777)) < 0){
        perror("While creating the message queue.");
        exit(0);
    }

    return 0;
}
