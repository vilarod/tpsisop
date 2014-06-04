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

//#include <sys/stat.h>
//#include <fcntl.h>

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/PROGRAMA/src/config.cfg"

//Tipo de servidor conectado
//#define  TIPO_KERNEL       1


//Mensajes aceptados
#define HANDSHAKE '31'
#define ENVIARPROGRAMA '11'

//Puerto
//#define IP "127.0.0.1"
//#define PORT "6007"

#define MAXLONG 1024
int ObtenerPuertoKernel();
char* ObtenerIpKernel();


int main(int argc, char* argv[])
{


    int index;    //para parametros

    for(index = 0; index < argc; index++)//parametros
     	printf("  Parametro %d: %s\n", index, argv[index]);

    //argv[0] es el path: /home/utnso/tp-2014/1c-garras/PROGRAMA/Debug/PROGRAM/
    //argv[1] es el nombre del programa

    //se creo el directorio ansisop en /usr/bin con sudo mkdir ansisop
    //se uso sudo para poder ejecutar comandos que requieren permisos de administrador
    /*El symbolic link se hizo por consola:
      	  sudo ln -s /home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/PROGRAMA /usr/bin/ansisop*/




    FILE* f;
    char* contents;
    size_t len;
    size_t bytesRead;

    f = fopen(argv[1], "r");//abre el archivo en modo read
    if (f == NULL) {
    	fprintf(stderr, "Error opening file: %s", argv[1]);//No existe el archivo
    	return 1;
     }
    fseek(f, 0, SEEK_END);//nos situamos al final del archivo
    len = ftell(f);//nos da la cantidad de bytes del archivo
    rewind(f);


    contents = (char*) malloc(sizeof(char) * len + 1);//leer lo que contiene
    contents[len] = '\0'; // solo es necesario para imprimir la salida con printf
    if (contents == NULL) {
    	fprintf(stderr, "Failed to allocate memory");//imprime error sino tiene memoria
    return 2;
    }

    bytesRead = fread(contents, sizeof(char), len, f);

    txt_close_file(f); //cierra el archivo


    printf("File length: %d, bytes read: %d\n", len, bytesRead);//imprime la cantidad de bytes del archivo
    printf("Contents:%s",contents);

    char **linea;
    char *separator;
    char* nuevo=(char*)malloc(len*sizeof(char) + 1);//aca guardo el programa como lo recibe el kernel
    strcpy(nuevo, "");
    separator = "\n";
    linea = string_split(contents, separator);//separa el programa ansisop en lineas
    int i;

    for(i=1; linea[i]!= NULL; i++)//elimino la primer linea del shebang
    {
    	//printf("%s",linea[i]);

    	string_append(&nuevo,linea[i]);//concatena todas las lineas del programa

    }
    printf("\n");
    printf("%s", nuevo);//verifico que tengo el programa sin la primer linea
    printf("\n");

    free(contents);
    free(nuevo);


        //ConexionConSocket();

        return 0;
}


int ObtenerPuertoKernel()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_KERNEL");

}

char* ObtenerIpKernel()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "IP_KERNEL");

}

void conectarAKERNEL(){
		Traza("Intentando conectar a umv.");
		socket = ConexionConSocket(KERNEL_PUERTO, KERNEL_IP);
		if (hacerhandshakeUMV(socket) == 0) {
			ErrorFatal("No se pudo conectar al kernel");
		}
}

int hacerhandshakeKERNEL() {
	char *respuestahandshake = string_new();
	char *mensaje = "31";

	EnviarDatos(socket, mensaje);
	RecibirDatos(socket, respuestahandshake);
	return analisarRespuestaUMV(respuestahandshake);

}
int analizarRespuestaKERNEL(char *mensaje) {
	if (mensaje[0] == 0) {
		Error("eL KERNEL nos devolvio un error: %s", mensaje);
		return 0;
	} else
		return 1;
}
int ConexionConSocket(int puerto, char* IP) {
	int socket;
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
