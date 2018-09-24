#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>

#define KB 1024
/*function declerations*/

void send_content(int fd, int connfd,int isDir,char* path);
void enqueue(int data);
void* dequeue(void* ptr);
void send_to_client(int connfd, char* line);

/*global vars*/
pthread_mutex_t lock;
pthread_cond_t emp;
int sock =0;
int max_threads =0;
int global_count=0;
int finished =1;
int length=0;

/*Queue*/
struct node{
    int info;
    struct node *ptr;
}*head,*tail;
struct sigaction ready, last;



int main(int argc, char** argv){

    int threads;
    int requests; 
    int port;
    int i; 
    int s; 
    int len_of_packet=1; 
    int fd;
    int port1; 
    int file;   
    char content[16000];
    char  *fileContent ;
    int thrd ;
    int rc;
    char method[KB];
    char path[KB];
    int s_len=0; 
    int r_len=0;
    int fileLen; 
    int parsing;
    unsigned int addr_size;
    struct sockaddr_in serv_addr, client_addr;
    struct stat st;
    struct dirent *entry;
    DIR *dir;
    static fd_set readsocks, socks;
    char buf[KB] ;
    char Uniform_Resource_Identifier[KB];
    int servfd; 
    int socket_of_accept; 

    off_t chunk=0;
    size_t readData;
    int sent;

    assert (argc ==3 || argc ==4);


    threads = strtol(argv[1], NULL, 10);
    if ((errno == ERANGE && (threads == LONG_MAX || threads == LONG_MIN))
            || (errno != 0 && threads == 0)) {
        perror("Error: strtol has been failed\n");
        exit(0);
    }
    max_threads = threads;
    requests = strtol(argv[2], NULL, 10);
    if ((errno == ERANGE && (requests == LONG_MAX || requests == LONG_MIN))
            || (errno != 0 && requests == 0)) {
        perror("Error: strtol has been failed\n");
        exit(0);
    }
    port = 80; 

    if (argc == 4){
        port = strtol(argv[3], NULL, 10);
        if ((errno == ERANGE && (port == LONG_MAX || port == LONG_MIN))
                || (errno != 0 && port == 0)) {
            perror("Error: strtol has been failed\n");
            exit(0);
        }
    }

    pthread_t threadsArr[threads];


    if(pthread_mutex_init(&lock,NULL)!=0){
        printf("Error: failed to init mutex\n");
        return 1;   
    }

    pthread_cond_init( &emp, NULL);


    head=NULL;
    tail=NULL;

    /*create threads*/
    for (thrd=0; thrd<threads;thrd++){
        rc = pthread_create(&threadsArr[thrd], NULL,  dequeue, (void*) &thrd);
        if (rc){
            printf("Error: failed to create thread\n");
            return 1;   
        }
    }

    /* create socket */
    servfd = socket(AF_INET, SOCK_STREAM, 0);
    sock = servfd;
    if (servfd == -1) {
        printf("Error creating socket\n");
        exit(1);
    }

    /* init prepernces */
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    /* bind */
    setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &len_of_packet,sizeof(int));
    if (bind(servfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1) {
        printf("Error: Bind has been failed\n");
        perror ("Bind: ");
        exit(1);
    }

    /*listen */
    if (listen(servfd, requests) == -1) {
        printf("Error: Listen has been failed\n");
        perror ("Listen: ");
        exit(1); 
    }

    FD_ZERO(&socks);
    FD_SET(servfd, &socks);

    int startLen = strlen("HTTP/1.0 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n\r\n"
            "<doctype !html><html><head><title></title>"
            "<style>body"
            "h1 { font-size:4cm; text-align: center; color: black;"
            "}</style></head>"
            "<body><h1>Service Unavailable (HTTP 503)</h1></body></html>\r\n");
    int endLen = strlen("</h1></body></html>\r\n");


    while(1){

        sigfillset(&ready.sa_mask);
        ready.sa_handler = SIG_IGN;
        ready.sa_flags = 0;

        if (sigaction(SIGINT, &ready,&last) || sigaction(SIGINT,&last,NULL)){
            finished=0;
            for(i=0; i<threads; i++){
                rc = pthread_join(threadsArr[i], NULL);
                if(rc != 0){
                    printf("Error: join has been failed\n");
                    perror("Join: ");
                    return 1;   
                }
            }
            pthread_mutex_destroy(&lock);
            close(sock);
        }


        addr_size = sizeof(client_addr);
        socket_of_accept = accept(servfd, (struct sockaddr *) &client_addr, &addr_size);
        if (socket_of_accept == -1) {
            printf("Error: Accept has been failed\n");
            perror ("Accept: ");
            exit(1);
        }


        enqueue(socket_of_accept);
        if (length <= requests){

            /*recieve message*/
            r_len = recv(socket_of_accept, buf, KB,0);
            if (r_len == -1){
                printf("Error: recieve has been  %d\n", socket_of_accept);
                close(socket_of_accept);
                exit(1);
            }

            sscanf(buf,"%s %s", method, Uniform_Resource_Identifier);
            sscanf(Uniform_Resource_Identifier,"%s", path);
            printf("method = %s, path = %s\n", method, path);

            /*GET OR POST*/
            if(strcmp(method, "GET")!= 0 && strcmp(method, "POST") != 0){
                char response[] = "HTTP/1.0 200 OK\r\n"
                        "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                        "<doctype !html><html><head><title></title>"
                        "<style>body"
                        "h1 { font-size:4cm; text-align: center; color: black;"
                        "}</style></head>"
                        "<body><h1>Service Unavailable (HTTP 503)</h1></body></html>\r\n";
                s_len = send(socket_of_accept, response, strlen(response), 0);
                if (s_len == -1){
                    printf("Error: failed to send message %s to client %d\n", response, socket_of_accept);
                    close(socket_of_accept);
                    exit(1);
                }
            }

            if (stat(path, &st) == -1){ /*file not found*/
                char response[] = "HTTP/1.0 200 OK\r\n"
                        "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                        "<doctype !html><html><head><title></title>"
                        "<style>body"
                        "h1 { font-size:4cm; text-align: center; color: black;"
                        "}</style></head>"
                        "<body><h1>Not Found  (HTTP 404)</h1></body></html>\r\n";
                s_len = send(socket_of_accept, response, strlen(response), 0);
                if (s_len == -1){
                    printf("Error: failed to send message %s to client %d\n", response, socket_of_accept);
                    close(socket_of_accept);
                    exit(1);
                }
                close(socket_of_accept);
            }else {

                if(S_ISDIR(st.st_mode)){/*directory*/

                    if((dir = opendir (path)) != NULL){


                        send_content(fd, socket_of_accept,1,path);

                        close(socket_of_accept);

                    }else{
                        printf("Error: failed to open directory: %s\n", path);
                    }


                }else if(S_ISREG(st.st_mode) && st.st_size != 0){/*regular file*/


                    fd=open(path,O_RDONLY);
                    send_content(fd, socket_of_accept,0,path);

                    close(file);
                    close(socket_of_accept);


                }

                else{/*empty file*/
                    char response[] = "HTTP/1.0 200 OK\r\n"
                            "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                            "<doctype !html><html><head><title></title>"
                            "<style>body"
                            "h1 { font-size:4cm; text-align: center; color: black;"
                            "}</style></head>"
                            "<body><h1> </h1></body></html>\r\n";
                    s_len = send(socket_of_accept, response, strlen(response), 0);
                    if (s_len == -1){
                        printf("Error: failed to send message %s to client %d\n", response, socket_of_accept);
                        close(socket_of_accept);
                        exit(1);

                    }
                }
            }

        }else{/*full queue*/
            /*send (HTTP 503)*/

            char response[3000] = "HTTP/1.0 200 OK\r\n"
                    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                    "<doctype !html><html><head><title></title>"
                    "<style>body"
                    "h1 { font-size:4cm; text-align: center; color: black;"
                    "}</style></head>"
                    "<body><h1>Service Unavailable  (HTTP 503)</h1></body></html>\r\n";
            s_len = send(socket_of_accept, response, strlen(response), 0);
            if (s_len == -1){
                printf("Error: unable to send a message %s to: %d\n", response, socket_of_accept);
                close(socket_of_accept);
                exit(1);

            }
            close(socket_of_accept);
        }


    }

    return 0;
}






void send_content(int fd, int connfd,int isDir,char* path){
    char line[KB];
    memset(line, 0, sizeof(line));

    send_to_client(connfd, "HTTP/1.0 200 OK\r\n\r\n");

    int return_value;
    if(!isDir){
        while ((return_value=read(fd, line, KB)) > 0) {
            send_to_client(connfd, line);
            memset(line, 0, sizeof(line));
        }
        if (return_value < 0){
            perror("Error read");
            send_to_client(connfd, "Error Resorces\n");
            return;
        }
    }
    else {
        DIR           *d;
        struct dirent *dir;
        d = opendir(path);
        if (d)
        {
            while ((dir = readdir(d)) != NULL)
            {
                strcpy(line,dir->d_name);
                strcat(line,"\n");
                send_to_client(connfd,line);
                memset(line, 0, sizeof(line));
            }

            closedir(d);
        }
    }
}


/*enqueue*/
void enqueue(int data)
{
    int rc;
    struct node* temp;

    temp = (struct node *)malloc(1*sizeof(struct node));
    if(temp == NULL){
        printf("Error: couldn't allocate memory\n");
        exit(1);
    }
    temp->ptr = NULL;
    temp->info = data;

    if (pthread_mutex_lock(&lock) !=0){
        printf("Error: failed to lock\n");
        exit(1);
    } 



    if (length == 0) /*empty queue*/
    {
        head = temp;

    }else 
    {
        tail->ptr = temp; 
    }

    tail = temp;
    length++;
    rc = pthread_cond_broadcast(&emp);
    assert(rc==0);
    if (pthread_mutex_unlock(&lock)!=0){
        printf("Error: failed to unlock\n");
        pthread_exit(0);
    }


    return;
}





void send_to_client(int connfd, char* line){
    int nsent, totalsent;
    int notwritten = strlen(line);

    /* keep looping until nothing left to write*/
    totalsent = 0;
    while (notwritten > 0){
        /* notwritten = how much we have left to write
            totalsent  = how much we've written so far
            nsent = how much we've written in last write() call */
        nsent = write(connfd, line + totalsent, notwritten);
        assert(nsent>=0); // check if error occured (client closed connection?)

        totalsent  += nsent;
        notwritten -= nsent;
    }
}







/*dequeue*/
void* dequeue(void* ptr)
{
    int rc;
    int i=0;
    int thread = *((int*)ptr);

    struct node* temp;

    while(finished){

        if (pthread_mutex_lock(&lock) !=0){
            printf("Error: lock has been failed\n");
            pthread_exit(0);
        }

        while (length == 0)
        {
            rc = pthread_cond_wait(&emp, &lock);
            assert(rc == 0);
        }

        int info = head->info;
        temp = head;
        head = head->ptr;
        free(temp);


        length--;
        rc = pthread_cond_broadcast(&emp);
        assert(rc==0);
        if (pthread_mutex_unlock(&lock)!=0){
            printf("Error: unlock has been failed\n");
            pthread_exit(0);
        }
    }
    pthread_exit(0);
}