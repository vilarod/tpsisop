/*
 ============================================================================
 Name        : UMV.c
 Author      : Garras
 Version     : 1.0
 Copyright   : Garras - UTN FRBA 2014
 Description : Hello World in C, Ansi-style
 Testing	 : Para probarlo es tan simple como ejecutar en el terminator la linea "$ telnet localhost 7000" y empezar a dialogar con el UMV.
 ============================================================================
 */

#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/config.h>
#include <string.h>
#include <commons/string.h>

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


int main(int argv, char** argc)
{
		printf("holaaaa gente");
		
	   return EXIT_SUCCESS;
}
