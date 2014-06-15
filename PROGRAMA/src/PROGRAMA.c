/*
 ============================================================================
 Name        : PROGRAMA.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include<commons/txt.h>
#include<commons/string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include "PROGRAMA.h"
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/PROGRAMA/src/config.cfg"

//Tipo de servidor conectado: KERNEL

//Mensajes 	enviados
#define HANDSHAKE "31"
#define ENVIARPROGRAMA "11"
#define CONFIRMACION "33"

//Mensajes aceptados
//MSJ_IMPRIMI_ESTO comienza con 2
//MSJ_FIN_DE_EJECUCION 4

//Tamaño buffer
#define BUFFERSIZE 1024

//Puerto
//#define IP "127.0.0.1"
//#define PORT "6007"
int ImprimirRespuesta(char *texto);


int main(int argc, char* argv[]) {

	int index;    //para parametros

	for (index = 0; index < argc; index++)    //parametros
		printf("  Parametro %d: %s\n", index, argv[index]);

	//argv[0] es el path: /home/utnso/tp-2014/1c-garras/PROGRAMA/Debug/PROGRAM/
	//argv[1] es el nombre del programa

	//se creo el directorio ansisop en /usr/bin con sudo mkdir ansisop
	//se uso sudo para poder ejecutar comandos que requieren permisos de administrador
	//El symbolic link se hizo por consola:
	// sudo ln -s /home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/PROGRAMA /usr/bin/ansisop*/

	FILE* f; //archivo que voy a leer
	char* contents=NULL;
	size_t len;
	size_t bytesRead;

	f = fopen(argv[1], "r");    //abre el archivo en modo read
	if (f == NULL ) {
		fprintf(stderr, "Error opening file: %s", argv[1]); //No existe el archivo
		return 1;
	}
	fseek(f, 0, SEEK_END);    //nos situamos al final del archivo
	len = ftell(f);    //nos da la cantidad de bytes del archivo
	rewind(f);

	contents = (char*) malloc(sizeof(char) * len + 1);    //para guardar lo que lee
	contents[len] = '\0'; // para indicar que termina el texto
	if (contents == NULL ) {
		fprintf(stderr, "Failed to allocate memory"); //imprime error sino tiene memoria
		return 2;
	}

	bytesRead = fread(contents, sizeof(char), len, f); //bytes leidos

	printf("File length: %d, bytes read: %d\n", len, bytesRead); //imprime la cantidad de bytes del archivo
	printf("Contents:\n %s", contents); //imprime el programa tal como esta en el script
	txt_close_file(f);//cierro el archivo

	char **linea;
	char *separator = NULL;
	char *nuevo = NULL;
	nuevo = (char*) malloc(len * sizeof(char) + 1); //aca guardo el programa sin "\n"
	strcpy(nuevo, "");
	separator = "\n";

	linea = string_split(contents, separator); //separa el programa ansisop en lineas

	int i;

	for (i = 1; linea[i] != NULL ; i++) //elimino la primer linea del hashbang
			string_append(&nuevo, linea[i]); //concatena todas las lineas del programa


	printf("\n");
	printf("%s", nuevo); //verifico que tengo el programa sin la primer linea
	printf("\n");

	char *programa = NULL;
	programa = (char*) malloc(len * sizeof(char)); //aca guardo el programa que envio al kernel
	programa = strdup(nuevo);
	int largo;
	largo = strlen(programa);
	printf("el tamanio del programa es: %d\n", largo);
	//string_append(&programa, "\0");//si tengo que agregar el "\0" agrego esta linea y sumo 2 en el malloc

	conectarAKERNEL(programa);

	free(contents);
	free(nuevo);
	free(programa);
	contents = NULL;
	nuevo = NULL;
	programa = NULL;
	return 0;
}

int ObtenerPuertoKERNEL() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_KERNEL");

}

char* ObtenerIpKERNEL() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_string_value(config, "IP_KERNEL");

}

void conectarAKERNEL(char *archivo) { //tengo que agregar programa para poder enviarlo
	Traza("Intentando conectar a kernel");
	int soquete = ConexionConSocket(5000, "127.0.0.1"); //el puerto 5000 y el ip 127.0.01 son para probar
	if (hacerhandshakeKERNEL(soquete, archivo) == 0) { //donde esta el puerto y la ip va: ObtenerPuertoKERNEL()y ObtenerIpKERNEL()
		ErrorFatal("No se pudo conectar al kernel");
	}
}
int hacerhandshakeKERNEL(int sockfd, char *programa) {
	char respuestahandshake[BUFFERSIZE];

	EnviarDatos(sockfd, HANDSHAKE); //HANDSHAKE reemplaza a "31"
	RecibirDatos(sockfd, respuestahandshake);
	if(respuestahandshake[0] == '0')
	{
		printf("Error del KERNEL");
		exit(1);
	}
    AnalizarRespuestaKERNEL(respuestahandshake);

	EnviarDatos(sockfd, programa); //envio el programa
	RecibirDatos(sockfd, respuestahandshake);
	if(respuestahandshake[0] == '0')
	{
			printf("Error del KERNEL");
			exit(1);
	}

	int finDeEjecucion;
	finDeEjecucion = AnalizarSiEsFinDeEjecucion(respuestahandshake);
	while (finDeEjecucion!=0) {
		RecibirDatos(sockfd, respuestahandshake);

		finDeEjecucion = AnalizarSiEsFinDeEjecucion(respuestahandshake);

		if (finDeEjecucion != 0)
		{
			AnalizarRespuestaKERNEL(respuestahandshake);
			ImprimirRespuesta(respuestahandshake);
		    EnviarConfirmacionDeRecepcionDeDatos(sockfd);
		}

	}
	txt_write_in_stdout("Fin de ejecucion");

	return AnalizarRespuestaKERNEL(respuestahandshake);
}
int ImprimirRespuesta(char *mensaje)
{

	if(mensaje[0] == '2')
		printf("%s", (mensaje + 1));

	return 0;
}

int EnviarConfirmacionDeRecepcionDeDatos(sockfd) {

	EnviarDatos(sockfd, CONFIRMACION);
	return 0;
}

int AnalizarSiEsFinDeEjecucion(char *respuestahandshake) {

	if (respuestahandshake[0] == '4')
		return 0;
	else
		return 1;
}

int AnalizarRespuestaKERNEL(char *mensaje) {
	if (mensaje[0] == '0') {
		Error("eL KERNEL nos devolvio un error: %s", mensaje);
		return 0;
	} else
		return 1;
}
int ConexionConSocket(int puerto, char* IP) { //crea el socket y me retorna el int
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
	//ordenacion de bytes de la red es htons = "h" de ordenacion de la maquina [Host Byte Order], "to" (a), "n" de "network" y "s" de "short"
	their_addr.sin_family = AF_INET; //tipo de conexion
	their_addr.sin_port = htons(puerto); //tipo de servicio puerto
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

//Nos ponemos a la escucha de las peticiones que nos envie el kernel //aca si recibo 0 bytes es que se desconecto el otro, cerrar el hilo.
	if ((bytecount = recv(socket, buffer, BUFFERSIZE, 0)) == -1)
		Error(
				"Ocurrio un error al intentar recibir datos el kernel. Socket: %d",
				socket);

	Traza("RECIBO datos. socket: %d. buffer: %s", socket, (char*) buffer);
    return bytecount;
}

int EnviarDatos(int socket, void *buffer) {
	int bytecount;

	if ((bytecount = send(socket, buffer, strlen(buffer), 0)) == -1)
		Error("No puedo enviar información al kernel. Socket: %d", socket);

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
