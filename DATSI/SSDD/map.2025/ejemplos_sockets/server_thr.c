// EJEMPLO DE SERVIDOR MULTITHREAD CUYOS CLIENTES ENVÍAN UNA PETICIÓN POR CONEXIÓN
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <pthread.h>
#include "common_srv.h"

// información que se la pasa el thread creado
typedef struct thread_info {
    int socket; // añadir los campos necesarios
} thread_info;

// función del thread
// solo recibe una petición por conexión
void *connection_handler(void *arg){
    int integer, res;
    thread_info *thinf = arg; // argumento recibido

    if ((res=recv(thinf->socket, &integer, sizeof(int), MSG_WAITALL))!=sizeof(int)) {
        if (res!=0) perror("error en read");
        close(thinf->socket);
        return NULL;
    }
    integer = ntohl(integer); // a formato de host
    printf("Recibido: %d\n", integer);
    ++integer;
    integer = htonl(integer); // a formato de red
    if (write(thinf->socket, &integer, sizeof(int))!=sizeof(int)) {
        perror("error en write");
    }
    close(thinf->socket);
    free(thinf);
    printf("conexión del cliente cerrada\n");
    return NULL;
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

    // prepara atributos adecuados para crear thread "detached"
    pthread_t thid;
    pthread_attr_t atrib_th;
    pthread_attr_init(&atrib_th); // evita pthread_join
    pthread_attr_setdetachstate(&atrib_th, PTHREAD_CREATE_DETACHED);
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
        // crea el thread de servicio
        thread_info *thinf = malloc(sizeof(thread_info));
        thinf->socket=s_conec;
        pthread_create(&thid, &atrib_th, connection_handler, thinf);
    }
    close(s); // cierra el socket general
    return 0;
}
