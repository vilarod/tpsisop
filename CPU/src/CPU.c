/*
 ============================================================================
 Name        : CPU.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include "CPU.h"
#include <parser/parser.h>
#include <errno.h>



#include <fcntl.h>
#include <resolv.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>

//includes para sockets
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/CPU/src/config.cfg"

//Mi PUERTO e IP
#define MI_PUERTO 0 //Que elija cualquier puerto que no esté en uso
#define MI_IP INADDR_ANY //Que use la IP de la maquina en donde ejecuta

//Puerto destino de UMV, de momento lo pongo como variable global
//Lo tomo por archivo configuracion
int UMV_PUERTO;


void ConexionConSocket()
{
	int socketConec;
	//struct hostent *soy_yo;
	struct sockaddr_in dest_UMV; //Con esto me quiero conectar con UMV
	socketConec = socket(AF_INET,SOCK_STREAM,0);
		//Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
	   if(socketConec == -1)
		      perror("Este socket tiene errores!");

	//Le pongo valores de la UMV
	dest_UMV.sin_family=AF_INET;
	dest_UMV.sin_port=htons(UMV_PUERTO);
	dest_UMV.sin_addr.s_addr= MI_PUERTO;

	   //Controlo si puedo conectarme
	   if ((connect(socketConec,(struct sockaddr*)&dest_UMV,sizeof(struct sockaddr)))==-1)
		      perror("No me puedo conectar!");

	   puts("Entre a conexionConSocket!");

	}





int main(void) {


	UMV_PUERTO = ObtenerPuertoUMV();
	printf("El puerto de la memoria es: %d ", UMV_PUERTO);
	ConexionConSocket();
		return EXIT_SUCCESS;

	}

//Para enviar datos

int Enviar (int sRemoto, void * buffer)
{
	int cantBytes;
	cantBytes= send(sRemoto,buffer,strlen(buffer),0);
	if (cantBytes ==-1)
		perror("No lo puedo enviar todo junto!");
	return cantBytes;

}

//Para recibir datos

int Recibir (int sRemoto, void * buffer)
{
	int cantBytes;
	cantBytes = recv(sRemoto,buffer,strlen(buffer),0);
	if (cantBytes == -1)
		perror("Surgió un error al recibir los datos!");
	if (cantBytes == 0)
		perror("El proceso remoto se desconecto!");
	return cantBytes;

}

//Cerrar conexión

void Cerrar (int sRemoto)
{
	close(sRemoto);
}

//Para leer desde el archivo de configuracion

int ObtenerPuertoUMV()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_UMV");
}



