// USA SELECT PARA ESPERAR SIMULTÁNEAMENTE RESPUESTAS DE DOS SERVIDORES
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include "common_cln.h"

int main(int argc, char *argv[]) {
    int s1, s2, smax, ndesc;
    fd_set desc_sockets, desc_sockets_copia; 
    if (argc!=5) {
        fprintf(stderr, "Usage: %s server_host1 port1 server_host2 port2\n", argv[0]);
        return 1;
    }
    // se conecta con los dos servidores
    if ((s1=create_socket_cln_by_name(argv[1], argv[2]))<0) return 1;
    if ((s2=create_socket_cln_by_name(argv[3], argv[4]))<0) return 1;
    smax = s1<s2 ? s2:s1; // descriptor mayor: necesario para el select

    // envía mensaje a los dos servidores
    int integer = htonl(getpid());
    if (write(s1, &integer, sizeof(int))<0) {
        perror("error en write"); close(s1); close(s2); return 1;
    }
    if (write(s2, &integer, sizeof(int))<0) {
        perror("error en write"); close(s1); close(s2); return 1;
    }
    // supervisa actividad en los dos descriptores
    FD_ZERO(&desc_sockets); 
    FD_SET(s1, &desc_sockets); 
    FD_SET(s2, &desc_sockets); 
    // solo espera dos mensajes de respuesta
    for (int n=2; n; ) {
	// select modifica la variable: sacamos una copia
        desc_sockets_copia=desc_sockets;
	// esperamos actividad de entrada en descriptores supervisados;
	// puede haber actividad por recepción mensaje o cierre de conexión
        if ((ndesc=select(smax+1 , &desc_sockets_copia, NULL, NULL, NULL))<0) {
            perror("error en select"); close(s1); close(s2); return 1;
	}
	// select indica que hay ndesc descriptores con actividad
        for (int i=0; ndesc; i++) {
            if (FD_ISSET(i, &desc_sockets_copia)) { // descriptor i activo?
                ndesc--; 
                if (recv(i, &integer, sizeof(int), MSG_WAITALL)<=0) {
                    // actividad por cierre de conexión
                    close(i);
                    FD_CLR(i, &desc_sockets); // dejo de supervisarlo
                }
		else {
                    // actividad por recepción de mensaje
                    printf("recibido por descriptor %d: %d\n", i, ntohl(integer));
		    n--;
		}
            }
        }
    }
    return 0;
}
