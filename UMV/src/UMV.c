/*
 ============================================================================
 Name        : UMV.c
 Author      : Garras
 Version     : 1.0
 Copyright   : Garras - UTN FRBA 2014
 Description : Hello World in C, Ansi-style
 Para probarlo es tan simple como ejecutar en el terminator la linea "$ telnet localhost 7000" y empezar a dialogar con el UMV.
 ============================================================================
 */


#include <fcntl.h>
#include <string.h>
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
#include <string.h>

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


/** Puerto  */
#define PORT       7000

/** Número máximo de hijos */
#define MAX_CHILDS 3

/** Longitud del buffer  */
#define BUFFERSIZE 512

int AtiendeCliente(int socket, struct sockaddr_in addr);
int DemasiadosClientes(int socket, struct sockaddr_in addr);
void error(int code, char *err);
void reloj(int loop);
void HiloOrquestadorDeConexiones();
void HiloConsola();
int RecibirDatos(int socket, void *buffer);
int ObtenerComandoMSJ(char buffer[]);
void ComandoHandShake(char *buffer, struct in_addr direccionIP );

int main(int argv, char** argc){

	int childConsola;

    //De entrada vamos a necesitar 2 hilos.
	//Uno dedicado a responder los comandos que se puedan ingresar por CONSOLA
	//Otro dedicado a orquestar los clientes que se quieran conectar con la UMV
    switch ( childConsola=fork() )
              {
              case -1:  // Error
                error(4, "No se puede crear el proceso Orquestador De Conexiones");
                break;
              case 0:   // Somos proceso hijo, que se encargará de administrar las conexiones a la UMV y sus peticiones.
            	  HiloOrquestadorDeConexiones();
            	  break;
              default:  // Somos proceso padre, el proceso CONSOLA.
            	  HiloConsola();
                break;
              }

    return 0;
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
	    my_addr.sin_port = htons(PORT);
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

void HiloConsola()
{
	 while(1)
	 {
	   char cadena [100];
	   printf ("Si queres poder tiper en consola: ");
	   fgets (cadena, 100, stdin);
	   printf("El comando que ingresaste es: %s \n", cadena);
	  }
}

    /* No usamos addr, pero lo dejamos para el futuro */
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

int ObtenerComandoMSJ(char buffer[])
{
	//Hay que obtener el comando dado el buffer. NMR
	int a = buffer[1];
	return a;
}

void ComandoHandShake(char *buffer, struct in_addr direccionIP )
{
	 memset(buffer, 0, BUFFERSIZE);
	 sprintf(buffer, "Hola %s, ¿cómo estás?\n", inet_ntoa(direccionIP));
}

int AtiendeCliente(int socket, struct sockaddr_in addr)
{

	//Es el ID del programa con el que está trabajando actualmente el HILO.
	//Nos es de gran utilidad para controlar los permisos de acceso (lectura/escritura) del programa.
	//(en otras palabras que no se pase de vivo y quiera acceder a una posicion de memoria que no le corresponde.)
	// int id_Programa;

	// Es el encabezado del mensaje. Nos dice que acción se le está solicitando al UMV
	 int tipo_mensaje;

	// Dentro del buffer se guarda el mensaje recibido por el cliente.
    char buffer[BUFFERSIZE];

    // La variable AUX es una variable auxiliar de uso libre.
    //char aux[BUFFERSIZE];

    // Cantidad de bytes recibidos.
    int bytecount;

    // La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
    int desconexionCliente=0;

    // Código de salida por defecto
    int code=0;

    while (!desconexionCliente)
    {
    	//Recibimos los datos del cliente
    	bytecount = RecibirDatos(socket, buffer);

    	//Analisamos que peticion nos está haciendo (obtenemos el comando)
        tipo_mensaje = ObtenerComandoMSJ(buffer);

        //Evaluamos los comandos
        switch ( tipo_mensaje )
        {
    		case MSJ_GET_BYTES:

    			break;
    		case MSJ_SET_BYTES:

    			break;
    		case MSJ_HANDSHAKE:
    			 ComandoHandShake(buffer,  addr.sin_addr);
    			break;
    		case MSJ_CAMBIO_PROCESO:

    			break;
    		case MSJ_CREAR_SEGMENTO:

    			break;
    		case MSJ_DESTRUIR_SEGMENTO:

    			break;
    		default:
    		    memset(buffer, 0, BUFFERSIZE);
    		        sprintf(buffer, "No no no\n");
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
        // NMR:esto ponelo como un metodo.
    if((bytecount = send(socket, buffer, strlen(buffer), 0))== -1)
      error(6, "No puedo enviar información");
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
