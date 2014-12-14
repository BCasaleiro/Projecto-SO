#include "include.h"
#include <sys/msg.h>

typedef struct{
    int request_type;
    char file[LENGTH_SCRIPT_NAME];
    int thread_number;
    char request_arrival[LENGTH_TIME];
    char request_handled[LENGTH_TIME];
}stat_struct;

int mqid, total_din, total_stat, total_refused;
char sv_start[LENGTH_TIME];

void get_message();
void writeHourPresent(char time_string[LENGTH_TIME]);

void sighup_handler();
