#include "manconfig.h"

int configuration()
{
    int i;
    FILE* fp;
    char line[LENGTH_SCRIPT_NAME];
    config_struct* config;

    config = (config_struct*) shmat(shmid, NULL, 0);

    #ifdef DEBUG
        printf("Checking if config file exists\n");
    #endif

    if( (fp = fopen(CONFIG_FILE_NAME, "r")) == NULL){
        perror("Config file missing\n");
        return 1;
    }

    sem_wait(mutex);

    //Port
    fgets(line, LENGTH_SCRIPT_NAME-1, fp);
    config->port = atoi(line);

    //Number of threads
    fgets(line, LENGTH_SCRIPT_NAME-1, fp);
    config->n_threads = atoi(line);

    //Scheduller Policy
    fgets(line, LENGTH_SCRIPT_NAME-1, fp);
    config->policy = atoi(line);

    //Scripts
    for(i = 0; i < N_SCRIPTS; i++){
        fgets(config->scripts[i], LENGTH_SCRIPT_NAME-1, fp);
    }

    sem_post(mutex);

    return 0;
}
