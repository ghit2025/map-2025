#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "manager.h"
#include "common.h"
#include "common_srv.h"
#include "srv_addr_arr.h"

typedef struct thread_info {
    int socket;
    struct sockaddr_in addr;
    srv_addr_arr *srv_arr;
} thread_info;

static void *connection_handler(void *arg) {
    thread_info *th = arg;
    int res;
    int opcode;
    if ((res=recv(th->socket, &opcode, sizeof(opcode), MSG_WAITALL))!=sizeof(opcode)) {
        if (res!=0) perror("error en recv");
        close(th->socket);
        free(th);
        return NULL;
    }
    opcode = ntohl(opcode);
    if (opcode == OP_WORKER_REGISTER) {
        unsigned short port;
        if ((res=recv(th->socket, &port, sizeof(port), MSG_WAITALL))!=sizeof(port)) {
            if (res!=0) perror("error en recv");
            close(th->socket);
            free(th);
            return NULL;
        }
        srv_addr_arr_add(th->srv_arr, th->addr.sin_addr.s_addr, port);
        srv_addr_arr_print("después de alta", th->srv_arr);
        int ack=htonl(0);
        write(th->socket, &ack, sizeof(ack));
    } else if (opcode == OP_RESERVE_WORKER) {
        int n;
        if ((res=recv(th->socket, &n, sizeof(n), MSG_WAITALL))!=sizeof(n)) {
            if (res!=0) perror("error en recv");
            close(th->socket);
            free(th);
            return NULL;
        }
        n = ntohl(n);
        if (n<1) n=1; /* por compatibilidad futura */
        int srv_id;
        unsigned int ip=0;
        unsigned short port=0;
        if (srv_addr_arr_alloc(th->srv_arr, 1, &srv_id)==0) {
            srv_addr_arr_get(th->srv_arr, srv_id, &ip, &port);
            srv_addr_arr_print("después de reserva", th->srv_arr);
        }
        write(th->socket, &ip, sizeof(ip));
        write(th->socket, &port, sizeof(port));
        char dummy;
        recv(th->socket, &dummy, sizeof(dummy), 0);
        if (ip!=0 || port!=0) {
            srv_addr_arr_free(th->srv_arr, 1, &srv_id);
            srv_addr_arr_print("después de liberar", th->srv_arr);
        }
    }
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
    srv_addr_arr *srv_arr = srv_addr_arr_create();
    if (!srv_arr) {
        close(s); return 1;
    }

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
        thread_info *th=malloc(sizeof(thread_info));
        th->socket = s_conec;
        th->addr = clnt_addr;
        th->srv_arr = srv_arr;
        pthread_create(&thid, &atrib_th, connection_handler, th);
    }
    close(s);
    return 0;
}
