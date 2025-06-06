#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "worker.h"
#include "manager.h"
#include "common.h"
#include "common_cln.h"

static int file_info(const char *f, int *is_reg_file, int *is_dir, int *is_readable, int *is_writable, int *is_executable, size_t *fsize) {
    struct stat st;
    if (stat(f, &st) == -1) return -1;
    if (is_reg_file) *is_reg_file = S_ISREG(st.st_mode);
    if (is_dir) *is_dir = S_ISDIR(st.st_mode);
    if (is_readable) *is_readable = access(f, R_OK) == 0;
    if (is_writable) *is_writable = access(f, W_OK) == 0;
    if (is_executable) *is_executable = access(f, X_OK) == 0;
    if (fsize) *fsize = st.st_size;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc<8) {
        fprintf(stderr, "Usage: %s manager_host manager_port num_workers blocksize input output program args...\n", argv[0]);
        return 1;
    }
    // recogida de argumentos
    int nworkers = atoi(argv[3]);
    int blocksize = atoi(argv[4]);
    char * input = argv[5];
    char * output = argv[6];
    char * program = argv[7];

    // verificación de argumentos
    if (nworkers < 1) {
        fprintf(stderr, "número de workers inválido\n"); return 1;
    }
    if (blocksize < 1) {
        fprintf(stderr, "blocksize inválido\n"); return 1;
    }
    int res, is_dir, is_reg_file, is_readable, is_writable, is_executable;
    size_t input_size;
    res = file_info(input, &is_reg_file, NULL, &is_readable, NULL, NULL, &input_size);
    if ((res == -1) || !is_reg_file  || !is_readable) {
        fprintf(stderr, "fichero de entrada inválido\n"); return 1;
    }
    res = file_info(output, NULL, &is_dir, NULL, &is_writable, NULL, NULL);
    if ((res == -1) || !is_dir || !is_writable) {
        fprintf(stderr, "directorio de salida inválido\n"); return 1;
    }
    res = file_info(program, &is_reg_file, NULL, NULL, NULL, &is_executable, NULL);
    if ((res == -1) || !is_reg_file || !is_executable) {
        fprintf(stderr, "programa inválido\n"); return 1;
    }

    int sock_mgr = create_socket_cln_by_name(argv[1], argv[2]);
    if (sock_mgr < 0) return 1;

    int opcode = htonl(MSG_TYPE_RESERVE_WORKER);
    if (write(sock_mgr, &opcode, sizeof(int)) != sizeof(int)) {
        perror("error en write");
        close(sock_mgr); return 1;
    }

    int nw_net = htonl(nworkers);
    if (write(sock_mgr, &nw_net, sizeof(int)) != sizeof(int)) {
        perror("error en write");
        close(sock_mgr); return 1;
    }

    unsigned int ip;
    unsigned short port;
    if ((res=recv(sock_mgr, &ip, sizeof(ip), MSG_WAITALL))!=sizeof(ip)) {
        if (res!=0) perror("error en recv");
        close(sock_mgr); return 1;
    }
    if ((res=recv(sock_mgr, &port, sizeof(port), MSG_WAITALL))!=sizeof(port)) {
        if (res!=0) perror("error en recv");
        close(sock_mgr); return 1;
    }

    if (ip==0 && port==0) {
        close(sock_mgr);
        return 2;
    }

    int sock_worker = create_socket_cln_by_addr(ip, port);
    if (sock_worker < 0) { close(sock_mgr); return 1; }

    /* fase 3: se envía al worker la información de la tarea */
    int len_net;

    len_net = htonl(strlen(program)+1);
    if (write(sock_worker, &len_net, sizeof(int)) != sizeof(int)) {
        perror("error en write");
        close(sock_worker); close(sock_mgr); return 1;
    }
    if (write(sock_worker, program, strlen(program)+1) != (ssize_t)(strlen(program)+1)) {
        perror("error en write");
        close(sock_worker); close(sock_mgr); return 1;
    }

    len_net = htonl(strlen(input)+1);
    if (write(sock_worker, &len_net, sizeof(int)) != sizeof(int)) {
        perror("error en write");
        close(sock_worker); close(sock_mgr); return 1;
    }
    if (write(sock_worker, input, strlen(input)+1) != (ssize_t)(strlen(input)+1)) {
        perror("error en write");
        close(sock_worker); close(sock_mgr); return 1;
    }

    len_net = htonl(strlen(output)+1);
    if (write(sock_worker, &len_net, sizeof(int)) != sizeof(int)) {
        perror("error en write");
        close(sock_worker); close(sock_mgr); return 1;
    }
    if (write(sock_worker, output, strlen(output)+1) != (ssize_t)(strlen(output)+1)) {
        perror("error en write");
        close(sock_worker); close(sock_mgr); return 1;
    }

    /* espera la respuesta del worker */
    int reply;
    if ((res=recv(sock_worker, &reply, sizeof(int), MSG_WAITALL))!=sizeof(int)) {
        if (res!=0) perror("error en recv");
        close(sock_worker); close(sock_mgr); return 1;
    }

    reply = ntohl(reply);
    (void)reply; /* en esta fase no se utiliza el valor */
    close(sock_worker);
    close(sock_mgr);

    return 0;
}

