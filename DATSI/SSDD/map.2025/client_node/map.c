#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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

    /* fase de reserva de trabajadores */
    int s_mgr = create_socket_cln_by_name(argv[1], argv[2]);
    if (s_mgr < 0) return 1;

    int op = htonl(OP_RESERVE_WORKER);
    if (write(s_mgr, &op, sizeof(op)) != sizeof(op)) {
        perror("error en write");
        close(s_mgr); return 1;
    }
    int n = htonl(1); /* por ahora solo se reserva 1 */
    if (write(s_mgr, &n, sizeof(n)) != sizeof(n)) {
        perror("error en write");
        close(s_mgr); return 1;
    }
    unsigned int ip;
    unsigned short port;
    if (recv(s_mgr, &ip, sizeof(ip), MSG_WAITALL) != sizeof(ip) ||
        recv(s_mgr, &port, sizeof(port), MSG_WAITALL) != sizeof(port)) {
        perror("error en recv");
        close(s_mgr); return 1;
    }

    if (ip==0 && port==0) { /* no workers disponibles */
        close(s_mgr);
        return 2;
    }

    int s_worker = create_socket_cln_by_addr(ip, port);
    if (s_worker < 0) {
        close(s_mgr);
        return 1;
    }

    /* envia informacion inicial al worker */
    int blks_net = htonl(blocksize);
    if (write(s_worker, input, strlen(input)+1)!=strlen(input)+1 ||
        write(s_worker, output, strlen(output)+1)!=strlen(output)+1 ||
        write(s_worker, program, strlen(program)+1)!=strlen(program)+1 ||
        write(s_worker, &blks_net, sizeof(blks_net))!=sizeof(blks_net)) {
        perror("error en write");
        close(s_worker);
        close(s_mgr);
        return 1;
    }

    int nblocks = (input_size + blocksize - 1) / blocksize;
    int ack;
    for (int i=0; i<nblocks; i++) {
        int bnet = htonl(i);
        if (write(s_worker, &bnet, sizeof(bnet)) != sizeof(bnet)) {
            perror("error en write");
            close(s_worker);
            close(s_mgr);
            return 1;
        }
        if (recv(s_worker, &ack, sizeof(ack), MSG_WAITALL)!=sizeof(ack)) {
            perror("error en recv");
            close(s_worker);
            close(s_mgr);
            return 1;
        }
    }

    close(s_worker);
    close(s_mgr);
    return 0;
}

