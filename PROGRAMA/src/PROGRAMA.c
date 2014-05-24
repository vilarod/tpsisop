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
#include <unistd.h>
#include "PROGRAMA.h"


//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/PROGRAMA/src/config.cfg"

//Tipo de cliente conectado
#define  TIPO_KERNEL       1


//Mensajes aceptados


//Puerto
#define IP "127.0.0.1"
#define PUERTO "6667"
#define PACKAGESIZE 10240
#define MAXLONG 1000
//int conectar();
//FILE* txt_open_for_append(char* );
void txt_close_file(FILE* file);

int main(int argc, char* argv[]) {
    int i;
	size_t len;
    size_t bytesRead;
    char* contents;
    FILE* f;


    int index;    //para interprete

    for(index = 0; index < argc; index++)//interprete
     	printf("  Parametro %d: %s\n", index, argv[index]);

        //argv[0] es el path: /home/utnso/tp-2014/1c-garras/PROGRAMA/Debug/PROGRAM/
        //argv[1] es el nombre del programa

    	//*se creo el directorio ansisop en /usr/bin con sudo mkdir ansisop */
    	//se uso sudo para poder ejecutar comandos que requieren permisos de administrador
    	/*El symbolic link se hizo por consola:
      	  sudo ln -s /home/utnso/tp-2014-1c-garras/PROGRAMA/Debug/PROGRAMA /usr/bin/ansisop*/

    	f = (char*)malloc(MAXLONG * (sizeof(char)));//asigno memoria al programa.ansisop
    	//    strcpy(programa,argv[1]);


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
