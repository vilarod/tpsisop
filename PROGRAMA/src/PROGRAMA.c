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
#include<commons/log.h>
#include <stdbool.h>
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
#include <error.h>
#include <commons/process.h>
#include <commons/temporal.h>

//Ruta del config
#define PATH_CONFIG "config.cfg"
//#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/config.cfg"

//Tipo de servidor conectado: KERNEL

//Mensajes 	enviados
#define HANDSHAKE "H"
#define ENVIARPROGRAMA "E"
#define CONFIRMACION "S"

//Mensajes aceptados
//MSJ_IMPRIMI_ESTO comienza con "I" y termina con "\0"
//MSJ_FIN_DE_EJECUCION "F"

//Tamaño buffer
#define BUFFERSIZE 1024

//Puerto
//#define IP "127.0.0.1"
//#define PORT "6000"

int main(int argc, char* argv[]) {
	//log
	char* temp_file = tmpnam(NULL );

	logger = log_create(temp_file, "PROGRAMA", ImprimirTrazaPorConsola,
			LOG_LEVEL_TRACE);

	int index;    //para parametros
	for (index = 0; index < argc; index++)    //parametros
		printf("  Parametro %d: %s\n", index, argv[index]);

	//argv[0] es el path: /home/utnso/tp-2014/1c-garras/PROGRAMA/Debug/PROGRAMA/
	//argv[1] es el nombre del programa

	//se creo el directorio ansisop en /usr/bin con sudo mkdir ansisop
	//se uso sudo para poder ejecutar comandos que requieren permisos de administrador
	//El symbolic link se hizo por consola:
	// sudo ln -s /home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/PROGRAMA /usr/bin/ansisop

	FILE* file; //archivo que voy a leer
	char* contents = NULL;
	size_t len;
	size_t bytesRead;
	char* begin = "./";
	char* nombreArchivo = NULL;
	nombreArchivo = (char*) malloc(strlen(argv[1] - 2));

	if (string_starts_with(argv[1], begin))
		nombreArchivo = string_substring_from(argv[1], 2); //nombre del archivo sin ./ adelante
	else
		traza("%s\n", "Error al ingresar el archivo");
	printf("nombre archivo: %s\n", nombreArchivo);
	file = fopen(nombreArchivo, "r");    //abre el archivo en modo read
	if (file == NULL ) {
		traza("No existe el archivo %s\n", argv[1]); //No existe el archivo
		exit(1);
	}
	traza("Abre el archivo:%s\n", argv[1]);
	free(nombreArchivo);
	nombreArchivo = NULL;

	fseek(file, 0, SEEK_END);    //nos situamos al final del archivo
	len = ftell(file);    //nos da la cantidad de bytes del archivo
	rewind(file);

	contents = (char*) malloc(sizeof(char) * len + 1); //para guardar lo que lee
	contents[len] = '\0'; // para indicar que termina el texto
	if (contents == NULL ) {
		traza("%s\n", "Error no hay memoria disponible");
		exit(1);
	}

	bytesRead = fread(contents, sizeof(char), len, file); //bytes leidos
	printf("File length: %d, bytes read: %d\n", len, bytesRead); //imprime la cantidad de bytes del archivo
	traza("El programa en el script es:\n %s\n", contents);

	txt_close_file(file); //cierro el archivo

//	char **linea;
//	char *separator = NULL;
	char *nuevo = NULL;

	nuevo = string_new();

	char* sub;
	sub = string_new();
	int final = 0;
	sub = string_substring(contents, final, 1);
	final++;
	while (string_equals_ignore_case(sub, "\n") == 0) {
		sub = string_substring(contents, final, 1);
		final++;
	}
	free(sub);


	char *programa;

	programa = string_new(); //aca guardo el programa que envio al kernel
	programa = string_substring(contents, final, strlen(contents) - final);
	traza("El programa sin la primer linea:\n %s\n", programa);
	printf("%s", programa); //verifico que tengo el programa sin la primer linea
	printf("\n");

	int largo;
	largo = strlen(programa);
	printf("el tamanio del programa es: %d\n", largo);
	//string_append(&programa, "\0");//si tengo que agregar el "\0" agrego esta linea y sumo 2 en el malloc

	conectarAKERNEL(programa);

	free(nuevo);
	free(programa);

	nuevo = NULL;
	programa = NULL;

	return (EXIT_SUCCESS);

}

int obtenerPuertoKERNEL() {
	t_config* config = config_create(PATH_CONFIG);

	return (config_get_int_value(config, "PUERTO_KERNEL"));

}

char* obtenerIpKERNEL() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_string_value(config, "IP_KERNEL");

}

void conectarAKERNEL(char *archivo) { //el parametro es el programa
	traza("%s\n", "Intentando conectar a kernel");
	int puerto = obtenerPuertoKERNEL(); //descomentar para la prueba en laboratorio
	char *IP = obtenerIpKERNEL(); //idem anterior
	int soquete = conexionConSocket(puerto, IP); // para la prueba en laboratorio

	//int soquete = conexionConSocket(5000, "127.0.0.1"); //el puerto 5000 y el ip 127.0.01 son para prueba local
	if (hacerhandshakeKERNEL(soquete, archivo) == 0) {
		ErrorFatal("No se pudo conectar al kernel");
	}
}
int hacerhandshakeKERNEL(int sockfd, char *programa) {
	char respuestahandshake[BUFFERSIZE];

	enviarDatos(sockfd, HANDSHAKE); //HANDSHAKE ES H
	traza("%s\n", "Envio handshake");
	recibirDatos(sockfd, respuestahandshake);
	if (respuestahandshake[0] == 'N') {
		printf("Error del KERNEL");
		exit(1);
	} else
		traza("%s\n", "Kernel recibio hanshake");
	analizarRespuestaKERNEL(respuestahandshake);
	char* msj = string_new();
	string_append(&msj, "E");
	string_append(&msj, programa);
	traza("%s\n", "Envio programa");
	enviarDatos(sockfd, msj); //envio el programa
	recibirDatos(sockfd, respuestahandshake);
	if (respuestahandshake[0] == 'N') {
		printf("Error del KERNEL");
		exit(1);
	}
	traza("%s", "Kernel recibio el programa");
	int finDeEjecucion;
	finDeEjecucion = analizarSiEsFinDeEjecucion(respuestahandshake);
	while (finDeEjecucion != 0) {
		recibirDatos(sockfd, respuestahandshake);

		finDeEjecucion = analizarSiEsFinDeEjecucion(respuestahandshake);

		if (finDeEjecucion != 0) {
			analizarRespuestaKERNEL(respuestahandshake);
			imprimirRespuesta(respuestahandshake);
			enviarConfirmacionDeRecepcionDeDatos(sockfd);
		}

	}
	txt_write_in_stdout("Fin de ejecucion\n");

	return analizarRespuestaKERNEL(respuestahandshake);

}
int imprimirRespuesta(char *mensaje) {

	if ((string_starts_with(mensaje, "2"))
			&& (string_ends_with(mensaje, "\0"))) {
		printf("%s\n", string_substring(mensaje, 1, (strlen(mensaje) - 4)));
		traza("%s\n", "Se imprime el mensaje enviado por el kernel");
	}

	return 0;
}

int enviarConfirmacionDeRecepcionDeDatos( sockfd) {

	enviarDatos(sockfd, CONFIRMACION);
	traza("%s\n", "Envio confirmacion de recepcion de datos");
	return 0;
}

int analizarSiEsFinDeEjecucion(char *respuestahandshake) {

	if (respuestahandshake[0] == 'F')
		return 0;
	else
		return 1;
}

int analizarRespuestaKERNEL(char *mensaje) {
	if (mensaje[0] == '0') {
		Error("eL KERNEL nos devolvio un error: %s", mensaje);
		return 0;
	} else
		return 1;
}
int conexionConSocket(int puerto, char* IP) { //crea el socket y me retorna el int
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

int recibirDatos(int socket, char *buffer) {
	int bytecount;
// memset se usa para llenar el buffer con 0s
	memset(buffer, 0, BUFFERSIZE);

//Nos ponemos a la escucha de las peticiones que nos envie el kernel //aca si recibo 0 bytes es que se desconecto el otro, cerrar el hilo.
	if ((bytecount = recv(socket, buffer, BUFFERSIZE, 0)) == -1)
		Error(
				"Ocurrio un error al intentar recibir datos el kernel. Socket: %d",
				socket);

	traza("RECIBO datos. socket: %d. buffer: %s\n", socket, (char*) buffer);
	return bytecount;
}

int enviarDatos(int socket, void *buffer) {
	int bytecount;

	if ((bytecount = send(socket, buffer, strlen(buffer), 0)) == -1)
		Error("No puedo enviar información al kernel. Socket: %d\n", socket);

	traza("ENVIO datos. socket: %d. buffer: %s\n", socket, (char*) buffer);

	return bytecount;
}

//void Error1(int code, char *err) {
//	char *msg = (char*) malloc(strlen(err) + 14);
//	sprintf(msg, "Error %d: %s\n", code, err);
//	fprintf(stderr, "%s", msg);
//	exit(1);
//}

//void cerrar(int sRemoto) {
//
//	close(sRemoto);
//}

//void cerrarSocket(int socket) {
//	close(socket);
//	traza("Se cerró el socket (%d).", socket);
//}

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
void traza(const char* mensaje, ...) {
	if (ImprimirTrazaPorConsola) {
		char* nuevo;
		va_list arguments;
		va_start(arguments, mensaje);
		nuevo = string_from_vformat(mensaje, arguments);

		//printf("TRAZA--> %s \n", nuevo);
		log_trace(logger, "%s", nuevo);

		va_end(arguments);

	}
}
