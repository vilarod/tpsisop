/*
 ============================================================================
 Name        : UMV.c
 Author      : Garras
 Version     : 1.0
 Copyright   : Garras - UTN FRBA 2014
 Description : Hello World in C, Ansi-style
 Testing	 : Para probarlo es tan simple como ejecutar en el terminator la linea "$ telnet localhost 7000" y empezar a dialogar con el UMV.
 A tener en cuenta: organizar codigo : ctrl+Mayúscula+f
 PARA HACER
 - algun dump?
 - permitir definir lugar del archivo y nombre--
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
#include <commons/collections/list.h>
#include <commons/log.h>
#include <semaphore.h>

#include "UMV.h"

#if 1 // CONSTANTES //
//Ruta del config
#define PATH_CONFIG "config.cfg"

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
#define COMANDO_CONSOLA_GRABA_SIEMPRE_ARCHIVO 				13
#define COMANDO_ACTIVAR_TRAZA 								14

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
char CMD_GRABA_SIEMPRE_ARCHIVO[6] = { 'A', 'R', 'C', 'H', 'I', '\0' };
char CMD_ACTIVAR_TRAZA[6] = { 'T', 'R', 'A', 'Z', 'A', '\0' };

/** Longitud del buffer  */
#define BUFFERSIZE 40000

// Algoritmos de asignacion de memoria
#define ALGORITMO_WORST_FIT       'W'
#define ALGORITMO_FIRST_FIT       'F'

#define NOMBRE_ARCHIVO_CONSOLA     "Archivo_UMV.txt"
#define NOMBRE_ARCHIVO_LOG 		   "Log_UMV.txt"

#endif

#if 1 // VARIABLES GLOBALES //
// - Base de la memoria principal de la UMV
char* g_BaseMemoria;

// Tamaño de la memoria
int g_TamanioMemoria;

// - Puerto por el cual escucha el programa
int g_Puerto;

// - Bandera que controla la ejecución o no del programa. Si está en 0 el programa se cierra.
int g_Ejecutando = 1;

// - Bandera que controla si se imprimen comentarios adicionales durante la ejecución..
int g_ImprimirTrazaPorConsola = 0;

// - Algoritmo utilizado para asignacion de memoria (Por defecto FIRST FIT)
char g_AlgoritmoAsignacion = ALGORITMO_FIRST_FIT;

// Listado con todos los segmentos de los programas.
t_list * g_ListaSegmentos;

// Semaforo mutex para controlar que nadie quiera acceder a la memoria mientras esta se compacta.
// sem_t s_AccesoAListadoSegmentos;
pthread_rwlock_t s_AccesoAListadoSegmentos = PTHREAD_RWLOCK_INITIALIZER;

//Mensaje de error global.
char* g_MensajeError;

// Retardo (en milisegundos) para contestar una solicitud a un cliente
int g_Retardo = 0;

// Archivo donde descargar info impresa por consola
FILE * g_ArchivoConsola;

// Bandera que indica si siempre se graba a archivo (ni siquiera consula)
int g_GrabarSiempreArchivo = 1;

// Logger del commons
t_log* logger;

#endif

#if 1 // ESTRUCTURAS //
// Estructura para manejar los segmentos de los programas.
struct _t_segmento
{
	int IdPrograma;
	int IdSegmento;
	int Inicio;
	int Tamanio;
	char* UbicacionMP;
};

static t_segmento *segmento_create(int idPrograma, int idSegmento, int inicio, int tamanio, char* ubicacionMP)
{
	t_segmento *new = malloc(sizeof(t_segmento));
	new->IdPrograma = idPrograma;
	new->IdSegmento = idSegmento;
	new->Inicio = inicio;
	new->Tamanio = tamanio;
	new->UbicacionMP = ubicacionMP;
	return new;
}

static void segmento_destroy(t_segmento *self)
{
	free(self);
}

#endif

int main(int argv, char** argc)
{
	// Instanciamos el archivo donde se grabará lo solicitado por consola
	g_ArchivoConsola = fopen(NOMBRE_ARCHIVO_CONSOLA, "wt");
	g_MensajeError = malloc(1 * sizeof(char));
	//char* temp_file = tmpnam(NULL);
	logger = log_create(NOMBRE_ARCHIVO_LOG, "UMV", true, LOG_LEVEL_TRACE);

	// Definimos los hilos principales
	pthread_t hOrquestadorConexiones, hConsola;

	// Levantamos el archivo de configuracion.
	LevantarConfig();

	// Intentamos reservar la memoria principal
	reservarMemoriaPrincipal();

	// Creamos las estructuras necesarias para manejar la UMV
	InstanciarTablaSegmentos();

	// Arrancamos los hilos
	pthread_create(&hOrquestadorConexiones, NULL, (void*) HiloOrquestadorDeConexiones, NULL );
	pthread_create(&hConsola, NULL, (void*) HiloConsola, NULL );

	// Cuando se cierra la consola se cierra la UMV.
	pthread_join(hConsola, (void **) NULL );
	// pthread_join(hOrquestadorConexiones, (void **) NULL );

	// Liberamos recursos
	if (g_BaseMemoria != NULL )
		free(g_BaseMemoria);
	list_clean_and_destroy_elements(g_ListaSegmentos, (void*) segmento_destroy);
	if (g_MensajeError != NULL )
		free(g_MensajeError);

	// Cerramos el archivo.
	fclose(g_ArchivoConsola);

	return EXIT_SUCCESS;
}

#if 1 // METODOS PARA CONSOLA //
void HiloConsola()
{
	while (g_Ejecutando)
	{
		printf("\n--> Ingresar comando. (HELP para obtener una lista con los comandos) : ");

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
				g_Ejecutando = 0;
				break;
			case COMANDO_CONSOLA_GRABA_SIEMPRE_ARCHIVO:
				ConsolaComandoGrabaSiempreArchivo();
				break;
			case COMANDO_ACTIVAR_TRAZA:
				ConsolaComandoActivarTraza();
				break;
			default:
				Error("COMANDO INVALIDO");
				break;
		}

	}

	//Traza("Fin del programa");
	log_trace(logger, "FIN DEL PROGRAMA");
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

	if (strncmp(buffer, CMD_GRABA_SIEMPRE_ARCHIVO, sizeof(CMD_GRABA_SIEMPRE_ARCHIVO) - 1) == 0)
		return COMANDO_CONSOLA_GRABA_SIEMPRE_ARCHIVO;

	if (strncmp(buffer, CMD_ACTIVAR_TRAZA, sizeof(CMD_ACTIVAR_TRAZA) - 1) == 0)
		return COMANDO_ACTIVAR_TRAZA;

	return 0;
}

void ConsolaComandoHelp()
{
	char *mensajeHelp = string_new();

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
	string_append_with_format(&mensajeHelp, "%s: Definir si siempre se descarga info consultada a archivo. \n", CMD_GRABA_SIEMPRE_ARCHIVO);
	string_append_with_format(&mensajeHelp, "%s: Definir si se muestra la traza de ejecución por consola. \n", CMD_ACTIVAR_TRAZA);

	printf("%s", mensajeHelp);

	free(mensajeHelp);
}

void ConsolaComandoLeerMemoria()
{
	int ok = 0;

	int idPrograma;
	int base;
	int desplazamiento;
	char* buffer;
	char grabarArchivo[2];
	int longitudBuffer;

	printf("--> Ingrese ID de programa:   ");
	scanf("%d", &idPrograma);
	printf("--> Ingrese la base del segmento:   ");
	scanf("%d", &base);
	printf("--> Ingrese el desplazamiento:   ");
	scanf("%d", &desplazamiento);
	printf("--> Ingrese la cantidad de bytes que desea leer:   ");
	scanf("%d", &longitudBuffer);
	if (!g_GrabarSiempreArchivo)
	{
		printf("--> ¿Grabar en archivo? (S/N):   ");
		scanf("%s", grabarArchivo);
	}

	buffer = malloc(longitudBuffer * sizeof(char));
	ok = LeerMemoria(idPrograma, base, desplazamiento, longitudBuffer, buffer);
	ImprimirResuladoDeLeerMemoria(ok, idPrograma, base, desplazamiento, longitudBuffer, buffer, TraducirSiNo(grabarArchivo[0]));
	if (buffer != NULL )
		free(buffer);
}

void ConsolaComandoEscribirMemoria()
{
	int ok = 0;

	int idPrograma;
	int base;
	int desplazamiento;
	char buffer[200];
	char grabarArchivo[2];
	int longitudBuffer;

	printf("--> Ingrese ID de programa:   ");
	scanf("%d", &idPrograma);
	printf("--> Ingrese la base del segmento:   ");
	scanf("%d", &base);
	printf("--> Ingrese el desplazamiento:   ");
	scanf("%d", &desplazamiento);
	printf("--> Ingrese los bytes que desea grabar:   ");
	scanf("%s", buffer);

	if (!g_GrabarSiempreArchivo)
	{
		printf("--> ¿Grabar en archivo? (S/N):   ");
		scanf("%s", grabarArchivo);
	}

	longitudBuffer = strlen(buffer);

	ok = EscribirMemoria(idPrograma, base, desplazamiento, longitudBuffer, buffer);

	ImprimirResuladoDeEscribirMemoria(ok, idPrograma, base, desplazamiento, longitudBuffer, buffer, TraducirSiNo(grabarArchivo[0]));
}

void ConsolaComandoCrearSegmento()
{
	int idPrograma;
	int tamanio;
	int idSegmento;
	char grabarArchivo[2];

	printf("--> Ingrese ID de programa:   ");
	scanf("%d", &idPrograma);
	printf("--> Ingrese el tamaño del segmento:   ");
	scanf("%d", &tamanio);
	if (!g_GrabarSiempreArchivo)
	{
		printf("--> ¿Grabar en archivo? (S/N):   ");
		scanf("%s", grabarArchivo);
	}

	idSegmento = CrearSegmento(idPrograma, tamanio);

	ImprimirResuladoDeCrearSegmento(idPrograma, idSegmento, tamanio, TraducirSiNo(grabarArchivo[0]));
}

void ConsolaComandoDestruirSegmento()
{
	int idPrograma;
	int ok;
	char grabarArchivo[2];

	printf("--> Ingrese ID de programa:   ");
	scanf("%d", &idPrograma);
	if (!g_GrabarSiempreArchivo)
	{
		printf("--> ¿Grabar en archivo? (S/N):   ");
		scanf("%s", grabarArchivo);
	}

	ok = DestruirSegmentos(idPrograma);

	ImprimirResuladoDeDestruirSegmento(idPrograma, ok, TraducirSiNo(grabarArchivo[0]));
}

void ConsolaComandoDefinirAlgoritmo()
{
	char algoritmo[2];

	printf("--> Algoritmo actual: %s", NombreDeAlgoritmo(g_AlgoritmoAsignacion));
	printf("--> Algoritmo a definir (W = Worst-fit, F = First-fit):   ");
	scanf("%s", algoritmo);

	if (ValidarCodigoAlgoritmo(algoritmo[0]))
	{
		g_AlgoritmoAsignacion = algoritmo[0];
		Imprimir(g_GrabarSiempreArchivo, "--> Seteo OK. Algoritmo actual: %s", NombreDeAlgoritmo(g_AlgoritmoAsignacion));
	}
	else
		printf("--> Código de algoritmo incorrecto. Solo se admite W o F (W = Worst-fit, F = First-fit)");

}

void ConsolaComandoGrabaSiempreArchivo()
{
	char grabarArchivo[2];

	printf("--> ¿Grabar siempre a archivo? (S/N):   ");
	scanf("%s", grabarArchivo);

	g_GrabarSiempreArchivo = TraducirSiNo(grabarArchivo[0]);
}

void ConsolaComandoDefinirRetardo()
{
	printf("--> Retardo actual (milisegundos): %d", g_Retardo);
	printf("--> Retardo nuevo (milisegundos):   ");
	scanf("%d", &g_Retardo);
	Imprimir(g_GrabarSiempreArchivo, "--> Seteo OK. Retardo actual (milisegundos): %d", g_Retardo);
}

void ConsolaComandoActivarTraza()
{
	char activarTraza[2];

	printf("--> ¿Imprimir Traza? (S/N):   ");
	scanf("%s", activarTraza);

	g_ImprimirTrazaPorConsola = TraducirSiNo(activarTraza[0]);
}

void ConsolaComandoCompactarMemoria()
{
	char grabarArchivo[2];
	printf("--> ¿Grabar en archivo? (S/N):   ");
	if (!g_GrabarSiempreArchivo)
	{
		scanf("%s", grabarArchivo);
	}

	Imprimir(g_GrabarSiempreArchivo, "\n--> MEMORIA ANTES DE COMPACTACION:   \n");
	ImprimirListadoSegmentos(TraducirSiNo(grabarArchivo[0]));
	Imprimir(g_GrabarSiempreArchivo, "\n--> COMPACTANDO...  ");
	CompactarMemoria();
	Imprimir(g_GrabarSiempreArchivo, "--> FIN COMPACTACION.   \n");
	Imprimir(g_GrabarSiempreArchivo, "\n--> MEMORIA DESPUES DE COMPACTACION:   \n");
	ImprimirListadoSegmentos(TraducirSiNo(grabarArchivo[0]));
}

void ConsolaComandoDumpEstructuras()
{
	int idPrograma;
	char todosLosSegmentos[2];
	char grabarArchivo[2];

	printf("--> ¿Quiere imprimir información todos los segmentos? (S/N):   ");
	scanf("%s", todosLosSegmentos);

	if (TraducirSiNo(todosLosSegmentos[0]))
	{
		if (!g_GrabarSiempreArchivo)
		{
			printf("\n--> ¿Grabar en archivo? (S/N):   ");
			scanf("%s", grabarArchivo);
		}

		ImprimirListadoSegmentos(TraducirSiNo(grabarArchivo[0]));

	}
	else
	{
		printf("--> Ingrese el id de programa del cual quiere imprimir informacion de sus segmentos:   ");
		scanf("%d", &idPrograma);
		if (!g_GrabarSiempreArchivo)
		{
			printf("--> ¿Grabar en archivo? (S/N):   ");
			scanf("%s", grabarArchivo);
		}

		ImprimirListadoSegmentosDePrograma(TraducirSiNo(grabarArchivo[0]), idPrograma);
	}
}

void ConsolaComandoDumpMemoriaPrincipal()
{

}

void ConsolaComandoDumpContenidoMemoriaPrincipal()
{
	int idPrograma;
	char todosLosSegmentos[2];
	char grabarArchivo[2];

	printf("--> ¿Quiere imprimir la memoria de todos los segmentos? (S/N):   ");
	scanf("%s", todosLosSegmentos);

	if (TraducirSiNo(todosLosSegmentos[0]))
	{
		if (!g_GrabarSiempreArchivo)
		{
			printf("\n--> ¿Grabar en archivo? (S/N):   ");
			scanf("%s", grabarArchivo);
		}

		ImprimirMemoriaSegmentos(TraducirSiNo(grabarArchivo[0]));

	}
	else
	{
		printf("--> Ingrese el id de programa del cual quiere imprimir su memoria:   ");
		scanf("%d", &idPrograma);
		if (!g_GrabarSiempreArchivo)
		{
			printf("--> ¿Grabar en archivo? (S/N):   ");
			scanf("%s", grabarArchivo);
		}

		ImprimirMemoriaSegmentosDePrograma(TraducirSiNo(grabarArchivo[0]), idPrograma);
	}
}

#endif

#if 1 // METODOS QUE IMPRIMEN //
void ImprimirResuladoDeCrearSegmento(int idPrograma, int idSegmento, int tamanio, int imprimirArchivo)
{
	if (idSegmento == -1)
		Imprimir(imprimirArchivo, "\nNo se pudo crear un segmento en la memoria. Id programa: %d, Tamaño solicitado segmento: %d\n", idPrograma, tamanio);
	else
	{
		Imprimir(imprimirArchivo, "%s", "\nSe creó el siguiente segmento en la memoria: \n");
		t_segmento* aux = ObtenerInfoSegmento(idPrograma, idSegmento);
		ImprimirEncabezadoDeListadoSegmentos(imprimirArchivo);
		ImprimirListadoSegmentosDeProgramaTSeg(imprimirArchivo, aux);
	}
}

void ImprimirResuladoDeDestruirSegmento(int idPrograma, int ok, int imprimirArchivo)
{
	if (ok)
		Imprimir(imprimirArchivo, "\nSe destruyeron los segmentos asociados al programa. Id programa: %d\n", idPrograma);
	else
		Imprimir(imprimirArchivo, "\nNo se pudieron destruir los segmentos asociados al programa. Id programa: %d\n", idPrograma);

	ImprimirResumenUsoMemoria(imprimirArchivo);
}

void ImprimirMemoriaSegmentos(int imprimirArchivo)
{
	int index = 0;

	ImprimirBaseMemoria(imprimirArchivo);

	void _list_elements(t_segmento *seg)
	{
		ImprimirMemoriaSegmentosDeProgramaTSeg(imprimirArchivo, seg);
		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	ImprimirResumenUsoMemoria(imprimirArchivo);
}

void ImprimirMemoriaSegmentosDePrograma(int imprimirArchivo, int idPrograma)
{
	int index = 0;
	int espacioTotalOcupado = 0;
	ImprimirBaseMemoria(imprimirArchivo);

	void _list_elements(t_segmento *seg)
	{
		if (seg->IdPrograma == idPrograma)
		{
			espacioTotalOcupado = espacioTotalOcupado + seg->Tamanio;
			ImprimirMemoriaSegmentosDeProgramaTSeg(imprimirArchivo, seg);
		}

		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	Imprimir(espacioTotalOcupado, "TOTAL Memoria usada por programa (Bytes): %d", espacioTotalOcupado);
}

void ImprimirMemoriaSegmentosDeProgramaTSeg(int imprimirArchivo, t_segmento *seg)
{
	int bytesPorLinea = 8;

	ImprimirEncabezadoDeListadoSegmentos(imprimirArchivo);
	ImprimirListadoSegmentosDeProgramaTSeg(imprimirArchivo, seg);

	char * buffer = malloc(seg->Tamanio * sizeof(char));
	memset(buffer, 0, seg->Tamanio * sizeof(char));
	memcpy(buffer, seg->UbicacionMP, seg->Tamanio * sizeof(char));

	int id = 0;

	Imprimir(imprimirArchivo, "\n CONTENIDO DE MEMORIA \n", "");

	while (id < seg->Tamanio)
	{
		char* aux = malloc(bytesPorLinea * sizeof(char));
		memset(aux, 0, bytesPorLinea * sizeof(char));
		memcpy(aux, buffer + (id * sizeof(char)), bytesPorLinea * sizeof(char));
		Imprimir(imprimirArchivo, "-%10d-   %s\n", id, aux);
		if (aux != NULL )
			free(aux);
		id = id + bytesPorLinea;
	}

	if (buffer != NULL )
		free(buffer);

}

void ImprimirListadoSegmentos(int imprimirArchivo)
{
	int index = 0;

	ImprimirBaseMemoria(imprimirArchivo);
	ImprimirEncabezadoDeListadoSegmentos(imprimirArchivo);

	void _list_elements(t_segmento *seg)
	{
		ImprimirListadoSegmentosDeProgramaTSeg(imprimirArchivo, seg);
		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	ImprimirResumenUsoMemoria(imprimirArchivo);
}

void ImprimirListadoSegmentosDePrograma(int imprimirArchivo, int idPrograma)
{
	int index = 0;

	ImprimirBaseMemoria(imprimirArchivo);
	ImprimirEncabezadoDeListadoSegmentos(imprimirArchivo);

	void _list_elements(t_segmento *seg)
	{
		if (seg->IdPrograma == idPrograma)
		{
			ImprimirListadoSegmentosDeProgramaTSeg(imprimirArchivo, seg);
		}

		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);
}

void ImprimirListadoSegmentosDeProgramaTSeg(int imprimirArchivo, t_segmento *seg)
{
	Imprimir(imprimirArchivo, "|%11d|%11d|%11d|%11d|%22u|\n", seg->IdPrograma, seg->IdSegmento, seg->Inicio, seg->Tamanio, (unsigned int) seg->UbicacionMP);
}

void ImprimirEncabezadoDeListadoSegmentos(int imprimirArchivo)
{
	Imprimir(imprimirArchivo, "%s\n", "|  ID PROG  |  ID SEG   |  INICIO   |  TAMANIO  |     UBICACION MP     |");
}

void ImprimirBaseMemoria(int imprimirArchivo)
{
	Imprimir(imprimirArchivo, "BASE MEMORIA: %u\n", (unsigned int) g_BaseMemoria);
}

void ImprimirResumenUsoMemoria(int imprimirArchivo)
{
	Imprimir(imprimirArchivo, "MEMORIA TOTAL (Bytes): %d\n", g_TamanioMemoria);
	Imprimir(imprimirArchivo, "MEMORIA USADA (Bytes): %d\n", obtenerTotalMemoriaEnUso());
}

void ImprimirResuladoDeEscribirMemoria(int ok, int idPrograma, int base, int desplazamiento, int cantidadBytes, char* buffer, int imprimirArchivo)
{
	if (ok)
	{
		Imprimir(imprimirArchivo, "Se escribió en la memoria correctamente\n");
	}
	else
	{
		Imprimir(imprimirArchivo, "Ocurrio un error al intentar escribir la memoria.\n Error: %s", g_MensajeError);
	}
}

void ImprimirResuladoDeLeerMemoria(int ok, int idPrograma, int base, int desplazamiento, int cantidadBytes, char* buffer, int imprimirArchivo)
{
	if (ok)
	{
		Imprimir(imprimirArchivo, "Bytes Leidos: %s", buffer);
	}
	else
	{
		Imprimir(imprimirArchivo, "Ocurrio un error al intentar escribir la memoria.\n Error: %s", g_MensajeError);
	}
}

void Imprimir(int ImprimirArchivo, const char* mensaje, ...)
{
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);

	printf("%s", nuevo);

	if (ImprimirArchivo | g_GrabarSiempreArchivo)
	{
		fprintf(g_ArchivoConsola, "%s", nuevo);
		fflush(g_ArchivoConsola);

		log_info(logger, "%s", nuevo);
	}

	va_end(arguments);
	if (nuevo != NULL )
		free(nuevo);
}
#endif

#if 1 // METODOS MANEJO DE ERRORES //
void Error(const char* mensaje, ...)
{
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);

	fprintf(stderr, "\nERROR: %s\n", nuevo);
	log_error(logger, "%s", nuevo);

	va_end(arguments);
	if (nuevo != NULL )
		free(nuevo);
}

void Traza(const char* mensaje, ...)
{
	/*char* nuevo = string_new();

	 va_list arguments;
	 va_start(arguments, mensaje);
	 nuevo = string_from_vformat(mensaje, arguments);

	 if (g_ImprimirTrazaPorConsola)
	 {
	 printf("TRAZA--> %s \n", nuevo);
	 }

	 log_trace(logger, "%s", nuevo);

	 free(nuevo);
	 va_end(arguments);*/
}

void ErrorFatal(const char* mensaje, ...)
{
	char* nuevo;
	va_list arguments;
	va_start(arguments, mensaje);
	nuevo = string_from_vformat(mensaje, arguments);
	printf("\nERROR FATAL--> %s \n", nuevo);
	log_error(logger, "\nERROR FATAL--> %s \n", nuevo);
	char fin;

	printf("El programa se cerrara. Presione ENTER para finalizar la ejecución.");
	scanf("%c", &fin);

	va_end(arguments);
	if (nuevo != NULL )
		free(nuevo);
	exit(EXIT_FAILURE);
}

void SetearErrorGlobal(const char* mensaje, ...)
{
	va_list arguments;
	va_start(arguments, mensaje);
	if (g_MensajeError != NULL )
	// NMR COMENTADO POR ERROR A ULTIMO MOMENTO	free(g_MensajeError);
	g_MensajeError = string_from_vformat(mensaje, arguments);
	va_end(arguments);
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
	my_addr.sin_port = htons(g_Puerto);
	my_addr.sin_addr.s_addr = htons(INADDR_ANY );
	memset(&(my_addr.sin_zero), '\0', 8 * sizeof(char));

	if (bind(socket_host, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		ErrorFatal("Error al hacer el Bind. El puerto está en uso");

	if (listen(socket_host, 10) == -1) // el "10" es el tamaño de la cola de conexiones.
		ErrorFatal("Error al hacer el Listen. No se pudo escuchar en el puerto especificado");

	//Traza("El socket está listo para recibir conexiones. Numero de socket: %d, puerto: %d", socket_host, g_Puerto);
	log_trace(logger, "SOCKET LISTO PARA RECBIR CONEXIONES. Numero de socket: %d, puerto: %d", socket_host, g_Puerto);

	while (g_Ejecutando)
	{
		int socket_client;

		size_addr = sizeof(struct sockaddr_in);

		if ((socket_client = accept(socket_host, (struct sockaddr *) &client_addr, &size_addr)) != -1)
		{
			//Traza("Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, socket_client);
			log_trace(logger, "NUEVA CONEXION ENTRANTE. Se ha conectado el cliente (%s) por el puerto (%d). El número de socket del cliente es: %d", inet_ntoa(client_addr.sin_addr), client_addr.sin_port, socket_client);
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

/*char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos)
 {
 *bytesRecibidos = 0;
 int tamanioNuevoBuffer = 0;
 int mensajeCompleto = 0;
 int cantidadBytesRecibidos = 0;
 int cantidadBytesAcumulados = 0;

 // memset se usa para llenar el buffer con 0s
 char bufferAux[BUFFERSIZE];
 if (buffer != NULL )
 free(buffer);
 buffer = string_new();
 //buffer = realloc(buffer, 1 * sizeof(char)); //--> el buffer ocupa 0 lugares (todavia no sabemos que tamaño tendra)
 //memset(buffer, 0, 1 * sizeof(char));

 while (!mensajeCompleto) // Mientras que no se haya recibido el mensaje completo
 {
 memset(bufferAux, 0, BUFFERSIZE * sizeof(char)); //-> llenamos el bufferAux con barras ceros.

 //Nos ponemos a la escucha de las peticiones que nos envie el cliente. //aca si recibo 0 bytes es que se desconecto el otro, cerrar el hilo.
 if ((*bytesRecibidos = *bytesRecibidos + recv(socket, bufferAux, BUFFERSIZE, 0)) == -1)
 {
 Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d", socket);
 mensajeCompleto = 1;
 }
 else
 {
 cantidadBytesRecibidos = strlen(bufferAux);
 cantidadBytesAcumulados = strlen(buffer);
 tamanioNuevoBuffer = cantidadBytesRecibidos + cantidadBytesAcumulados;

 if (tamanioNuevoBuffer > 0)
 {
 buffer = realloc(buffer, tamanioNuevoBuffer * sizeof(char)); //--> el tamaño del buffer sera el que ya tenia mas los caraceteres nuevos que recibimos

 sprintf(buffer, "%s%s", (char*) buffer, bufferAux); //--> el buffer sera buffer + nuevo recepcio
 }

 //Verificamos si terminó el mensaje
 if (cantidadBytesRecibidos < BUFFERSIZE)
 mensajeCompleto = 1;
 }
 }

 //Traza("RECIBO datos. socket: %d. buffer: %s", socket, (char*) buffer);
 log_trace(logger, "RECIBO DATOS. socket: %d. buffer: %s", socket, (char*) buffer);

 return buffer; //--> buffer apunta al lugar de memoria que tiene el mensaje completo completo.
 }*/

char* RecibirDatos(int socket, char *buffer, int *bytesRecibidos)
{
	*bytesRecibidos = 0;
	if (buffer != NULL )
	{
		free(buffer);
	}

	char* bufferAux = malloc(BUFFERSIZE * sizeof(char));
	memset(bufferAux, 0, BUFFERSIZE * sizeof(char)); //-> llenamos el bufferAux con barras ceros.

	if ((*bytesRecibidos = *bytesRecibidos + recv(socket, bufferAux, BUFFERSIZE, 0)) == -1)
	{
		Error("Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d", socket);
	}
	else
	{

	}

	log_trace(logger, "RECIBO DATOS. socket: %d. buffer: %s", socket, (char*) bufferAux);
	return bufferAux; //--> buffer apunta al lugar de memoria que tiene el mensaje completo completo.
}

int EnviarDatos(int socket, char *buffer)
{
// Retardo antes de contestar una solicitud (Se solicita en enunciado de TP)
	sleep(g_Retardo/1000);

	int bytecount;
	int cantidadDeBytesAEnviar = strlen(buffer);

	if ((bytecount = send(socket, buffer, cantidadDeBytesAEnviar, 0)) == -1)
		Error("No puedo enviar información a al clientes. Socket: %d", socket);

	//Traza("ENVIO datos. socket: %d. buffer: %s", socket, (char*) buffer);
	log_trace(logger, "ENVIO DATOS. socket: %d. buffer: %s", socket, (char*) buffer);

	return bytecount;
}

void CerrarSocket(int socket)
{
	close(socket);
	//Traza("SOCKET SE CIERRA: (%d).", socket);
	log_trace(logger, "SOCKET SE CIERRA: (%d).", socket);
}

#endif

#if 1 // METODOS CONFIGURACION //
void LevantarConfig()
{
	t_config* config = config_create(PATH_CONFIG);

	// Obtenemos tamaño de la memoria (cantidad de bytes del 'gran malloc')
	g_TamanioMemoria = config_get_int_value(config, "TAMANIO_MEMORIA");

	// Obtenemos el puerto del config
	g_Puerto = config_get_int_value(config, "PUERTO");

	// Obtenemos el algoritmo por defecto
	char* algoritmo = config_get_string_value(config, "ALGORITMO");
	g_AlgoritmoAsignacion = algoritmo[0];
	if (algoritmo != NULL )
		free(algoritmo);

	// Obtenemos el retardo por defecto
	g_Retardo = config_get_int_value(config, "RETARDO");

	// Obtenemos si por defecto graba siempre archivos de descarga de solicitudes por consola
	g_GrabarSiempreArchivo = config_get_int_value(config, "GRABA_ARCHIVO");

	// Obtenemos si por defecto se imprime la traza de ejecución por consola
	g_ImprimirTrazaPorConsola = config_get_int_value(config, "IMPRIME_TRAZA");

	if (config != NULL )
		free(config);
}

#endif

#if 1 // METODOS MANEJO MEMORIA //
void InstanciarTablaSegmentos()
{
	g_ListaSegmentos = list_create();
	srand(getpid()); // --> esto inicializa la semilla para la generacion de numeros aleatorios.
//	sem_init(&s_AccesoAListadoSegmentos, 0, 1); //-> inicializamos en 1 el semaforo para controlar el acceso a la memoria mientras se compacta.
}

void reservarMemoriaPrincipal()
{
// Reservamos la memoria
	g_BaseMemoria = (char*) malloc(g_TamanioMemoria);
// Rellenamos con ceros.
	memset(g_BaseMemoria, '0', g_TamanioMemoria * sizeof(char));

// si no podemos salimos y cerramos el programa.
	if (g_BaseMemoria == NULL )
	{
		ErrorFatal("No se pudo reservar la memoria.");
	}
	else
	{
		//Traza("Se reservó la memoria principal OK. Tamaño de la memoria (%d)", g_TamanioMemoria);
		log_trace(logger, "MEMORIA PRINCIPAL RESERVADA. Tamaño de la memoria (%d)", g_TamanioMemoria);
	}

}

// Si esta ok retorna id del segmento. si no retorna -1
int CrearSegmento(int idPrograma, int tamanio)
{
// Atributos a calcular:
	int idSegmento = -1;
	int inicioSegmento;
	char* ubicacionMP;

// Obtener su ubicacion en la MP (Base real)
	ubicacionMP = CalcularUbicacionMP(tamanio);

	if (tamanio > 0)
	{
		if (ubicacionMP == NULL )
		{
			// Si no se pudo asignar, trato de compactar e intentar de nuevo
			CompactarMemoria();

			ubicacionMP = CalcularUbicacionMP(tamanio);
		}

		if (ubicacionMP != NULL ) 		// Si la direccion se pudo calcular
		{
			// Calcular el ID del segmento:
			idSegmento = CalcularIdSegmento(idPrograma);

			// Obtener su inicio (base virtual)
			inicioSegmento = CalcularInicioSegmento(idPrograma);

			// Agregar el nodo en la lista 	(La insersion siempre es ordenada por ubicacion en MP)
			AgregarSegmentoALista(idPrograma, idSegmento, inicioSegmento, tamanio, ubicacionMP);

			//Traza("Se creó un segmento nuevo. Id programa: %d, Tamaño solicitado segmento: %d", idPrograma, tamanio);
			log_trace(logger, "CREADO NUEVO SEGMENTO. Id programa: %d, Tamaño solicitado segmento: %d", idPrograma, tamanio);
		}
		else
			//Traza("No se pudo crear un segmento en la memoria. Id programa: %d, Tamaño solicitado segmento: %d", idPrograma, tamanio);
			log_trace(logger, "NO SE PUDO CREAR SEGMENTO. Id programa: %d, Tamaño solicitado segmento: %d", idPrograma, tamanio);
	}
	else
		//Traza("No se pudo crear un segmento en la memoria. El tamaño debe ser mayor a 0. Id programa: %d, Tamaño solicitado segmento: %d", idPrograma, tamanio);
		log_trace(logger, "NO SE PUDO CREAR SEGMENTO. Id programa: %d, Tamaño solicitado segmento: %d", idPrograma, tamanio);

	return idSegmento;
}

int DestruirSegmentos(int idPrograma)
{
	pthread_rwlock_wrlock(&s_AccesoAListadoSegmentos);

	bool _is_program(t_segmento *p)
	{
		return p->IdPrograma == idPrograma;
	}

	while (list_remove_by_condition(g_ListaSegmentos, (void*) _is_program) != NULL )
	{
		// borra el segmento
	}

	//Traza("Se borraron los segmentos del programa. Id programa: %d.", idPrograma);
	log_trace(logger, "SEGMENTOS BORRADOS. Id programa: %d.", idPrograma);

	pthread_rwlock_unlock(&s_AccesoAListadoSegmentos);

	return 1;
}

void CompactarMemoria()
{
	pthread_rwlock_wrlock(&s_AccesoAListadoSegmentos);

	int index = 0;
	int esElPrimero = 1;
	char* inicioSegmentoAnterior;
	char* inicioSegmentoNuevo;
	char* finSegmentoAnterior;

	void _list_elements(t_segmento *seg) // -> voy a recorrer todos los elementos de la lista
	{
		if (esElPrimero) // --> si es el primer segmento, lo mando al principio de la la memoria
		{
			inicioSegmentoAnterior = seg->UbicacionMP; // --> el viejo inicio de segmento
			inicioSegmentoNuevo = g_BaseMemoria; // --> el nuevo inicio de segmento es el origen de la memoria

			memcpy(inicioSegmentoNuevo, inicioSegmentoAnterior, seg->Tamanio * sizeof(char)); // --> copiamos la memoria

			seg->UbicacionMP = inicioSegmentoNuevo; // --> la vieja base ahora es la nueva

			finSegmentoAnterior = seg->UbicacionMP + seg->Tamanio - 1; // Calculamos el fin del segmento.
			esElPrimero = 0;
		}
		else // --> si no es el primero, lo mando al final del anterior segmento
		{
			inicioSegmentoAnterior = seg->UbicacionMP; // --> el viejo inicio de segmento
			inicioSegmentoNuevo = finSegmentoAnterior + 1; // --> el nuevo inicio de segmento es el fin del anterior + 1

			memcpy(inicioSegmentoNuevo, inicioSegmentoAnterior, seg->Tamanio * sizeof(char)); // --> copiamos la memoria

			seg->UbicacionMP = inicioSegmentoNuevo; // --> la vieja base ahora es la nueva

			finSegmentoAnterior = seg->UbicacionMP + seg->Tamanio - 1;  // Calculamos el fin del segmento.
		}

		index++;
	}

// Realizo la busqueda de un lugar para el segmento
	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	//Traza("Se compactó la memoria.");
	log_trace(logger, "MEMORIA COMPACTADA");

	pthread_rwlock_unlock(&s_AccesoAListadoSegmentos);
}

void AgregarSegmentoALista(int idPrograma, int idSegmento, int inicio, int tamanio, char* ubicacionMP)
{
	pthread_rwlock_wrlock(&s_AccesoAListadoSegmentos);

// Lo agregamos a la lista (al final)
	list_add(g_ListaSegmentos, segmento_create(idPrograma, idSegmento, inicio, tamanio, ubicacionMP));

// Ordenamos la lista por ubicacion en MP (La lista siempre tiene que estar ordenada por ubicacion en MP
	bool _ubicacion_mp(t_segmento *primero, t_segmento *segundo)
	{
		return primero->UbicacionMP < segundo->UbicacionMP;
	}

	list_sort(g_ListaSegmentos, (void*) _ubicacion_mp);

	pthread_rwlock_unlock(&s_AccesoAListadoSegmentos);
}

int CalcularIdSegmento(int idPrograma)
{
// Buscar en la lista la cantidad de programas que hay con el mismo IDprog y sumarle 1

	int index = 0;
	int cantidadSegmentosDePrograma = 0;

	void _list_elements(t_segmento *seg)
	{
		if (seg->IdPrograma == idPrograma)
			cantidadSegmentosDePrograma++;
		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	return cantidadSegmentosDePrograma + 1;
}

int CalcularInicioSegmento(int idPrograma)
{
// Agarro el segmento correspondiente al programa que tiene mayot inicio y hago (inicio + tamaño + numero aleatorio entre 0 y 1000)

	int index = 0;
	int inicio = 0;
	int tamanio = 0;

	void _list_elements(t_segmento *seg)
	{
		if (seg->IdPrograma == idPrograma)
			if (inicio < seg->Inicio)
			{
				inicio = seg->Inicio;
				tamanio = seg->Tamanio;
			}

		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	inicio = inicio + tamanio + numeroAleatorio(0, 1000);

	return inicio;
}

t_segmento* ObtenerInfoSegmento(int idPrograma, int idSegmento)
{
	pthread_rwlock_rdlock(&s_AccesoAListadoSegmentos);

	t_segmento* aux = NULL;
	int index = 0;

	void _list_elements(t_segmento *seg)
	{
		if ((seg->IdPrograma == idPrograma) & (seg->IdSegmento == idSegmento))
			aux = seg;

		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	pthread_rwlock_unlock(&s_AccesoAListadoSegmentos);

	return aux;
}

char* CalcularUbicacionMP(int tamanioSegmento)
{
	if (g_AlgoritmoAsignacion == ALGORITMO_WORST_FIT)
	{
		return CalcularUbicacionMP_WorstFit(tamanioSegmento);
	}
	else
	{
		return CalcularUbicacionMP_FirstFit(tamanioSegmento);
	}
}

char* CalcularUbicacionMP_WorstFit(int tamanioRequeridoSegmento)
{
	char* retorno = NULL;

	pthread_rwlock_rdlock(&s_AccesoAListadoSegmentos);

// Obtenemos la canitdad de segmentos deifnidos
	int cantidadSegmentos = ObtenerCantidadSegmentos();
	if (cantidadSegmentos == 0)
	{
// -> si no hay quiere decir que este es el primer segmento, le damos la primer ubicacion en MP
		if (tamanioRequeridoSegmento < g_TamanioMemoria) // -> siempre y cuando tengamos memoria suficiente
			retorno = g_BaseMemoria;
	}
	else
	{
// Necesitamos encontrar el "peor" lugar para ubicar el segmento.
// nos conviene recorrer la lista e ir analizando el espacio entre un segmento y otro.
// De todos esos quedarse con el mayor (el inicio de ese espacio = fin segmento anterior + tamaño)

// Sino... hay que calcularla...

		int index = 0;
		char* finSegmentoActual;
		char* inicioProximoSegmento;
		int espacioEntreSegmentos;
		int mayorEspacio = 0;
		t_segmento *aux;

		void _list_elements(t_segmento *seg) // -> voy a recorrer todos los elementos de la lista
		{
			// Calculo donde va a terminar el segmento seg
			finSegmentoActual = seg->UbicacionMP + seg->Tamanio - 1;

			// Calculo donde empieza el proximo segmento
			if (cantidadSegmentos > index + 1)
			{
				// Si no es el ultimo segmento de la lista
				// Se calcula como la ubicacion del proximo segmento
				aux = list_get(g_ListaSegmentos, index + 1);
				inicioProximoSegmento = aux->UbicacionMP;
			}
			else
			{
				// Si es el ultimo segmento de la lista
				// Se calcula como el fin de la memoria (ultima posicion de memoria) (BaseMemoria + TamañoTotalMEeoria)
				inicioProximoSegmento = g_BaseMemoria + g_TamanioMemoria;
			}

			// Calculo espacio entre segmentos
			espacioEntreSegmentos = inicioProximoSegmento - finSegmentoActual;

			// Si es mayor al mayor, lo anoto como nuevo mayor espacio libre.
			if (espacioEntreSegmentos > mayorEspacio)
			{
				mayorEspacio = espacioEntreSegmentos;
				retorno = finSegmentoActual + 1; //-> el retorno es el inicio de ese espacio libre
			}

			index++;
		}

// Realizo la busqueda de un lugar para el segmento
		list_iterate(g_ListaSegmentos, (void*) _list_elements);

// Si el mayor espacio es menor al tamaño del segmento que se quiere ubicar, entonces no hay lugar para el segmento.
		if (mayorEspacio < tamanioRequeridoSegmento)
			retorno = NULL;
	}

	pthread_rwlock_unlock(&s_AccesoAListadoSegmentos);
	return retorno;
}

char* CalcularUbicacionMP_FirstFit(int tamanioRequeridoSegmento)
{
	pthread_rwlock_rdlock(&s_AccesoAListadoSegmentos);

	char* retorno = NULL;
// Obtenemos la canitdad de segmentos deifnidos
	int cantidadSegmentos = ObtenerCantidadSegmentos();
	if (cantidadSegmentos == 0)
	{
// -> si no hay quiere decir que este es el primer segmento, le damos la primer ubicacion en MP
		if (tamanioRequeridoSegmento < g_TamanioMemoria) // -> siempre y cuando tengamos memoria suficiente
			retorno = g_BaseMemoria;
	}
	else
	{
// Sino... hay que calcularla...

		int ubicado = 0;
		int index = 0;
		char* finSegmentoActual;
		char* inicioProximoSegmento;
		int espacioEntreSegmentos;
		t_segmento *aux;

		void _list_elements(t_segmento *seg) // -> voy a recorrer todos los elementos de la lista
		{
			if (ubicado == 0) // --> si ya lo pude ubicar en MP, listo, no hago nada.. sino lo trato de ubicar
			{
				// Calculo donde va a terminar el segmento seg
				finSegmentoActual = seg->UbicacionMP + seg->Tamanio - 1;

				// Calculo donde empieza el proximo segmento
				if (cantidadSegmentos > index + 1)
				{
					// Si no es el ultimo segmento de la lista
					// Se calcula como la ubicacion del proximo segmento
					aux = list_get(g_ListaSegmentos, index + 1);
					inicioProximoSegmento = aux->UbicacionMP;
				}
				else
				{
					// Si es el ultimo segmento de la lista
					// Se calcula como el fin de la memoria (ultima posicion de memoria) (BaseMemoria + TamañoTotalMEeoria)
					inicioProximoSegmento = g_BaseMemoria + g_TamanioMemoria;
				}

				espacioEntreSegmentos = inicioProximoSegmento - finSegmentoActual;

				if (espacioEntreSegmentos > tamanioRequeridoSegmento)
				{
					retorno = finSegmentoActual + 1;
					ubicado = 1;
				}
			}

			index++;
		}

// Realizo la busqueda de un lugar para el segmento
		list_iterate(g_ListaSegmentos, (void*) _list_elements);
	}

	pthread_rwlock_unlock(&s_AccesoAListadoSegmentos);

	return retorno;
}

int ObtenerCantidadSegmentos()
{
	bool _true(void *seg)
	{
		return 1;
	}

	return list_count_satisfying(g_ListaSegmentos, _true);
}

int obtenerTotalMemoriaEnUso()
{
	int index = 0;
	int total = 0;

	void _list_elements(t_segmento *seg)
	{
		total = total + seg->Tamanio;
		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	return total;
}

int EscribirMemoria(int idPrograma, int base, int desplazamiento, int cantidadBytes, char* buffer)
{
	pthread_rwlock_rdlock(&s_AccesoAListadoSegmentos);

// Primero verificamos que el programa pueda acceder a ese lugar de memoria
	int ok = VerificarAccesoMemoria(idPrograma, base, desplazamiento, cantidadBytes);

	if (ok)
	{
// Si se puede acceder escribo la memoria
		char* baseSegmento;
		baseSegmento = ObtenerUbicacionMPEnBaseAUbicacionVirtual(idPrograma, base);
		memcpy((baseSegmento + (desplazamiento * sizeof(char))), buffer, cantidadBytes * sizeof(char));
		//Traza("Se escribió en memoria. Id programa: %d, base logica: %d, base real: %u,  desplazamiento: %d, cantidad de bytes: %d, buffer: %s", idPrograma, base, (unsigned int) baseSegmento, desplazamiento, cantidadBytes, buffer);
		log_trace(logger, "ESCRITURA MEMORIA. Id programa: %d, base logica: %d, base real: %u,  desplazamiento: %d, cantidad de bytes: %d, buffer: %s", idPrograma, base, (unsigned int) baseSegmento, desplazamiento, cantidadBytes, buffer);
	}

	pthread_rwlock_unlock(&s_AccesoAListadoSegmentos);

	return ok;
}

int LeerMemoria(int idPrograma, int base, int desplazamiento, int cantidadBytes, char* buffer)
{
	pthread_rwlock_rdlock(&s_AccesoAListadoSegmentos);

	memset(buffer, 0, cantidadBytes * sizeof(char));

// Primero verificamos que el programa pueda acceder a ese lugar de memoria
	int ok = VerificarAccesoMemoria(idPrograma, base, desplazamiento, cantidadBytes);

	if (ok)
	{
// Si se puede acceder escribo la memoria
		char* baseSegmento;
		baseSegmento = ObtenerUbicacionMPEnBaseAUbicacionVirtual(idPrograma, base);
		memcpy(buffer, (baseSegmento + (desplazamiento * sizeof(char))), (cantidadBytes) * sizeof(char));
		//Traza("Se leyó  de memoria. Id programa: %d, base logica: %d, base real: %u,  desplazamiento: %d, cantidad de bytes: %d, buffer: %s", idPrograma, base, (unsigned int) baseSegmento, desplazamiento, cantidadBytes, buffer);
		log_trace(logger, "LECTURA MEMORIA. Id programa: %d, base logica: %d, base real: %u,  desplazamiento: %d, cantidad de bytes: %d, buffer: %s", idPrograma, base, (unsigned int) baseSegmento, desplazamiento, cantidadBytes, buffer);
	}

	pthread_rwlock_unlock(&s_AccesoAListadoSegmentos);

	return ok;
}

char * ObtenerUbicacionMPEnBaseAUbicacionVirtual(int idPrograma, int base)
{
	char* retorno = NULL;
	t_segmento* aux = NULL;
	int index = 0;

	void _list_elements(t_segmento *seg)
	{
		if ((seg->IdPrograma == idPrograma) & (seg->Inicio == base))
			aux = seg;

		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	if (aux != NULL )
		retorno = aux->UbicacionMP;

	return retorno;
}

// Retorta 1 si se puede accedeer a esa posicion de memoria, 0 si no y deja el mensaje de error en la variable global.
int VerificarAccesoMemoria(int idPrograma, int base, int desplazamiento, int cantidadBytes)
{
	int accesoOk = 0;
	t_segmento* aux = NULL;
	int index = 0;

	void _list_elements(t_segmento *seg)
	{
		if ((seg->IdPrograma == idPrograma) & (seg->Inicio == base))
			aux = seg;

		index++;
	}

	list_iterate(g_ListaSegmentos, (void*) _list_elements);

	if (aux == NULL )
	{

		SetearErrorGlobal("SEGMENTATION FAULT. El programa (%d) no tiene asignado un segmento con base %d", idPrograma, base);
		//Traza("Ocurrio una violación en el acceso a segmentos. %s", g_MensajeError);
		log_trace(logger, "SOLICITUD INVALIDA %s", g_MensajeError);
	}
	else
	{
		int posicionSolicitada = base + desplazamiento + cantidadBytes;
		int posicionMaximaDelSegmento = aux->Inicio + aux->Tamanio;
		if (posicionSolicitada > posicionMaximaDelSegmento)
		{
			SetearErrorGlobal("SEGMENTATION FAULT. El programa (%d) no puede acceder a la posicion de memoria %d. (El segmento termina en la posicion %d)", idPrograma, posicionSolicitada, posicionMaximaDelSegmento);
			//Traza("Ocurrio una violación en el acceso a segmentos. %s", g_MensajeError);
			log_trace(logger, "SOLICITUD INVALIDA %s", g_MensajeError);
		}
		else
			accesoOk = 1;
	}

	return accesoOk;
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
	int tipo_Cliente = 0;

// Es el encabezado del mensaje. Nos dice que acción se le está solicitando al UMV
	int tipo_mensaje = 0;

// Dentro del buffer se guarda el mensaje recibido por el cliente.
	char* buffer;
	buffer = malloc(1 * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.

// Cantidad de bytes recibidos.
	int bytesRecibidos;

// La variable fin se usa cuando el cliente quiere cerrar la conexion: chau chau!
	int desconexionCliente = 0;

// Código de salida por defecto
	int code = 0;

	while ((!desconexionCliente) & g_Ejecutando)
	{
		//	buffer = realloc(buffer, 1 * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.
		if (buffer != NULL )
			free(buffer);
		buffer = string_new();
		//Recibimos los datos del cliente
		buffer = RecibirDatos(socket, buffer, &bytesRecibidos);

		if (bytesRecibidos > 0)
		{
			//Analisamos que peticion nos está haciendo (obtenemos el comando)
			tipo_mensaje = ObtenerComandoMSJ(buffer);

			//Evaluamos los comandos
			switch (tipo_mensaje)
			{
				case MSJ_GET_BYTES:
					buffer = ComandoGetBytes(buffer, id_Programa, tipo_Cliente);
					break;
				case MSJ_SET_BYTES:
					buffer = ComandoSetBytes(buffer, id_Programa, tipo_Cliente);
					break;
				case MSJ_HANDSHAKE:
					buffer = ComandoHandShake(buffer, &tipo_Cliente);
					break;
				case MSJ_CAMBIO_PROCESO:
					buffer = ComandoCambioProceso(buffer, &id_Programa);
					break;
				case MSJ_CREAR_SEGMENTO:
					buffer = ComandoCrearSegmento(buffer, tipo_Cliente);
					break;
				case MSJ_DESTRUIR_SEGMENTO:
					buffer = ComandoDestruirSegmento(buffer, tipo_Cliente);
					break;
				default:
					buffer = RespuestaClienteError(buffer, "El ingresado no es un comando válido");
					break;
			}

			// Enviamos datos al cliente.
			EnviarDatos(socket, buffer);
		}
		else
			desconexionCliente = 1;

	}

	CerrarSocket(socket);

	return code;
}

int ObtenerComandoMSJ(char* buffer)
{
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
	return posicionDeBufferAInt(buffer, 0);
}

char* ComandoHandShake(char *buffer, int *tipoCliente)
{
// Formato del mensaje: CD
// C = codigo de mensaje ( = 3)
// D = Tipo cliente (1  = KERNEL , 2 = CPU)

	log_trace(logger, "COMANDO HANDSHAKE. Tipo cliente: %d", tipoCliente);

	int tipoDeCliente = posicionDeBufferAInt(buffer, 1);

	if (EsTipoClienteValido(tipoDeCliente))
	{
		*tipoCliente = tipoDeCliente;
		buffer = RespuestaClienteOk(buffer);
	}
	else
	{
		*tipoCliente = 0;
		SetearErrorGlobal("HANDSHAKE ERROR. Tipo de cliente invalido (%d). Solo puede ser '1' = KERNEL o '2' = CPU.", posicionDeBufferAInt(buffer, 1));
		buffer = RespuestaClienteError(buffer, g_MensajeError);
	}

	return buffer;
}

char* ComandoGetBytes(char *buffer, int idProg, int tipoCliente)
{
// Lee la memoria
// Formato del mensaje: CABBBBBBBBBCDDDDDDDDDFOOOOOOOOOMMMMMMMM....
// C = codigo de mensaje ( = 2)
// A = Cantidad de digitos que tiene la base
// BBBBBBBBB = Base del segmento (hasta 9 digitos)
// C = Cantidad de digitos que tiene el desplazamiento
// DDDDDDDDD = desplazamiento (hasta 9 digitos)
// F = Cantidad de digitos que tiene la cantidad de caracteres que se quieren leer
// OOOOOOOOO = Cantidad de caracteres que se quieren leer (hasta 9 digitos)

// Retorna: 1 + Bytes si se leyo ok
//			0 + mensaje error si no se pudo leer

	log_trace(logger, "COMANDO GET BYTES. Id prog: %d", idProg);

	int ok = 0;

	int base = 0;
	int desplazamiento = 0;
	int longitudBuffer = 0;

	int cantidadDigitosBase = 0;
	int cantidadDigitosDesplazamiento = 0;
	int cantidadDigitoslongitudBuffer = 0;

	cantidadDigitosBase = posicionDeBufferAInt(buffer, 1);
	base = subCadenaAInt(buffer, 2, cantidadDigitosBase);

	cantidadDigitosDesplazamiento = posicionDeBufferAInt(buffer, 2 + cantidadDigitosBase);
	desplazamiento = subCadenaAInt(buffer, 2 + cantidadDigitosBase + 1, cantidadDigitosDesplazamiento);

	cantidadDigitoslongitudBuffer = posicionDeBufferAInt(buffer, 2 + cantidadDigitosBase + 1 + cantidadDigitosDesplazamiento);
	longitudBuffer = subCadenaAInt(buffer, 2 + cantidadDigitosBase + 1 + cantidadDigitosDesplazamiento + 1, cantidadDigitoslongitudBuffer);

	char* lectura = malloc(longitudBuffer * sizeof(char));
	ok = LeerMemoria(idProg, base, desplazamiento, longitudBuffer, lectura);

	if (ok)
	{
		if (buffer != NULL )
			free(buffer);
		buffer = string_new();
		string_append(&buffer, "1");
		string_append(&buffer, lectura);

		/*int tamanio = (longitudBuffer + 1) * sizeof(char);
		 buffer = realloc(buffer, tamanio * sizeof(char));
		 memset(buffer, 0, tamanio * sizeof(char));
		 sprintf(buffer, "%s%s", "1", lectura);*/
	}
	else
	{
		char* stringErrorAux = string_new();
		string_append(&stringErrorAux, g_MensajeError);
		SetearErrorGlobal("ERROR LEER MEMORIA. %s. Id programa: %d, base: %d, desplazamiento: %d, longitud buffer: %d", stringErrorAux, idProg, base, desplazamiento, longitudBuffer);
		buffer = RespuestaClienteError(buffer, g_MensajeError);
		if (stringErrorAux != NULL )
			free(stringErrorAux);
	}

	if (lectura != NULL )
		free(lectura);
	return buffer;
}

char* ComandoSetBytes(char *buffer, int idProg, int tipoCliente)
{

// Escribe en memoria
// Formato del mensaje: CABBBBBBBBBCDDDDDDDDDFOOOOOOOOOMMMMMMMM....
// C = codigo de mensaje ( = 2)
// A = Cantidad de digitos que tiene la base
// BBBBBBBBB = Base del segmento (hasta 9 digitos)
// C = Cantidad de digitos que tiene el desplazamiento
// DDDDDDDDD = desplazamiento (hasta 9 digitos)
// F = Cantidad de digitos que tiene la cantidad de caracteres del mensaje
// OOOOOOOOO = Cantidad de caracteres del mensaje (hasta 9 digitos)
// MMMMMMMM... Mensaje, hasta OOOOOOOOO caraceteres

	log_trace(logger, "COMANDO SET BYTES. Id prog: %d", idProg);

	int ok = 0;

	int base = 0;
	int desplazamiento = 0;
	int longitudBuffer = 0;

	int cantidadDigitosBase = 0;
	int cantidadDigitosDesplazamiento = 0;
	int cantidadDigitoslongitudBuffer = 0;

	cantidadDigitosBase = posicionDeBufferAInt(buffer, 1);
	base = subCadenaAInt(buffer, 2, cantidadDigitosBase);

	cantidadDigitosDesplazamiento = posicionDeBufferAInt(buffer, 2 + cantidadDigitosBase);
	desplazamiento = subCadenaAInt(buffer, 2 + cantidadDigitosBase + 1, cantidadDigitosDesplazamiento);

	cantidadDigitoslongitudBuffer = posicionDeBufferAInt(buffer, 2 + cantidadDigitosBase + 1 + cantidadDigitosDesplazamiento);
	longitudBuffer = subCadenaAInt(buffer, 2 + cantidadDigitosBase + 1 + cantidadDigitosDesplazamiento + 1, cantidadDigitoslongitudBuffer);

	int posicionComienzoMensaje = 2 + cantidadDigitosBase + 1 + cantidadDigitosDesplazamiento + 1 + cantidadDigitoslongitudBuffer;
	int logitud = strlen(buffer);
	if ((posicionComienzoMensaje + longitudBuffer) <= logitud) // Validamos me pasen la cantidad de bytes que piden que tengo que escribir
	{
		char * mensaje = string_substring(buffer, posicionComienzoMensaje, longitudBuffer);
		ok = EscribirMemoria(idProg, base, desplazamiento, longitudBuffer, mensaje);

		if (ok)
		{
			buffer = RespuestaClienteOk(buffer);
		}
		else
		{
			char* stringErrorAux = string_new();
			string_append(&stringErrorAux, g_MensajeError);
			SetearErrorGlobal("ERROR ESCRIBIR MEMORIA. %s. Id programa: %d, base: %d, desplazamiento: %d, longitud buffer: %d, buffer: %s", stringErrorAux, idProg, base, desplazamiento, longitudBuffer, mensaje);
			buffer = RespuestaClienteError(buffer, g_MensajeError);
			if (stringErrorAux != NULL )
				free(stringErrorAux);
		}

		if (mensaje != NULL )
			free(mensaje);
	}
	else
	{
		SetearErrorGlobal("ERROR ESCRIBIR MEMORIA. Se solicitó grabar una cantidad de bytes pero se pasaron menos. Id programa: %d, base: %d, desplazamiento: %d, longitud buffer: %d", idProg, base, desplazamiento, longitudBuffer);
		buffer = RespuestaClienteError(buffer, g_MensajeError);
	}
	return buffer;
}

char* ComandoCambioProceso(char *buffer, int *idProg)
{
//Cambia el proceso activo sobre el que el cliente está trabajando. Así el hilo sabe con que proceso trabaja el cliente.

// Formato del mensaje: CDPPPPPPPPP
// C = codigo de mensaje ( = 4)
// D = Cantidad de digitos que tiene el Id del programa
// PPPPPPPPP = ID del programa (hasta 9 digitos)

	log_trace(logger, "COMANDO CAMBIO PROCESO ACTIVO");

	log_trace(logger, "COMANDO CAMBIO PROCESO ACTIVO - ID PROG ANTERIOR: %d ", *idProg);

	int cantidadDigitosCodProg = posicionDeBufferAInt(buffer, 1);
	*idProg = subCadenaAInt(buffer, 2, cantidadDigitosCodProg);

	log_trace(logger, "COMANDO CAMBIO PROCESO ACTIVO - ID PROG NUEVO: %d ", *idProg);

	if (*idProg > 0)
	{
		buffer = RespuestaClienteOk(buffer);
	}
	else
	{
		SetearErrorGlobal("ERROR CAMBIO PROGRAMA ACTIVO. MENSAJE ENVIADO = %s", buffer);
		buffer = RespuestaClienteError(buffer, g_MensajeError);
	}

	return buffer;
}

char* ComandoCrearSegmento(char *buffer, int tipoCliente)
{
//Crea un segmento para un programa.
// Formato del mensaje: CDPPPPPPPPPETTTTTTTTT
// C = codigo de mensaje ( = 5)
// D = Cantidad de digitos que tiene el Id del programa
// PPPPPPPPP = ID del programa (hasta 9 digitos)
// E = Cantidad de digitos que tiene el Tamaño del segmento
// TTTTTTTTT = tamaño del segmento (hasta 9 digitos)

// Retorno:
// 0: error (continuado del msj)
// Sino retorna la base del segmento con el sigueinte formato
// 1 + D + BBBBBBBBB
// Donde
// D = Digitos de la base
// BBBBBBBBB = base (hasta 9 digitos)

	log_trace(logger, "COMANDO CREAR SEGMENTO");

	if (tipoCliente == TIPO_KERNEL)
	{
		int idPrograma = 0;
		int idSegmento = 0;
		int tamanio = 0;
		int cantidadDigitosCodProg = 0;
		int cantidadDigitosTamanio = 0;

		cantidadDigitosCodProg = posicionDeBufferAInt(buffer, 1);
		idPrograma = subCadenaAInt(buffer, 2, cantidadDigitosCodProg);
		cantidadDigitosTamanio = posicionDeBufferAInt(buffer, 1 + 1 + cantidadDigitosCodProg);
		tamanio = subCadenaAInt(buffer, 1 + 1 + cantidadDigitosCodProg + 1, cantidadDigitosTamanio);

		idSegmento = CrearSegmento(idPrograma, tamanio);

		if (idSegmento == -1)
		{
			SetearErrorGlobal("No se pudo crear un segmento en la memoria. Id programa: %d, Tamaño solicitado segmento: %d", idPrograma, tamanio);
			buffer = RespuestaClienteError(buffer, g_MensajeError);
		}
		else
		{
			t_segmento* aux = ObtenerInfoSegmento(idPrograma, idSegmento);
			int tamanio = (cantidadDigitos(aux->Inicio) + 2) * sizeof(char);
			buffer = realloc(buffer, tamanio * sizeof(char));
			memset(buffer, 0, tamanio * sizeof(char));
			sprintf(buffer, "%s%d%d", "1", cantidadDigitos(aux->Inicio), aux->Inicio);
		}
	}
	else
	{
		SetearErrorGlobal("ERROR CREAR SEGMENTO. El tipo de cliente que puede solicitar esta operacion solo puede ser KERNEL (1), usted es del tipo (%d) ", tipoCliente);
		buffer = RespuestaClienteError(buffer, g_MensajeError);
	}

	return buffer;
}

char* ComandoDestruirSegmento(char *buffer, int tipoCliente)
{
// Destruye los segmentos de un programa.
// Formato del mensaje: CDPPPPPPPPP
// C = codigo de mensaje ( = 6)
// D = Cantidad de digitos que tiene el Id del programa
// PPPPPPPPP = ID del programa (hasta 9 digitos)

	log_trace(logger, "COMANDO DESTRUIR SEGMENTO");

	if (tipoCliente == TIPO_KERNEL)
	{
		int ok = 0;
		int idPrograma = 0;
		int cantidadDigitosCodProg = 0;

		cantidadDigitosCodProg = chartToInt(buffer[1]);
		idPrograma = atoi(string_substring(buffer, 2, cantidadDigitosCodProg));

		ok = DestruirSegmentos(idPrograma);

		if (ok)
		{
			buffer = RespuestaClienteOk(buffer);
		}
		else
		{
			SetearErrorGlobal("No se pudo destruir los segmentos. Id programa: %d", idPrograma);
			buffer = RespuestaClienteError(buffer, g_MensajeError);
		}
	}
	else
	{
		SetearErrorGlobal("ERROR DESTRUIR SEGMENTO. El tipo de cliente que puede solicitar esta operacion solo puede ser KERNEL (1), usted es del tipo (%d) ", tipoCliente);
		buffer = RespuestaClienteError(buffer, g_MensajeError);
	}

	return buffer;
}

char* RespuestaClienteOk(char *buffer)
{
	/*int tamanio = sizeof(char) * 2;
	 buffer = realloc(buffer, tamanio * sizeof(char));
	 memset(buffer, 0, tamanio * sizeof(char));
	 sprintf(buffer, "%s", "1");*/

	if (buffer != NULL )
		free(buffer);

	buffer = string_new();
	string_append(&buffer, "1");

	return buffer;
}

char* RespuestaClienteError(char *buffer, char *msj)
{
	//int cantidadBytesBuffer = strlen(buffer);
	if (buffer != NULL )
		free(buffer);
	buffer = string_new();
	string_append(&buffer, "0");
	string_append(&buffer, msj);

	/*	int tamanio = (strlen(msj) + 1) * sizeof(char);
	 buffer = realloc(buffer, tamanio * sizeof(char));
	 memset(buffer, 0, tamanio * sizeof(char));
	 sprintf(buffer, "%s%s", "0", msj);*/
	free(msj);
	return buffer;
}

#endif

#if 1 // OTROS METODOS //
int chartToInt(char x)
{
	int numero = 0;
	char * aux = string_new();
	string_append_with_format(&aux, "%c", x);
	//char* aux = malloc(1 * sizeof(char));
	//sprintf(aux, "%c", x);
	numero = strtol(aux, (char **) NULL, 10);

	if (aux != NULL )
		free(aux);
	return numero;
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
			break;
	}

	return "Error, codigo de algoritmo invalido.";
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

			break;
	}

	return 0;
}

int EsTipoClienteValido(int tipoCliente)
{
	switch (tipoCliente)
	{
		case TIPO_KERNEL:
			return 1;
			break;
		case TIPO_CPU:
			return 1;
			break;
		default:

			break;
	}

	return 0;
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

			break;
	}

	return 0;
}

int numeroAleatorio(int desde, int hasta)
{
	return rand() % (hasta - desde + 1) + hasta;
}

int cantidadCaracteres(char* buffer)
{
	int cantidad;
	char* bufferAux = NULL;
	sprintf(bufferAux, "%s/0", buffer);
	cantidad = strlen(bufferAux);
	if (bufferAux != NULL )
		free(bufferAux);
	return cantidad;
}

int cantidadDigitos(int num)
{
	int contador = 1;

	while (num / 10 > 0)
	{
		num = num / 10;
		contador++;
	}

	return contador;
}

int subCadenaAInt(char* text, int start, int length)
{
	int retorno = 0;
	int logitudBuffer = 0;
	logitudBuffer = strlen(text);

	if (logitudBuffer >= (start + length))
	{
		char* aux;
		aux = string_substring(text, start, length);
		retorno = atoi(aux);
		if (aux != NULL )
			free(aux);
	}
	return retorno;
}

int posicionDeBufferAInt(char* buffer, int posicion)
{
	int logitudBuffer = 0;
	logitudBuffer = strlen(buffer);

	if (logitudBuffer <= posicion)
		return 0;
	else
		return chartToInt(buffer[posicion]);
}

#endif
