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
//para el socket
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "PROGRAMA.h"


//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/PROGRAMA/src/config.cfg"

//Tipo de cliente conectado
#define  TIPO_KERNEL       1


//Mensajes aceptados
#define MSG_HELLO          1
#define MSG_SEND_PROGRAM   2
#define MSG_PRINT          3
#define MSG_PRINT_TEXT     4
#define MSG_NO_MEMORY      5
#define MSG_NO_CPU         6
#define MSG_SHOW_VARIABLE  7

//Puerto
#define IP "127.0.0.1"
#define PUERTO "6667"
#define PACKAGESIZE 10240

//int conectar();
//FILE* txt_open_for_append(char* );
//void txt_write_in_file(FILE* file, char* bytes);
//void txt_write_in_stdout(char* string);
void txt_close_file(FILE* file);

int main(int argc, char* argv[]) {
	int i;
	size_t len;
    size_t bytesRead;
    char* contents;
    FILE* f;
    char* programa;


    int index;    //para interprete

    for(index = 0; index < argc; index++)//interprete
     	printf("  Parametro %d: %s\n", index, argv[index]);

        //me imprime el path: /home/utnso/tp-2014/1c-garras/PROGRAMA/Debug/PROGRAM
        //el path es argv[0] ahi tengo programa1.ansisop

    programa = (char*)malloc(strlen(argv[1])+1);//este es el programa que tengo que enviar al kernel
    strcpy(programa,argv[1]);



    f = fopen(programa, "r");//abre el archivo en modo read
    if (f == NULL) {
	fprintf(stderr, "Error opening file: %s", argv[1]);//No existe el archivo
	return 1;
    }


    fseek(f, 0, SEEK_END);//para saber el tamaño
    len = ftell(f);
    rewind(f);


    contents = (char*) malloc(sizeof(char) * len + 1);//leer lo que contiene
    contents[len] = '\0'; // solo es necesario para imprimir la salida con printf
    if (contents == NULL) {
	fprintf(stderr, "Failed to allocate memory");//imprime error sino tiene memoria
	return 2;
    }

    bytesRead = fread(contents, sizeof(char), len, f);

    txt_close_file(f); //cierra el archivo


    printf("File length: %d, bytes read: %d\n", len, bytesRead);
    printf("Contents:\n");

    if((contents[0]=='#') && (contents[1]=='!')){
    	for(i=2; i <= len; i++)
    	printf("%c", contents[i]);//muestra en pantalla el programa1.ansisop sin #!

    free(contents);
    }

    /*int sockfd; //descriptor del socket
    int leng; //tamanio de la estructura
    struct sockaddr_in address; //declaracion de la estructura
    int result; //resultado de la conexion
    char ch[1024]; //cadena del servidor
    char c[1024]; //cadena del cliente
    int buf; //almacenamiento del tamanio del cliente
    int inicio = 0; //indicador inicio de sesion
    char cs[1024]; //cadena del server
    int bufs; //almacenamiento del tamanio cadena del server
    int ciclo=0;//variable para ciclo lectura
    char ipserver[10];
    int puerto;

    system("clear");
    printf("direccion IP del servidor a conectarse: "); //solicitando IP servidor
    scanf("%s", ipserver); //leer IP del server
    printf("puerto para conexion: "); //solicitando el puerto
    scanf("%d", &puerto); //leer el puerto
    system("clear");
    //ciclo que permite el envio de info de forma indefinida
    while(ciclo == 0){
    	//crear socket de tipo AF_INET
    	sockfd = socket(AF_INET, SOCK_STREAM, 0);//llamado de la estructura con datos
    	address.sin_family = AF_INET; //tipo estructura
    	address.sin_addr = inet_addr(ipserver);//ver cual es el error??
    	if(inicio == 0){
    		printf("\n-------------------SESION INICIADA------------------------\n");
    		read(sockfd, ch, 1024);//leer la cadena del servidor
    		printf("%s\n", ch); //imprimir la cadena
    		inicio = 1; //cambiar valor de variable sesion
    	}
    //leer del teclado la informacion de envio al server
    	printf("cliente dice:");
    	scanf("%c[^\n]", c);//leer la cadena con espacios incluidos
    	buf = strlen(c); //obtener el tamanio de la cadena
    	if((c[0]=='A' && c[1]== 'D' && c[2]=='I' && c[3]=='O' && c[4]=='S'&& c[5]== '\0')||
    	   (c[0]=='a' && c[1]== 'd' && c[2]=='i' && c[3]=='o' && c[4]=='s' && c[5]== '\0')){
    		printf("cliente finalizado\n");
    		printf("\n-----------------SESION FINALIZADA--------------------------\n");
    		write(sockfd, "El cliente se ha desconectado", 30);
    		close(sockfd); //cerrar el descriptor del socket
    		break;
    	}
    	write(sockfd, c, buf + 1); //envio informacion al servidor
    	read(sockfd, cs, 1024); //leer la cadena del servidor
    	printf("servidor dijo: %s", cs); //imprimir la cadena
    	close(sockfd); //cerrar el descriptor del socket*/
       if(conectar() != 0){
       	printf("ERRROR NO SE PUEDE CONECTAR");
       }
        return 0;
}



int ObtenerTamanioMemoriaConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "TAMANIO_MEMORIA");

}
/*FILE* txt_open_for_append(char* archivo ) {
   	return fopen(archivo, "a");
   	if(archivo = NUll)
   		printf("ERROR NO SE PUEDE ABRIR EL ARCHIVO");

}

void txt_write_in_file(FILE* file, char* bytes) {
   	fprintf(file, "%s", bytes);
   	fflush(file);
   }

void txt_write_in_stdout(char* string) {
   	printf("%s", string);
   	fflush(stdout);
   }*/

void txt_close_file(FILE* file) {
   	fclose(file);
}

int conectar(){

		struct addrinfo hints;
		struct addrinfo *serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
		hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

		getaddrinfo(IP, PUERTO, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion


		/*
		 * 	Ya se quien y a donde me tengo que conectar... ¿Y ahora?
		 *	Tengo que encontrar una forma por la que conectarme al server... Ya se! Un socket!
		 *
		 * 	Obtiene un socket (un file descriptor -todo en linux es un archivo-), utilizando la estructura serverInfo que generamos antes.
		 *
		 */
		int serverSocket;
		serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

		/*
		 * 	Perfecto, ya tengo el medio para conectarme (el archivo), y ya se lo pedi al sistema.
		 * 	Ahora me conecto!
		 *
		 */
		connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen);
		freeaddrinfo(serverInfo);	// No lo necesitamos mas

		/*
		 *	Estoy conectado! Ya solo me queda una cosa:
		 *
		 *	Enviar datos!
		 *
		 *	Vamos a crear un paquete (en este caso solo un conjunto de caracteres) de size PACKAGESIZE, que le enviare al servidor.
		 *
		 *	Aprovechando el standard immput/output, guardamos en el paquete las cosas que ingrese el usuario en la consola.
		 *	Ademas, contamos con la verificacion de que el usuario escriba "exit" para dejar de transmitir.
		 *
		 */
		int enviar = 1;
		char message[PACKAGESIZE];

		printf("Conectado al servidor. Bienvenido al sistema, ya puede enviar mensajes. Escriba 'exit' para salir\n");

		while(enviar){
			fgets(message, PACKAGESIZE, stdin);			// Lee una linea en el stdin (lo que escribimos en la consola) hasta encontrar un \n (y lo incluye) o llegar a PACKAGESIZE.
			if (!strcmp(message,"exit")) enviar = 0;			// Chequeo que el usuario no quiera salir
			if (enviar) send(serverSocket, message, strlen(message) + 1, 0); 	// Solo envio si el usuario no quiere salir.
		}


		/*
		 *	Listo! Cree un medio de comunicacion con el servidor, me conecte con y le envie cosas...
		 *
		 *	...Pero me aburri. Era de esperarse, ¿No?
		 *
		 *	Asique ahora solo me queda cerrar la conexion con un close();
		 */

		close(serverSocket);
		return 0;

		/* ADIO'! */
}
