#include "manstatistic.h"

void get_message(){
    FILE *fp;
    stat_struct info;
    char line[101];

    signal(SIGHUP, sighup_handler);

    total_din = 0;
    total_stat = 0;
    total_refused = 0;
    fp = fopen(STATISTICS_FILE_NAME, "a");

    while(1){
        //Waits to receive a message (FIFO)
        //sleep(3);
        if(msgrcv(mqid, &info, sizeof(stat_struct), 0, 0) != 0) {
            perror("Failed to receive Message");
        }

        #ifdef DEBUG
            printf("Message received: %s handled at %s\n", info.file, info.request_handled);
        #endif

        if(info.request_type == 0){
            total_din++;
        }
        else if(info.request_type == 1){
            total_stat++;
        }
        else if(info.request_type == 2){
            total_refused++;
        }

        snprintf(line, 1000 * sizeof(char),"[%d] File: '%s' Thread: %d Time: '%s' '%s'\n", info.request_type, info.file, info.thread_number, info.request_arrival, info.request_handled);
        printf("%s", line);
        //fprintf(fp, "%s", line);
        //sleep(3);
    }

    fclose(fp);
}

void writeHourPresent(char* time_string)
{
    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);
    char c_time_string[LENGTH_TIME];
    int hours=aTime->tm_hour ;
    int minutes=aTime->tm_min;
    int seconds = aTime->tm_sec;
    //HORA////////

    /* Print to stdout. */
    sprintf(c_time_string, "%02d:%02d:%02d", hours, minutes, seconds);
    strcpy(time_string, c_time_string);
}

void sighup_handler(){
    char current_time[LENGTH_TIME];

    writeHourPresent(current_time);

    printf("Boot time '%s'\tCurrent time '%s'\n", sv_start, current_time);
    printf("Total static requests: %d\n", total_stat);
    printf("Total dynamic requests: %d\n", total_din);
    printf("Total refused requests: %d\n", total_refused);
}
