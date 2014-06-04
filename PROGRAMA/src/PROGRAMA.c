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
		socket = ConexionConSocket(7000, ObtenerIpKernel());//el puerto es de la umv para probar
		if (hacerhandshakeKERNEL(socket) == 0) {
			ErrorFatal("No se pudo conectar al kernel");
		}
}
	
int analizarRespuestaKERNEL(char *mensaje) {
	if (mensaje[0] == 0) {
		Error("eL KERNEL nos devolvio un error: %s", mensaje);
		return 0;
	} else
		return 1;
}

void txt_close_file(FILE* file) {

   	fclose(file);
}

