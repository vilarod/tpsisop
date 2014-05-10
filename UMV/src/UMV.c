/*
 ============================================================================
 Name        : UMV.c
 Author      : Garras
 Version     : 1.0
 Copyright   : Garras - UTN FRBA 2014
 Description : Hello World in C, Ansi-style
 Testing	 : Para probarlo es tan simple como ejecutar en el terminator la linea "$ telnet localhost 7000" y empezar a dialogar con el UMV.
 ============================================================================
 */

#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/config.h>
#include <string.h>
#include <commons/string.h>

#include "UMV.h"

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/UMV/src/config.cfg"

//Tipo de cliente conectado
#define TIPO_KERNEL       1
#define TIPO_CPU       	  2

//Mensajes aceptados
#define MSJ_GET_BYTES             1
#define MSJ_SET_BYTES             2
#define MSJ_HANDSHAKE             3
#define MSJ_CAMBIO_PROCESO        4
#define MSJ_CREAR_SEGMENTO        5
#define MSJ_DESTRUIR_SEGMENTO     6

//Comandos Consola aceptados
#define COMANDO_CONSOLA_HELP				1
#define COMANDO_CONSOLA_LEER_MEMORIA		2
#define COMANDO_CONSOLA_ESCRIBIR_MEMORIA	3
#define COMANDO_CONSOLA_CREAR_SEGMENTO		4
#define COMANDO_CONSOLA_DESTRUIR_SEGMENTO	5

char CMD_HELP[4] = {'H', 'E', 'L', 'P'};
char CMD_LEER_MEMORIA[7] = {'M', 'E', 'M', 'L', 'E', 'E', 'R'};


/** Número máximo de hijos */
#define MAX_CHILDS 3

/** Longitud del buffer  */
#define BUFFERSIZE 512

// - Variables globales
char* BaseMemoria;
int Puerto;




// Estructura para manejar los segmentos de los programas.

struct Segmento
{
	char IdSegmento[5];
	int Inicio;
	int Tamanio;
	char* UnicacionMP;
	struct Segmento * sig;
};

struct Programa
{
	int IdPrograma;
	struct Segmento * segmentos;
	struct Programa * sig;
};

// Listado de programas.
struct Programa * listaProgramas;

int main(int argv, char** argc)
{
	// Definimos los hilos principales
	pthread_t hOrquestadorConexiones, hConsola;

	// Intentamos reservar la memoria principal
	reservarMemoriaPrincipal();

	// Obtenemos el puerto de la configuración
	Puerto = ObtenerPuertoConfig();

	InstanciarTablaSegmentos();

	pthread_create(&hOrquestadorConexiones, NULL, (void*) HiloOrquestadorDeConexiones, NULL);
	pthread_create(&hConsola, NULL, (void*) HiloConsola, NULL);
	pthread_join(hOrquestadorConexiones, (void **) NULL);
	pthread_join(hConsola, (void **) NULL);

	free(BaseMemoria);

    return EXIT_SUCCESS;
}

void InstanciarTablaSegmentos()
{
	listaProgramas = NULL;
}

void reservarMemoriaPrincipal()
{
	// Obtenemos el tamaño de la memoria del config
	int tamanioMemoria = ObtenerTamanioMemoriaConfig();
	// Reservamos la memoria
	BaseMemoria = (char*) malloc(tamanioMemoria);

	// si no podemos salimos y cerramos el programa.
	if (BaseMemoria == NULL)
	{
		ErrorFatal("No se pudo reservar la memoria.");
	}

}

void ErrorFatal(char mensaje[])
{
	char cadena [100];
	printf("%s\n",mensaje);

	printf("El programa se cerrara. Presione ENTER para finalizar la ejecución.");
	fgets (cadena, 100, stdin);

	exit(EXIT_FAILURE);
}

int ObtenerTamanioMemoriaConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "TAMANIO_MEMORIA");
}
int ObtenerPuertoConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO");
}

void HiloOrquestadorDeConexiones()
{
	 	int socket_host;
	    struct sockaddr_in client_addr;
	    struct sockaddr_in my_addr;

	    struct timeval tv;      /// Para el timeout del accept

	    socklen_t size_addr = 0;

	    int socket_client;
	    fd_set rfds;        // Conjunto de descriptores a vigilar
	    int childcount=0;
	    int exitcode;

	    int childpid;

	    int pidstatus;

	    int activated=1;
	    int loop=0;

	    socket_host = socket(AF_INET, SOCK_STREAM, 0);
	    if(socket_host == -1)
	      error(1, "No puedo inicializar el socket");

	    my_addr.sin_family = AF_INET ;
	    my_addr.sin_port = htons(Puerto);
	    my_addr.sin_addr.s_addr = INADDR_ANY ;


	    if( bind( socket_host, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 )
	      error(2, "El puerto está en uso"); /* Error al hacer el bind() */

	    if(listen( socket_host, 10) == -1 )
	      error(3, "No puedo escuchar en el puerto especificado");

	    size_addr = sizeof(struct sockaddr_in);

	    while(activated)
	            	      {
	            	      reloj(loop);
	            	      // select() se carga el valor de rfds
	            	      FD_ZERO(&rfds);
	            	      FD_SET(socket_host, &rfds);

	            	      // select() se carga el valor de tv
	            	      tv.tv_sec = 0;
	            	      tv.tv_usec = 500000;    // Tiempo de espera

	            	      if (select(socket_host+1, &rfds, NULL, NULL, &tv))
	            	        {
	            	          if((socket_client = accept( socket_host, (struct sockaddr*)&client_addr, &size_addr))!= -1)
	            	            {
	            	          loop=-1;        // Para reiniciar el mensaje de Esperando conexión...
	            	          printf("\nSe ha conectado %s por su puerto %d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
	            	          switch ( childpid=fork() )
	            	            {
	            	            case -1:  // Error
	            	              error(4, "No se puede crear el proceso hijo");
	            	              break;
	            	            case 0:   // Somos proceso hijo, el encargado de administrar el hilo que atenderá al cliente conectado
	            	              if (childcount<MAX_CHILDS)
	            	                exitcode=AtiendeCliente(socket_client, client_addr);
	            	              else
	            	                exitcode=DemasiadosClientes(socket_client, client_addr);

	            	              exit(exitcode); // Código de salida

	            	            default:  // Somos proceso padre, osea el proceso que orquesta todos los hilos que atienden a los clientes.
	            	              childcount++; // Acabamos de tener un hijo
	            	              close(socket_client); // Nuestro hijo se las apaña con el cliente que entró, para nosotros ya no existe.
	            	              break;
	            	            }
	            	            }
	            	          else
	            	            fprintf(stderr, "ERROR AL ACEPTAR LA CONEXIÓN\n");
	            	        }

	            	      // Miramos si se ha cerrado algún hijo últimamente
	            	      childpid=waitpid(0, &pidstatus, WNOHANG);
	            	      if (childpid>0)
	            	        {
	            	          childcount--;   // Se acaba de morir un hijo, que en paz descance

	            	          // Muchas veces nos dará 0 si no se ha muerto ningún hijo, o -1 si no tenemos hijos con errno=10 (No child process). Así nos quitamos esos mensajes

	            	          if (WIFEXITED(pidstatus))
	            	            {

	            	          // Tal vez querremos mirar algo cuando se ha cerrado un hijo correctamente
	            	          if (WEXITSTATUS(pidstatus)==99)
	            	            {
	            	              printf("\nSe ha pedido el cierre del programa\n");
	            	              activated=0;
	            	            }
	            	            }
	            	        }
	            	      loop++;
	            	      }

	    close(socket_host);
}

void ConsolaComandoHelp()
{
	  printf ("Aca se imprime la ayuda con los comandos. IMPLEMENTAR");
}

void ConsolaComandoLeerMemoria()
{

}

void ConsolaComandoEscribirMemoria()
{

}

void ConsolaComandoCrearSegmento()
{
	int idPrograma;
	int tamanio;
	printf ("--> Id de programa?");
	scanf("%d",&idPrograma);
	printf ("--> Tamaño segmento");
	scanf("%d",&tamanio);

	AgregarSegmentoALista(idPrograma, tamanio);
}

void AgregarSegmentoALista(int idPrograma, int tamanio)
{
	listaProgramas = malloc(sizeof(struct Programa));
	listaProgramas->IdPrograma = idPrograma;
	listaProgramas->segmentos = malloc(sizeof(struct Segmento ));
	listaProgramas->segmentos->Inicio = 1;
	listaProgramas->segmentos->Tamanio  = tamanio;
	listaProgramas->segmentos->UnicacionMP  = BaseMemoria + 1;
}

void ConsolaComandoDestruirSegmento()
{

}

int ObtenerComandoConsola(char buffer[])
{
	// NMR TERMINAR DE DEFINIRLOS COMO LA GENTE
	int compareLimit = 4;

	if(strncmp(buffer, CMD_HELP,  sizeof(CMD_HELP)) == 0)
		return COMANDO_CONSOLA_HELP;

	if(strncmp(buffer, CMD_LEER_MEMORIA,  sizeof(CMD_LEER_MEMORIA)) == 0)
		return COMANDO_CONSOLA_LEER_MEMORIA;

	if(strncmp(buffer, "MEMESCRIBIR", compareLimit) == 0)
		return COMANDO_CONSOLA_ESCRIBIR_MEMORIA;

	if(strncmp(buffer, "SEGCREAR", compareLimit) == 0)
		return COMANDO_CONSOLA_CREAR_SEGMENTO;

	if(strncmp(buffer, "SEGDESTRUIR", compareLimit) == 0)
		return COMANDO_CONSOLA_DESTRUIR_SEGMENTO;

	return 0;
}

void HiloConsola()
{
	 while(1)
	 {
	   char comando [100];
	   printf ("Escribir comando. (HELP para obtener una lista con los comandos)");
	   fgets (comando, 100, stdin);

	   int comando_introducido = 0;
	   comando_introducido = ObtenerComandoConsola(comando);

	   //Evaluamos los comandos
	        switch ( comando_introducido )
	        {
	        	case COMANDO_CONSOLA_HELP:
	        		ConsolaComandoHelp();
	        		break;
	    		case COMANDO_CONSOLA_LEER_MEMORIA:
	    			ConsolaComandoLeerMemoria();
	    			break;
	    		case COMANDO_CONSOLA_ESCRIBIR_MEMORIA:
	    			ConsolaComandoEscribirMemoria();
	    			break;
	    		case COMANDO_CONSOLA_CREAR_SEGMENTO:
	    			ConsolaComandoCrearSegmento();
	    			break;
	    		case COMANDO_CONSOLA_DESTRUIR_SEGMENTO:
	    			ConsolaComandoDestruirSegmento();
	    			break;

	    		// ACA FALTAN VARIOS COMANDOS PARA IMPLEMENTAR
	    		default:
	    			 printf(" -- COMANDO INVALIDO -- ");
	    			break;
	        }

	 }
}

// No usamos addr, pero lo dejamos para el futuro
int DemasiadosClientes(int socket, struct sockaddr_in addr)
{
    char buffer[BUFFERSIZE];
    int bytecount;

    memset(buffer, 0, BUFFERSIZE);

    sprintf(buffer, "Demasiados clientes conectados. Por favor, espere unos minutos\n");

    if((bytecount = send(socket, buffer, strlen(buffer), 0))== -1)
      error(6, "No puedo enviar información");

    close(socket);

    return 0;
}

int RecibirDatos(int socket, void *buffer)
{
	int bytecount;
	// memset se usa para llenar el buffer con 0s
    	memset(buffer, 0, BUFFERSIZE);

     //Nos ponemos a la escucha de las peticiones que nos envie el cliente.
     if((bytecount = recv(socket, buffer, BUFFERSIZE, 0))== -1)
    		error(5, "Ocurrio un error al intentar recibir datos desde uno de los clientes.");

     return bytecount;
}

int EnviarDatos(int socket, void *buffer)
{
	int bytecount;

	  if((bytecount = send(socket, buffer, strlen(buffer), 0))== -1)
	      error(6, "No puedo enviar información");

     return bytecount;
}

int chartToInt(char x)
{
	char str[1];
	str[0] = x;
	int a = atoi(str);
	return a;
}

int ObtenerComandoMSJ(char buffer[])
{
	//Hay que obtener el comando dado el buffer.
	//El comando está dado por el primer caracter, que tiene que ser un número.
	return chartToInt(buffer[0]);
}

void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente )
{
	(*idProg) = chartToInt(buffer[1]);
	(*tipoCliente) = chartToInt(buffer[2]);

	 memset(buffer, 0, BUFFERSIZE);
	 sprintf(buffer, "HandShake: OK! INFO-->  idPRog: %d, tipoCliente: %d ", *idProg, *tipoCliente );
}

void ComandoGetBytes(char *buffer, int idProg)
{
	//Retorna los bytes que el programa quiere.
	//Hay que validar varias cosas, entre ellas que el programa tenga derechos para acceder al segmento que se está pidiendo.

	 memset(buffer, 0, BUFFERSIZE);
	 sprintf(buffer, "GetBytes: OK! INFO-->  idPRog: %d", idProg);
}

void ComandoSetBytes(char *buffer, int idProg)
{
	//Graba los bytes que el programa quiere.
	//Hay que validar varias cosas. Seguro que el idProg sirve de algo.

	 memset(buffer, 0, BUFFERSIZE);
	 sprintf(buffer, "SetBytes: OK! INFO-->  idPRog: %d", idProg);
}

void ComandoCambioProceso(char *buffer, int *idProg)
{
	//Cambia el proceso activo sobre el que el cliente está trabajando. Así el hilo sabe con que proceso trabaja el cliente.
	//Hay que validar varias cosas seguro

	int idProgViejo = *idProg;
	(*idProg) = chartToInt(buffer[1]);

	 sprintf(buffer, "SetBytes: OK! INFO-->  idPRog NUEVO: %d, idPRog VIEJO: %d", *idProg, idProgViejo );
}

void ComandoCrearSegmento(char *buffer, int idProg)
{
	//Crea un segmento para un programa.
	//NMR: no me queda claro para que quiere el IdProg, se supone que el hilo ya lo sabe por el handshake y el cambioProceso.
	int idProgParam = chartToInt(buffer[1]);
	int taman = chartToInt(buffer[2]);

	 sprintf(buffer, "Crear Segmento: OK! INFO-->  idPRog: %d, idPRog-Parametro: %d, tamaño: %d", idProg, idProgParam ,taman);
}

void ComandoDestruirSegmento(char *buffer, int idProg)
{
	//Graba los bytes que el programa quiere.
	//NMR: no me queda claro para que quiere el IdProg, se supone que el hilo ya lo sabe por el handshake y el cambioProceso.
	int idProgParam = chartToInt(buffer[1]);
	int taman = chartToInt(buffer[2]);

	 sprintf(buffer, "Destruir Segmento: OK! INFO-->  idPRog: %d, idPRog-Parametro: %d, tamaño: %d", idProg, idProgParam ,taman);
}

int AtiendeCliente(int socket, struct sockaddr_in addr)
{

	//Es el ID del programa con el que está trabajando actualmente el HILO.
	//Nos es de gran utilidad para controlar los permisos de acceso (lectura/escritura) del programa.
	//(en otras palabras que no se pase de vivo y quiera acceder a una posicion de memoria que no le corresponde.)
	 int id_Programa = 0;
	 int tipo_Conexion = 0;

	// Es el encabezado del mensaje. Nos dice que acción se le está solicitando al UMV
	 int tipo_mensaje = 0;

	// Dentro del buffer se guarda el mensaje recibido por el cliente.
    char buffer[BUFFERSIZE];

    // La variable AUX es una variable auxiliar de uso libre.
    //char aux[BUFFERSIZE];

    // Cantidad de bytes recibidos.
    //int bytecount;

    // La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
    int desconexionCliente=0;

    // Código de salida por defecto
    int code=0;

    while (!desconexionCliente)
    {
    	//Recibimos los datos del cliente
    	RecibirDatos(socket, buffer);

    	//Analisamos que peticion nos está haciendo (obtenemos el comando)
        tipo_mensaje = ObtenerComandoMSJ(buffer);

        //Evaluamos los comandos
        switch ( tipo_mensaje )
        {
    		case MSJ_GET_BYTES:
    			ComandoGetBytes(buffer, id_Programa);
    			break;
    		case MSJ_SET_BYTES:
    			ComandoSetBytes(buffer, id_Programa);
    			break;
    		case MSJ_HANDSHAKE:
    			 ComandoHandShake(buffer, &id_Programa, &tipo_Conexion);
    			break;
    		case MSJ_CAMBIO_PROCESO:
    			ComandoCambioProceso(buffer, &id_Programa);
    			break;
    		case MSJ_CREAR_SEGMENTO: //NMR: hay que ver quien tiene acceso a estas operaciones, solo el PLP? hace falta que le pase el IDprog? no lo tiene ya con el handshake?
    			ComandoCrearSegmento(buffer, id_Programa);
    			break;
    		case MSJ_DESTRUIR_SEGMENTO:
    			ComandoDestruirSegmento(buffer, id_Programa);
    			break;
    		default:
    		    memset(buffer, 0, BUFFERSIZE);
    		        sprintf(buffer, "El ingresado no es un comando válido\n");
    			break;
        }

     /*

    // Comando HOLA - Saluda y dice la IP

     if (strncmp(buffer, "HOLA", 4)==0)
      {
        memset(buffer, 0, BUFFERSIZE);
        sprintf(buffer, "Hola %s, ¿cómo estás?\n", inet_ntoa(addr.sin_addr));
      }
    //Comando EXIT - Cierra la conexión actual
    else if (strncmp(buffer, "EXIT", 4)==0)
      {
        memset(buffer, 0, BUFFERSIZE);
        sprintf(buffer, "Hasta luego. Vuelve pronto %s\n", inet_ntoa(addr.sin_addr));
        desconexionCliente=1;
      }
   // Comando CERRAR - Cierra el servidor
    else if (strncmp(buffer, "CERRAR", 6)==0)
      {
        memset(buffer, 0, BUFFERSIZE);
        sprintf(buffer, "Adiós. Cierro el servidor\n");
        desconexionCliente=1;
        code=99;        // Salir del programa
      }
    else
      {
        sprintf(aux, "ECHO: %s\n", buffer);
        strcpy(buffer, aux);
      }
*/

       // Enviamos datos al cliente.
       // NMR: aca luego habra que agregar un retardo segun pide el TP
       EnviarDatos(socket, buffer);

    }

    close(socket);
    return code;
}

void reloj(int loop)
{
  if (loop==0)
    printf("[SERVIDOR] Esperando conexión  ");
/*
  printf("\033[1D");        //Introducimos código ANSI para retroceder 2 caracteres
 switch (loop%4)
    {
    case 0: printf("|"); break;
    case 1: printf("/"); break;
    case 2: printf("-"); break;
    case 3: printf("\\"); break;
    default:           // No debemos estar aquí
    break;
    }
*/
  fflush(stdout);       /* Actualizamos la pantalla */
}

void error(int code, char *err)
{
  char *msg=(char*)malloc(strlen(err)+14);
  sprintf(msg, "Error %d: %s\n", code, err);
  fprintf(stderr, "%s", msg);
  exit(1);
}
