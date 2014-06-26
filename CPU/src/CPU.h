/*
 * CPU.h
 *
 *  Created on: 26/04/2014
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/collections/dictionary.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>



/* Definición del pcb */
typedef struct PCBs
{
  int id;
  int segmentoCodigo;
  int segmentoStack;
  int cursorStack;
  int indiceCodigo;
  int indiceEtiquetas;
  int programCounter;
  int sizeContextoActual;
  int sizeIndiceEtiquetas;
} PCB;




void
AtenderSenial(int s);

void *
SENIAL(void *arg);
int
estoyConectado();

void
iniciarPCB(PCB* prog);
void
destruirEstructuras();
void
CambioProcesoActivo();

//deserializar

PCB*
desearilizar_PCB(char* estructura, int pos, int* cantguiones);
void
deserializarDesplLong(char * msj, int* despl, int* longi);

//serializar
char*
getUMV(int base, int dsp, int tam);
char*
serializar_PCB(PCB* prog);
void
serCadena(char ** msj, char* agr);
int
setUMV(int ptro, int dsp, int tam, char* valor);
//int saludar(int sld,int tipo, int sRemoto); //el handshake

//Obtener datos de conexión
int
ObtenerPuertoKernel();
char*
ObtenerIPKernel();
int
ObtenerPuertoUmv();
char*
ObtenerIPUmv();

//Manejo de conexiones
int
Enviar(int sRemoto, char * buffer);
void
ConexionConSocket(int* Conec, int socketConec, struct sockaddr_in dest);
void
Cerrar(int sRemoto);
int
seguirConectado();
void
AvisarDescAKernel();
void
procesoTerminoQuantum(int que, char* donde, int cuanto);
int
Recibir(int sRemoto, char* buffer);
int
crearSocket(int socketConec);
struct sockaddr_in
prepararDestino(struct sockaddr_in dest, int puerto, char* ip);

void
inciarVariables();
char*
valoresVariablesContextoActual();

void
agregarDicValoresVariables(char* var, void* valor);

void
ErrorFatal(const char* mensaje, ...);
void
Traza(const char* mensaje, ...);
void
Error(const char* mensaje, ...);
char*
RecibirDatos(int socket, char *buffer, int *bytesRecibidos);

//Recibir-Enviar datos con el kernel
int
RecibirProceso(); //para recibir el pcb, el retardo y el quantum
int
PedirSentencia(char** sentencia); //para recibir la instruccion

void
AbortarProceso();

//Ejecutar
void
parsearYejecutar(char* instr);
void
limpiarEstructuras();
void
RecuperarDicVariables();
void
RecuperarDicEtiquetas();
void
esperarTiempoRetardo();

void
grabar_valor(t_nombre_compartida variable, t_valor_variable valor);

//Primitivas

void
prim_wait(t_nombre_semaforo identificador_semaforo);
void
prim_signal(t_nombre_semaforo identificador_semaforo);
void
prim_asignar(t_puntero dir, t_valor_variable val);
t_valor_variable
prim_obtenerValorCompartida(t_nombre_compartida var);  //listo
t_valor_variable
prim_asignarValorCompartida(t_nombre_compartida var, t_valor_variable val);
void
prim_llamarSinRetorno(t_nombre_etiqueta etiq);
void
prim_llamarConRetorno(t_nombre_etiqueta etiq, t_puntero donde_ret);
void
prim_finalizar(void);
void
prim_retornar(t_valor_variable ret);
void
prim_imprimir(t_valor_variable valor_mostrar);
t_valor_variable
prim_dereferenciar(t_puntero dir_var);
void
prim_irAlLabel(t_nombre_etiqueta etiq);
t_puntero
prim_obtenerPosicionVariable(t_nombre_variable id_var);
t_puntero
prim_definirVariable(t_nombre_variable id_var);
void
prim_imprimirTexto(char* texto);
void
prim_entradaSalida(t_nombre_dispositivo disp, int tiempo);

#endif /* CPU_H_ */

