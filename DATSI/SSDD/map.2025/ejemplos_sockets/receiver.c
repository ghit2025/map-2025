// SERVIDOR CORRESPONDIENTE A sender.c
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <pthread.h>
#include "common_srv.h"

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

    addr_size=sizeof(clnt_addr);
    // acepta la conexión
    if ((s_conec=accept(s, (struct sockaddr *)&clnt_addr, &addr_size))<0){
        perror("error en accept");
        close(s);
        return 1;
    }
    printf("conectado cliente con ip %s y puerto %u (formato red)\n",
                inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);
    int entero;
    int longitud;
    char *string;

    if (recv(s_conec, &entero, sizeof(int), MSG_WAITALL)!=sizeof(int)) {
        perror("error en recv");
        return 1;
    }
    entero = ntohl(entero);
    printf("Recibido entero: %d\n", entero);

    // luego llega el string, que viene precedido por su longitud
    if (recv(s_conec, &longitud, sizeof(int), MSG_WAITALL)!=sizeof(int)) {
        perror("error en recv");
        return 1;
    }
    longitud = ntohl(longitud);
    string = malloc(longitud+1); // +1 para el carácter nulo
    // ahora sí llega el string
    if (recv(s_conec, string, longitud, MSG_WAITALL)!=longitud) {
        perror("error en recv");
        return 1;
    }
    string[longitud]='\0';       // añadimos el carácter nulo
    printf("Recibido string: %s\n", string);
    close(s_conec);
    close(s); 
    return 0;
}
