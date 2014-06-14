/*
 ============================================================================
 Name        : Kernel.c
 Author      : Garras
 Version     : 1.0
 Copyright   : Garras - UTN FRBA 2014
 Description : Ansi-style
 Testing	 : Para probarlo es tan simple como ejecutar en el terminator
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
#include <commons/collections/list.h>
#include <pthread.h>
#include <stdio.h>
#include "Kernel.h"

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/KERNEL/src/config.cfg"

//Tipo de cliente conectado
#define TIPO_CPU       	  2
#define TIPO_PROGRAMA     1

//Mensajes aceptados
#define MSJ_HANDSHAKE             3
#define MSJ_RECIBO_PROGRAMA       1
#define MSJ_IMPRIMI_ESTO	      2

#define HANDSHAKEUMV '31'

/** Longitud del buffer  */
#define BUFFERSIZE 1024

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

typedef struct CPUs {
	int id;
	int programa;
	int libre;
} CPU;

CPU CPU1;
PCB PCB1;
t_list *listCPU;
int socketumv;
int main(int argv, char** argc) {

	//Obtener puertos e ip de la umv
	UMV_PUERTO = ObtenerPuertoUMV();
	UMV_IP = ObtenerIPUMV();
	Puerto = ObtenerPuertoConfig();
	PuertoPCP = ObtenerPuertoPCPConfig();

	//Inicializo semaforo
	seminit( &readyCont, 0);
	seminit( &CPUCont, 0);
	seminit( &finalizarCont, 0);
	seminit( &imprimirCont, 0);

	//Crear Listas de estados
	//PCB * NUEVO, LISTO;
	//NUEVO = PCB * list_create();
	//LISTO = PCB * list_create();

	//Creacion del hilo plp
	pthread_t plp;
	pthread_create(&plp, NULL, PLP, NULL );
	pthread_join(plp, NULL );

	printf("Finalizado\n");

	return EXIT_SUCCESS;
}

/* Hilo de PLP (lo que ejecuta) */
void *PLP(void *arg) {

	//Me conecto a la UMV
	conectarAUMV();

	//Creo el hilo de pcp
	pthread_t pcp, escuchaFinEImprimir;
	pthread_create(&pcp, NULL, PCP, NULL );
	pthread_join(pcp, NULL );

	crearEscucha();

	pthread_create(&escuchaFinEImprimir, NULL, IMPRIMIRYFIN, NULL );
	pthread_join(escuchaFinEImprimir, NULL );

	return NULL ;
}

void *IMPRIMIRYFIN(void *arg) {

	//los waits y while 1 para las cola de fin y de imprimir algo

	return NULL ;
}

/* Hilo de PCP (lo que ejecuta) */
void *PCP(void *arg) {

	/// MAGIA DE DANI ///

	listCPU = list_create();
	//crear hilo de escucha CPU
	pthread_t plpCPU, plpReady, plpBloqueado;
	pthread_create(&plpCPU, NULL, HiloOrquestadorDeCPU, NULL );


	//crear hilo de ready
	pthread_create(&plpReady, NULL, moverEjecutar, NULL );


	//crear hilo de bloqueado
	pthread_create(&plpBloqueado, NULL, moverReady, NULL );

	pthread_join(plpCPU, NULL );
	pthread_join(plpReady, NULL );
	pthread_join(plpBloqueado, NULL );

	return NULL ;
}

void *HiloOrquestadorDeCPU()
{
	int socket_host;
	struct sockaddr_in client_addr;
	struct sockaddr_in my_addr;
	int yes = 1;
	socklen_t size_addr = 0;

	socket_host = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_host == -1)
		ErrorFatal("No se pudo inicializar el socket que escucha a los clientes");

	if (setsockopt(socket_host, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		ErrorFatal("Error al hacer el 'setsockopt'");
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PuertoPCP);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8* sizeof(char));

	if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		ErrorFatal("Error al hacer el Bind. El puerto está en uso");

	if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
		ErrorFatal("Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

	Traza("El socket está listo para recibir conexiones. Numero de socket: %d, puerto: %d", socket_host, PuertoPCP);

	while (1)
	{
		int socket_client;

		size_addr = sizeof(struct sockaddr_in);

		if ((socket_client = accept(socket_host, (struct sockaddr *) &client_addr, &size_addr)) != -1)
		{
			Traza("Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, socket_client);

			// Aca hay que crear un nuevo hilo, que será el encargado de atender al cliente
			pthread_t hNuevoCliente;
			pthread_create(&hNuevoCliente, NULL, (void*) AtiendeClienteCPU, (void *) socket_client);
		}
		else
		{
			Error("ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
		}
	}

	CerrarSocket(socket_host);
}


void *escuharCPU(void *arg) {

	//Abrir socket de escuchar CPU

	return NULL ;
}

void *moverEjecutar(void *arg) {

	//buscar CPU libre y mandar PCB

	return NULL ;
}

void *moverReady(void *arg) {

	//mandar a ready los bloqueados

	return NULL ;
}

void crearEscucha() {
	//while(1);
	//Crear select

	int AtiendeCliente(int sockete); //Handshake y recibir programa

	return;
}
int pedirMemoriaUMV(int socketumv) {
	char respuestaMemoria[BUFFERSIZE];
	char *mensaje = "5";
	//ENVIAR TODOS LOS CREAR SEGMENTOS
	EnviarDatos(socketumv, mensaje);
	RecibirDatos(socketumv, respuestaMemoria);

	return analisarRespuestaUMV(respuestaMemoria);
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

int ObtenerPuertoPCPConfig() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTOPCP");
}

void conectarAUMV() {
	Traza("Intentando conectar a umv.");

	socketumv = ConexionConSocket(UMV_PUERTO, UMV_IP);
	if (hacerhandshakeUMV(socketumv) == 0) {
		ErrorFatal("No se pudo conectar a la umv.");
	}
}

int hacerhandshakeUMV(int sockfd) {
	char respuestahandshake[BUFFERSIZE];
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

int RecibirDatos(int socket, char *buffer) {
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
int ObtenerComandoMSJ(char buffer[]) {
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
	return chartToInt(buffer[0]);
}
void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente) {
	(*idProg) = chartToInt(buffer[1]);
	(*tipoCliente) = chartToInt(buffer[2]);

	memset(buffer, 0, BUFFERSIZE);
	sprintf(buffer, "HandShake: OK! INFO-->  idPRog: %d, tipoCliente: %d ",
			*idProg, *tipoCliente);
}
int chartToInt(char x) {
	char str[1];
	str[0] = x;
	int a = atoi(str);
	return a;
}

void ComandoRecibirPrograma(char *buffer, int *idProg, int *tipoCliente) {
	(*idProg) = chartToInt(buffer[1]);
	(*tipoCliente) = chartToInt(buffer[2]);

	memset(buffer, 0, BUFFERSIZE);

	//Recibir programa
	//CrearPCB();
	if (pedirMemoriaUMV(socketumv)) {
		//Responder programa too ok
	} else {
		//responder programa too mal
		// * Crear segmento 1° cod mensaje (5)
		// * Parametros a pasar 2° cantidad de dijitos del id programa
		// *  3° id programa
		// *  4° cantidad dijitos tamaño
		// *  5° tamaño
		// *  Destruir seg: idem menos 4° y 5°, cod mensaje (6)*/
	}
}

int AtiendeCliente(int sockete) {

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

	// Código de salida por defecto
	int code = 0;

	bytesRecibidos = RecibirDatos(sockete, buffer);

	if (bytesRecibidos > 0) {
		//Analisamos que peticion nos está haciendo (obtenemos el comando)
		tipo_mensaje = ObtenerComandoMSJ(buffer);

		//Evaluamos los comandos
		switch (tipo_mensaje) {
		case MSJ_HANDSHAKE:
			ComandoHandShake(buffer, &id_Programa, &tipo_Conexion);
			EnviarDatos(sockete, "1");
			break;
		case MSJ_RECIBO_PROGRAMA:
			ComandoRecibirPrograma(buffer, &id_Programa, &tipo_Conexion);
			EnviarDatos(sockete, "1");
		}
	}
	return code;
}

int AtiendeClienteCPU(void * arg)
{
	int socket = (int) arg;

//Es el ID del programa con el que está trabajando actualmente el HILO.
//Nos es de gran utilidad para controlar los permisos de acceso (lectura/escritura) del programa.
//(en otras palabras que no se pase de vivo y quiera acceder a una posicion de memoria que no le corresponde.)
//	int id_CPU = 0;
	int tipo_Cliente = 0;

// Es el encabezado del mensaje. Nos dice que acción se le está solicitando al UMV
	int tipo_mensaje = 0;

// Dentro del buffer se guarda el mensaje recibido por el cliente.
	char* buffer;
	buffer = malloc(1 * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.

// Cantidad de bytes recibidos.
	int bytesRecibidos;

// La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
	int desconexionCliente = 0;

// Código de salida por defecto
	int code = 0;

	while ((!desconexionCliente))
	{
		buffer = realloc(buffer, 1 * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.

		//Recibimos los datos del cliente
		buffer = RecibirDatos2(socket, buffer, &bytesRecibidos);

		if (bytesRecibidos > 0)
		{
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			tipo_mensaje = ObtenerComandoMSJ(buffer);

			//Evaluamos los comandos
			switch (tipo_mensaje)
			{

				case MSJ_HANDSHAKE:
					buffer = ComandoHandShake2(buffer, &tipo_Cliente);
					//crear nueva CPU
					break;
				default:
					//buffer = RespuestaClienteError(buffer, "El ingresado no es un comando válido");
					break;
			}

			// Enviamos datos al cliente.
			EnviarDatos(socket, buffer);
		}
		else
			desconexionCliente = 1;

	}

	CerrarSocket(socket);

	return code;
}

char* RecibirDatos2(int socket, char *buffer, int *bytesRecibidos)
{
	*bytesRecibidos = 0;
	int tamanioNuevoBuffer = 0;
	int mensajeCompleto = 0;
	int cantidadBytesRecibidos = 0;
	int cantidadBytesAcumulados = 0;

	// memset se usa para llenar el buffer con 0s
	char bufferAux[BUFFERSIZE];

	buffer = realloc(buffer, 1 * sizeof(char)); //--> el buffer ocupa 0 lugares (todavia no sabemos que tamaño tendra)
	memset(buffer, 0, 1* sizeof(char));

	while (!mensajeCompleto) // Mientras que no se haya recibido el mensaje completo
	{
		memset(bufferAux, 0, BUFFERSIZE* sizeof(char)); //-> llenamos el bufferAux con barras ceros.

		//Nos ponemos a la escucha de las peticiones que nos envie el cliente. //aca si recibo 0 bytes es que se desconecto el otro, cerrar el hilo.
		if ((*bytesRecibidos = *bytesRecibidos + recv(socket, bufferAux, BUFFERSIZE, 0)) == -1)
			Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d", socket);

		cantidadBytesRecibidos = strlen(bufferAux);
		cantidadBytesAcumulados = strlen(buffer);
		tamanioNuevoBuffer = cantidadBytesRecibidos + cantidadBytesAcumulados;

		if (tamanioNuevoBuffer > 0)
		{
			buffer = realloc(buffer, tamanioNuevoBuffer * sizeof(char)); //--> el tamaño del buffer sera el que ya tenia mas los caraceteres nuevos que recibimos

			sprintf(buffer, "%s%s", (char*) buffer, bufferAux); //--> el buffer sera buffer + nuevo recepcio
		}

		//Verificamos si terminó el mensaje
		if (cantidadBytesRecibidos < BUFFERSIZE)
			mensajeCompleto = 1;
	}

	Traza("RECIBO datos. socket: %d. buffer: %s", socket, (char*) buffer);

	return buffer; //--> buffer apunta al lugar de memoria que tiene el mensaje completo completo.
}

char* ComandoHandShake2(char *buffer, int *tipoCliente)
{
// Formato del mensaje: CD
// C = codigo de mensaje ( = 3)

	int tipoDeCliente = posicionDeBufferAInt(buffer, 1);

	if (tipoDeCliente == TIPO_CPU)
	{
		*tipoCliente = tipoDeCliente;
		buffer = RespuestaClienteOk(buffer);
	}
	else
	{
		*tipoCliente = 0;
		//SetearErrorGlobal("HANDSHAKE ERROR. Tipo de cliente invalido (%d). KERNEL o '2' = CPU.", posicionDeBufferAInt(buffer, 1));
		//buffer = RespuestaClienteError(buffer, g_MensajeError);
	}

	return buffer;
}

int posicionDeBufferAInt(char* buffer, int posicion)
{
	int logitudBuffer = 0;
	logitudBuffer = strlen(buffer);

	if (logitudBuffer <= posicion)
		return 0;
	else
		return chartToInt(buffer[posicion]);
}

char* RespuestaClienteOk(char *buffer)
{
	int tamanio = sizeof(char) * 2;
	buffer = realloc(buffer, tamanio* sizeof(char));
	memset(buffer, 0, tamanio* sizeof(char));
	sprintf(buffer, "%s", "1");
	return buffer;
}


void seminit(psem_t *ps, int n) {
	pthread_mutex_init(&ps->semdata, NULL );

	pthread_mutex_init(&ps->sem_mutex, NULL );

	/* para inicializar el semaforo binario a 0 */

	pthread_mutex_lock(&ps->sem_mutex);

	ps->n = n;
}

void semwait(psem_t *ps) {

	pthread_mutex_lock(&ps->semdata);

	ps->n--;
	if (ps->n < 0) {
		pthread_mutex_unlock(&ps->semdata);

		pthread_mutex_lock(&ps->sem_mutex);
	}

	else
		pthread_mutex_unlock(&ps->semdata);

}

void semsig(psem_t *ps) {
	pthread_mutex_lock(&ps->semdata);

	ps->n++;
	if (ps->n < 1)
		pthread_mutex_unlock(&ps->sem_mutex);

	pthread_mutex_unlock(&ps->semdata);
}

void semdestroy(psem_t *ps) {
	pthread_mutex_destroy(&ps->semdata);
	pthread_mutex_destroy(&ps->sem_mutex);
}

