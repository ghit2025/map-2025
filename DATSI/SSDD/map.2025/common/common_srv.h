// DEFINICIÓN DE FUNCIONALIDADES COMUNES RELACIONADAS CON UN SERVIDOR DE SOCKETS
// INCLUYE LA DEFINICIÓN DE UNA FUNCIÓN QUE CREA UN SOCKET DE SERVIDOR.
// USADAS EN LA PRÁCTICA POR MANAGER Y WORKERS.

#ifndef _COMMON_SRV_H
#define _COMMON_SRV_H        1

// Inicializa el socket, le asigna un puerto y lo prepara para aceptar conexiones.
// Tiene dos modos de operación:
//    - Si se quiere que el SO asigne el puerto, se debe especificar
//    un 0 en el primer parámetro obteniendo en el segundo parámetro
//    el puerto asignado en formato de red.
//    - Si se quiere usar un determinado puerto, se debe especificar
//    este en el primer parámetro usando formato local, pudiendo
//    especificar un NULL en el segundo parámetro
int create_socket_srv(unsigned short req_port, unsigned short *alloc_port);

#endif // _COMMON_SRV_H
