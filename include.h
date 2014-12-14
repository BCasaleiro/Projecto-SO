#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define STATISTICS_FILE_NAME "statistics.txt"
#define CONFIG_FILE_NAME "config.txt"

#define LENGTH_SCRIPT_NAME 31
#define LENGTH_DESCRIPTOR 5
#define LENGTH_TIME 9
#define N_SCRIPTS 3
#define N_PROC 3

#define DEBUG
