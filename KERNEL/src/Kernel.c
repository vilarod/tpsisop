/*
 ============================================================================
 Name        : Kernel.c
 Author      : Garras
 Version     : 1.0
 Copyright   : Garras - UTN FRBA 2014
 Description : Ansi-style
 Testing	 : Para probarlo es tan simple como ejecutar en el terminator
 ============================================================================
 */

#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <commons/config.h>
#include <string.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <stdio.h>
#include "Kernel.h"

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/KERNEL/src/config.cfg"

//Tipo de cliente conectado
#define TIPO_CPU       	  2
#define TIPO_PROGRAMA     1

//Mensajes aceptados
#define MSJ_RECIBO_PROGRAMA       	1
#define MSJ_IMPRIMI_ESTO	      	2
#define MSJ_HANDSHAKE             	3
#define MSJ_CPU_IMPRIMI	      		'2'
#define MSJ_CPU_HANDSHAKE           '3'
#define MSJ_CPU_FINAlQUAMTUM		'4'
#define MSJ_CPU_OBTENERVALORGLOBAL	'5'
#define MSJ_CPU_GRABARVALORGLOBAL	'6'
#define MSJ_CPU_ABANDONA			'7'
#define MSJ_CPU_WAIT				'8'
#define MSJ_CPU_SIGNAL				'9'
#define MSJ_CPU_FINAlIZAR			'F'
#define MSJ_CPU_LIBERAR				'L'

#define HANDSHAKEUMV '31'

/** Longitud del buffer  */
#define BUFFERSIZE 1024 * 4
#define DIRECCION INADDR_ANY

int socketumv;
int main(int argv, char** argc) {

	//Obtener puertos e ip de la umv
	UMV_PUERTO = ObtenerPuertoUMV();
	UMV_IP = ObtenerIPUMV();
	Puerto = ObtenerPuertoConfig();
	PuertoPCP = ObtenerPuertoPCPConfig();

	//Obtener datos del Kernell
	Quamtum = ObtenerQuamtum();
	Retardo = ObtenerRetardo();
	Multi = ObtenerMulti();
	//Obtener las listas
	llenarSemaforoConfig();
	llenarVarGlobConfig();
	llenarDispositConfig();

	//Inicializo semaforo
	seminit(&newCont, 0);
	seminit(&readyCont, 0);
	seminit(&CPUCont, 0);
	seminit(&finalizarCont, 0);
	seminit(&imprimirCont, 0);
	seminit(&multiCont, Multi);

	//Crear Listas de estados
	listaNew = list_create();
	listCPU = list_create();
	listaReady = list_create();
	listaFin = list_create();
	listaDispositivos = list_create();
	listaImprimir = list_create();
	listaVarGlobal = list_create();

	//Creacion del hilo plp
	pthread_t plp;
	pthread_create(&plp, NULL, PLP, NULL );
	pthread_join(plp, NULL );

	//liberamos recursos
	list_clean_and_destroy_elements(listCPU, (void*) cpu_destroy);

	printf("Finalizado\n");

	return EXIT_SUCCESS;
}

/* Hilo de PLP (lo que ejecuta) */
void *PLP(void *arg) {

	//Me conecto a la UMV
	conectarAUMV();

	//Creo el hilo de pcp
	pthread_t pcp, escuchaFinEImprimir;
	pthread_create(&pcp, NULL, PCP, NULL );
	pthread_join(pcp, NULL );

	crearEscucha();

	pthread_create(&escuchaFinEImprimir, NULL, IMPRIMIRYFIN, NULL );
	pthread_join(escuchaFinEImprimir, NULL );

	return NULL ;
}

void *IMPRIMIRYFIN(void *arg) {

	//los waits y while 1 para las cola de fin y de imprimir algo

	return NULL ;
}

/* Hilo de PCP (lo que ejecuta) */
void *PCP(void *arg) {

	/// MAGIA DE DANI ///

	listCPU = list_create();
	//crear hilo de escucha CPU
	pthread_t plpCPU, plpNew, plpReady, plpBloqueado;
	pthread_create(&plpCPU, NULL, HiloOrquestadorDeCPU, NULL );

	//crear hilo de new
	pthread_create(&plpNew, NULL, moverReadyDeNew, NULL );

	//crear hilo de ready
	pthread_create(&plpReady, NULL, moverEjecutar, NULL );

	//crear hilo de bloqueado
	pthread_create(&plpBloqueado, NULL, hiloDispositivos, NULL );

	pthread_join(plpCPU, NULL );
	pthread_join(plpReady, NULL );
	pthread_join(plpBloqueado, NULL );

	return NULL ;
}

void *moverEjecutar(void *arg) {
	t_CPU* auxcpu;
	PCB* auxPCB;
	t_list* auxList;
	int edatos = 0;
	char* buffer;
	char* cadena;
	//buscar CPU libre y mandar PCB
	while (1) {
		semwait(&readyCont);
		semwait(&CPUCont);
		pthread_mutex_lock(&mutexReady);
		buffer = malloc(1 * sizeof(char) + 1);
		buffer = "1";
		pthread_mutex_lock(&mutexCPU);
		auxcpu = encontrarCPULibre();
		if (auxcpu != NULL ) {
			if (list_size(listaReady) > 0) {
				auxList = list_take(listaReady, 1);
				auxPCB = list_get(auxList, 0);
				auxcpu->idPCB = auxPCB;
				string_append(&buffer, string_itoa(Quamtum));
				string_append(&buffer, "-");
				string_append(&buffer, string_itoa(Retardo));
				string_append(&buffer, "-");
				cadena = serializar_PCB(auxPCB);
				string_append(&buffer, cadena);
				edatos = EnviarDatos(auxcpu->idCPU, buffer);
				auxcpu->libre = 1;
				if (edatos > 0) {
					list_remove(listaReady, 0);
				}
				list_clean(auxList);
			}
			pthread_mutex_unlock(&mutexCPU);
			pthread_mutex_unlock(&mutexReady);
			free(buffer);
		}
	}
	return NULL ;
}

void *hiloDispositivos(void *arg) {
	//crear los hilos de los dipositivos
	pthread_mutex_lock(&mutexDispositivos);
	t_list * listaHiloDispositivos = list_create();
	int cantDispotivos = list_size(listaDispositivos);
	int i;
	for (i = 0; i < cantDispotivos; i++) {
		list_add(listaHiloDispositivos, hilos_create());
	}
	t_hilos* auxHilos;
	t_HIO* auxDisp;
	//crear Hilos
	for (i = 0; i < cantDispotivos; i++) {
		auxHilos = list_get(listaHiloDispositivos, i);
		auxDisp = list_get(listaDispositivos, i);
		pthread_create(&(auxHilos->hilo), NULL, bloqueados_fnc,
				(void*) auxDisp);
	}
	pthread_mutex_unlock(&mutexDispositivos);
	//Esperar hilos
	for (i = 0; i < cantDispotivos; i++) {
		auxHilos = list_get(listaHiloDispositivos, i);
		pthread_join(auxHilos->hilo, NULL );
	}
	list_clean_and_destroy_elements(listaHiloDispositivos,
			(void*) hilos_destroy);
	return NULL ;
}

void *bloqueados_fnc(void *arg) {
	t_HIO* HIO = (t_HIO *) arg;
	t_list* auxLista;
	t_bloqueado* auxBloq;
	PCB* auxPCB;
	while (1) {
		semwait(&(HIO->bloqueadosCont));
		pthread_mutex_lock(&(HIO->mutexBloqueados));
		if (list_size(HIO->listaBloqueados) > 0) {
			auxLista = list_take_and_remove(HIO->listaBloqueados, 1);
			pthread_mutex_unlock(&(HIO->mutexBloqueados));
			auxBloq = list_get(auxLista, 0);
			//Procesando HIO
			sleep(HIO->valor * auxBloq->tiempo);
			//Pasar a Ready
			auxPCB = auxBloq->idPCB;
			auxBloq->idPCB = NULL;
			pthread_mutex_lock(&mutexReady);
			list_add(listaReady, auxPCB);
			pthread_mutex_unlock(&mutexReady);
			list_clean_and_destroy_elements(auxLista,
					(void*) bloqueado_destroy);
		} else {
			pthread_mutex_unlock(&(HIO->mutexBloqueados));
		}

	}
	return NULL ;
}

void *moverReadyDeNew(void *arg) {
	t_list *aux;
	//mandar a ready de new
	while (1) {
		semwait(&multiCont);
		semwait(&newCont);
		pthread_mutex_lock(&mutexNew);
		if (list_size(listaNew) > 0) {
			aux = list_take_and_remove(listaNew, 1);
			pthread_mutex_unlock(&mutexNew);
			pthread_mutex_lock(&mutexReady);
			list_add_all(listaReady, aux);
			pthread_mutex_unlock(&mutexReady);
			semsig(&readyCont);
		} else {
			pthread_mutex_unlock(&mutexNew);
		}

	}

	return NULL ;
}

void crearEscucha() {
//	fd_set master;   // conjunto maestro de descriptores de fichero
//	fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
//	struct sockaddr_in myaddr;     // dirección del servidor
//	struct sockaddr_in remoteaddr; // dirección del cliente
//	int fdmax;        // número máximo de descriptores de fichero
//	int listener;     // descriptor de socket a la escucha
//	int newfd;        // descriptor de socket de nueva conexión aceptada
//	char buf[BUFFERSIZE];    // buffer para datos del cliente
//	int nbytes;
//	int yes = 1;        // para setsockopt() SO_REUSEADDR, más abajo
//	int addrlen;
//	int i, j;
//	FD_ZERO(&master);    // borra los conjuntos maestro y temporal
//	FD_ZERO(&read_fds);
//	// obtener socket a la escucha
//	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//		perror("error al crear socket");
//		exit(1);
//	}
//	// obviar el mensaje "address already in use" (la dirección ya se está usando)
//	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
//			== -1) {
//		perror("setsockopt, la direccion ya se esta usando");
//		exit(1);
//	}
//	// enlazar
//	myaddr.sin_family = AF_INET;
//	myaddr.sin_addr.s_addr = INADDR_ANY;
//	myaddr.sin_port = htons(Puerto);
//	memset(&(myaddr.sin_zero), '\0', 8);
//	if (bind(listener, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
//		perror("error en el bind");
//		exit(1);
//	}
//	// escuchar
//	if (listen(listener, 10) == -1) {
//		perror("error en el listen");
//		exit(1);
//	}
//	// añadir listener al conjunto maestro
//	FD_SET(listener, &master);
//	// seguir la pista del descriptor de fichero mayor
//	fdmax = listener; // por ahora es éste
//	// bucle principal
//	for (;;) {
//		read_fds = master; // cópialo
//		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL ) == -1) {
//			perror("select");
//			exit(1);
//		}
//		// explorar conexiones existentes en busca de datos que leer
//		for (i = 0; i <= fdmax; i++) {
//			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
//				if (i == listener) {
//					// gestionar nuevas conexiones
//					addrlen = sizeof(remoteaddr);
//					if ((newfd = accept(listener,
//							(struct sockaddr *) &remoteaddr, &addrlen)) == -1) {
//						perror("accept");
//					} else {
//						FD_SET(newfd, &master); // añadir al conjunto maestro
//						if (newfd > fdmax) {    // actualizar el máximo
//							fdmax = newfd;
//						}
//						printf("selectserver: new connection from %s on "
//								"socket %d\n", inet_ntoa(remoteaddr.sin_addr),
//								newfd);
//					}
//				} else {
//					// gestionar datos de un cliente
//					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
//						// error o conexión cerrada por el cliente
//						if (nbytes == 0) {
//							// conexión cerrada
//							printf("selectserver: socket %d hung up\n", i);
//						} else {
//							perror("recv");
//						}
//						close(i); // bye!
//						FD_CLR(i, &master); // eliminar del conjunto maestro
//					} else {
//						// tenemos datos de algún cliente
//						for (j = 0; j <= fdmax; j++) {
//							// ¡enviar a todoos el mundo!
//							if (FD_ISSET(j, &master)) {
//								// excepto al listener y a nosotros mismos
//								if (j != listener && j != i) {
//									if (send(j, buf, nbytes, 0) == -1) {
//										perror("send");
//									}
//								}
//							}
//						}
//					}
//				} // Esto es ¡TAN FEO! demasiado feo
//			}
//		}
//	}
//
//	int AtiendeCliente(int sockete); //Handshake y recibir programa

	return;
}

int pedirMemoriaUMV(int socketumv) {
	char respuestaMemoria[BUFFERSIZE];
	char *mensaje = "5";
	//ENVIAR TODOS LOS CREAR SEGMENTOS
	EnviarDatos(socketumv, mensaje);
	RecibirDatos(socketumv, respuestaMemoria);

	return analisarRespuestaUMV(respuestaMemoria);
}

int ObtenerPuertoUMV() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO_UMV");
}

char *ObtenerIPUMV() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_string_value(config, "IP_UMV");
}

int ObtenerPuertoConfig() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTO");
}

int ObtenerPuertoPCPConfig() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "PUERTOPCP");
}

int ObtenerQuamtum() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "QUAMTUM");
}

int ObtenerRetardo() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "RETARDO");
}

int ObtenerMulti() {
	t_config* config = config_create(PATH_CONFIG);

	return config_get_int_value(config, "MULTIPROGRAMACION");
}
char* obtenerCadenaSem() {
	t_config* config = config_create(PATH_CONFIG);

	return (config_get_string_value(config, "SEMAFOROS"));
}
char* obtenerCadenaValSem() {
	t_config* config = config_create(PATH_CONFIG);

	return (config_get_string_value(config, "VALOR_SEMAFORO"));

}
void llenarSemaforoConfig() {
	char* cadenaSem;
	cadenaSem = obtenerCadenaSem(); //obtengo cadena del archivo de config de sem
	char* cadenaValoresSem;
	cadenaValoresSem = obtenerCadenaValSem(); //obtengo cadena  del archivo de config de valores semaforo

	int sfin = 1;
	char* sub1, *sub2;
	char* nombre1, *nombre2;
	sub1 = "";
	sub2 = "";
	nombre1 = "";
	nombre2 = "";
	int valor;
	sub1 = malloc(1 * sizeof(char));
	nombre1 = malloc(1 * sizeof(char));
	sub2 = malloc(1 * sizeof(char));
	nombre2 = malloc(1 * sizeof(char));
	int inicio1 = 1;  // supongo q el caracter 0 es [
	int inicio2 = 1;  // supongo q el caracter 0 es [
	while (sfin) {
		while ((string_equals_ignore_case(sub1, ",") == 0)
				|| (string_equals_ignore_case(sub1, "]") == 0)) {
			string_append(&nombre1, sub1);
			sub1 = string_substring(cadenaSem, inicio1, 1);
			inicio1++;
		}
		while ((string_equals_ignore_case(sub2, ",") == 0)
				|| (string_equals_ignore_case(sub2, "]") == 0)) {
			string_append(&nombre2, sub2);
			sub2 = string_substring(cadenaValoresSem, inicio2, 1);
			inicio2++;
		}
		valor = atoi(nombre2);
		list_add(listaSemaforos, sem_create(nombre1, valor));
		if ((string_equals_ignore_case(sub1, "]") == 0)) {
			nombre1 = "";
			nombre2 = "";
		} else {
			sfin = 0;
		}
		char* obtenerCadenaDispositivos() {
			t_config* config = config_create(PATH_CONFIG);

			return (config_get_string_value(config, "ID_IHIO"));

		}
		char* obtenerCadenaValDisp() {

			t_config* config = config_create(PATH_CONFIG);

			return (config_get_string_value(config, "HIO"));

		}
		void llenarDispositConfig() {
			char* cadenaDisposit;
			cadenaDisposit = obtenerCadenaDispositivos(); //obtengo cadena del archivo de config de dispositivos
			char* cadenaValDisp;
			cadenaValDisp = obtenerCadenaValDisp(); //obtengo cadena del archivo  de config de valores dispositivos

			int sfin = 1;
			char* sub1, *sub2;
			char* nombre1, *nombre2;
			sub1 = "";
			sub2 = "";
			nombre1 = "";
			nombre2 = "";
			int valor;
			sub1 = malloc(1 * sizeof(char));
			nombre1 = malloc(1 * sizeof(char));
			sub2 = malloc(1 * sizeof(char));
			nombre2 = malloc(1 * sizeof(char));
			int inicio1 = 1;  // supongo q el caracter 0 es [
			int inicio2 = 1;  // supongo q el caracter 0 es [
			while (sfin) {
				while ((string_equals_ignore_case(sub1, ",") == 0)
						|| (string_equals_ignore_case(sub1, "]") == 0)) {
					string_append(&nombre1, sub1);
					sub1 = string_substring(cadenaDisposit, inicio1, 1);
					inicio1++;
				}
				while ((string_equals_ignore_case(sub2, ",") == 0)
						|| (string_equals_ignore_case(sub2, "]") == 0)) {
					string_append(&nombre2, sub2);
					sub2 = string_substring(cadenaValDisp, inicio2, 1);
					inicio2++;
				}
				valor = atoi(nombre2);
				list_add(listaDispositivos, HIO_create(nombre1, valor));
				if ((string_equals_ignore_case(sub1, "]") == 0)) {
					nombre1 = "";
					nombre2 = "";
				} else {
					sfin = 0;
				}
			}

		}
		char* obtenerCadenaVarGlob() {
			t_config* config = config_create(PATH_CONFIG);

			return (config_get_string_value(config, "VARIABLES_GLOBALES")); //hay que ver como lo cargan en el config

		}
		char* obtenerCadenaValVarGlob() {

			t_config* config = config_create(PATH_CONFIG);

			return (config_get_string_value(config, "VALOR_VARIABLE_GLOBAL")); //hay que ver como lo cargane en el config

		}

		void llenarVarGlobConfig() {
			char* cadenaVarGlob;
			cadenaVarGlob = obtenerCadenaVarGlob(); //obtengo cadena del archivo de config de variables globales
			char* cadenaValVarGlob;
			cadenaValVarGlob = obtenerCadenaValVarGlob(); //obtengo cadena  del archivo  de config de variables globales

			int sfin = 1;
			char* sub1, *sub2;
			char* nombre1, *nombre2;
			sub1 = "";
			sub2 = "";
			nombre1 = "";
			nombre2 = "";
			int valor;
			sub1 = malloc(1 * sizeof(char));
			nombre1 = malloc(1 * sizeof(char));
			sub2 = malloc(1 * sizeof(char));
			nombre2 = malloc(1 * sizeof(char));
			int inicio1 = 1;  // supongo q el caracter 0 es [
			int inicio2 = 1;  // supongo q el caracter 0 es [
			while (sfin) {
				while ((string_equals_ignore_case(sub1, ",") == 0)
						|| (string_equals_ignore_case(sub1, "]") == 0)) {
					string_append(&nombre1, sub1);
					sub1 = string_substring(cadenaVarGlob, inicio1, 1);
					inicio1++;
				}
				while ((string_equals_ignore_case(sub2, ",") == 0)
						|| (string_equals_ignore_case(sub2, "]") == 0)) {
					string_append(&nombre2, sub2);
					sub2 = string_substring(cadenaValVarGlob, inicio2, 1);
					inicio2++;
				}
				valor = atoi(nombre2);
				list_add(listaVarGlobal, varGlobal_create(nombre1, valor));
				if ((string_equals_ignore_case(sub1, "]") == 0)) {
					nombre1 = "";
					nombre2 = "";
				} else {
					sfin = 0;
				}
			}

		}

		void conectarAUMV() {
			Traza("Intentando conectar a umv.");

			socketumv = ConexionConSocket(UMV_PUERTO, UMV_IP);
			if (hacerhandshakeUMV(socketumv) == 0) {
				ErrorFatal("No se pudo conectar a la umv.");
			}
		}

		int hacerhandshakeUMV(int sockfd) {
			char respuestahandshake[BUFFERSIZE];
			char *mensaje = "31";

			EnviarDatos(sockfd, mensaje);
			RecibirDatos(sockfd, respuestahandshake);
			return analisarRespuestaUMV(respuestahandshake);

		}

		int analisarRespuestaUMV(char *mensaje) {
			if (mensaje[0] == 0) {
				Error("La umv nos devolvio un error: %s", mensaje);
				return 0;
			} else
				return 1;
		}

		int ConexionConSocket(int puerto, char* IP) {
			int sockfd;
//Ip de lo que quieres enviar: ifconfig desde terminator , INADDR_ANY para local
			struct hostent *he, *gethostbyname();
			struct sockaddr_in their_addr;
			he = gethostbyname(IP);

			if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
				ErrorFatal("Error al querer crear el socket. puerto %d, ip %s",
						puerto, IP);
				exit(1);
			}

			their_addr.sin_family = AF_INET; // Ordenación de bytes de la máquina
			their_addr.sin_port = htons(puerto); // short, Ordenación de bytes de la red
			bcopy(he->h_addr, &(their_addr.sin_addr.s_addr),he->h_length); //their_addr.sin_addr = *((struct in_addr *)he->h_addr);
			memset(&(their_addr.sin_zero), '\0', 8); // poner a cero el resto de la estructura

			if (connect(sockfd, (struct sockaddr *) &their_addr,
					sizeof(struct sockaddr)) == -1) {
				ErrorFatal("Error al querer conectar. puerto %d, ip %s", puerto,
						IP);
				exit(1);
			}

			return sockfd;
		}

		int RecibirDatos(int socket, char *buffer) {
			int bytecount;
// memset se usa para llenar el buffer con 0s
			memset(buffer, 0, BUFFERSIZE);

//Nos ponemos a la escucha de las peticiones que nos envie el cliente. //aca si recibo 0 bytes es que se desconecto el otro, cerrar el hilo.
			if ((bytecount = recv(socket, buffer, BUFFERSIZE, 0)) == -1)
				Error(
						"Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",
						socket);

			Traza("RECIBO datos. socket: %d. buffer: %s", socket,
					(char*) buffer);

			return bytecount;
		}

		int EnviarDatos(int socket, void *buffer) {
			int bytecount;

			if ((bytecount = send(socket, buffer, strlen(buffer), 0)) == -1)
				Error("No puedo enviar información a al clientes. Socket: %d",
						socket);

			Traza("ENVIO datos. socket: %d. buffer: %s", socket,
					(char*) buffer);

			return bytecount;
		}

		void error(int code, char *err) {
			char *msg = (char*) malloc(strlen(err) + 14);
			sprintf(msg, "Error %d: %s\n", code, err);
			fprintf(stderr, "%s", msg);
			exit(1);
		}
		void Cerrar(int sRemoto) {

			close(sRemoto);
		}
		void CerrarSocket(int socket) {
			close(socket);
			Traza("Se cerró el socket (%d).", socket);
		}
		void ErrorFatal(char mensaje[], ...) {
			char* nuevo;
			va_list arguments;
			va_start(arguments, mensaje);
			nuevo = string_from_vformat(mensaje, arguments);

			fprintf(stderr, "\nERROR FATAL: %s\n", nuevo);

			va_end(arguments);

			char fin;

			printf(
					"El programa se cerrara. Presione ENTER para finalizar la ejecución.");
			scanf("%c", &fin);

			exit(EXIT_FAILURE);
		}
		void Error(const char* mensaje, ...) {
			char* nuevo;
			va_list arguments;
			va_start(arguments, mensaje);
			nuevo = string_from_vformat(mensaje, arguments);

			fprintf(stderr, "\nERROR: %s\n", nuevo);

			va_end(arguments);

		}
		void Traza(const char* mensaje, ...) {
			if (ImprimirTrazaPorConsola) {
				char* nuevo;
				va_list arguments;
				va_start(arguments, mensaje);
				nuevo = string_from_vformat(mensaje, arguments);

				printf("TRAZA--> %s \n", nuevo);

				va_end(arguments);

			}
		}
		int ObtenerComandoMSJ(char buffer[]) {
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
			return chartToInt(buffer[0]);
		}
		void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente) {
			(*idProg) = chartToInt(buffer[1]);
			(*tipoCliente) = chartToInt(buffer[2]);

			memset(buffer, 0, BUFFERSIZE);
			sprintf(buffer,
					"HandShake: OK! INFO-->  idPRog: %d, tipoCliente: %d ",
					*idProg, *tipoCliente);
		}
		int chartToInt(char x) {
			char str[1];
			str[0] = x;
			int a = atoi(str);
			return a;
		}

		void ComandoRecibirPrograma(char *buffer, int idProg) {
//void ComandoRecibirPrograma(char *buffer, int *idProg, int *tipoCliente) {
//	memset(buffer, 0, BUFFERSIZE);
//	PCB programa;
//	programa.id = id;
//	programa.programCounter= 0; // o 1
//	programa.segmentoCodigo;
//	programa.segmentoStack;
//	programa.cursorStack;
//	//int indiceCodigo;
//	//int indiceEtiquetas;
//	//int programCounter;
//	//int sizeContextoActual;
//	//int sizeIndiceEtiquetas;
//
//
//	if (pedirMemoriaUMV(socketumv,programa)) {
//		//Responder programa too ok
//	} else {
//		//responder programa too mal
//		// * Crear segmento 1° cod mensaje (5)
//		// * Parametros a pasar 2° cantidad de dijitos del id programa
//		// *  3° id programa
//		// *  4° cantidad dijitos tamaño
//		// *  5° tamaño
//		// *  Destruir seg: idem menos 4° y 5°, cod mensaje (6)*/
//	}
		}

		int AtiendeCliente(int sockete) {

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

			// Código de salida por defecto
			int code = 0;

			bytesRecibidos = RecibirDatos(sockete, buffer);

			if (bytesRecibidos > 0) {
				//Analisamos que peticion nos está haciendo (obtenemos el comando)
				tipo_mensaje = ObtenerComandoMSJ(buffer);

				//Evaluamos los comandos
				switch (tipo_mensaje) {
				case MSJ_HANDSHAKE:
					ComandoHandShake(buffer, &id_Programa, &tipo_Conexion);
					EnviarDatos(sockete, "1");
					break;
				case MSJ_RECIBO_PROGRAMA:
					ComandoRecibirPrograma(buffer, sockete);
					EnviarDatos(sockete, "1");
				}
			}
			return code;
		}

		char* RecibirDatos2(int socket, char *buffer, int *bytesRecibidos) {
			*bytesRecibidos = 0;
			int tamanioNuevoBuffer = 0;
			int mensajeCompleto = 0;
			int cantidadBytesRecibidos = 0;
			int cantidadBytesAcumulados = 0;

			// memset se usa para llenar el buffer con 0s
			char bufferAux[BUFFERSIZE];

			buffer = realloc(buffer, 1 * sizeof(char)); //--> el buffer ocupa 0 lugares (todavia no sabemos que tamaño tendra)
			memset(buffer, 0, 1 * sizeof(char));

			while (!mensajeCompleto) // Mientras que no se haya recibido el mensaje completo
			{
				memset(bufferAux, 0, BUFFERSIZE * sizeof(char)); //-> llenamos el bufferAux con barras ceros.

				//Nos ponemos a la escucha de las peticiones que nos envie el cliente. //aca si recibo 0 bytes es que se desconecto el otro, cerrar el hilo.
				if ((*bytesRecibidos = *bytesRecibidos
						+ recv(socket, bufferAux, BUFFERSIZE, 0)) == -1)
					Error(
							"Ocurrio un error al intentar recibir datos desde uno de los clientes. Socket: %d",
							socket);

				cantidadBytesRecibidos = strlen(bufferAux);
				cantidadBytesAcumulados = strlen(buffer);
				tamanioNuevoBuffer = cantidadBytesRecibidos
						+ cantidadBytesAcumulados;

				if (tamanioNuevoBuffer > 0) {
					buffer = realloc(buffer, tamanioNuevoBuffer * sizeof(char)); //--> el tamaño del buffer sera el que ya tenia mas los caraceteres nuevos que recibimos

					sprintf(buffer, "%s%s", (char*) buffer, bufferAux); //--> el buffer sera buffer + nuevo recepcio
				}

				//Verificamos si terminó el mensaje
				if (cantidadBytesRecibidos < BUFFERSIZE)
					mensajeCompleto = 1;
			}

			Traza("RECIBO datos. socket: %d. buffer: %s", socket,
					(char*) buffer);

			return buffer; //--> buffer apunta al lugar de memoria que tiene el mensaje completo completo.
		}

		char ObtenerComandoCPU(char buffer[]) {
//Hay que obtener el comando dado el buffer.
//El comando está dado por el primer caracter, que tiene que ser un número.
			return buffer[0];
		}

		char* ComandoHandShake2(char *buffer, int *tipoCliente) {
// Formato del mensaje: CD
// C = codigo de mensaje ( = 3)

			//int tipoDeCliente = posicionDeBufferAInt(buffer, 1);
			int tipoDeCliente = chartToInt(buffer[1]);
			if (tipoDeCliente == TIPO_CPU) {
				*tipoCliente = tipoDeCliente;
				buffer = RespuestaClienteOk(buffer);
			} else {
				*tipoCliente = 0;
				//SetearErrorGlobal("HANDSHAKE ERROR. Tipo de cliente invalido (%d). KERNEL o '2' = CPU.", posicionDeBufferAInt(buffer, 1));
				//buffer = RespuestaClienteError(buffer, g_MensajeError);
			}

			return buffer;
		}

		int posicionDeBufferAInt(char* buffer, int posicion) {
			int logitudBuffer = 0;
			logitudBuffer = strlen(buffer);

			if (logitudBuffer <= posicion)
				return 0;
			else
				return chartToInt(buffer[posicion]);
		}

		char* RespuestaClienteOk(char *buffer) {
			int tamanio = sizeof(char) * 2;
			buffer = realloc(buffer, tamanio * sizeof(char));
			memset(buffer, 0, tamanio * sizeof(char));
			sprintf(buffer, "%s", "1");
			return buffer;
		}

		void seminit(psem_t *ps, int n) {
			pthread_mutex_init(&ps->semdata, NULL );

			pthread_mutex_init(&ps->sem_mutex, NULL );

			/* para inicializar el semaforo binario a 0 */

			pthread_mutex_lock(&ps->sem_mutex);

			ps->n = n;
		}

		void semwait(psem_t *ps) {

			pthread_mutex_lock(&ps->semdata);

			ps->n--;
			if (ps->n < 0) {
				pthread_mutex_unlock(&ps->semdata);

				pthread_mutex_lock(&ps->sem_mutex);
			}

			else
				pthread_mutex_unlock(&ps->semdata);

		}

		void semsig(psem_t *ps) {
			pthread_mutex_lock(&ps->semdata);

			ps->n++;
			if (ps->n < 1)
				pthread_mutex_unlock(&ps->sem_mutex);

			pthread_mutex_unlock(&ps->semdata);
		}

		void semdestroy(psem_t *ps) {
			pthread_mutex_destroy(&ps->semdata);
			pthread_mutex_destroy(&ps->sem_mutex);
		}

		void agregarNuevaCPU(int socket) {
			list_add(listCPU, cpu_create(socket));
		}

		bool encontrarInt(int actual, int expected) {
			return actual == expected;
		}

		t_CPU* encontrarCPULibre() {
			int _is_CPU_Libre(t_CPU *p) {
				return encontrarInt(p->libre, 0);
			}
			if (list_any_satisfy(listCPU, (void*) _is_CPU_Libre)) {
				t_CPU * aux = list_find(listCPU, (void*) _is_CPU_Libre);
				return aux;
			}
			return NULL ;

		}

		t_CPU* encontrarCPU(int idcpu) {
			int _is_CPU(t_CPU *p) {
				return encontrarInt(p->idCPU, idcpu);
			}
			if (list_any_satisfy(listCPU, (void*) _is_CPU)) {
				t_CPU * aux = list_find(listCPU, (void*) _is_CPU);
				return aux;
			}
			return NULL ;

		}

		void *HiloOrquestadorDeCPU() {
			int nbytesRecibidos;
			char* buffer;
			buffer = malloc(1 * sizeof(char));
			//	int id_CPU = 0;
			int tipo_Cliente = 0;

			// Es el encabezado del mensaje. Nos dice que acción se le está solicitando al UMV
			char tipo_mensaje = '0';

//int yes=1;        // para setsockopt() SO_REUSEADDR, más abajo
//	struct sockaddr_in myaddr;     // dirección del servidor

			fd_set master;   // conjunto maestro de descriptores de fichero
			fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
			int fdmax;        // número máximo de descriptores de fichero
			int socketEscucha;     // descriptor de socket a la escucha
			int socketNuevaConexion; // descriptor de socket de nueva conexión aceptada
			int i;
			FD_ZERO(&master);    // borra los conjuntos maestro y temporal
			FD_ZERO(&read_fds);

			struct sockaddr_in socketInfo;
//	char buffer[BUFF_SIZE];
			int optval = 1;

			// Crear un socket:
			// AF_INET: Socket de internet IPv4
			// SOCK_STREAM: Orientado a la conexion, TCP
			// 0: Usar protocolo por defecto para AF_INET-SOCK_STREAM: Protocolo TCP/IPv4
			if ((socketEscucha = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				perror("socket");
				//return 1;
			}

			// Hacer que el SO libere el puerto inmediatamente luego de cerrar el socket.
			setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &optval,
					sizeof(optval));

			socketInfo.sin_family = AF_INET;
			socketInfo.sin_addr.s_addr = DIRECCION; //Notar que aca no se usa inet_addr()
			socketInfo.sin_port = htons(PuertoPCP);

// Vincular el socket con una direccion de red almacenada en 'socketInfo'.
			if (bind(socketEscucha, (struct sockaddr*) &socketInfo,
					sizeof(socketInfo)) != 0) {

				perror("Error al bindear socket escucha");
				//return EXIT_FAILURE;
			}

// Escuchar nuevas conexiones entrantes.
			if (listen(socketEscucha, 10) != 0) {

				perror("Error al poner a escuchar socket");
				//return EXIT_FAILURE;
			}

			printf("Escuchando conexiones entrantes.\n");
			// añadir listener al conjunto maestro
			FD_SET(socketEscucha, &master);
			// seguir la pista del descriptor de fichero mayor
			fdmax = socketEscucha; // por ahora es éste

			for (;;) {
				read_fds = master; // cópialo
				if (select(fdmax + 1, &read_fds, NULL, NULL, NULL ) == -1) {
					perror("select");
					exit(1);
				}
				// explorar conexiones existentes en busca de datos que leer
				for (i = 0; i <= fdmax; i++) {
					if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
						if (i == socketEscucha) {
							// gestionar nuevas conexiones
							// addrlen = sizeof(remoteaddr);
							if ((socketNuevaConexion = accept(socketEscucha,
									NULL, 0)) == -1) {
								perror("accept");
							} else {
								FD_SET(socketNuevaConexion, &master); // añadir al conjunto maestro
								if (socketNuevaConexion > fdmax) { // actualizar el máximo
									fdmax = socketNuevaConexion;
								}

								printf(
										"selectserver: una nueva conneccion socket %d\n",
										socketNuevaConexion);
							}
						} else {

							// gestionar datos de un cliente
							// Recibir hasta BUFF_SIZE datos y almacenarlos en 'buffer'.

							buffer = realloc(buffer, 1 * sizeof(char)); //-> de entrada lo instanciamos en 1 byte, el tamaño será dinamico y dependerá del tamaño del mensaje.
							//Recibimos los datos del cliente
							buffer = RecibirDatos2(i, buffer, &nbytesRecibidos);
							Traza("nos ponemos a recibir %d", nbytesRecibidos);
							if (nbytesRecibidos <= 0) {
								// error o conexión cerrada por el cliente
								if (nbytesRecibidos == 0) {
									// conexión cerrada
									printf("selectserver: socket %d hung up\n",
											i);
								} else {
									perror("recv");
								}
								pthread_mutex_lock(&mutexCPU);
								eliminarCpu(i);
								pthread_mutex_lock(&mutexCPU);
								close(i); // bye!
								FD_CLR(i, &master); // eliminar del conjunto maestro
							} else {

								tipo_mensaje = ObtenerComandoCPU(buffer);
								Traza("Tipos de mensaje: %c", tipo_mensaje);
								//Evaluamos los comandos
								switch (tipo_mensaje) {
								case MSJ_CPU_IMPRIMI:
									//manda a la lista de imprimir
									comandoImprimir(buffer, i);
									break;
								case MSJ_CPU_HANDSHAKE:
									Traza("HandShake");
									buffer = ComandoHandShake2(buffer,
											&tipo_Cliente);
									Traza("tipo de cliente: %d", tipo_Cliente);
									//crear nueva CPU
									if (tipo_Cliente == TIPO_CPU) {
										pthread_mutex_lock(&mutexCPU);
										Traza("creando CPU");
										agregarNuevaCPU(i);
										pthread_mutex_unlock(&mutexCPU);
									}
									EnviarDatos(i, buffer);
									break;

								case MSJ_CPU_FINAlQUAMTUM:
									comandoFinalQuamtum(buffer, i);
									break;
								case MSJ_CPU_OBTENERVALORGLOBAL:
									comandoObtenerValorGlobar(buffer, i);
									break;
								case MSJ_CPU_GRABARVALORGLOBAL:
									comandoGrabarValorGlobar(buffer, i);
									break;
								case MSJ_CPU_ABANDONA:
									//elimina cpu
									pthread_mutex_lock(&mutexCPU);
									eliminarCpu(i);
									pthread_mutex_lock(&mutexCPU);
									close(i); // bye!
									FD_CLR(i, &master); // eliminar del conjunto maestro
									break;
								case MSJ_CPU_WAIT:
									//Resta la variable del semaforo
									comandoWait(buffer, i);
									break;
								case MSJ_CPU_SIGNAL:
									//suma la varariable del semaforo
									comandoSignal(buffer, i);
									break;
								case MSJ_CPU_FINAlIZAR:
									//Termino programa y mandar a FIN
									comandoFinalizar(i, buffer);
									break;
								case MSJ_CPU_LIBERAR:
									//CPU pasa a libre
									comandoLiberar(i);
									break;
								default:
									//buffer = RespuestaClienteError(buffer, "El ingresado no es un comando válido");
									break;

								}

								buffer[0] = '\0';
							}
						}
					}
				}
			}

			return 0;
		}

		PCB* desearilizar_PCB(char* estructura, int* pos) {
			printf("%s\n", estructura);
			char* sub = "";
			sub = malloc(1 * sizeof(char));
			PCB* est_prog;
			est_prog = (struct PCBs *) malloc(sizeof(PCB));
			int aux;
			int i;
			int indice = *pos;
			int inicio = *pos;

			iniciarPCB(est_prog);
			for (aux = 1; aux < 10; aux++) {
				for (i = 0; string_equals_ignore_case(sub, "-") == 0; i++) {
					sub = string_substring(estructura, inicio, 1);
					inicio++;
				}
				switch (aux) {
				case 1:
					est_prog->id = atoi(
							string_substring(estructura, indice, i));
					break;
				case 2:
					est_prog->segmentoCodigo = atoi(
							string_substring(estructura, indice, i));
					break;
				case 3:
					est_prog->segmentoStack = atoi(
							string_substring(estructura, indice, i));
					break;
				case 4:
					est_prog->cursorStack = atoi(
							string_substring(estructura, indice, i));
					break;
				case 5:
					est_prog->indiceCodigo = atoi(
							string_substring(estructura, indice, i));
					break;
				case 6:
					est_prog->indiceEtiquetas = atoi(
							string_substring(estructura, indice, i));
					break;
				case 7:
					est_prog->programCounter = atoi(
							string_substring(estructura, indice, i));
					break;
				case 8:
					est_prog->sizeContextoActual = atoi(
							string_substring(estructura, indice, i));
					break;
				case 9:
					est_prog->sizeIndiceEtiquetas = atoi(
							string_substring(estructura, indice, i));
					break;
				}
				sub = "";
				indice = inicio;
			}
			*pos = inicio;
			//free(sub);
			return est_prog;
		}

		void iniciarPCB(PCB* prog) {
			prog->id = 0;
			prog->segmentoCodigo = 0;
			prog->segmentoStack = 0;
			prog->cursorStack = 0;
			prog->indiceCodigo = 0;
			prog->indiceEtiquetas = 0;
			prog->programCounter = 0;
			prog->sizeContextoActual = 0;
			prog->sizeIndiceEtiquetas = 0;
		}

		char* serializar_PCB(PCB* prog) {
			char* cadena;
			cadena = malloc(1 * sizeof(char));

			string_append(&cadena, string_itoa(prog->id));
			string_append(&cadena, "-");
			string_append(&cadena, string_itoa(prog->segmentoCodigo));
			string_append(&cadena, "-");
			string_append(&cadena, string_itoa(prog->segmentoStack));
			string_append(&cadena, "-");
			string_append(&cadena, string_itoa(prog->cursorStack));
			string_append(&cadena, "-");
			string_append(&cadena, string_itoa(prog->indiceCodigo));
			string_append(&cadena, "-");
			string_append(&cadena, string_itoa(prog->indiceEtiquetas));
			string_append(&cadena, "-");
			string_append(&cadena, string_itoa(prog->programCounter));
			string_append(&cadena, "-");
			string_append(&cadena, string_itoa(prog->sizeContextoActual));
			string_append(&cadena, "-");
			string_append(&cadena, string_itoa(prog->sizeIndiceEtiquetas));
			string_append(&cadena, "-");
			return cadena;

		}

		void comandoLiberar(int socket) {
			pthread_mutex_lock(&mutexCPU);
			t_CPU* aux = encontrarCPU(socket);
			if (aux != NULL ) {
				aux->libre = 0;
			}
			pthread_mutex_lock(&mutexCPU);
		}

		void eliminarCpu(int idcpu) {
			int _is_CPU_ID(t_CPU *p) {
				return encontrarInt(p->idCPU, idcpu);
			}
			if (list_any_satisfy(listCPU, (void*) _is_CPU_ID)) {
				list_remove_by_condition(listCPU, (void*) _is_CPU_ID);
			}
		}

		void comandoFinalQuamtum(char *buffer, int socket) {
			PCB* auxPCB;
			int pos, tiempo;
			pos = 1;
			char* nombre;
			char* disp;
			auxPCB = desearilizar_PCB(buffer, &pos);
			int pos2 = pos + 2;
			switch (buffer[pos]) {
			case '0': //termino quamtum colocar en ready
				pthread_mutex_lock(&mutexReady);
				list_add(listaReady, auxPCB);
				pthread_mutex_unlock(&mutexReady);
				semsig(&readyCont);
				Traza("Final Quamtum mover a ready. Programa: %d. buffer: %s",
						auxPCB->id);
				break;
			case '1':	//Hay que hacer I/O
				disp = obtenerParteDelMensaje(buffer, &pos2);
				tiempo = atoi(obtenerParteDelMensaje(buffer, &pos2));
				t_HIO* auxHIO = encontrarDispositivo(disp);
				if (auxHIO != NULL ) {
					pthread_mutex_lock(&(auxHIO->mutexBloqueados));
					list_add(auxHIO->listaBloqueados,
							bloqueado_create(auxPCB, tiempo));
					pthread_mutex_unlock(&(auxHIO->mutexBloqueados));
					semsig(&(auxHIO->bloqueadosCont));
					Traza("Final Quamtum programa: %d. Pide Dispositivo: %s",
							auxPCB->id, (char*) auxHIO->nombre);
				} else {
					pthread_mutex_lock(&mutexFIN);
					list_add(listaFin,
							final_create(auxPCB, 1,
									"Dispositivo no encontrado"));
					pthread_mutex_unlock(&mutexFIN);
					semsig(&finalizarCont);
				}
				break;
			case '2':	//Bloqueado por semaforo
				nombre = obtenerNombreMensaje(buffer, pos2);
				pthread_mutex_lock(&mutexSemaforos);
				t_sem* auxSem = encontrarSemaforo(nombre);
				if (auxSem != NULL ) {
					if (auxSem->valor < 0) {
						//Bloquear Programa por semaforo
						list_add(auxSem->listaSem, auxPCB);
						Traza(
								"Final Quamtum programa: %d. bloqueado por semaforo: %s",
								auxPCB->id, (char*) auxSem->nombre);
					} else {
						//Desbloquar Programa
						pthread_mutex_lock(&mutexReady);
						list_add(listaReady, auxPCB);
						pthread_mutex_unlock(&mutexReady);
						semsig(&readyCont);
					}
				} else {
					pthread_mutex_lock(&mutexFIN);
					list_add(listaFin,
							final_create(auxPCB, 1, "semaforo no encontrado"));
					pthread_mutex_unlock(&mutexFIN);
					semsig(&finalizarCont);
				}
				pthread_mutex_unlock(&mutexSemaforos);
				break;
			}
			//Buscar CPU y Borrar Programa
			borrarPCBenCPU(socket);
		}

		void comandoWait(char* buffer, int socket) {
			char* nombre = obtenerNombreMensaje(buffer, 1);
			int n;
			pthread_mutex_unlock(&mutexSemaforos);
			t_sem* auxSem = encontrarSemaforo(nombre);
			if (auxSem != NULL ) {
				auxSem->valor--;
				Traza("Wait semaforo: %s. valor: %d", (char*) auxSem->nombre,
						auxSem->valor);
				if (auxSem->valor < 0) {
					n = EnviarDatos(socket, "0");
				} else {
					n = EnviarDatos(socket, "1");
				}
			} else {
				n = EnviarDatos(socket, "A");
				pthread_mutex_lock(&mutexCPU);
				t_CPU *aux = encontrarCPU(socket);
				PCB* aux1;
				if (aux != NULL ) {
					aux1 = aux->idPCB;
					aux->idPCB = NULL;
					pthread_mutex_lock(&mutexFIN);
					list_add(listaFin,
							final_create(aux1, 1, "Semaforo no encontrado"));
					pthread_mutex_unlock(&mutexFIN);
					semsig(&finalizarCont);
				}
				pthread_mutex_unlock(&mutexCPU);
			}
			if (n < 0) {
				//TODO error enviar datos
			}
			pthread_mutex_unlock(&mutexSemaforos);
		}

		char* obtenerNombreMensaje(char* buffer, int pos) {
			char* sub;
			char* nombre;
			sub = "";
			sub = malloc(1 * sizeof(char));
			nombre = "";
			nombre = malloc(1 * sizeof(char));
			int final = pos;
			while (string_equals_ignore_case(sub, "-") == 0) {
				string_append(&nombre, sub);
				sub = string_substring(buffer, final, 1);
				final++;
			}
			return nombre;
		}

		t_sem* encontrarSemaforo(char* nombre) {
			int _is_sem(t_sem *p) {
				return string_equals_ignore_case(p->nombre, nombre);
			}
			t_sem *aux = list_find(listaSemaforos, (void*) _is_sem);
			return aux;
		}

		void comandoSignal(char* buffer, int socket) {
			char* nombre = obtenerNombreMensaje(buffer, 1);
			int n, cant, i;
			pthread_mutex_unlock(&mutexSemaforos);
			t_sem* auxSem = encontrarSemaforo(nombre);
			if (auxSem != NULL ) {
				auxSem->valor++;
				n = EnviarDatos(socket, "1");
				Traza("signal semaforo: %s valor: %d", (char*) auxSem->nombre,
						auxSem->valor);
				if (auxSem->valor == 0) {
					cant = list_size(auxSem->listaSem);
					if (cant > 0) {
						pthread_mutex_lock(&mutexReady);
						list_add_all(listaReady, auxSem->listaSem);
						pthread_mutex_unlock(&mutexReady);
						Traza("Desbloquea programas por semaforo: %s",
								(char*) auxSem->nombre);
					}
					for (i = 0; i < cant; i++) {
						semsig(&readyCont);
					}
				}
			} else {
				n = EnviarDatos(socket, "A");
				pthread_mutex_lock(&mutexCPU);
				t_CPU *aux = encontrarCPU(socket);
				PCB* aux1;
				if (aux != NULL ) {
					aux1 = aux->idPCB;
					aux->idPCB = NULL;
					pthread_mutex_lock(&mutexFIN);
					list_add(listaFin,
							final_create(aux1, 1, "Semaforo no encontrado"));
					pthread_mutex_unlock(&mutexFIN);
					semsig(&finalizarCont);
				}
				pthread_mutex_unlock(&mutexCPU);
			}
			if (n < 0) {
				//TODO error enviar datos
			}
			pthread_mutex_unlock(&mutexSemaforos);
		}

		void comandoFinalizar(int socket, char* buffer) {
			PCB* auxPCB;
			int pos = 1;
			auxPCB = desearilizar_PCB(buffer, &pos);
			//Pasa a estado fin
			pthread_mutex_lock(&mutexFIN);
			list_add(listaFin, final_create(auxPCB, 0, ""));
			pthread_mutex_unlock(&mutexFIN);
			Traza("Mandar a estado fin. programa: %d. buffer: ", auxPCB->id);
			semsig(&finalizarCont);
			//Borra el viejo PCB en la lista de CPU
			borrarPCBenCPU(socket);
		}

		void borrarPCBenCPU(int idCPU) {
			pthread_mutex_lock(&mutexCPU);
			t_CPU *aux = encontrarCPU(idCPU);
			PCB* aux1;
			if (aux != NULL ) {
				aux1 = aux->idPCB;
				aux->idPCB = NULL;
				free(aux1);
			}
			pthread_mutex_unlock(&mutexCPU);
		}

		char* obtenerParteDelMensaje(char* buffer, int* pos) {
			char* sub;
			char* nombre;
			sub = "";
			sub = malloc(1 * sizeof(char));
			nombre = "";
			nombre = malloc(1 * sizeof(char));
			int final = *pos;
			while (string_equals_ignore_case(sub, "-") == 0) {
				string_append(&nombre, sub);
				sub = string_substring(buffer, final, 1);
				final++;
			}
			*pos = final + 1;
			return nombre;
		}

		t_HIO* encontrarDispositivo(char* nombre) {
			int _is_dis(t_HIO *p) {
				return string_equals_ignore_case(p->nombre, nombre);
			}
			t_HIO *aux = list_find(listaDispositivos, (void*) _is_dis);
			return aux;
		}

		void comandoImprimir(char* buffer, int socket) {
			char* mensaje = obtenerNombreMensaje(buffer, 1);
			pthread_mutex_lock(&mutexCPU);
			t_CPU* auxCPU = encontrarCPU(socket);
			if (auxCPU != NULL ) {
				if (auxCPU->idPCB) {
					pthread_mutex_lock(&mutexImprimir);
					list_add(listaImprimir,
							imp_create(auxCPU->idPCB->id, mensaje));
					pthread_mutex_unlock(&mutexImprimir);
					semsig(&imprimirCont);
					Traza("Mandar a imprimir datos. programa: %d. buffer: %s",
							auxCPU->idPCB->id, (char*) mensaje);
				}
			}
			pthread_mutex_unlock(&mutexCPU);
		}

		void comandoObtenerValorGlobar(char* buffer, int socket) {
			//OBtener valor variable compartida y mandarlo a CPU
			char* variable = obtenerNombreMensaje(buffer, 1);
			char* respuesta = "";
			int ndatos;
			respuesta = malloc(1 * sizeof(char));
			t_varGlobal* auxVar = encontrarVarGlobal(variable);
			if (auxVar != NULL ) {
				string_append(&respuesta, "1");
				string_append(&buffer, string_itoa(auxVar->valor));
				string_append(&respuesta, "-");
				ndatos = EnviarDatos(socket, respuesta);
				if (ndatos > 0) {
					Traza("Envio CPU: %d, Respuesta: %s", socket,
							(char *) respuesta);
				} else {
					//TODO falla al enviar
				}
			} else {
				ndatos = EnviarDatos(socket, "A");
				pthread_mutex_lock(&mutexCPU);
				t_CPU *aux = encontrarCPU(socket);
				PCB* aux1;
				if (aux != NULL ) {
					aux1 = aux->idPCB;
					aux->idPCB = NULL;
					pthread_mutex_lock(&mutexFIN);
					list_add(listaFin,
							final_create(aux1, 1,
									"Variable global no encontrado"));
					pthread_mutex_unlock(&mutexFIN);
					semsig(&finalizarCont);
				}
				pthread_mutex_unlock(&mutexCPU);
			}
		}

		t_varGlobal* encontrarVarGlobal(char* nombre) {
			int _is_var(t_varGlobal *p) {
				return string_equals_ignore_case(p->nombre, nombre);
			}
			t_varGlobal *aux = list_find(listaVarGlobal, (void*) _is_var);
			return aux;
		}

		void comandoGrabarValorGlobar(char* buffer, int socket) {
			int pos = 1;
			char* variable;
			int valor;
			int ndatos;
			variable = obtenerParteDelMensaje(buffer, &pos);
			valor = atoi(obtenerParteDelMensaje(buffer, &pos));
			t_varGlobal* auxVar = encontrarVarGlobal(variable);
			if (auxVar != NULL ) {
				auxVar->valor = valor;
				ndatos = EnviarDatos(socket, "1");
				Traza("Se guardo %d en variable: %s", valor, (char *) variable);
				if (ndatos < 0) {
					//TODO error al enviar
				}
			} else {
				ndatos = EnviarDatos(socket, "A");
				pthread_mutex_lock(&mutexCPU);
				t_CPU *aux = encontrarCPU(socket);
				PCB* aux1;
				if (aux != NULL ) {
					aux1 = aux->idPCB;
					aux->idPCB = NULL;
					pthread_mutex_lock(&mutexFIN);
					list_add(listaFin,
							final_create(aux1, 1,
									"Variable global no encontrado"));
					pthread_mutex_unlock(&mutexFIN);
					semsig(&finalizarCont);
				}
			}
		}
