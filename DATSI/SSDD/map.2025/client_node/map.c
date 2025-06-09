#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
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

    int sm = create_socket_cln_by_name(argv[1], argv[2]);
    if (sm < 0) return 1;

    int opcode = htonl(MSG_TYPE_WORKER_RESERVE);
    int nworkers_net = htonl(nworkers);
    struct iovec rqv[2];
    rqv[0].iov_base = &opcode; rqv[0].iov_len = sizeof(int);
    rqv[1].iov_base = &nworkers_net; rqv[1].iov_len = sizeof(int);
    if (writev(sm, rqv, 2) < 0) { perror("error en writev"); close(sm); return 1; }

    unsigned int ips[nworkers];
    unsigned short ports[nworkers];
    for (int i=0; i<nworkers; i++) {
        if (recv(sm, &ips[i], sizeof(unsigned int), MSG_WAITALL)!=sizeof(unsigned int) ||
            recv(sm, &ports[i], sizeof(unsigned short), MSG_WAITALL)!=sizeof(unsigned short)) {
            perror("error en recv"); close(sm); return 1; }
    }
    close(sm);

    int input_len = strlen(input);
    int output_len = strlen(output);
    int program_len = strlen(program);

    int blocksize_net = htonl(blocksize);
    int input_len_net = htonl(input_len);
    int output_len_net = htonl(output_len);
    int program_len_net = htonl(program_len);

    size_t nblocks = (input_size + blocksize - 1) / blocksize;

    int sockets[nblocks];

    for (int i = 0; i < (int)nblocks; i++) {
        sockets[i] = create_socket_cln_by_addr(ips[i], ports[i]);
        if (sockets[i] < 0) { perror("error connect worker"); return 1; }

        int nblocks_net = htonl(1);
        struct iovec hdr[8];
        int h = 0;
        hdr[h].iov_base = &input_len_net; hdr[h++].iov_len = sizeof(int);
        hdr[h].iov_base = input; hdr[h++].iov_len = input_len;
        hdr[h].iov_base = &output_len_net; hdr[h++].iov_len = sizeof(int);
        hdr[h].iov_base = output; hdr[h++].iov_len = output_len;
        hdr[h].iov_base = &program_len_net; hdr[h++].iov_len = sizeof(int);
        hdr[h].iov_base = program; hdr[h++].iov_len = program_len;
        hdr[h].iov_base = &blocksize_net; hdr[h++].iov_len = sizeof(int);
        hdr[h].iov_base = &nblocks_net; hdr[h++].iov_len = sizeof(int);
        if (writev(sockets[i], hdr, h) < 0) { perror("error en writev"); return 1; }

        int block_net = htonl(i);
        if (write(sockets[i], &block_net, sizeof(int)) != sizeof(int)) {
            perror("error en write"); return 1; }
    }

    for (int i = 0; i < (int)nblocks; i++) {
        int ack;
        int res = recv(sockets[i], &ack, sizeof(int), MSG_WAITALL);
        if (res != sizeof(int)) { if (res!=0) perror("error en recv"); }
        close(sockets[i]);
    }

    return 0;
}

