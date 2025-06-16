#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
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
    return 0;
}

