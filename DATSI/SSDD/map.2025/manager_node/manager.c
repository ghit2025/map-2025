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

/* Se mantiene intacto el código de registro de trabajadores
 * añadiendo la funcionalidad de reserva sin eliminar líneas previas. */

typedef struct thread_info {
    int socket;
    struct sockaddr_in addr;
    srv_addr_arr *arr;
} thread_info;

static void *connection_handler(void *arg){
    thread_info *th = arg;
    int opcode, res;
    unsigned short port;
    int nworkers;

    if ((res=recv(th->socket, &opcode, sizeof(int), MSG_WAITALL))!=sizeof(int)){
        if (res!=0) perror("error en recv");
        close(th->socket);
        free(th);
        return NULL;
    }
    opcode = ntohl(opcode);
    if (opcode == MSG_TYPE_WORKER_REGISTER) {
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
    } else if (opcode == MSG_TYPE_RESERVE_WORKER) {
        if ((res=recv(th->socket, &nworkers, sizeof(int), MSG_WAITALL))!=sizeof(int)){
            if (res!=0) perror("error en recv");
            close(th->socket);
            free(th);
            return NULL;
        }
        nworkers = ntohl(nworkers);
        int *ids = malloc(sizeof(int)*nworkers);
        if (!ids) {
            close(th->socket);
            free(th);
            return NULL;
        }
        if (srv_addr_arr_alloc(th->arr, nworkers, ids)==-1) {
            unsigned int ip=0; unsigned short p=0;
            write(th->socket, &ip, sizeof(ip));
            write(th->socket, &p, sizeof(p));
            free(ids);
            close(th->socket);
            free(th);
            printf("conexión del cliente cerrada\n");
            return NULL;
        }
        srv_addr_arr_print("después de reserva", th->arr);
        unsigned int ip; unsigned short p;
        srv_addr_arr_get(th->arr, ids[0], &ip, &p);
        write(th->socket, &ip, sizeof(ip));
        write(th->socket, &p, sizeof(p));
        char dummy;
        recv(th->socket, &dummy, 1, 0);
        srv_addr_arr_free(th->arr, nworkers, ids);
        srv_addr_arr_print("después de liberar", th->arr);
        free(ids);
        close(th->socket);
        free(th);
        printf("conexión del cliente cerrada\n");
        return NULL;
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
