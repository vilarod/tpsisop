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

#include <sys/stat.h>
#include <fcntl.h>

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/PROGRAMA/src/config.cfg"

//Tipo de cliente conectado
#define  TIPO_KERNEL       1


//Mensajes aceptados


//Puerto
#define IP "127.0.0.1"
#define PORT "6007"
#define PUERTO_KERNEL "7001"
#define PACKAGESIZE 10240
#define MAXLONG 1024
#define N 1000
//FILE* txt_open_for_append(char* );
void txt_close_file(FILE* file);



int main(int argc, char* argv[]) {
    //int i;
	//size_t len;
    //size_t bytesRead;
    //char* contents;
    //FILE* f;


    int index;    //para parametros

    for(index = 0; index < argc; index++)//parametros
     	printf("  Parametro %d: %s\n", index, argv[index]);

        //argv[0] es el path: /home/utnso/tp-2014/1c-garras/PROGRAMA/Debug/PROGRAM/
        //argv[1] es el nombre del programa

    	//*se creo el directorio ansisop en /usr/bin con sudo mkdir ansisop */
    	//se uso sudo para poder ejecutar comandos que requieren permisos de administrador
    	/*El symbolic link se hizo por consola:
      	  sudo ln -s /home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/PROGRAMA /usr/bin/ansisop*/

    	/*char* path;
    	char* path1 = "/home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/";
    	path = strcat(path1, argv[1]);*/

        FILE* file;
        //char* programa=NULL;
          char** lineas;
          int i = 0;



    	file = fopen(argv[1], "r");//abre el archivo en modo read
        //programa = (char*)malloc((1024 * sizeof(char))+ 1);

    	if (file != NULL) {
    					char* buffer = calloc(1,(1024*sizeof(char) + 1));
    					fread(buffer, (1024*sizeof(char) + 1), 1, file);
    					//aca esta el buffer con el archivo ya cargado. falta sacarle la primer linea.
                        printf(" %s\n", buffer);
                        printf("longitud del buffer: %d\n", strlen(buffer));
                        lineas = string_split(buffer, "\n");


   		}
    	else
    		printf("El archivo no existe o esta vacio\n");
    	for(i= 1; lineas[i]!= '\0'; i++)
    	{
    		printf("%s", lineas[i]);
    	    printf("\\n");
    	}
    	printf("\n");
    	//tengo que concatenar las lineas[i] para tener el char* para el kernel




        //ConexionConSocket();

        return 0;
}


int ObtenerTamanioMemoriaConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "TAMANIO_MEMORIA");

}

void txt_close_file(FILE* file) {

   	fclose(file);
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
	//their_addr.sin_port = htons(PORT); // short, Ordenación de bytes de la red
	bcopy (he->h_addr, &(their_addr.sin_addr.s_addr),he->h_length);
//	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero),'\0', 8); // poner a cero el resto de laestructura

	if (connect(sockfd, (struct sockaddr *)&their_addr,
	sizeof(struct sockaddr)) == -1) {
	perror("connect");
	exit(1);
		}
	//conexion que funciona envia y recibe, envia comando por teclado y lo recibe.
	//int numbytes;
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
	//Enviar(sockfd,comando);
	//Recibir(sockfd,mensaje);
	printf("Recibi: %s \n",mensaje);
	}
     //Cerrar(sockfd);
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
	//close(sRemoto);
}
