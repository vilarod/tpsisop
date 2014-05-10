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
#include "Kernel.h"
//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/KERNEL/src/config.cfg"

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
	int cantidad;
	nodo *inicio;
	nodo *fin;

}puntero;




pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; /* mutex que controla acceso a la variable global */


int main(int argv, char** argc)
{
		printf("Creando Hilos PLP y PCP.\n");

		pthread_t plp,pcp;
		pthread_create(&plp, NULL , PLP , NULL);
		pthread_create(&pcp, NULL , PCP , NULL);

		pthread_join (plp,NULL);
		pthread_join(pcp,NULL);

		printf("Finalizado");

	   return EXIT_SUCCESS;
}


/* Hilo de PLP (lo que ejecuta) */
void *PLP(void *arg)
{
	pthread_t escuchaActiva;

			pthread_create(&escuchaActiva, NULL , escucha , NULL);


return NULL;
}

/* Hilo de PCP (lo que ejecuta) */
void *PCP(void *arg)
{

	return NULL;
}



int ObtenerTamanioMemoriaConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "TAMANIO_MEMORIA");
}


void *escucha(void *arg)
{
	 	int socket_host;
	    struct sockaddr_in client_addr;
	    struct sockaddr_in my_addr;

	    struct timeval tv;      /// Para el timeout del accept

	    socklen_t size_addr = 0;

	    int socket_client;
	    fd_set rfds;        // Conjunto de descriptores a vigilar
	    int childcount=0;


	    int childpid;

	    int pidstatus;

	    int activated=1;
	    int loop=0;

	    socket_host = socket(AF_INET, SOCK_STREAM, 0);
	    if(socket_host == -1)
	      error(1, "No puedo inicializar el socket");

	    my_addr.sin_family = AF_INET ;
	    my_addr.sin_port = htons(PORT);
	    my_addr.sin_addr.s_addr = INADDR_ANY ;


	    if( bind( socket_host, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 )
	      error(2, "El puerto está en uso"); /* Error al hacer el bind() */

	    if(listen( socket_host, 10) == -1 )
	      error(3, "No puedo escuchar en el puerto especificado");

	    size_addr = sizeof(struct sockaddr_in);

	    while(activated)
	            	      {

	            	      // select() se carga el valor de rfds
	            	      FD_ZERO(&rfds);
	            	      FD_SET(socket_host, &rfds);

	            	      // select() se carga el valor de tv
	            	      tv.tv_sec = 0;
	            	      tv.tv_usec = 500000;    // Tiempo de espera

	            	      if (select(socket_host+1, &rfds, NULL, NULL, &tv))
	            	        {
	            	          if((socket_client = accept( socket_host, (struct sockaddr*)&client_addr, &size_addr))!= -1)
	            	            {
	            	          loop=-1;        // Para reiniciar el mensaje de Esperando conexión...
	            	          printf("\nSe ha conectado %s por su puerto %d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
	            	          	            	            }
	            	          else
	            	            fprintf(stderr, "ERROR AL ACEPTAR LA CONEXIÓN\n");
	            	        }

	            	      // Miramos si se ha cerrado algún hijo últimamente
	            	      childpid=waitpid(0, &pidstatus, WNOHANG);
	            	      if (childpid>0)
	            	        {
	            	          childcount--;   // Se acaba de morir un hijo, que en paz descance

	            	          // Muchas veces nos dará 0 si no se ha muerto ningún hijo, o -1 si no tenemos hijos con errno=10 (No child process). Así nos quitamos esos mensajes

	            	          if (WIFEXITED(pidstatus))
	            	            {

	            	          // Tal vez querremos mirar algo cuando se ha cerrado un hijo correctamente
	            	          if (WEXITSTATUS(pidstatus)==99)
	            	            {
	            	              printf("\nSe ha pedido el cierre del programa\n");
	            	              activated=0;
	            	            }
	            	            }
	            	        }
	            	      loop++;
	            	      }

	    close(socket_host);
	    return NULL;
}

void error(int code, char *err)
{
  char *msg=(char*)malloc(strlen(err)+14);
  sprintf(msg, "Error %d: %s\n", code, err);
  fprintf(stderr, "%s", msg);
  exit(1);
}
