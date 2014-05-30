/*
 ============================================================================
 Name        : UMV.c
 Author      : Garras
 Version     : 1.0
 Copyright   : Garras - UTN FRBA 2014
 Description : Hello World in C, Ansi-style
 Testing	 : Para probarlo es tan simple como ejecutar en el terminator la linea "$ telnet localhost 7000" y empezar a dialogar con el UMV.
 A tener en cuenta: organizar codigo : ctrl+Mayúscula+f
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
#include <commons/collections/dictionary.h>
#include <semaphore.h>

#include "UMV.h"

#if 1 // CONSTANTES //
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
#define COMANDO_CONSOLA_HELP								1
#define COMANDO_CONSOLA_LEER_MEMORIA						2
#define COMANDO_CONSOLA_ESCRIBIR_MEMORIA					3
#define COMANDO_CONSOLA_CREAR_SEGMENTO						4
#define COMANDO_CONSOLA_DESTRUIR_SEGMENTO					5
#define COMANDO_CONSOLA_DEFINIR_RETARDO						6
#define COMANDO_CONSOLA_DEFINIR_ALGORITMO					7
#define COMANDO_CONSOLA_COMPACTAR_MEMORIA					8
#define COMANDO_CONSOLA_DUMP_ESTRUCTURAS					9
#define COMANDO_CONSOLA_DUMP_MEMORIA_PRINCIPAL				10
#define COMANDO_CONSOLA_DUMP_CONTENIDO_MEMORIA_PRINCIPAL	11
#define COMANDO_CONSOLA_CERRAR_PROGRAMA						12

char CMD_HELP[5] = { 'H', 'E', 'L', 'P', '\0' };
char CMD_LEER_MEMORIA[8] = { 'M', 'E', 'M', 'R', 'E', 'A', 'D', '\0' };
char CMD_ESCRIBIR_MEMORIA[9] = { 'M', 'E', 'M', 'W', 'R', 'I', 'T', 'E', '\0' };
char CMD_CREAR_SEGMENTO[7] = { 'S', 'E', 'G', 'N', 'E', 'W', '\0' };
char CMD_DESTRUIR_SEGMENTO[7] = { 'S', 'E', 'G', 'D', 'E', 'L', '\0' };
char CMD_DEFINIR_ALGORITMO[7] = { 'D', 'E', 'F', 'A', 'L', 'G', '\0' };
char CMD_DEFINIR_RETARDO[7] = { 'D', 'E', 'F', 'R', 'E', 'T', '\0' };
char CMD_COMPACTAR_MEMORIA[8] = { 'M', 'E', 'M', 'C', 'O', 'M', 'P', '\0' };
char CMD_DUMP_ESTRUCTURAS[12] = { 'D', 'U', 'M', 'P', 'E', 'S', 'T', 'R', 'U', 'C', 'T', '\0' };
char CMD_DUMP_MEMORIA_PRINCIPAL[8] = { 'D', 'U', 'M', 'P', 'M', 'E', 'M', '\0' };
char CMD_DUMP_CONTENIDO_MEMORIA_PRINCIPAL[12] = { 'D', 'U', 'M', 'P', 'R', 'E', 'A', 'D', 'M', 'E', 'M', '\0' };
char CMD_DUMP_CERRAR_PROGRAMA[6] = { 'C', 'L', 'O', 'S', 'E', '\0' };

/** Longitud del buffer  */
#define BUFFERSIZE 64

// Algoritmos de asignacion de memoria
#define ALGORITMO_WORST_FIT       'W'
#define ALGORITMO_FIRST_FIT       'F'

//Mensaje de error global.
char* MensajeError;

#endif

#if 1 // VARIABLES GLOBALES //
// - Base de la memoria principal de la UMV
char* BaseMemoria;

// - Puerto por el cual escucha el programa
int Puerto;

// - Bandera que controla la ejecución o no del programa. Si está en 0 el programa se cierra.
int Ejecutando = 1;

// - Bandera que controla si se imprimen comentarios adicionales durante la ejecución..
int ImprimirTrazaPorConsola = 1;

// - Algoritmo utilizado para asignacion de memoria
char AlgoritmoAsignacion = ALGORITMO_WORST_FIT;

// Diccionario con todos los segmentos de los programas.
t_dictionary *programasDiccionario;

#endif

#if 1 // ESTRUCTURAS //
// Estructura para manejar los segmentos de los programas.
struct t_segmento
{
	int IdSegmento;
	int Inicio;
	int Tamanio;
	char* UbicacionMP;
	struct t_segmento *sig;
};

#endif

int main(int argv, char** argc)
{
	// Definimos los hilos principales
	pthread_t hOrquestadorConexiones, hConsola;

	// Intentamos reservar la memoria principal
	reservarMemoriaPrincipal();

	// Obtenemos el puerto de la configuración
	Puerto = ObtenerPuertoConfig();

	InstanciarTablaSegmentos();

	pthread_create(&hOrquestadorConexiones, NULL, (void*) HiloOrquestadorDeConexiones, NULL );
	pthread_create(&hConsola, NULL, (void*) HiloConsola, NULL );

	pthread_join(hConsola, (void **) NULL );
	// pthread_join(hOrquestadorConexiones, (void **) NULL );

	free(BaseMemoria);

	return EXIT_SUCCESS;
}

#if 1 // METODOS PARA CONSOLA //
void HiloConsola()
{

	while (Ejecutando)
	{
		printf("\r\n --> Ingresar comando. (HELP para obtener una lista con los comandos)\n");

		char comando[15];
		int comando_introducido = 0;

		//Obtenemos el comando por consola
		scanf("%s", comando);

		//Analizamos el comando introducido
		comando_introducido = ObtenerComandoConsola(comando);

		//Le damos curso a la solicitud por consola
		switch (comando_introducido)
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
			case COMANDO_CONSOLA_DEFINIR_RETARDO:
				ConsolaComandoDefinirRetardo();
				break;
			case COMANDO_CONSOLA_DEFINIR_ALGORITMO:
				ConsolaComandoDefinirAlgoritmo();
				break;
			case COMANDO_CONSOLA_COMPACTAR_MEMORIA:
				ConsolaComandoCompactarMemoria();
				break;
			case COMANDO_CONSOLA_DUMP_ESTRUCTURAS:
				ConsolaComandoDumpEstructuras();
				break;
			case COMANDO_CONSOLA_DUMP_MEMORIA_PRINCIPAL:
				ConsolaComandoDumpMemoriaPrincipal();
				break;
			case COMANDO_CONSOLA_DUMP_CONTENIDO_MEMORIA_PRINCIPAL:
				ConsolaComandoDumpContenidoMemoriaPrincipal();
				break;
			case COMANDO_CONSOLA_CERRAR_PROGRAMA:
				Ejecutando = 0;
				break;
			default:
				Error("COMANDO INVALIDO");
				break;
		}

	}

	Traza("Fin del programa");
}

int ObtenerComandoConsola(char buffer[])
{
	if (strncmp(buffer, CMD_HELP, sizeof(CMD_HELP) - 1) == 0)
		return COMANDO_CONSOLA_HELP;

	if (strncmp(buffer, CMD_LEER_MEMORIA, sizeof(CMD_LEER_MEMORIA) - 1) == 0)
		return COMANDO_CONSOLA_LEER_MEMORIA;

	if (strncmp(buffer, CMD_ESCRIBIR_MEMORIA, sizeof(CMD_ESCRIBIR_MEMORIA) - 1) == 0)
		return COMANDO_CONSOLA_ESCRIBIR_MEMORIA;

	if (strncmp(buffer, CMD_CREAR_SEGMENTO, sizeof(CMD_CREAR_SEGMENTO) - 1) == 0)
		return COMANDO_CONSOLA_CREAR_SEGMENTO;

	if (strncmp(buffer, CMD_DESTRUIR_SEGMENTO, sizeof(CMD_DESTRUIR_SEGMENTO) - 1) == 0)
		return COMANDO_CONSOLA_DESTRUIR_SEGMENTO;

	if (strncmp(buffer, CMD_DEFINIR_RETARDO, sizeof(CMD_DEFINIR_RETARDO) - 1) == 0)
		return COMANDO_CONSOLA_DEFINIR_RETARDO;

	if (strncmp(buffer, CMD_DEFINIR_ALGORITMO, sizeof(CMD_DEFINIR_ALGORITMO) - 1) == 0)
		return COMANDO_CONSOLA_DEFINIR_ALGORITMO;

	if (strncmp(buffer, CMD_COMPACTAR_MEMORIA, sizeof(CMD_COMPACTAR_MEMORIA) - 1) == 0)
		return COMANDO_CONSOLA_COMPACTAR_MEMORIA;

	if (strncmp(buffer, CMD_DUMP_ESTRUCTURAS, sizeof(CMD_DUMP_ESTRUCTURAS) - 1) == 0)
		return COMANDO_CONSOLA_DUMP_ESTRUCTURAS;

	if (strncmp(buffer, CMD_DUMP_MEMORIA_PRINCIPAL, sizeof(CMD_DUMP_MEMORIA_PRINCIPAL) - 1) == 0)
		return COMANDO_CONSOLA_DUMP_MEMORIA_PRINCIPAL;

	if (strncmp(buffer, CMD_DUMP_CONTENIDO_MEMORIA_PRINCIPAL, sizeof(CMD_DUMP_CONTENIDO_MEMORIA_PRINCIPAL) - 1) == 0)
		return COMANDO_CONSOLA_DUMP_CONTENIDO_MEMORIA_PRINCIPAL;

	if (strncmp(buffer, CMD_DUMP_CERRAR_PROGRAMA, sizeof(CMD_DUMP_CERRAR_PROGRAMA) - 1) == 0)
		return COMANDO_CONSOLA_CERRAR_PROGRAMA;

	return 0;
}

void ConsolaComandoHelp()
{
	char *mensajeHelp = string_new();
	// NMR: hará falta que los comandos terminen en /0 ??

	string_append(&mensajeHelp, "Listado de comandos posibles: \n");
	string_append_with_format(&mensajeHelp, "%s: Retorna una lista con los comandos \n", CMD_HELP);
	string_append_with_format(&mensajeHelp, "%s: Dado un id de programa, una base, un desplazamiento y la cantidad de bytes, retorna esa posicion de memoria. Opcionalmente graba lo leido en un archivo. \n", CMD_LEER_MEMORIA);
	string_append_with_format(&mensajeHelp, "%s: Dado un id de programa, una base, un desplazamiento, la cantidad de bytes y bytes a grabar, graba en la memoria. Opcionalmente lo graba en un archivo. \n", CMD_ESCRIBIR_MEMORIA);
	string_append_with_format(&mensajeHelp, "%s: Crea un segmento de un tamaño dado para un programa dado. Opcionalmente graba lo realizado en un archivo. \n", CMD_CREAR_SEGMENTO);
	string_append_with_format(&mensajeHelp, "%s: Destruye todos los segmentos para un programa dado. Opcionalmente graba lo realizado en un archivo. \n", CMD_DESTRUIR_SEGMENTO);
	string_append_with_format(&mensajeHelp, "%s: Define el algoritmo utilizado para grabar en memoria (Worst-fit o first-fit). \n", CMD_DEFINIR_ALGORITMO);
	string_append_with_format(&mensajeHelp, "%s: Define el retardo (en milisegundos) que el sistema esperara antes de responder una solicitud a un cliente. \n", CMD_DEFINIR_RETARDO);
	string_append_with_format(&mensajeHelp, "%s: Fuerza la compactación de la memoria. \n", CMD_COMPACTAR_MEMORIA);
	string_append_with_format(&mensajeHelp, "%s: Retorna información de las estructuras internas de la UMV, de todos los programas o de alguno en particular. \n", CMD_DUMP_ESTRUCTURAS);
	string_append_with_format(&mensajeHelp, "%s: Retorna información del estado de la memoria principal. \n", CMD_DUMP_MEMORIA_PRINCIPAL);
	string_append_with_format(&mensajeHelp, "%s: Dado un desplazamiento y una cantidad de bytes, lee de la memoria. \n", CMD_DUMP_CONTENIDO_MEMORIA_PRINCIPAL);
	string_append_with_format(&mensajeHelp, "%s: Cierra el programa. \n", CMD_DUMP_CERRAR_PROGRAMA);

	printf("%s", mensajeHelp);
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
	char grabarArchivo[2];

	printf("\n--> Ingrese ID de programa:   ");
	scanf("%d", &idPrograma);
	printf("\n--> Ingrese el tamaño del segmento:   ");
	scanf("%d", &tamanio);
	printf("\n--> ¿Grabar en archivo? (S/N):   ");
	scanf("%s", grabarArchivo);

	CrearSegmento(idPrograma, tamanio);

	if (TraducirSiNo(grabarArchivo[0]))
	{
		// Grabo lo realizado en un archivo
	}
}

void ConsolaComandoDestruirSegmento()
{

}

void ConsolaComandoDefinirAlgoritmo()
{
	char algoritmo[2];

	printf("\n--> Algoritmo actual: %s", NombreDeAlgoritmo(AlgoritmoAsignacion));
	printf("\n--> Algoritmo a definir (W = Worst-fit, F = First-fit):   ");
	scanf("%s", algoritmo);

	if (ValidarCodigoAlgoritmo(algoritmo[0]))
	{
		AlgoritmoAsignacion = algoritmo[0];
		printf("--> Seteo OK. Algoritmo actual: %s", NombreDeAlgoritmo(AlgoritmoAsignacion));
	}
	else
		printf("--> Código de algoritmo incorrecto. Solo se admite W o F (W = Worst-fit, F = First-fit)");

}

void ConsolaComandoDefinirRetardo()
{

}

void ConsolaComandoCompactarMemoria()
{

}

void ConsolaComandoDumpEstructuras()
{

}

void ConsolaComandoDumpMemoriaPrincipal()
{

}

void ConsolaComandoDumpContenidoMemoriaPrincipal()
{

}

#endif

#if 1 // METODOS MANEJO DE ERRORES
void Error(const char* mensaje, ...)
{
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);

	fprintf(stderr, "\nERROR: %s\n", nuevo);

	va_end(arguments);

}

void Traza(const char* mensaje, ...)
{
	if (ImprimirTrazaPorConsola)
	{
		char* nuevo;
		va_list arguments;
		va_start(arguments, mensaje);
		nuevo = string_from_vformat(mensaje, arguments);

		printf("TRAZA--> %s \n", nuevo);

		va_end(arguments);

	}
}

void ErrorFatal(char mensaje[])
{
	Error(mensaje);
	char fin;

	printf("El programa se cerrara. Presione ENTER para finalizar la ejecución.");
	scanf("%c", &fin);

	exit(EXIT_FAILURE);
}

#endif

#if 1 // METODOS SOCKETS //
void HiloOrquestadorDeConexiones()
{
	int socket_host;
	struct sockaddr_in client_addr;
	struct sockaddr_in my_addr;
	int yes = 1;
	socklen_t size_addr = 0;

	socket_host = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_host == -1)
		ErrorFatal("No se pudo inicializar el socket que escucha a los clientes");

	if (setsockopt(socket_host, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		ErrorFatal("Error al hacer el 'setsockopt'");
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(Puerto);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8);

	if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		ErrorFatal("Error al hacer el Bind. El puerto está en uso");

	if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
		ErrorFatal("Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

	Traza("El socket está listo para recibir conexiones. Numero de socket: %d, puerto: %d", socket_host, Puerto);

	while (Ejecutando)
	{
		int socket_client;

		size_addr = sizeof(struct sockaddr_in);

		if ((socket_client = accept(socket_host, (struct sockaddr *) &client_addr, &size_addr)) != -1)
		{
			Traza("Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, socket_client);

			// Aca hay que crear un nuevo hilo, que será el encargado de atender al cliente
			pthread_t hNuevoCliente;
			pthread_create(&hNuevoCliente, NULL, (void*) AtiendeCliente, (void *) socket_client);
		}
		else
		{
			Error("ERROR AL ACEPTAR LA CONEXIÓN DE UN CLIENTE");
		}
	}

	CerrarSocket(socket_host);
}

int RecibirDatos(int socket, void *buffer)
{
	int bytecount;
	// memset se usa para llenar el buffer con 0s
	memset(buffer, 0, BUFFERSIZE);

	//Nos ponemos a la escucha de las peticiones que nos envie el cliente. //aca si recibo 0 bytes es que se desconecto el otro, cerrar el hilo.
	if ((bytecount = recv(socket, buffer, BUFFERSIZE, 0)) == -1)
		Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d", socket);

	Traza("RECIBO datos. socket: %d. buffer: %s", socket, (char*) buffer);

	return bytecount;
}

int EnviarDatos(int socket, void *buffer)
{
	int bytecount;

	if ((bytecount = send(socket, buffer, strlen(buffer), 0)) == -1)
		Error("No puedo enviar información a al clientes. Socket: %d", socket);

	Traza("ENVIO datos. socket: %d. buffer: %s", socket, (char*) buffer);

	return bytecount;
}

void CerrarSocket(int socket)
{
	close(socket);
	Traza("Se cerró el socket (%d).", socket);
}

#endif

#if 1 // METODOS CONFIGURACION //
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

#endif

#if 1 // METODOS MANEJO MEMORIA //
void InstanciarTablaSegmentos()
{
	programasDiccionario = dictionary_create();
}

void reservarMemoriaPrincipal()
{
	// Obtenemos el tamaño de la memoria del config
	int tamanioMemoria = ObtenerTamanioMemoriaConfig();
	// Reservamos la memoria
	BaseMemoria = (char*) malloc(tamanioMemoria);

	// si no podemos salimos y cerramos el programa.
	if (BaseMemoria == NULL )
	{
		ErrorFatal("No se pudo reservar la memoria.");
	}
	else
	{
		Traza("Se reservó la memoria principal OK. Tamaño de la memoria (%d)", tamanioMemoria);
	}

}

int CrearSegmento(int idPrograma, int tamanio)
{
	// dado un ID de programa y un tamaño del segmento tengo que:

	// Buscar el programa en el diccionario
		// Si está obtengo su lista de programas
		// si no está lo creo en el diccionario e instancio su lista de programas.


	// Obtener el Id del segmento.
		// Cantidad de segmentos en la lista + 1


	// Obtener su inicio (base virtual)
		// Agarro el ultimo segmento de la lista y hago (inicio + tamaño + numero aleatorio entre 0 y 500)

	// Obtener su ubicacion en la MP (Base real)
		// Si el algoritmo es First-fit
			//

	// si no pude compacto
		// Vuelvo a intentar
		// Si no puedo de nuevo es un error.

	//si pude

	// Agregar el nodo en la lista

	AgregarSegmentoALista(idPrograma, tamanio);
}

void AgregarSegmentoALista(int idPrograma, int tamanio)
{
	// Validar que el idPrograma tenga una entrada dentro del diccionario.

	/*
	 listaProgramas = malloc(sizeof(struct Programa));
	 listaProgramas->IdPrograma = idPrograma;
	 listaProgramas->segmentos = malloc(sizeof(struct Segmento ));
	 listaProgramas->segmentos->Inicio = 1;
	 listaProgramas->segmentos->Tamanio  = tamanio;
	 listaProgramas->segmentos->UnicacionMP  = BaseMemoria + 1;*/
}

#endif

#if 1 // METODOS ATENDER CLIENTE //
int AtiendeCliente(void * arg)
{
	int socket = (int) arg;

	//Es el ID del programa con el que está trabajando actualmente el HILO.
	//Nos es de gran utilidad para controlar los permisos de acceso (lectura/escritura) del programa.
	//(en otras palabras que no se pase de vivo y quiera acceder a una posicion de memoria que no le corresponde.)
	int id_Programa = 0;

	int tipo_Conexion = 0;

	// Es el encabezado del mensaje. Nos dice que acción se le está solicitando al UMV
	int tipo_mensaje = 0;

	// Dentro del buffer se guarda el mensaje recibido por el cliente.
	char buffer[BUFFERSIZE];

	// Cantidad de bytes recibidos.
	int bytesRecibidos;

	// La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
	int desconexionCliente = 0;

	// Código de salida por defecto
	int code = 0;

	while ((!desconexionCliente) & Ejecutando)
	{
		//Recibimos los datos del cliente
		bytesRecibidos = RecibirDatos(socket, buffer);

		if (bytesRecibidos > 0)
		{
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			tipo_mensaje = ObtenerComandoMSJ(buffer);

			//Evaluamos los comandos
			switch (tipo_mensaje)
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

			// Enviamos datos al cliente.
			// NMR: aca luego habra que agregar un retardo segun pide el TP int pthread_detach(pthread_self());
			EnviarDatos(socket, buffer);
		}
		else
			desconexionCliente = 1;

	}

	CerrarSocket(socket);

	return code;
}

int ObtenerComandoMSJ(char buffer[])
{
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
	return chartToInt(buffer[0]);
}

void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente)
{
	(*idProg) = chartToInt(buffer[1]);
	(*tipoCliente) = chartToInt(buffer[2]);

	memset(buffer, 0, BUFFERSIZE);
	sprintf(buffer, "HandShake: OK! INFO-->  idPRog: %d, tipoCliente: %d ", *idProg, *tipoCliente);
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

	sprintf(buffer, "Cambio proceso: OK! INFO-->  idPRog NUEVO: %d, idPRog VIEJO: %d", *idProg, idProgViejo);
}

void ComandoCrearSegmento(char *buffer, int idProg)
{
//Crea un segmento para un programa.
//NMR: no me queda claro para que quiere el IdProg, se supone que el hilo ya lo sabe por el handshake y el cambioProceso.
	int idProgParam = chartToInt(buffer[1]);
	int taman = chartToInt(buffer[2]);

	sprintf(buffer, "Crear Segmento: OK! INFO-->  idPRog: %d, idPRog-Parametro: %d, tamaño: %d", idProg, idProgParam, taman);
}

void ComandoDestruirSegmento(char *buffer, int idProg)
{
//Graba los bytes que el programa quiere.
//NMR: no me queda claro para que quiere el IdProg, se supone que el hilo ya lo sabe por el handshake y el cambioProceso.
	int idProgParam = chartToInt(buffer[1]);
	int taman = chartToInt(buffer[2]);

	sprintf(buffer, "Destruir Segmento: OK! INFO-->  idPRog: %d, idPRog-Parametro: %d, tamaño: %d", idProg, idProgParam, taman);
}

#endif

#if 1 // OTROS METODOS //
int chartToInt(char x)
{
	char str[1];
	str[0] = x;
	int a = atoi(str);
	return a;
}

char* NombreDeAlgoritmo(char idAlgorit)
{
	switch (idAlgorit)
	{
		case ALGORITMO_WORST_FIT:
			return "Worst-Fit";
			break;
		case ALGORITMO_FIRST_FIT:
			return "First-Fit";
			break;
		default:
			return "Error, codigo de algoritmo invalido.";
			break;
	}
}

int ValidarCodigoAlgoritmo(char idAlgorit)
{
	switch (idAlgorit)
	{
		case ALGORITMO_WORST_FIT:
			return 1;
			break;
		case ALGORITMO_FIRST_FIT:
			return 1;
			break;
		default:
			return 0;
			break;
	}
}

int TraducirSiNo(char caracter)
{
	switch (caracter)
	{
		case 'S':
			return 1;
			break;
		case 'N':
			return 0;
			break;
		default:
			return 0;
			break;
	}
}

#endif
