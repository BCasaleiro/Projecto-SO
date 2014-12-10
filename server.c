#include "server.h"

int main (int argc, char const *argv[])
{

    if(init()){

    }

    return 0;
}

int init()
{
    int i;

    for(i = 0; i < N_PROC; i++) {
        if( (id_proc[i] = fork()) == 0) {
            switch(i){
                case 0:
                    //Config Manager

                    break;
                case 1:
                    //Stat Manager
                    break;
                case 2:
                    //Main

                    break;
                default:
                    perror("While creating process\n");
                    return 1;
            }
        }
    }

    return 0;
}
