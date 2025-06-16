/*
 * PUEDE INCLUIR EN ESTE FICHERO LAS DEFICIONES QUE NECESITAN COMPARTIR
 * EL MANAGER, LOS WORKERS Y EL MAP, SI ES QUE LAS HUBIERA.
 */

#ifndef _COMMON_H
#define _COMMON_H        1

/* Códigos de operación intercambiados con el gestor */
#define OP_WORKER_REGISTER 1   /* Alta de un trabajador */
#define OP_RESERVE_WORKER 2    /* Solicitud de reserva desde el cliente */

#endif // _COMMON_H
