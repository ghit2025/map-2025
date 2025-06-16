#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
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
    int n = htonl(nworkers);
    if (write(s_mgr, &n, sizeof(n)) != sizeof(n)) {
        perror("error en write");
        close(s_mgr); return 1;
    }
    unsigned int *ips = malloc(sizeof(unsigned int)*nworkers);
    unsigned short *ports = malloc(sizeof(unsigned short)*nworkers);
    if (!ips || !ports) {
        perror("malloc");
        close(s_mgr); return 1;
    }
    for (int i=0;i<nworkers;i++) {
        if (recv(s_mgr, &ips[i], sizeof(unsigned int), MSG_WAITALL)!=sizeof(unsigned int) ||
            recv(s_mgr, &ports[i], sizeof(unsigned short), MSG_WAITALL)!=sizeof(unsigned short)) {
            perror("error en recv");
            close(s_mgr); return 1;
        }
    }

    int nblocks = (input_size + blocksize - 1) / blocksize;
    int num_used = nblocks < nworkers ? nblocks : nworkers;
    int *s_workers = calloc(num_used, sizeof(int));
    if (!s_workers) { perror("malloc"); close(s_mgr); return 1; }
    int blks_net = htonl(blocksize);
    for (int i=0;i<num_used;i++) {
        if (ips[i]==0 && ports[i]==0) {
            close(s_mgr); free(ips); free(ports); free(s_workers); return 2; }
        int s_worker = create_socket_cln_by_addr(ips[i], ports[i]);
        if (s_worker < 0) { close(s_mgr); free(ips); free(ports); free(s_workers); return 1; }
        s_workers[i]=s_worker;
        if (write(s_worker, input, strlen(input)+1)!=strlen(input)+1 ||
            write(s_worker, output, strlen(output)+1)!=strlen(output)+1 ||
            write(s_worker, program, strlen(program)+1)!=strlen(program)+1) {
            perror("error en write");
            close(s_worker); close(s_mgr); free(ips); free(ports); free(s_workers); return 1; }
        for (int a=8; a<argc; a++) {
            if (write(s_worker, argv[a], strlen(argv[a])+1) != strlen(argv[a])+1) {
                perror("error en write");
                close(s_worker); close(s_mgr); free(ips); free(ports); free(s_workers); return 1; }
        }
        if (write(s_worker, "", 1) != 1 ||
            write(s_worker, &blks_net, sizeof(blks_net))!=sizeof(blks_net)) {
            perror("error en write");
            close(s_worker); close(s_mgr); free(ips); free(ports); free(s_workers); return 1; }
        int bnet = htonl(i);
        if (write(s_worker, &bnet, sizeof(bnet)) != sizeof(bnet)) {
            perror("error en write");
            close(s_worker); close(s_mgr); free(ips); free(ports); free(s_workers); return 1; }
    }

    int next_block = num_used;
    int active = num_used;
    while (active > 0) {
        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = -1;
        for (int i=0;i<num_used;i++) {
            if (s_workers[i] != -1) {
                FD_SET(s_workers[i], &rfds);
                if (s_workers[i] > maxfd) maxfd = s_workers[i];
            }
        }
        if (select(maxfd+1, &rfds, NULL, NULL, NULL) < 0) {
            perror("select");
            for (int i=0;i<num_used;i++) if (s_workers[i]!=-1) close(s_workers[i]);
            close(s_mgr); free(ips); free(ports); free(s_workers); return 1;
        }
        for (int i=0;i<num_used;i++) {
            if (s_workers[i] != -1 && FD_ISSET(s_workers[i], &rfds)) {
                int ack;
                if (recv(s_workers[i], &ack, sizeof(ack), MSG_WAITALL)!=sizeof(ack)) {
                    perror("error en recv");
                    for (int j=0;j<num_used;j++) if (s_workers[j]!=-1) close(s_workers[j]);
                    close(s_mgr); free(ips); free(ports); free(s_workers); return 1;
                }
                if (next_block < nblocks) {
                    int bnet = htonl(next_block++);
                    if (write(s_workers[i], &bnet, sizeof(bnet)) != sizeof(bnet)) {
                        perror("error en write");
                        for (int j=0;j<num_used;j++) if (s_workers[j]!=-1) close(s_workers[j]);
                        close(s_mgr); free(ips); free(ports); free(s_workers); return 1;
                    }
                } else {
                    close(s_workers[i]);
                    s_workers[i] = -1;
                    active--;
                }
            }
        }
    }

    free(ips); free(ports); free(s_workers);
    close(s_mgr);
    return 0;
}

