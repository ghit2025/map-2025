//
// IMPLEMENTACIÓN DE UN TIPO DE DATOS QUE GESTIONA RESERVAS DE
// SERVIDORES TAL QUE DE CADA SERVIDOR SE GUARDA SU IP Y SU PUERTO.
//
// DISEÑADO ESPECÍFICAMENTE PARA LA PRÁCTICA.
// NO PUEDE MODIFICAR ESTE FICHERO.

// NO TIENE QUE REVISARLO: NO ES NECESARIO QUE CONOZCA LOS DETALLES DE LA
// IMPLEMENTACIÓN PARA USARLO. SOLO IMPORTA EL API.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "srv_addr_arr.h"

typedef struct srv_addr {
    unsigned int ip;
    unsigned short port;
} srv_addr;

// definición de tipos
struct entry {
    int free;
    srv_addr *elem;
};
#define MAGIC_ARR (('A' << 24) + ('R' << 16) + ('R' << 8) + 'A')

struct srv_addr_arr {
    int magic;
    int nentries;
    int nfree;
    int length;
    struct entry *collection;
    pthread_mutex_t mut;
    pthread_mutexattr_t mattr;
};

static int check_srv_addr_arr(const srv_addr_arr *a);
    
/* implementación de funciones de API */

// Crea un array de direcciones de servidores.
// Retorna una referencia al mismo o NULL en caso de error.
srv_addr_arr *srv_addr_arr_create(void) {
    srv_addr_arr *a = malloc(sizeof(srv_addr_arr));
    if (!a) return NULL;
    a->magic=MAGIC_ARR;
    a->nentries=a->length=a->nfree=0;
    a->collection=NULL;
    pthread_mutexattr_init(&a->mattr);
    pthread_mutexattr_settype(&a->mattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&a->mut, &a->mattr);
    return a;
}

// Inserta un nuevo servidor en el array.
// Retorna su ID si OK y -1 si error.
int srv_addr_arr_add(srv_addr_arr *a, unsigned int ip, unsigned short port) {
    if (check_srv_addr_arr(a)) return -1;
    int res=0;
    pthread_mutex_lock(&a->mut);
    a->nentries++;
    if (a->nentries>a->length) {
        a->collection=realloc(a->collection, a->nentries*sizeof(struct entry));
        if (!a->collection) {
	    res=-1; a->nentries--;
	}
	else 
            a->length++;
    }
    if (res!=-1) {
        srv_addr *sa = malloc(sizeof(srv_addr));
        if (!sa) {
            res=-1; a->nentries--;
        }
	else {
            sa->ip=ip;
            sa->port=port;
            struct entry e = {.free=1, .elem=sa};
            a->collection[a->nentries-1] = e;
            a->nfree++;
	    res=a->nentries-1;
        }
    }
    pthread_mutex_unlock(&a->mut);
    return res;
}

// Obtiene información (ip y puerto) del servidor dado su ID.
// Retorna 0 si OK y -1 si error.
int srv_addr_arr_get(const srv_addr_arr *a, int id, unsigned int *ip, unsigned short *port) {
    int err=0;
    if (check_srv_addr_arr(a)==-1) err=-1;
    else {
        pthread_mutex_lock((pthread_mutex_t *)&a->mut);
        if (id < 0 || id >= a->nentries)
            err = -1;
        else {
            srv_addr *sa = a->collection[id].elem;
            if (ip) *ip=sa->ip;
            if (port) *port=sa->port;
	}
        pthread_mutex_unlock((pthread_mutex_t *)&a->mut);
    }
    return err;
}
// Retorna el nº total de servidores en el array y -1 si error.
int srv_addr_arr_size(const srv_addr_arr *a){
    if (check_srv_addr_arr(a)) return -1;
    return a->nentries;
}
// Reserva el número de servidores indicado devolviendo sus ID en el parámetro
// srv. Retorna 0 si OK y -1 si error.
int srv_addr_arr_alloc(srv_addr_arr *a, int n, int *srv) {
    if (check_srv_addr_arr(a) || !srv) return -1;
    int err=0;
    pthread_mutex_lock((pthread_mutex_t *)&a->mut);
    if (n>a->nfree) err=-1;
    else {
        for (int i=0, s=0; i<a->nentries && s<n; i++)
            if (a->collection[i].free) {
                a->collection[i].free = 0;
                a->nfree--;
		srv[s++]=i;
	    }
    }
    pthread_mutex_unlock((pthread_mutex_t *)&a->mut);
    return err;
}

// Libera los servidores especificados en srv.
// Retorna 0 si OK y -1 si error.
int srv_addr_arr_free(srv_addr_arr *a, int n, const int *srv) {
    int err=0;
    if (check_srv_addr_arr(a) || !srv) err = -1;
    else {
        pthread_mutex_lock((pthread_mutex_t *)&a->mut);
        for (int i=0; i<n; i++) {
            int id=srv[i];
            if (id < 0 || id >= a->nentries) continue;
            if (!a->collection[id].free) {
                a->collection[i].free = 1;
                a->nfree++;
	    }
	}
        pthread_mutex_unlock((pthread_mutex_t *)&a->mut);
    }
    return err;
}

// Retorna el nº de servidores no reservados y -1 si error.
int srv_addr_arr_avail(const srv_addr_arr *a) {
    if (check_srv_addr_arr(a)) return -1;
    return a->nfree;
}

// Imprime lista de servidores mostrando un mensaje inicial
// Retorna 0 si OK y -1 si error.
int srv_addr_arr_print(const char* msg, srv_addr_arr *a) {
    if (check_srv_addr_arr(a)) return -1;
    if (msg) printf("%s\n", msg);
    pthread_mutex_lock((pthread_mutex_t *)&a->mut);
    for (int i=0; i<a->nentries; i++) {
	srv_addr *sa = a->collection[i].elem;
        struct in_addr in = {.s_addr=sa->ip};
	printf("Server %d: IP %s port %d state %s\n", i, inet_ntoa(in), sa->port, a->collection[i].free ? "free" : "allocated");
    }
    pthread_mutex_unlock((pthread_mutex_t *)&a->mut);
    return 0;
}

// implementación de función interna
static int check_srv_addr_arr(const srv_addr_arr *a){
    int res=0;
    if (a==NULL || a->magic!=MAGIC_ARR){
        res=-1; fprintf(stderr, "el srv_addr_arr especificado no es válido\n");
    }
    return res;
}
