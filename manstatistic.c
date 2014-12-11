#include "manstatistic.h"

void get_message(){
    FILE *fp;
    stat_struct info;
    char line[101];

    total_din = 0;
    total_stat = 0;
    total_refused = 0;
    fp = fopen(STATISTICS_FILE_NAME, "a");
    fprintf(fp, "ola\n");

    while(1){
        //Waits to receive a message (FIFO)
        msgrcv(mqid, &info, sizeof(stat_struct), 0, 0);

        if(info .request_type == 0){
            total_din++;
        }
        else if(info .request_type == 1){
            total_stat++;
        }
        else if(info .request_type == 2){
            total_refused++;
        }

        snprintf(line, 100 * sizeof(char),"[%d] File='%s' Thread=%d Time='%s' '%s'\n", info.request_type, info.file, info.thread_number, info.request_arrival, info.request_handled);
        printf("%s", line);
        fprintf(fp, "%s", line);
        sleep(3);
    }

    fclose(fp);
}

void writeHourPresent(char* time_string)
{
    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);
    int hours = aTime->tm_hour ;
    int minutes = aTime->tm_min;
    int seconds = aTime->tm_sec;

    snprintf(time_string, LENGTH_TIME, "%d:%d:%d", hours, minutes, seconds);
}
