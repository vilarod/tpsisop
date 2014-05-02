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

//includes para sockets
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/CPU/src/config.cfg"

//Mi PUERTO e IP
#define MI_PUERTO 0 //Que elija cualquier puerto que no esté en uso
#define MI_IP INADDR_ANY //Que use la IP de la maquina en donde ejecuta

//IP y Puerto destino de UMV, de momento lo pongo como variable global
//Los tomo por archivo configuracion
int UMV_PUERTO;
char* UMV_IP;


void ConexionConSocket()
{
	int socketConec;
	struct sockaddr_in my_addr;
	struct sockaddr_in dest_UMV; //Con esto me quiero conectar con UMV
	socketConec = socket(AF_INET,SOCK_STREAM,0);
		//Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
	   if(socketConec == -1)
		      error(1, "Este socket tiene errores!");
	//Le pongo mis valores a mi socket
	my_addr.sin_family=AF_INET;
	my_addr.sin_port= htons(MI_PUERTO);
	my_addr.sin_addr.s_addr= MI_IP;
	//Le pongo valores de la UMV
	dest_UMV.sin_family=AF_INET;
	dest_UMV.sin_port=htons(UMV_PUERTO);
	dest_UMV.sin_addr.s_addr= MI_IP; //esto tengo que cambiarlo despues
		//Controlo si el puerto está en uso (igual me prometio beej que con 0 tomaba un puerto libre)
	   if ((bind(socketConec,(struct sockaddr*)&my_addr,sizeof(struct sockaddr)))==-1)
		      error(2,"Este puerto está ocupado!");
	   //Controlo si puedo conectarme
	   if ((connect(socketConec,(struct sockaddr*)&dest_UMV,sizeof(struct sockaddr)))==-1)
		      error(3,"No me puedo conectar!");



}

int main(void) {


	UMV_PUERTO = ObtenerPuertoUMV();
	UMV_IP = ObtenerIPUMV();
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	printf("El puerto de la memoria es: %d ", UMV_PUERTO);
	printf("El IP de la memoria es: %s ", UMV_IP);
		return EXIT_SUCCESS;

	}

//Para enviar datos

int Enviar (int sRemoto, void * buffer)
{
	int cantBytes;
	cantBytes= send(sRemoto,buffer,strlen(buffer),0);
	if (cantBytes ==-1)
		error(4,"No lo puedo enviar todo junto!");
	return cantBytes;

}

//Para recibir datos

int Recibir (int sRemoto, void * buffer)
{
	int cantBytes;
	cantBytes = recv(sRemoto,buffer,strlen(buffer),0);
	if (cantBytes == -1)
		error(5,"Surgió un error al recibir los datos!");
	if (cantBytes == 0)
		error(6,"El proceso remoto se desconecto!");
	return cantBytes;

}

//Para leer desde el archivo de configuracion

int ObtenerPuertoUMV()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_UMV");
}

char* ObtenerIPUMV()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_string_value(config, "IP_UMV");
}


void error(int code, char *err)
{
  char *msg=(char*)malloc(strlen(err)+14);
  sprintf(msg, "Error %d: %s\n", code, err);
  fprintf(stderr, "%s", msg);
  exit(1);
}
