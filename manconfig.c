#include "manconfig.h"

int init_config()
{
    FILE* fp;

    #ifdef DEBUG
        printf("Checking if config file exists\n");
    #endif

    if((fp = fopen(CONFIG_FILE_NAME, "r")) == NULL) {
        perror("Config file missing\n");
        return 1;
    } else {
        fclose(fp);
    }

    #ifdef DEBUG
        printf("Creating shared memory\n");
    #endif

    if( (shmid = shmget(IPC_PRIVATE, sizeof(config_struct), IPC_CREAT|0700)) < 0) {
        perror("While creating shared memory\n");
        return 1;
    }
    config = shmat(shmid, NULL, 0);

    #ifdef DEBUG
        printf("Creating semaphore\n");
    #endif

    sem_unlink("MUTEX");
    mutex = sem_open("MUTEX", O_CREAT|O_EXCL, 0700, 1);

    return 0;
}
