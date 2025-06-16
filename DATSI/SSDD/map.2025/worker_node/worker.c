#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include "worker.h"
#include "manager.h"
#include "common.h"
#include "common_srv.h"
#include "common_cln.h"

#define OP_WORKER_REGISTER 1

int main(int argc, char *argv[]) {
    int s_srv, s_mgr, s_conec;
    unsigned int addr_size;
    struct sockaddr_in clnt_addr;
    unsigned short port;

    if (argc!=3) {
        fprintf(stderr, "Usage: %s manager_host manager_port\n", argv[0]);
        return 1;
    }
    if ((s_srv=create_socket_srv(0, &port)) < 0) return 1;
    printf("puerto asignado (formato de red) %u\n", port);

    if ((s_mgr=create_socket_cln_by_name(argv[1], argv[2])) < 0) {
        close(s_srv);
        return 1;
    }
    int op=htonl(OP_WORKER_REGISTER);
    if (write(s_mgr, &op, sizeof(op)) != sizeof(op)) {
        perror("error en write");
        close(s_mgr); close(s_srv); return 1;
    }
    if (write(s_mgr, &port, sizeof(port)) != sizeof(port)) {
        perror("error en write");
        close(s_mgr); close(s_srv); return 1;
    }
    int ack;
    if (recv(s_mgr, &ack, sizeof(ack), MSG_WAITALL) != sizeof(ack)) {
        perror("error en recv");
        close(s_mgr); close(s_srv); return 1;
    }
    close(s_mgr);

    while(1) {
        addr_size=sizeof(clnt_addr);
        if ((s_conec=accept(s_srv, (struct sockaddr *)&clnt_addr, &addr_size))<0){
            perror("error en accept");
            close(s_srv);
            return 1;
        }
        printf("conectado cliente con ip %s y puerto %u (formato red)\n",
                inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);
        close(s_conec);
        printf("conexión del cliente cerrada\n");
    }
    close(s_srv);
    return 0;
}
