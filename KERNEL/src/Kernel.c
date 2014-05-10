/*
 ============================================================================
 Name        : Kernel.c
 Author      : Garras
 Version     : 1.0
 Copyright   : Garras - UTN FRBA 2014
 Description : Ansi-style
 Testing	 : Para probarlo es tan simple como ejecutar en el terminator la linea "$ telnet localhost 7000" y empezar a dialogar con el UMV.
 ============================================================================
 */

#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <string.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <pthread.h>
#include <stdio.h>
//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/UMV/src/config.cfg"

//Tipo de cliente conectado
#define TIPO_KERNEL       1
#define TIPO_CPU       	  2

//Mensajes aceptados
#define MSJ_GET_BYTES             1
#define MSJ_SET_BYTES             2
#define MSJ_HANDSHAKE             3
#define MSJ_CAMBIO_PROCESO        4
#define MSJ_CREAR_SEGMENTO        5
#define MSJ_DESTRUIR_SEGMENTO     6

/** Puerto  */
#define PORT       7001

/** Número máximo de hijos */
#define MAX_CHILDS 3

/** Longitud del buffer  */
#define BUFFERSIZE 512

/* Grado de Multiprogramacion*/
#define Grado_de_multiprogramacion 10

/* Definición del pcb */
typedef struct PCBs
{
   int  id;
   int  segmentoCodigo;
   int  segmentoStack;
   int  cursorStack;
   int  indiceCodigo;
   int  indiceEtiquetas;
   int  programCounter;
   int  sizeContextoActual;
   int  sizeIndiceEtiquetas;
} PCB;

/* Definicion de los estados(colas) de los pcb */
typedef struct nodos
{
	PCB programas;
	int peso;
	struct nodos *next;
} nodo;

typedef struct punterosIdentificar {
	int tamano;
	nodo *inicio;
	nodo *fin;

}puntero;



int V= 1000; /*Var global*/
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla acceso a la variable global */

/* Hilo de PLP (lo que ejecuta) */
void *PLP(void *arg) {
	  int i;
	   for (i=0; i<1000; i++) {
	      pthread_mutex_lock(&m);
	      V= V + 1;
	      pthread_mutex_unlock(&m);
	    }
return NULL;
}

/* Hilo de PCP (lo que ejecuta) */
void *PCP(void *arg)
{
	   int i;
	   for (i=0; i<1000; i++) {
	      pthread_mutex_lock(&m);
	      V= V - 1;
	      pthread_mutex_unlock(&m);
	   }
return NULL;
}

int main(int argv, char** argc)
{
		printf("Creando Hilos PLP y PCP.");


		pthread_t plp,pcp;

		pthread_create(&plp, NULL , PLP , NULL);
		pthread_create(&pcp, NULL , PCP , NULL);
		
		pthread_join (plp,NULL);
		pthread_join(pcp,NULL);
		printf("  Al terminal valor de: V= %d\n", V);

	   return EXIT_SUCCESS;
}



int ObtenerTamanioMemoriaConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "TAMANIO_MEMORIA");
}
