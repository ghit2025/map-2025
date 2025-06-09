#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <libgen.h>
#include <limits.h>
#include "worker.h"
#include "manager.h"
#include "common.h"
#include "common_srv.h"
#include "common_cln.h"

static int process_block(const char *input, const char *output_dir,
                         const char *program, int block, int blocksize,
                         off_t file_size) {
    char outname[PATH_MAX];
    char base[PATH_MAX];
    strncpy(base, basename((char *)input), sizeof(base));
    base[sizeof(base)-1] = '\0';
    snprintf(outname, sizeof(outname), "%s/%s-%05d", output_dir, base, block);

    int fd_in = open(input, O_RDONLY);
    if (fd_in < 0) { perror("open input"); return -1; }
    int fd_out = open(outname, O_CREAT|O_WRONLY|O_TRUNC, 0666);
    if (fd_out < 0) { perror("open output"); close(fd_in); return -1; }

    int pipefd[2];
    if (pipe(pipefd) < 0) { perror("pipe"); close(fd_in); close(fd_out); return -1; }

    pid_t pid = fork();
    if (pid == -1) { perror("fork"); close(fd_in); close(fd_out); close(pipefd[0]); close(pipefd[1]); return -1; }
    if (pid == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
        execl(program, program, NULL);
        perror("exec");
        _exit(1);
    }

    close(pipefd[0]);
    off_t off = (off_t)block * blocksize;
    lseek(fd_in, off, SEEK_SET);
    size_t remain = blocksize;
    if (off + blocksize > file_size) remain = file_size - off;
    while (remain > 0) {
        ssize_t sent = sendfile(pipefd[1], fd_in, NULL, remain);
        if (sent <= 0) { perror("sendfile"); break; }
        remain -= sent;
    }
    close(fd_in);
    close(pipefd[1]);
    close(fd_out);
    int st; waitpid(pid, &st, 0);
    return 0;
}

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

        int len;
        if ((res=recv(s_conec, &len, sizeof(int), MSG_WAITALL))!=sizeof(int)) {
            if (res!=0) perror("error en recv");
            close(s_conec); continue;
        }
        len = ntohl(len);
        char input[len+1];
        if (recv(s_conec, input, len, MSG_WAITALL)!=len) { perror("error en recv"); close(s_conec); continue; }
        input[len]='\0';

        if ((res=recv(s_conec, &len, sizeof(int), MSG_WAITALL))!=sizeof(int)) { if(res!=0) perror("error en recv"); close(s_conec); continue; }
        len = ntohl(len);
        char output[len+1];
        if (recv(s_conec, output, len, MSG_WAITALL)!=len) { perror("error en recv"); close(s_conec); continue; }
        output[len]='\0';

        if ((res=recv(s_conec, &len, sizeof(int), MSG_WAITALL))!=sizeof(int)) { if(res!=0) perror("error en recv"); close(s_conec); continue; }
        len = ntohl(len);
        char program[len+1];
        if (recv(s_conec, program, len, MSG_WAITALL)!=len) { perror("error en recv"); close(s_conec); continue; }
        program[len]='\0';

        int blocksize_net, nblocks_net;
        if (recv(s_conec, &blocksize_net, sizeof(int), MSG_WAITALL)!=sizeof(int)) { perror("error en recv"); close(s_conec); continue; }
        if (recv(s_conec, &nblocks_net, sizeof(int), MSG_WAITALL)!=sizeof(int)) { perror("error en recv"); close(s_conec); continue; }
        int blocksize = ntohl(blocksize_net);
        int nblocks = ntohl(nblocks_net);

        struct stat st;
        if (stat(input, &st) == -1) { perror("stat"); close(s_conec); continue; }
        off_t file_size = st.st_size;

        for (int i=0; i<nblocks; i++) {
            int block_net;
            res = recv(s_conec, &block_net, sizeof(int), MSG_WAITALL);
            if (res!=sizeof(int)) { if(res!=0) perror("error en recv"); break; }
            int block = ntohl(block_net);
            printf("procesando bloque %d\n", block);
            process_block(input, output, program, block, blocksize, file_size);
            int ack = htonl(0);
            if (write(s_conec, &ack, sizeof(int))!=sizeof(int)) { perror("error en write"); break; }
        }

        close(s_conec);
        printf("conexión del cliente cerrada\n");
    }
    close(s);
    return 0;
}

