#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <string.h>
#include "worker.h"
#include "manager.h"
#include "common.h"
#include "common_srv.h"
#include "common_cln.h"

static int recv_string(int s, char *buf, size_t size) {
    size_t i=0; char c;
    while (i<size-1) {
        int n=recv(s,&c,1,0);
        if (n<=0) return -1;
        buf[i++]=c;
        if (c=='\0') break;
    }
    if (i==size-1) buf[i-1]='\0';
    return 0;
}

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

        char input[1024], output[1024], program[1024];
        if (recv_string(s_conec, input, sizeof(input))<0 ||
            recv_string(s_conec, output, sizeof(output))<0 ||
            recv_string(s_conec, program, sizeof(program))<0) {
            perror("error en recv");
            close(s_conec);
            continue;
        }

        char **args=NULL;
        int nargs=0;
        args = malloc(2*sizeof(char*));
        if (!args) { perror("malloc"); close(s_conec); continue; }
        args[nargs++] = program;
        args[nargs] = NULL;
        while (1) {
            char buf[1024];
            if (recv_string(s_conec, buf, sizeof(buf))<0) { perror("error en recv"); break; }
            if (buf[0]=='\0') break;
            char *dup=strdup(buf);
            char **tmp=realloc(args, sizeof(char*)*(nargs+2));
            if (!dup || !tmp) { perror("malloc"); free(dup); break; }
            args=tmp; args[nargs++]=dup; args[nargs]=NULL;
        }
        args[nargs]=NULL;

        int blocksize_net;
        if (recv(s_conec, &blocksize_net, sizeof(blocksize_net), MSG_WAITALL) != sizeof(blocksize_net)) {
            perror("error en recv");
            for (int i=1;i<nargs;i++) free(args[i]);
            free(args);
            close(s_conec);
            continue;
        }
        int blocksize = ntohl(blocksize_net);

        char *base = strrchr(input, '/');
        base = base ? base + 1 : input;

        while (1) {
            int bnet;
            int r = recv(s_conec, &bnet, sizeof(bnet), MSG_WAITALL);
            if (r == 0) break; /* conexión cerrada por el cliente */
            if (r != sizeof(bnet)) { perror("error en recv"); break; }
            int block = ntohl(bnet);
            printf("procesando bloque %d\n", block);

            int pipefd[2];
            if (pipe(pipefd)<0) { perror("pipe"); break; }

            pid_t pid=fork();
            if (pid==0) {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);

                char out_file[1024];
                snprintf(out_file, sizeof(out_file), "%s/%s-%05d", output, base, block);
                int fd_out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd_out < 0) { perror("open"); _exit(1); }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);

                execvp(program, args);
                perror("execvp");
                _exit(1);
            } else if (pid>0) {
                close(pipefd[0]);
                int fd_in = open(input, O_RDONLY);
                if (fd_in >= 0) {
                    off_t offset = (off_t)block * blocksize;
                    size_t count = blocksize;
                    struct stat st;
                    if (fstat(fd_in, &st) == 0 && offset + count > st.st_size) {
                        if (offset < st.st_size) count = st.st_size - offset; else count = 0;
                    }
                    off_t off = offset;
                    while (count > 0) {
                        ssize_t sent = sendfile(pipefd[1], fd_in, &off, count);
                        if (sent <= 0) break;
                        count -= sent;
                    }
                    close(fd_in);
                }
                close(pipefd[1]);
                int st; waitpid(pid, &st, 0);
                int ack=htonl(0);
                write(s_conec, &ack, sizeof(ack));
            } else {
                perror("fork");
                close(pipefd[0]);
                close(pipefd[1]);
                break;
            }
        }

        for (int i=1; i<nargs; i++) free(args[i]);
        free(args);
        close(s_conec);
        printf("conexión del cliente cerrada\n");
    }
    close(s_srv);
    return 0;
}
