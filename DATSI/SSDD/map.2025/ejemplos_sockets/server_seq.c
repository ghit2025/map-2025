// EJEMPLO DE SERVIDOR SECUENCIAL
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include "common_srv.h"

// solo recibe una petición por conexión
void connection_handler(int s){
    int integer, res;

    if ((res=recv(s, &integer, sizeof(int), MSG_WAITALL))!=sizeof(int)) {
        if (res!=0) perror("error en read");
        close(s);
        return;
    }
    integer = ntohl(integer); // a formato de host
    printf("Recibido: %d\n", integer);
    ++integer;
    integer = htonl(integer); // a formato de red
    if (write(s, &integer, sizeof(int))!=sizeof(int)) {
        perror("error en write");
    }
    close(s);
    printf("conexión del cliente cerrada\n");
}
int main(int argc, char *argv[]) {
    int s, s_conec;
    unsigned int addr_size;
    struct sockaddr_in clnt_addr;

    if (argc!=2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return 1;
    }
    // inicializa el socket y lo prepara para aceptar conexiones
    if ((s=create_socket_srv(atoi(argv[1]), NULL)) < 0) return 1;

    while(1) {
        addr_size=sizeof(clnt_addr);
        // acepta la conexión
        if ((s_conec=accept(s, (struct sockaddr *)&clnt_addr, &addr_size))<0){
            perror("error en accept");
            close(s);
            return 1;
        }
        printf("conectado cliente con ip %s y puerto %u (formato red)\n",
                inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);
	connection_handler(s_conec);
    }
    close(s); // cierra el socket general
    return 0;
}
