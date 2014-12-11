#include "include.h"
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "manconfig.h"
#include "manstatistic.h"

// Header of HTTP reply to client
#define	SERVER_STRING 	"Server: simpleserver/0.1.0\r\n"
#define HEADER_1	"HTTP/1.0 200 OK\r\n"
#define HEADER_2	"Content-Type: text/html\r\n\r\n"

#define GET_EXPR	"GET /"
#define CGI_EXPR	"cgi-bin/"
#define SIZE_BUF	1024

pid_t id_proc[N_PROC];

char buf[SIZE_BUF];
char req_buf[SIZE_BUF];
char buf_tmp[SIZE_BUF];
int port,socket_conn,new_conn;

int init();
void main_process();
void create_thread_pool(int n_threads);
void* worker(void* idp);
void* scheduller(void* idp);

int fireup(int port);
void get_request(int socket);
void execute_script(int socket);
void send_page(int socket);
void identify(int socket);
int read_line(int socket,int n);
void not_found(int socket);
void cannot_execute(int socket);
