// EJEMPLO DE CLIENTE QUE ENVÍA CON UNA SOLA OPERACIÓN UN ENTERO Y UN STRING. 
// PARA ENVIAR EL STRING, AL SER DE TAMAÑO VARIABLE, TRANSMITE ANTES SU TAMAÑO.
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include "common_cln.h"

int main(int argc, char *argv[]) {
    int s;
    if (argc!=3) {
        fprintf(stderr, "Usage: %s server_host port\n", argv[0]);
        return 1;
    }
    // inicializa el socket y se conecta al servidor
    if ((s=create_socket_cln_by_name(argv[1], argv[2]))<0) return 1;

    // datos a enviar
    int entero = 1234567;
    char *string = "hola";

    struct iovec iov[3]; // hay que enviar 3 elementos
    int nelem = 0;

    // preparo el envío del entero convertido a formato de red
    int entero_net = htonl(entero);
    iov[nelem].iov_base=&entero_net;
    iov[nelem++].iov_len=sizeof(int);

    // preparo el envío del string mandando antes su longitud
    int longitud_str = strlen(string); // no incluye el carácter nulo
    int longitud_str_net = htonl(longitud_str);
    iov[nelem].iov_base=&longitud_str_net;
    iov[nelem++].iov_len=sizeof(int);
    iov[nelem].iov_base=string; // no se usa & porque ya es un puntero
    iov[nelem++].iov_len=longitud_str;

    if (writev(s, iov, nelem)<0) {
        perror("error en writev"); close(s); return 1;
    }
    close(s); // cierra el socket
    return 0;
}
