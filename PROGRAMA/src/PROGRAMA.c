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
#define putchar() putc((c), stdout)
#define getchar() getc(stdin)
//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/UMV/src/config.cfg"
void filecopy(FILE*, FILE*);

int main(void) {

	/*int tamanioMemoria = ObtenerTamanioMemoriaConfig();*/
	FILE* ptr_fichero;                      //creo un puntero de tipo FILE
	char* ptr_nombre = "programa1.ansisop"; //creo un puntero que apunta a programa1.ansisop
	ptr_fichero = fopen(ptr_nombre, "r");   //abre el archivo en modo lectura
	printf("Archivo: %s -> ", ptr_nombre);
	if(ptr_fichero != NULL)
	{
		printf("creado(ABIERTO)\n");      //abre el archivo
	    filecopy(ptr_fichero, stdout);    //copia el archivo
	}
	else
	{
		printf("Error\n");
		return 1;
	}

}
void filecopy(FILE *ifp, FILE *ofp)//quiero copiar el archivo para ver lo que leyo
{	int c;                         //no me da ningun resultado
    while((c = getc(ifp))!= EOF)
          putc((c),ofp);

}

int ObtenerTamanioMemoriaConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "TAMANIO_MEMORIA");
}
