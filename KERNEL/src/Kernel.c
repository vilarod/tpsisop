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





#include <netdb.h>
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
#define MAXDATASIZE 100
//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/KERNEL/src/config.cfg"

//Tipo de cliente conectado
#define TIPO_CPU       	  1
#define TIPO_PROGRAMA     2
#define TIPO_MEMORIA      3


//Mensajes aceptados
#define MSJ_HANDSHAKE             1
#define MSJ_RECIBO_PROGRAMA       2

/** Puerto  */
#define PORT       7000

/** Número máximo de hijos */
#define MAX_CHILDS 3

/** Longitud del buffer  */
#define BUFFERSIZE 512

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

/* mutex que controla acceso a la seccion critica */
//pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int UMV_PUERTO;

int main(int argv, char** argc)
{
		printf("Creando Hilos PLP y PCP.\n");

		pthread_t plp,pcp;
		pthread_create(&plp, NULL , PLP , NULL);
		pthread_create(&pcp, NULL , PCP , NULL);

		pthread_join (plp,NULL);
		pthread_join(pcp,NULL);

		printf("Finalizado\n");

	   return EXIT_SUCCESS;
}


/* Hilo de PLP (lo que ejecuta) */
void *PLP(void *arg)
{
	pthread_t escuchaYopera;

			pthread_create(&escuchaYopera, NULL , escuchaPLP , NULL);
			pthread_join (escuchaYopera,NULL);
return NULL;
}

/* Hilo de PCP (lo que ejecuta) */
void *PCP(void *arg)
{
	return NULL;
}


void *escuchaPLP(void *arg)
{
		UMV_PUERTO = ObtenerPuertoUMV();
		ConexionConSocket();
		/*Solicitar coneccion umv(); si no se conecta se aborta todo*/
		/* ip y puerto esta en mi config */

		/*hanshake 1°char: cod mensaje (3)
		 * 2° char: tipo  (1)
		 * RESPUESTA GENERICA que dice 1° char es 1 o 0 (1 ok, 0 todo mal luego del 0 todo el mensaje termina en /o)
		 *
		 * Crear segmento 1° cod mensaje (5)
		 * Parametros a pasar 2° cantidad de dijitos del id programa
		 *  3° id programa
		 *  4° cantidad dijitos tamaño
		 *  5° tamaño
		 *  Destruir seg: idem menos 4° y 5°, cod mensaje (6)*/


	 	int socket_host;
	    struct sockaddr_in client_addr;
	    struct sockaddr_in my_addr;

	    struct timeval tv;      /// Para el timeout del accept

	    socklen_t size_addr = 0;

	    int socket_client;
	    fd_set rfds;        // Conjunto de descriptores a vigilar

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

int ObtenerPuertoUMV()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_UMV");
}

void ConexionConSocket()
{
	int sockfd;
	//Ip de lo que quieres enviar: ifconfig desde terminator , INADDR_ANY para local
	struct hostent *he, *gethostbyname();
	struct sockaddr_in their_addr;
	he=gethostbyname("10.5.2.192");

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket");
	exit(1);
	}
	their_addr.sin_family = AF_INET; // Ordenación de bytes de la máquina
	their_addr.sin_port = htons(PORT); // short, Ordenación de bytes de la red
	bcopy (he->h_addr, &(their_addr.sin_addr.s_addr),he->h_length);
//	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero),'\0', 8); // poner a cero el resto de laestructura

	if (connect(sockfd, (struct sockaddr *)&their_addr,
	sizeof(struct sockaddr)) == -1) {
	perror("connect");
	exit(1);
		}
	//conexion que funciona envia y recibe, envia comando por teclado y lo recibe.
	int numbytes;
	char mensaje[100];

	/*Enviar(sockfd,"hola");
	mensaje[Recibir(sockfd,mensaje)] = '\0';
	printf("Recibi: %s \n",mensaje);

	Enviar(sockfd,"como estas");
	numbytes=recv(sockfd, mensaje, 99, 0);
	mensaje[numbytes] = '\0';
	printf("Recibi: %s \n",mensaje);
	*/
	//el while de prueba para conectarse
	while(1){
	char comando[20];
	fgets(comando,20,stdin);
	Enviar(sockfd,comando);
	Recibir(sockfd,mensaje);
	printf("Recibi: %s \n",mensaje);
	}
     Cerrar(sockfd);
}


int Enviar (int sRemoto, char *buffer)
{
	int cantBytes;
	cantBytes= send(sRemoto,buffer,strlen(buffer),0);
	if (cantBytes ==-1)
		perror("No lo puedo enviar todo junto!");

	puts("Entre a ENVIAR!");
	//printf("%d",cantBytes);

	return cantBytes;

}

int Recibir (int socks, char * buffer)
{
	int numbytes;

	numbytes=recv(socks, buffer, 99, 0);
	buffer[numbytes] = '\0';

	return numbytes;
}

void Cerrar (int sRemoto)
{
	close(sRemoto);
}
