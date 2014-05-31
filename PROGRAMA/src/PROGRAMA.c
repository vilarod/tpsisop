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
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h> //stat(), fstat(), lstat()
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
#define MAXLONG 1000
//int conectar();
//FILE* txt_open_for_append(char* );
void txt_close_file(FILE* file);

int main(int argc, char* argv[]) {
    /*int i;
	size_t len;
    size_t bytesRead;
    char* contents;
    FILE* f;*/


    int index;    //para interprete

    for(index = 0; index < argc; index++)//interprete
     	printf("  Parametro %d: %s\n", index, argv[index]);

        //argv[0] es el path: /home/utnso/tp-2014/1c-garras/PROGRAMA/Debug/PROGRAM/
        //argv[1] es el nombre del programa

    	//*se creo el directorio ansisop en /usr/bin con sudo mkdir ansisop */
    	//se uso sudo para poder ejecutar comandos que requieren permisos de administrador
    	/*El symbolic link se hizo por consola:
      	  sudo ln -s /home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/PROGRAMA /usr/bin/ansisop*/

    	char* path;
    	char* path1 = "/home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/";
    	path = strcat(path1, argv[1]);

        struct stat stat_file;
    	stat(path, &stat_file);  //ver que es lo que hace???
    	FILE* file = NULL;

    	file = fopen(path, "r");

    	if (file != NULL) {
    					char* buffer = calloc(1, stat_file.st_size + 1);
    					fread(buffer, stat_file.st_size, 1, file);
    					//aca esta el buffer con el archivo ya cargado. falta sacarle la primer linea.

    		}

    	/*f = malloc(MAXLONG * (sizeof(char)));//asigno memoria al programa.ansisop



    	f = fopen(argv[1], "r");//abre el archivo en modo read
        	if (f == NULL) {
    	fprintf(stderr, "Error opening file: %s", argv[1]);//No existe el archivo
    	//return 1;
        }



        fseek(f, 0, SEEK_END);//para saber el tamaño
        len = ftell(f);
        rewind(f);


        contents = (char*) malloc(sizeof(char) * len + 1);//leer lo que contiene
        contents[len] = '\0'; // solo es necesario para imprimir la salida con printf
        if (contents == NULL) {
    	fprintf(stderr, "Failed to allocate memory");//imprime error sino tiene memoria
    	//return 2;
        }

        bytesRead = fread(contents, sizeof(char), len, f);

        txt_close_file(f); //cierra el archivo


        printf("File length: %d, bytes read: %d\n", len, bytesRead);
        printf("Contents:\n");
        //tengo que hacer que lea por lineas
        if((contents[0]=='#') && (contents[1]=='!')){
        	for(i=2; i <= len; i++)
        	printf("%c", contents[i]);//muestra en pantalla el programa1.ansisop sin #!

        free(contents);
        }*/
        ConexionConSocket();

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
