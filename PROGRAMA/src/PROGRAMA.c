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

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/UMV/src/config.cfg"

int main(int argc, char* argv[]) {
    if (argc < 2) {
	printf("Path: %s programa1.ansisop\n", argv[0]);//me imprime la ruta hasta el programa.ansisop
	return 1;
    }

    size_t len;
    size_t bytesRead;
    char* contents;
    FILE* f;

    f = fopen(argv[1], "r");//abre el archivo en modo read
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


    fclose(f);//cierra el archivo

    printf("File length: %d, bytes read: %d\n", len, bytesRead);
    printf("Contents:\n%s", contents);

    free(contents);
    return 0;
}


int ObtenerTamanioMemoriaConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "TAMANIO_MEMORIA");
}
