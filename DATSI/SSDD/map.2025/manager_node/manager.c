#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "manager.h"
#include "common.h"
#include "common_srv.h"
#include "srv_addr_arr.h"

typedef struct thread_info {
    int socket;
    struct sockaddr_in addr;
    srv_addr_arr *arr;
} thread_info;

static void *connection_handler(void *arg){
    thread_info *th = arg;
    int opcode, res;
    unsigned short port;

    if ((res=recv(th->socket, &opcode, sizeof(int), MSG_WAITALL))!=sizeof(int)){
        if (res!=0) perror("error en recv");
        close(th->socket);
        free(th);
        return NULL;
    }
    opcode = ntohl(opcode);
    if ((res=recv(th->socket, &port, sizeof(unsigned short), MSG_WAITALL))!=sizeof(unsigned short)){
        if (res!=0) perror("error en recv");
        close(th->socket);
        free(th);
        return NULL;
    }
    if (opcode == 1) {
        srv_addr_arr_add(th->arr, th->addr.sin_addr.s_addr, port);
        srv_addr_arr_print("después de alta", th->arr);
    }
    int reply = htonl(0);
    write(th->socket, &reply, sizeof(int));
    close(th->socket);
    free(th);
    printf("conexión del cliente cerrada\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    int s, s_conec;
    unsigned int addr_size;
    struct sockaddr_in clnt_addr;

    if (argc!=2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return 1;
    }

    if ((s=create_socket_srv(atoi(argv[1]), NULL)) < 0) return 1;

    srv_addr_arr *arr = srv_addr_arr_create();
    if (!arr) { perror("error creando array"); close(s); return 1; }

    pthread_t thid;
    pthread_attr_t atrib_th;
    pthread_attr_init(&atrib_th);
    pthread_attr_setdetachstate(&atrib_th, PTHREAD_CREATE_DETACHED);

    while(1) {
        addr_size=sizeof(clnt_addr);
        if ((s_conec=accept(s, (struct sockaddr *)&clnt_addr, &addr_size))<0){
            perror("error en accept");
            close(s); return 1;
        }
        printf("conectado cliente con ip %s y puerto %u (formato red)\n",
                inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);
        thread_info *inf = malloc(sizeof(thread_info));
        inf->socket = s_conec;
        inf->addr = clnt_addr;
        inf->arr = arr;
        pthread_create(&thid, &atrib_th, connection_handler, inf);
    }
    close(s);
    return 0;
}
