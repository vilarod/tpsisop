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
#define TIPO_CPU       	  2
#define TIPO_PROGRAMA     1

//Mensajes aceptados
#define MSJ_HANDSHAKE             3
#define MSJ_RECIBO_PROGRAMA       1
#define MSJ_IMPRIMI_ESTO	      2
#define MSJ_ADIOS		          4

#define HANDSHAKEUMV '31'

/** Longitud del buffer  */
#define BUFFERSIZE 512

/* Definición del pcb */
typedef struct PCBs {
	int id;
	int segmentoCodigo;
	int segmentoStack;
	int cursorStack;
	int indiceCodigo;
	int indiceEtiquetas;
	int programCounter;
	int sizeContextoActual;
	int sizeIndiceEtiquetas;
} PCB;

int Ejecutando = 1;
int ImprimirTrazaPorConsola = 1;
int Puerto;
int sockfd;
int UMV_PUERTO;
char *UMV_IP;

int main(int argv, char** argc) {

	printf("Creando Hilos PLP y PCP.\n");
	//PCB * NUEVO, LISTO;

	//Obtener puertos e ip de la umv
	Puerto = ObtenerPuertoConfig();
	UMV_PUERTO = ObtenerPuertoUMV();
	UMV_IP = ObtenerIPUMV();

	//Crear Listas de estados
	//NUEVO = PCB * list_create();
	//LISTO = PCB * list_create();
	//Creacion de los hilos principales
	pthread_t plp, pcp;
	pthread_create(&plp, NULL, PLP, NULL );
	pthread_create(&pcp, NULL, PCP, NULL );

	pthread_join(plp, NULL );
	pthread_join(pcp, NULL );

	printf("Finalizado\n");

	return EXIT_SUCCESS;
}

/* Hilo de PLP (lo que ejecuta) */
void *PLP(void *arg) {

	conectarAUMV();
	crearSocketEscucha();
	// * Crear segmento 1° cod mensaje (5)
	// * Parametros a pasar 2° cantidad de dijitos del id programa
	// *  3° id programa
	// *  4° cantidad dijitos tamaño
	// *  5° tamaño
	// *  Destruir seg: idem menos 4° y 5°, cod mensaje (6)*/

	return NULL ;
}

/* Hilo de PCP (lo que ejecuta) */
void *PCP(void *arg) {

	/// MAGIA DE DANI ///

	return NULL ;
}

int crearSocketEscucha() {
	int socket_host;
	struct sockaddr_in client_addr;
	struct sockaddr_in my_addr;
	int yes = 1;
	socklen_t size_addr = 0;

	socket_host = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_host == -1)
		ErrorFatal(
				"No se pudo inicializar el socket que escucha a los clientes");

	if (setsockopt(socket_host, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		ErrorFatal("Error al hacer el 'setsockopt'");
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(Puerto);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8);

	if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		ErrorFatal("Error al hacer el Bind. El puerto está en uso");

	if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
		ErrorFatal(
				"Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

	Traza(
			"El socket está listo para recibir conexiones. Numero de socket: %d, puerto: %d",
			socket_host, Puerto);

	return socket_host;

	while (Ejecutando) {
		int socket_client;

		size_addr = sizeof(struct sockaddr_in);

		if ((socket_client = accept(socket_host,
				(struct sockaddr *) &client_addr, &size_addr)) != -1) {
			Traza(
					"Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d",
					inet_ntoa(client_addr.sin_addr), client_addr.sin_port,
					socket_client);

			// Aca hay que crear un nuevo hilo, que será el encargado de atender al cliente
			pthread_t hNuevoCliente;
			pthread_create(&hNuevoCliente, NULL, (void*) AtiendeCliente,
					(void *) socket_client);
		} else {
			Error("ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
		}
	}

	CerrarSocket(socket_host);
	return 1;
}

int ObtenerPuertoUMV() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_UMV");
}

char *ObtenerIPUMV() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_string_value(config, "IP_UMV");
}

int ObtenerPuertoConfig() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO");
}

void conectarAUMV() {
	Traza("Intentando conectar a umv.");
	sockfd = ConexionConSocket(UMV_PUERTO, UMV_IP);
	if (hacerhandshakeUMV(sockfd) == 0) {
		ErrorFatal("No se pudo conectar a la umv.");
	}
}

int hacerhandshakeUMV() {
	char *respuestahandshake = string_new();
	char *mensaje = "31";

	EnviarDatos(sockfd, mensaje);
	RecibirDatos(sockfd, respuestahandshake);
	return analisarRespuestaUMV(respuestahandshake);

}

int analisarRespuestaUMV(char *mensaje) {
	if (mensaje[0] == 0) {
		Error("La umv nos devolvio un error: %s", mensaje);
		return 0;
	} else
		return 1;
}

int ConexionConSocket(int puerto, char* IP) {
	int sockfd;
//Ip de lo que quieres enviar: ifconfig desde terminator , INADDR_ANY para local
	struct hostent *he, *gethostbyname();
	struct sockaddr_in their_addr;
	he = gethostbyname(IP);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		ErrorFatal("Error al querer crear el socket. puerto %d, ip %s", puerto,
				IP);
		exit(1);
	}

	their_addr.sin_family = AF_INET; // Ordenación de bytes de la máquina
	their_addr.sin_port = htons(puerto); // short, Ordenación de bytes de la red
	bcopy(he->h_addr, &(their_addr.sin_addr.s_addr),he->h_length); //their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8); // poner a cero el resto de la estructura

	if (connect(sockfd, (struct sockaddr *) &their_addr,
			sizeof(struct sockaddr)) == -1) {
		ErrorFatal("Error al querer conectar. puerto %d, ip %s", puerto, IP);
		exit(1);
	}

	return sockfd;
}

int RecibirDatos(int socket, void *buffer) {
	int bytecount;
// memset se usa para llenar el buffer con 0s
	memset(buffer, 0, BUFFERSIZE);

//Nos ponemos a la escucha de las peticiones que nos envie el cliente. //aca si recibo 0 bytes es que se desconecto el otro, cerrar el hilo.
	if ((bytecount = recv(socket, buffer, BUFFERSIZE, 0)) == -1)
		Error(
				"Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",
				socket);

	Traza("RECIBO datos. socket: %d. buffer: %s", socket, (char*) buffer);

	return bytecount;
}

int EnviarDatos(int socket, void *buffer) {
	int bytecount;

	if ((bytecount = send(socket, buffer, strlen(buffer), 0)) == -1)
		Error("No puedo enviar información a al clientes. Socket: %d", socket);

	Traza("ENVIO datos. socket: %d. buffer: %s", socket, (char*) buffer);

	return bytecount;
}

void error(int code, char *err) {
	char *msg = (char*) malloc(strlen(err) + 14);
	sprintf(msg, "Error %d: %s\n", code, err);
	fprintf(stderr, "%s", msg);
	exit(1);
}
void Cerrar(int sRemoto) {

	close(sRemoto);
}
void CerrarSocket(int socket) {
	close(socket);
	Traza("Se cerró el socket (%d).", socket);
}
void ErrorFatal(char mensaje[], ...) {
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);

	fprintf(stderr, "\nERROR FATAL: %s\n", nuevo);

	va_end(arguments);

	char fin;

	printf(
			"El programa se cerrara. Presione ENTER para finalizar la ejecución.");
	scanf("%c", &fin);

	exit(EXIT_FAILURE);
}
void Error(const char* mensaje, ...) {
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);

	fprintf(stderr, "\nERROR: %s\n", nuevo);

	va_end(arguments);

}
void Traza(const char* mensaje, ...) {
	if (ImprimirTrazaPorConsola) {
		char* nuevo;
		va_list arguments;
		va_start(arguments, mensaje);
		nuevo = string_from_vformat(mensaje, arguments);

		printf("TRAZA--> %s \n", nuevo);

		va_end(arguments);

	}
}
int ObtenerComandoMSJ(char buffer[])
{
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
	return chartToInt(buffer[0]);
}
void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente)
{
	(*idProg) = chartToInt(buffer[1]);
	(*tipoCliente) = chartToInt(buffer[2]);

	memset(buffer, 0, BUFFERSIZE);
	sprintf(buffer, "HandShake: OK! INFO-->  idPRog: %d, tipoCliente: %d ", *idProg, *tipoCliente);
}
int chartToInt(char x)
{
	char str[1];
	str[0] = x;
	int a = atoi(str);
	return a;
}

int AtiendeCliente(void * arg) {
	int socket = (int) arg;

	//Es el ID del programa con el que está trabajando actualmente el HILO.
	//Nos es de gran utilidad para controlar los permisos de acceso (lectura/escritura) del programa.
	//(en otras palabras que no se pase de vivo y quiera acceder a una posicion de memoria que no le corresponde.)
	int id_Programa = 0;

	int tipo_Conexion = 0;

	// Es el encabezado del mensaje. Nos dice que acción se le está solicitando al UMV
	int tipo_mensaje = 0;

	// Dentro del buffer se guarda el mensaje recibido por el cliente.
	char buffer[BUFFERSIZE];

	// Cantidad de bytes recibidos.
	int bytesRecibidos;

	// La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
	int desconexionCliente = 0;

	// Código de salida por defecto
	int code = 0;

	while ((!desconexionCliente) & Ejecutando) {
		//Recibimos los datos del cliente
		bytesRecibidos = RecibirDatos(socket, buffer);

		if (bytesRecibidos > 0) {
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			tipo_mensaje = ObtenerComandoMSJ(buffer);

			//Evaluamos los comandos
			switch (tipo_mensaje) {
			case MSJ_HANDSHAKE:
				ComandoHandShake(buffer, &id_Programa, &tipo_Conexion);
				break;

				// Enviamos datos al cliente.
				// NMR: aca luego habra que agregar un retardo segun pide el TP int pthread_detach(pthread_self());
				EnviarDatos(socket, buffer);
			}
			EnviarDatos(socket, buffer);
		} else
			desconexionCliente = 1;

	}

	CerrarSocket(socket);

	return code;
}

