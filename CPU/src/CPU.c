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



#include <fcntl.h>
#include <resolv.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/CPU/src/config.cfg"

//Mi PUERTO e IP
#define MI_PUERTO 0 //Que elija cualquier puerto que no esté en uso
#define MI_IP INADDR_ANY //Que use la IP de la maquina en donde ejecuta

//Puerto destino de UMV, de momento lo pongo como variable global
//Lo tomo por archivo configuracion
int UMV_PUERTO;




void ConexionConSocket()
{
	int socketConec;
	//struct hostent *soy_yo;
	struct sockaddr_in dest_UMV; //Con esto me quiero conectar con UMV
	socketConec = socket(AF_INET,SOCK_STREAM,0);
		//Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
	   if(socketConec == -1)
		      perror("Este socket tiene errores!");

	//Le pongo valores de la UMV
	dest_UMV.sin_family=AF_INET;
	dest_UMV.sin_port=htons(UMV_PUERTO);
	dest_UMV.sin_addr.s_addr= MI_PUERTO;

	   //Controlo si puedo conectarme
	   if ((connect(socketConec,(struct sockaddr*)&dest_UMV,sizeof(struct sockaddr)))==-1)
		      perror("No me puedo conectar!");

	   puts("Entre a conexionConSocket!");


	}

//Para enviar datos

int Enviar (int sRemoto, void * buffer)
{
	int cantBytes;
	cantBytes= send(sRemoto,buffer,strlen(buffer),0);
	if (cantBytes ==-1)
		perror("No lo puedo enviar todo junto!");
	return cantBytes;

}

//Para recibir datos

int Recibir (int sRemoto, void * buffer)
{
	int cantBytes;
	cantBytes = recv(sRemoto,buffer,strlen(buffer),0);
	if (cantBytes == -1)
		perror("Surgió un error al recibir los datos!");
	if (cantBytes == 0)
		perror("El proceso remoto se desconecto!");
	return cantBytes;

}

//Cerrar conexión

void Cerrar (int sRemoto)
{
	close(sRemoto);
}

//Para leer desde el archivo de configuracion

int ObtenerPuertoUMV()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_UMV");
}




int main(void) {


	UMV_PUERTO = ObtenerPuertoUMV();
	printf("El puerto de la memoria es: %d ", UMV_PUERTO);
	ConexionConSocket();

		return EXIT_SUCCESS;

	}





// Primitivas
//esto es solamente un bosquejo para ir armandolas


void asignar(t_puntero direccion_variable, t_valor_variable valor)
{
	//direccion_variable=valor;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable)
{
	t_valor_variable valor;
	valor=1; //inicializo de prueba

	//Acá tengo que obtener el valor de la variable mediante la sys call al kernel
	// obtener_valor(variable);

	return valor; //devuelve el valor de la variable
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor)
{
	t_valor_variable valor_asig;
		valor_asig=1; //inicializo de prueba

	//Acá también uso sys call a kernel
	// grabar_valor(variable,valor);

	return valor_asig; //devuelve el valor asignado
}

t_puntero_instruccion llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
	t_puntero_instruccion instr;
     instr=1; //inicializo de prueba

	//preservar el contexto de ejecucion actual y resetear estructuras. necesito un contexto vacio ahora

	return instr;
}


t_puntero_instruccion llamarConRetorno(t_nombre_etiqueta etiqueta,
					  t_puntero donde_retornar)
{
	t_puntero_instruccion instr;
	instr=1; //inicializo de prueba

	//preservar el contexto actual para retornar al mismo

	return instr;
}

t_puntero_instruccion finalizar(void)
{
	t_puntero_instruccion instr;
	instr=1; //inicializo de prueba
	//recuperar pc y contexto apilados en stack.Si finalizo, devolver -1

	return instr;
}

t_puntero_instruccion retornar (t_valor_variable retorno)
{
	t_puntero_instruccion instr;
	instr=1; //inicializo de prueba
	//acá tengo que volver a retorno

	return instr;
}

int imprimir (t_valor_variable valor_mostrar)
{
	int cant_caracteres;
	cant_caracteres=1; //inicializo de prueba

	//acá me conecto con el kernel y le paso el mensaje

	return cant_caracteres;
}

t_valor_variable dereferenciar(t_puntero direccion_variable)
{
	t_valor_variable valor;
	valor=1;//inicializo de prueba

	//tiene que leer el valor que se guarda en esa posición

	return valor; //devuelve el valor encontrado
}

t_puntero_instruccion irAlLabel(t_nombre_etiqueta etiqueta)
{
	t_puntero_instruccion primer_instr;
	primer_instr=1;
	//busco la primer instruccion ejecutable

	return primer_instr; //devuelve la instruccion encontrada
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
	t_puntero posicion;
	posicion=1; //inicializo de prueba
	//busco la posicion de la variable

	return posicion; //devuelvo la posicion
}

t_puntero definirVariable(t_nombre_variable identificador_variable)
{
	t_puntero pos_var_stack;
	pos_var_stack=1; //inicializo de prueba
	// reserva espacio para la variable,
	//la registra en el stack y en el diccionario de variables

	return pos_var_stack; //devuelvo la pos en el stack
}

int imprimirTexto(char* texto)
{
	int cant_caracteres;
		cant_caracteres=1; //inicializo de prueba

		//acá me conecto con el kernel y le paso el mensaje

		return cant_caracteres;
}

int entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
	int tiempo2;
	tiempo2=1; //inicializo de prueba

	//acá pido por sys call a kernel
	// entrada_salida(dispositivo,tiempo);

	return tiempo2;
}

/*
int wait(t_nombre_semaforo identificador_semaforo)
{
	int bloquea;
	bloquea=0;//incializo de prueba

	//sys call con kernel
	// wait(identificador_semaforo);

	return bloquea;
}

int signal(t_nombre_semaforo identificador_semaforo)
{
	int desbloquea;
	desbloquea=1; //inicializo de prueba

	//sys call con kernel
	// signal(identificador_semaforo);

	return desbloquea;
}
*/
