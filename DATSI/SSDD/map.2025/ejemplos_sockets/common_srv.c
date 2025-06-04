// IMPLEMENTACIÓN DE FUNCIONALIDADES COMUNES RELACIONADAS CON UN SERVIDOR DE SOCKETS
// INCLUYE LA IMPLEMENTACIÓN DE UNA FUNCIÓN QUE CREA UN SOCKET DE SERVIDOR

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include "common_srv.h"

// Inicializa el socket, le asigna un puerto y lo prepara para aceptar conexiones.
// Tiene dos modos de operación:
//    - Si se quiere que el SO asigne el puerto, se debe especificar
//    un 0 en el primer parámetro obteniendo en el segundo parámetro
//    el puerto asignado en formato de red.
//    - Si se quiere usar un determinado puerto, se debe especificar
//    este en el primer parámetro usando formato local, pudiendo
//    especificar un NULL en el segundo parámetro
int create_socket_srv(unsigned short req_port, unsigned short *alloc_port) {
    int s;
    struct sockaddr_in addr;
    unsigned int td=sizeof(addr);
    int opcion=1;
    if ((s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("error creando socket"); return -1;
    }
    // Para reutilizar puerto inmediatamente si se rearranca el servidor
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion))<0){
        perror("error en setsockopt"); close(s); return -1;
    }
    // asocia el socket al puerto especificado (si 0, SO elige el puerto)
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(req_port);
    addr.sin_family=PF_INET;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("error en bind"); close(s); return -1;
    }
    // devuelve el puerto asignado, en formato de red,
    // si lo han pedido (2º parámetro!=NULL);
    if (alloc_port) {
        if (getsockname(s, (struct sockaddr *) &addr, &td)<0) {
            perror("error en getsockname"); close(s); return -1;
        }
        *alloc_port = addr.sin_port;
    }
    // establece el nº máx. de conexiones pendientes de aceptar
    if (listen(s, 5) < 0) {
        perror("error en listen"); close(s); return -1;
    }
    return s;
}
