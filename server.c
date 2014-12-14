#include "server.h"

int main (int argc, char const *argv[])
{
    int i;
    config_struct* config;

    if(init() == 1){
        perror("In Init\n");
        return 1;
    }

    //signal(SIGINT, sigint_handler);
    //signal(SIGTSTP, catch_ctrlz);
    //signal(SIGHUP, sighup_handler);

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
                    main_process();
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

void main_process()
{
    int n_threads;
    config_struct* config;
    pthread_t scheduller_thread;
    int id_scheduller;

    config = shmat(shmid, NULL, 0);

    sem_wait(mutex);
    n_threads = config->n_threads;
    sem_post(mutex);

    create_buffer(n_threads);

    sem_unlink("BUFFER_MUTEX");
    buffer_mutex = sem_open("BUFFER_MUTEX", O_CREAT|O_EXCL, 0700, 1);

    sem_unlink("EMPTY");
    empty = sem_open("EMPTY", O_CREAT|O_EXCL, 0700, n_threads);

    sem_unlink("FULL");
    full = sem_open("FULL", O_CREAT|O_EXCL, 0700, 0);

    pthread_create(&scheduller_thread, NULL, scheduller, &id_scheduller);

    create_thread_pool(n_threads);

    pthread_join(scheduller_thread, NULL);
}

void create_buffer(int n_threads)
{
    if((buffer = malloc(2 * n_threads * sizeof(buffer_struct))) == NULL){
        perror("While allocating space for buffer\n");
    }
}

void create_thread_pool(int n_threads)
{
    int i;

    #ifdef DEBUG
        printf("Creating thread pool\n");
    #endif

    if((threads = malloc(n_threads * sizeof(pthread_t))) == NULL){
        perror("While allocating space for thread pool\n");
        return;
    }

    if((id_threads = malloc(n_threads * sizeof(int))) == NULL){
        perror("While allocating space for thread pool id\n");
        return;
    }

    for(i = 0; i < n_threads; i++){
        id_threads[i] = i;
        pthread_create(&threads[i], NULL, worker, &id_threads[i]);
    }

    for(i = 0; i < n_threads; i++){
        pthread_join(threads[i], NULL);
    }
}

void* worker(void* idp)
{
    pthread_t thread_id = *((pthread_t*) idp);
    int request_type, socket, n_threads, thread_number, i;
    char file[LENGTH_SCRIPT_NAME];
    char request_arrival[LENGTH_TIME];
    char request_handled[LENGTH_TIME];
    config_struct* config;
    stat_struct msg;

    writeHourPresent(request_arrival);

    config = shmat(shmid, NULL, 0);

    sem_wait(mutex);
    n_threads = config->n_threads;
    sem_post(mutex);

    for(i = 0; i < n_threads; i++){
        if(threads[i] == thread_id){
            thread_number = i + 1;
        }
    }

    while(1){
        sem_wait(full);
        sem_wait(buffer_mutex);

        #ifdef DEBUG_THREADS
            printf("\nTREAD WORKING\nThread pulling out of buffer\n");
        #endif
        printf("Request: %d\tFile: %s\tSocket: %d\n", buffer[0].request_type, buffer[0].file, buffer[0].socket);
        request_type = buffer[0].request_type;
        strcpy(file, buffer[0].file);
        socket = buffer[0].socket;

        #ifdef DEBUG_THREADS
            printf("Erasing pulled buffer request\n");
        #endif

        delete_element();

        sem_post(buffer_mutex);
        sem_post(empty);

        #ifdef DEBUG
            printf("Thread nº%d starting to work\tRequest type: %d\tFile: %s\tSocket: %d\n", thread_number, request_type, file, socket);
        #endif

        if(request_type == 0){
            send_page(new_conn);
        } else if(request_type == 1){
            execute_script(new_conn);
        } else {

        }

        /*writeHourPresent(request_handled);

        msg.request_type = request_type;
        msg.thread_number = thread_number;
        strcpy(msg.file, file);
        strcpy(msg.request_arrival, request_arrival);
        strcpy(msg.request_handled, request_handled);

        #ifdef DEBUG_THREADS
            printf("Sending mensage\n");
        #endif

        if(msgsnd(mqid, &msg, sizeof(msg), 0) != 0){
            perror("Failed to send message\n");
            return NULL;
        } else {
            #ifdef DEBUG
                printf("Message send successfully\n");
            #endif
        }*/

        // Terminate connection with client
        close(new_conn);
    }
    return NULL;
}

void* scheduller(void* idp)
{
    int socket_conn, i, policy, n_threads;
    char allowed_scripts[N_SCRIPTS][LENGTH_SCRIPT_NAME];
    config_struct* config;
    struct sockaddr_in client_name;
    socklen_t client_name_len = sizeof(client_name);

    config = shmat(shmid, NULL, 0);
    buffer_write = 0;

    sem_wait(mutex);
    port = config->port;
    policy = config->policy;
    n_threads = config->n_threads;
    for(i = 0; i < N_SCRIPTS; i++){
        strcpy(allowed_scripts[i], config->scripts[i]);
    }
    sem_post(mutex);

    buffer_size = n_threads * 2;

    if ( (socket_conn = fireup(port)) == -1) {
        exit(1);
    }

    // Serve requests
    while (1)
    {
        // Accept connection on socket
        if ( (new_conn = accept(socket_conn,(struct sockaddr *)&client_name,&client_name_len)) == -1 ) {
            printf("Error accepting connection\n");
            exit(1);
        }

        #ifdef DEBUG
            printf("\nIndentify new client\n");
        #endif

        // Identify new client
        identify(new_conn);

        // Process request
        get_request(new_conn);

        #ifdef DEBUG
            printf("Requested: %s\tBuffer Write Pos: %d\t Buffer Size: %d\n", req_buf, buffer_write, buffer_size);
        #endif

        if(buffer_write < buffer_size){

            sem_wait(empty);
            sem_wait(buffer_mutex);

            validate(allowed_scripts);

            #ifdef DEBUG
                printf("Validated request type: %d\n", buffer[buffer_write].request_type);
            #endif

            strcpy(buffer[buffer_write].file, req_buf);
            buffer[buffer_write].socket = new_conn;

            buffer_write++;

            printf("Request: %d\tFile: %s\tSocket: %d\n", buffer[0].request_type, buffer[0].file, buffer[0].socket);

            //sort_buffer(policy);

            sem_post(buffer_mutex);
            sem_post(full);

            #ifdef DEBUG
                printf("Done! Threads work now...\n");
            #endif
        } else {
            //Buffer cheio
            send(new_conn, "Buffer cheio\n", strlen("Buffer cheio\n") + 1, 0);
        }
    }

    return NULL;
}

void sort_buffer(int policy)
{
    //0-FIFO
    //1-STAT
    //2-DIN
    int i;
    int k = 0;
    buffer_struct* aux;

    if(buffer_write > 1){
        return;
    }

    if((aux = malloc(buffer_size * sizeof(buffer_struct)) ) == NULL){
        perror("While creating auxiliar buffer for sorting\n");
        return;
    }

    //FIFO não é necessário ordenar
    if(policy == 1){
        for(i = 0; i < buffer_write; i++){
            if(buffer[i].request_type == 0){
                aux[k].request_type = buffer[i].request_type;
                strcpy(aux[k].file, buffer[i].file);
                aux[k].socket = buffer[i].socket;
                k++;
            }
        }
        for(i = 0; i < buffer_write; i++){
            if(buffer[i].request_type == 1){
                aux[k].request_type = buffer[i].request_type;
                strcpy(aux[k].file, buffer[i].file);
                aux[k].socket = buffer[i].socket;
                k++;
            }
        }
    } else if(policy == 2){
        for(i = 0; i < buffer_write; i++){
            if(buffer[i].request_type == 1){
                aux[k].request_type = buffer[i].request_type;
                strcpy(aux[k].file, buffer[i].file);
                aux[k].socket = buffer[i].socket;
                k++;
            }
        }
        for(i = 0; i < buffer_write; i++){
            if(buffer[i].request_type == 0){
                aux[k].request_type = buffer[i].request_type;
                strcpy(aux[k].file, buffer[i].file);
                aux[k].socket = buffer[i].socket;
                k++;
            }
        }
    }

    for(i = 0; i < buffer_write; i++){
        buffer[i].request_type = aux[i].request_type;
        strcpy(buffer[i].file, aux[i].file);
        buffer[i].socket = aux[i].socket;
    }

    free(aux);
}


void validate(char allowed_scripts[N_SCRIPTS][LENGTH_SCRIPT_NAME])
{
    int i, type;
    FILE* fp;


    #ifdef DEBUG_VALIDATE
        printf("Enter validate: ");
    #endif
    type = get_type();
    printf("type: %d\n", type);
    if(type == 0){
        sprintf(buf_tmp,"htdocs/%s",req_buf);
        if((fp = fopen(buf_tmp, "r")) != NULL){
            fclose(fp);
            buffer[buffer_write].request_type = 0;
            return;
        }
        buffer[buffer_write].request_type = 2;
    } else if(type == 1){
        sprintf(buf_tmp,"scripts/%s",req_buf);
        for(i = 0; i < N_SCRIPTS; i++){

            if(strcmp(req_buf, allowed_scripts[i]) == 0){
                if((fp = fopen(buf_tmp, "r")) != NULL){
                    fclose(fp);
                    buffer[buffer_write].request_type = 1;
                    return;
                }
            }
        }
        buffer[buffer_write].request_type = 2;
    } else {
        buffer[buffer_write].request_type = 2;
    }
}

int get_type()
{
    char file_temp[LENGTH_SCRIPT_NAME];
    char* token;
    strcpy(file_temp, req_buf);

    token = strtok(file_temp, ".");
    token = strtok(NULL, " ");
    printf("'%s'\t", token);

    if(strcmp(token, "html") == 0){
        #ifdef DEBUG_VALIDATE
            printf("Static request, ");
        #endif
        return 0;
    } else if(strcmp(token, "sh") == 0){
        #ifdef DEBUG_VALIDATE
            printf("Dynamic request, ");
        #endif
        return 1;
    } else {
        #ifdef DEBUG_VALIDATE
            printf("File type request unsupported, ");
        #endif
        return 2;
    }
}


void delete_element(){
    int i;

    #ifdef DEBUG_THREADS
        printf("Moving buffer elements one left\n");
    #endif

    for(i = 1; i < buffer_write; i++){
        buffer[i - 1].request_type = buffer[i].request_type;
        strcpy(buffer[i - 1].file, buffer[i].file);
        buffer[i - 1].socket = buffer[i].socket;
    }

    buffer_write--;
}

// Creates, prepares and returns new socket
int fireup(int port)
{
    int new_sock;
    struct sockaddr_in name;

    // Creates socket
    if ((new_sock = socket(PF_INET, SOCK_STREAM, 0))==-1) {
        printf("Error creating socket\n");
        return -1;
    }

    // Binds new socket to listening port
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(new_sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        printf("Error binding to socket\n");
        return -1;
    }

    // Starts listening on socket
    if (listen(new_sock, 5) < 0) {
        printf("Error listening to socket\n");
        return -1;
    }

    return(new_sock);
}

// Processes request from client
void get_request(int socket)
{
    int i,j;
    int found_get;

    found_get=0;
    while ( read_line(socket,SIZE_BUF) > 0 ) {
        if(!strncmp(buf,GET_EXPR,strlen(GET_EXPR))) {
            // GET received, extract the requested page/script
            found_get=1;
            i=strlen(GET_EXPR);
            j=0;
            while( (buf[i]!=' ') && (buf[i]!='\0') )
                req_buf[j++]=buf[i++];
                req_buf[j]='\0';
            }
        }

        // Currently only supports GET
        if(!found_get) {
            printf("Request from client without a GET\n");
            // exit(1);
        }
        // If no particular page is requested then we consider htdocs/index.html
        if(!strlen(req_buf))
            sprintf(req_buf,"index.html");

            #ifdef DEBUG
                //printf("get_request: client requested the following page: %s\n",req_buf); STOR
            #endif

            return;
        }


        // Send message header (before html page) to client
void send_header(int socket)
    {
        #ifdef DEBUG
        //printf("send_header: sending HTTP header to client\n"); STOR
        #endif
        sprintf(buf,HEADER_1);
        send(socket,buf,strlen(HEADER_1),0);
        sprintf(buf,SERVER_STRING);
        send(socket,buf,strlen(SERVER_STRING),0);
        sprintf(buf,HEADER_2);
        send(socket,buf,strlen(HEADER_2),0);

        return;
    }


// Execute script in /cgi-bin
void execute_script(int socket)
{
    printf("execute_script\n");
    char data[SIZE_BUF];

    char buf_tmp3[512];
    char command[512];


    FILE * fp;
    FILE * pf;


    // Searchs for page in directory htdocs
    sprintf(buf_tmp,"scripts/%s",req_buf);

    #ifdef DEBUG
    printf("send_script: searching for %s\n",buf_tmp);
    #endif

    // Verifies if file exists
    if((fp=fopen(buf_tmp,"r"))==NULL) {
        // Page not found, send error to client
        printf("send_script: script %s not found, alerting client\n",buf_tmp);
        cannot_execute(socket);
    }
    else {
        send_header(socket);
        while(fgets(buf_tmp3,512,fp))
        {
            snprintf(command,512,"%s", buf_tmp3);
            pf=popen(command, "r");


            if(!pf)
            {
                fprintf(stderr, "could not open pipe for output.\n");
                return;
            }


            send(socket, "<html><body>", strlen("<html><body>"),0);

            while(fgets(data, SIZE_BUF, pf))
            {
                send(socket,data,strlen(data),0);
                send(socket, "<p>", strlen("<p>"),0);
            }

            fclose(pf);
            send(socket, "</body></html>", strlen("</body></html>"),0);
        }
        fclose(fp);
    }
    return;
}


// Send html page to client
void send_page(int socket)
{
    FILE * fp;

    // Searchs for page in directory htdocs
    sprintf(buf_tmp,"htdocs/%s",req_buf);

    #ifdef DEBUG
    //printf("send_page: searching for %s\n",buf_tmp); STOR
    #endif

    // Verifies if file exists
    if((fp=fopen(buf_tmp,"rt"))==NULL) {
        // Page not found, send error to client
        printf("send_page: page %s not found, alerting client\n",buf_tmp);
        not_found(socket);
    }
    else {
        // Page found, send to client

        // First send HTTP header back to client
        send_header(socket);

        printf("send_page: sending page %s to client\n",buf_tmp);
        while(fgets(buf_tmp,SIZE_BUF,fp))
            send(socket,buf_tmp,strlen(buf_tmp),0);

            // Close file
            fclose(fp);
        }

        return;

    }


// Identifies client (address and port) from socket
void identify(int socket)
{
    char ipstr[INET6_ADDRSTRLEN];
    socklen_t len;
    struct sockaddr_in *s;
    int port;
    struct sockaddr_storage addr;

    len = sizeof addr;
    getpeername(socket, (struct sockaddr*)&addr, &len);

    // Assuming only IPv4
    s = (struct sockaddr_in *)&addr;
    port = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

    //printf("identify: received new request from %s port %d\n",ipstr,port); STOR

    return;
}


// Reads a line (of at most 'n' bytes) from socket
int read_line(int socket,int n)
{
    int n_read;
    int not_eol;
    int ret;
    char new_char;

    n_read=0;
    not_eol=1;

    while (n_read<n && not_eol) {
        ret = read(socket,&new_char,sizeof(char));
        if (ret == -1) {
            printf("Error reading from socket (read_line)\n");
            return -1;
        }
        else if (ret == 0) {
            return 0;
        }
        else if (new_char=='\r') {
            not_eol = 0;
            // consumes next byte on buffer (LF)
            read(socket,&new_char,sizeof(char));
            continue;
        }
        else {
            buf[n_read]=new_char;
            n_read++;
        }
    }

    buf[n_read]='\0';
    #ifdef DEBUG
    //printf("read_line: new line read from client socket: %s\n",buf); STOR
    #endif

    return n_read;
}

// Sends a 404 not found status message to client (page not found)
void not_found(int socket)
{
    sprintf(buf,"HTTP/1.0 404 NOT FOUND\r\n");
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,SERVER_STRING);
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,"Content-Type: text/html\r\n");
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,"\r\n");
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,"<HTML><TITLE>Not Found</TITLE>\r\n");
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,"<BODY><P>Resource unavailable or nonexistent.\r\n");
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,"</BODY></HTML>\r\n");
    send(socket,buf, strlen(buf), 0);

    return;
}

// Send a 5000 internal server error (script not configured for execution)
void cannot_execute(int socket)
{
    sprintf(buf,"HTTP/1.0 500 Internal Server Error\r\n");
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,"Content-type: text/html\r\n");
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,"\r\n");
    send(socket,buf, strlen(buf), 0);
    sprintf(buf,"<P>Error prohibited CGI execution.\r\n");
    send(socket,buf, strlen(buf), 0);

    return;
}

void sigint_handler(){
    int i;
    int size_proc = sizeof(id_proc)/sizeof(id_proc[0]);
    int size_thread = sizeof(threads)/sizeof(threads[0]);

    printf("Server terminating...");

    for(i = 0; i < size_thread; i++){
        printf("Canceling thread\n");
        pthread_cancel(threads[i]);
    }

    for(i = 0; i < size_proc; i++){
        kill(id_proc[i], SIGKILL);
    }

    printf("Closing Sockets\n");
    close(socket_conn);
    close(new_conn);

}

void catch_ctrlz(){
    raise(SIGHUP);
}
