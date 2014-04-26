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






//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/UMV/src/config.cfg"


int main(void) {


	int tamanioMemoria = ObtenerTamanioMemoriaConfig();

}

int ObtenerTamanioMemoriaConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "TAMANIO_MEMORIA");
}
