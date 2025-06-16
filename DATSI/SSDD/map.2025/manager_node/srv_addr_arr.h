//
// DEFINICIÓN DE UN TIPO DE DATOS QUE GESTIONA RESERVAS DE
// SERVIDORES TAL QUE DE CADA SERVIDOR SE GUARDA SU IP Y SU PUERTO.
//
// DISEÑADO ESPECÍFICAMENTE PARA LA PRÁCTICA.
// NO PUEDE MODIFICAR ESTE FICHERO.

/*
 * No hay funciones destructivas porque la práctica no lo requiere.
 */

#ifndef _SRV_ADDR_ARR_H
#define _SRV_ADDR_ARR_H      1

// Tipo opaco para ocultar la implementación
typedef struct srv_addr_arr srv_addr_arr;

/* API */

// Crea un array de direcciones de servidores.
// Retorna una referencia al mismo o NULL en caso de error.
srv_addr_arr *srv_addr_arr_create(void);

// Inserta un nuevo servidor en el array.
// Retorna su ID si OK y -1 si error.
// DE CARA AL CORRECTOR DE LA PRÁCTICA, ip Y port DEBEN ESPECIFICARSE EN FORMATO DE RED
int srv_addr_arr_add(srv_addr_arr *a, unsigned int ip, unsigned short port);

// Obtiene información (ip y puerto) del servidor dado su ID.
// Retorna 0 si OK y -1 si error.
int srv_addr_arr_get(const srv_addr_arr *a, int id, unsigned int *ip, unsigned short *port);

// Retorna el nº total de servidores en el array y -1 si error.
int srv_addr_arr_size(const srv_addr_arr *a);

// Reserva el número de servidores indicado devolviendo sus ID en el parámetro
// srv. Retorna 0 si OK y -1 si error.
int srv_addr_arr_alloc(srv_addr_arr *a, int n, int *srv);

// Libera los servidores especificados en srv.
// Retorna 0 si OK y -1 si error.
int srv_addr_arr_free(srv_addr_arr *a, int n, const int *srv);

// Retorna el nº de servidores no reservados y -1 si error.
int srv_addr_arr_avail(const srv_addr_arr *a);

// Imprime lista de servidores mostrando un mensaje inicial
// Retorna 0 si OK y -1 si error.
int srv_addr_arr_print(const char *msg, srv_addr_arr *a);

#endif // _SRV_ADDR_ARR_H
