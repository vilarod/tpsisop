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
#include <parser/metadata_program.h>
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
int KERNEL_PUERTO;


//guardo pcb y q
//struct PCB PCB_prog; //estructura del pcb
int quantum;



void ConexionConSocket(int *Conec,int socketConec,struct sockaddr_in destino)
{


   *Conec=1;
   //Controlo si puedo conectarme
   if ((connect(socketConec,(struct sockaddr*)&destino,sizeof(struct sockaddr)))==-1)
       perror("No me puedo conectar UMV!");

   puts("Entre a conexionConSocket!");


	}

//Para enviar datos

int Enviar (int sRemoto, void * buffer)

{
	int cantBytes;
	cantBytes= send(sRemoto,buffer,strlen(buffer),0);
	if (cantBytes ==-1)
		perror("No lo puedo enviar todo junto!");

	puts("Entre a ENVIAR!");
	printf("%d",cantBytes);

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

int ObtenerPuertoKERNEL()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_KERNEL");
}


int RecibirProceso()
{
	//aca voy a intentar recibir un PCB y su Q
	return 0; //devuelve 0 si no tengo, 1 si tengo
}


/*
char* PedirSentencia(puntero indiceCodigo)
{
	//aca hare algo para enviarle el pedido a la umv y recibir lo solicitado
}
*/

void procesoTerminoQuantum()
{
	//le aviso al kernel que el proceso termino el Q
}

void parsearYejecutar (char* instr)
{

	AnSISOP_funciones * funciones=0; //primitivas...pero, que onda??
		//por lo que vi, tendria que empaquetar todas las primitivas en un tipo
		//ya que el parser hace  funciones.AnSISOP_lafuncionquesea

	AnSISOP_kernel * f_kernel=0; //esto es wait y signal

	//aca hay que invocar al parser y ejecutar las primitivas que
	//estan mas abajito

	analizadorLinea(instr,funciones,f_kernel);

}

void salvarContextoProg()
{
	//necesito actualizar los segmentos del programa en la UMV
	//actualizar el PC en el PCB
}

void limpiarEstructuras()
{
	//destruyo estructuras auxiliares
}

int seguirConectado()

{
	//consultare si he recibido la señal SIGUSR1,de ser asi, CHAU!
	return 1;
}

void AvisarDescAKernel()

{
	//aviso al kernel que me voy :(
}

int crearSocket(int socketConec)
{

	socketConec = socket(AF_INET,SOCK_STREAM,0);
	//Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
	if(socketConec == -1)
		perror("Este socket tiene errores!");
return socketConec;
}

struct sockaddr_in prepararDestino(struct sockaddr_in destino,int puerto,int ip)
{

	 //Con esto me quiero conectar con UMV o KERNEL
	destino.sin_family=AF_INET;
	destino.sin_port=htons(puerto);
	destino.sin_addr.s_addr= ip;

return destino;
}

int main(void)

{
	int socketConexion=0;
	int CONECTADO_UMV=0;
	int tengoProg=0; //esto lo uso para controlar si tengo un prog que ejecutar
	struct sockaddr_in dest_UMV;
	char* sentencia=""; //esto no se de que tipo va a ser, por ahora char*

	//Llegué y quiero leer los datos de conexión
	//donde esta el kernel?donde esta la umv?

	UMV_PUERTO = ObtenerPuertoUMV();
	KERNEL_PUERTO = ObtenerPuertoKERNEL();

	socketConexion=crearSocket(socketConexion);
	dest_UMV=prepararDestino(dest_UMV,UMV_PUERTO,MI_IP);


	//Ahora que se donde estan, me quiero conectar con los dos
	//tendré que tener los descriptores de socket por fuera?CREO QUE SI..
	ConexionConSocket(&CONECTADO_UMV,socketConexion,dest_UMV);

	while (CONECTADO_UMV==1) //mientras estoy conectado...
	{

	    printf("estoy en el while de conectado_umv!!");

	         char* mensaje="";

	      int c=Enviar(socketConexion,"124");
	      printf("%d",c);
	      int r=Recibir(socketConexion,mensaje);
	              printf("%s",mensaje);
	              printf("%d",r);


	      c=Enviar(socketConexion,"322");
	         printf("%d",c);
	          r=Recibir(socketConexion,mensaje);
	          printf("%s",mensaje);
	          printf("%d",r);
	      char* chh="";
	       scanf("%c",chh);


		while (tengoProg==0) //me fijo si tengo un prog que ejecutar
		{
			tengoProg= RecibirProceso();
		}

		//una vez que tengo el programa, mientras el quantum sea mayor
		//a cero tengo que ejecutar las sentencias
		while (quantum > 0)
		{
			//PCB_prog.PC ++; //incremento el program counter
			//sentencia= PedirSentencia(PCB_prog.indiceCodigo); //le pido a la umv la sentencia a ejecutar
			parsearYejecutar(sentencia);
			quantum --; //decremento el Q
		}

		//una vez que el proceso termino el quantum
		//quiero salvar el contexto
		salvarContextoProg();
		procesoTerminoQuantum(); //ahora le aviso al kernel que el proceso
								//ha finalizado
		//el enunciado dice que cuando finalizo la ejecucion de un programa
		//tengo que destruir las estructuras correspondientes
		limpiarEstructuras();
		//cuando termino la ejecución de ese
		CONECTADO_UMV=seguirConectado();
	}

	//me voy a desconectar, asi que... antes le tengo que
	//avisar al kernel asi me saca de sus recursos

	AvisarDescAKernel();

	//Cerrar (conexion); //cierro el socket

 return EXIT_SUCCESS;

	}





// Primitivas
//esto es solamente un bosquejo para ir armandolas


void AnSISOP_asignar(t_puntero direccion_variable, t_valor_variable valor)
{
	//direccion_variable=valor;
}

t_valor_variable AnSISOP_obtenerValorCompartida(t_nombre_compartida variable)
{
	t_valor_variable valor;
	valor=1; //inicializo de prueba

	//Acá tengo que obtener el valor de la variable mediante la sys call al kernel
	// obtener_valor(variable);

	return valor; //devuelve el valor de la variable
}

t_valor_variable AnSISOP_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor)
{
	t_valor_variable valor_asig;
		valor_asig=1; //inicializo de prueba

	//Acá también uso sys call a kernel
	// grabar_valor(variable,valor);

	return valor_asig; //devuelve el valor asignado
}

t_puntero_instruccion AnSISOP_llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
	t_puntero_instruccion instr;
     instr=1; //inicializo de prueba

	//preservar el contexto de ejecucion actual y resetear estructuras. necesito un contexto vacio ahora

	return instr;
}


t_puntero_instruccion AnSISOP_llamarConRetorno(t_nombre_etiqueta etiqueta,
					  t_puntero donde_retornar)
{
	t_puntero_instruccion instr;
	instr=1; //inicializo de prueba

	//preservar el contexto actual para retornar al mismo

	return instr;
}

t_puntero_instruccion AnSISOP_finalizar(void)
{
	t_puntero_instruccion instr;
	instr=1; //inicializo de prueba
	//recuperar pc y contexto apilados en stack.Si finalizo, devolver -1

	return instr;
}

t_puntero_instruccion AnSISOP_retornar (t_valor_variable retorno)
{
	t_puntero_instruccion instr;
	instr=1; //inicializo de prueba
	//acá tengo que volver a retorno

	return instr;
}

int AnSISOP_imprimir (t_valor_variable valor_mostrar)
{
	int cant_caracteres;
	cant_caracteres=1; //inicializo de prueba

	//acá me conecto con el kernel y le paso el mensaje

	return cant_caracteres;
}

t_valor_variable AnSISOP_dereferenciar(t_puntero direccion_variable)
{
	t_valor_variable valor;
	valor=1;//inicializo de prueba

	//tiene que leer el valor que se guarda en esa posición

	return valor; //devuelve el valor encontrado
}

t_puntero_instruccion AnSISOP_irAlLabel(t_nombre_etiqueta etiqueta)
{
	t_puntero_instruccion primer_instr;
	char* ptr_etiquetas="";
	t_size tam_etiquetas=0;


	primer_instr= metadata_buscar_etiqueta(etiqueta,ptr_etiquetas,tam_etiquetas);

	//busco la primer instruccion ejecutable

	return primer_instr; //devuelve la instruccion encontrada
}

t_puntero AnSISOP_obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
	t_puntero posicion;
	posicion=1; //inicializo de prueba
	//busco la posicion de la variable

	return posicion; //devuelvo la posicion
}

t_puntero AnSISOP_definirVariable(t_nombre_variable identificador_variable)
{
	t_puntero pos_var_stack;
	pos_var_stack=1; //inicializo de prueba
	// reserva espacio para la variable,
	//la registra en el stack y en el diccionario de variables

	return pos_var_stack; //devuelvo la pos en el stack
}

int AnSISOP_imprimirTexto(char* texto)
{
	int cant_caracteres;
		cant_caracteres=1; //inicializo de prueba

		//acá me conecto con el kernel y le paso el mensaje

		return cant_caracteres;
}

int AnSISOP_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
	int tiempo2;
	tiempo2=1; //inicializo de prueba

	//acá pido por sys call a kernel
	// entrada_salida(dispositivo,tiempo);

	return tiempo2;
}


int AnSISOP_wait(t_nombre_semaforo identificador_semaforo)
{
	int bloquea;
	bloquea=0;//incializo de prueba

	//sys call con kernel
	// wait(identificador_semaforo);

	return bloquea;
}

int AnSISOP_signal(t_nombre_semaforo identificador_semaforo)
{
	int desbloquea;
	desbloquea=1; //inicializo de prueba

	//sys call con kernel
	// signal(identificador_semaforo);

	return desbloquea;
}
