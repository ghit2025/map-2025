#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <string.h>
#include <sys/wait.h>
#include "worker.h"
#include "manager.h"
#include "common.h"
#include "common_srv.h"
#include "common_cln.h"

int main(int argc, char *argv[]) {
    if (argc!=3) {
        fprintf(stderr, "Usage: %s manager_host manager_port\n", argv[0]);
        return 1;
    }
    int s, s_conec, s_mngr, res;
    unsigned int addr_size;
    struct sockaddr_in clnt_addr;
    unsigned short srv_port;

    if ((s=create_socket_srv(0, &srv_port)) < 0) return 1;
    printf("puerto asignado (formato de red) %u\n", srv_port);

    if ((s_mngr=create_socket_cln_by_name(argv[1], argv[2]))<0){
        close(s); return 1;
    }
    int opcode = htonl(1); // identificador de alta
    if (write(s_mngr, &opcode, sizeof(int))!=sizeof(int)){
        perror("error en write"); close(s_mngr); close(s); return 1;
    }
    if (write(s_mngr, &srv_port, sizeof(unsigned short))!=sizeof(unsigned short)){
        perror("error en write"); close(s_mngr); close(s); return 1;
    }
    int reply;
    if ((res=recv(s_mngr, &reply, sizeof(int), MSG_WAITALL))!=sizeof(int)){
        if (res!=0) perror("error en recv");
        close(s_mngr); close(s); return 1;
    }
    close(s_mngr);

    while(1) {
        addr_size=sizeof(clnt_addr);
        if ((s_conec=accept(s, (struct sockaddr *)&clnt_addr, &addr_size))<0){
            perror("error en accept");
            close(s); return 1;
        }
        printf("conectado cliente con ip %s y puerto %u (formato red)\n",
                inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);

        /* fase 3: recepción de información de la tarea */
        int len, res;
        if ((res=recv(s_conec, &len, sizeof(int), MSG_WAITALL))!=sizeof(int)){
            if (res!=0) perror("error en recv");
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }
        len = ntohl(len);
        char *program = malloc(len);
        if (!program){
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }
        if ((res=recv(s_conec, program, len, MSG_WAITALL))!=len){
            if (res!=0) perror("error en recv");
            free(program);
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }

        if ((res=recv(s_conec, &len, sizeof(int), MSG_WAITALL))!=sizeof(int)){
            if (res!=0) perror("error en recv");
            free(program);
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }
        len = ntohl(len);
        char *input = malloc(len);
        if (!input){
            free(program);
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }
        if ((res=recv(s_conec, input, len, MSG_WAITALL))!=len){
            if (res!=0) perror("error en recv");
            free(input); free(program);
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }

        if ((res=recv(s_conec, &len, sizeof(int), MSG_WAITALL))!=sizeof(int)){
            if (res!=0) perror("error en recv");
            free(input); free(program);
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }
        len = ntohl(len);
        char *output = malloc(len);
        if (!output){
            free(input); free(program);
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }
        if ((res=recv(s_conec, output, len, MSG_WAITALL))!=len){
            if (res!=0) perror("error en recv");
            free(output); free(input); free(program);
            close(s_conec);
            printf("conexión del cliente cerrada\n");
            continue;
        }

        pid_t pid=fork();
        if (pid==0) {
            /* proceso hijo: ejecuta el programa */
            close(s);
            close(s_conec);
            execlp(program, program, (char *)NULL);
            perror("error en execlp");
            _exit(1);
        }

        int status;
        waitpid(pid, &status, 0);

        int reply = htonl(WIFEXITED(status) ? WEXITSTATUS(status) : -1);
        write(s_conec, &reply, sizeof(int));

        free(output);
        free(input);
        free(program);
        close(s_conec);
        printf("conexión del cliente cerrada\n");
    }
    close(s);
    return 0;
}

