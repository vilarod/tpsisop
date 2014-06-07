/*
 * CPU.h
 *
 *  Created on: 26/04/2014
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_
#include<parser/parser.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>


/* Definición del pcb */
typedef struct PCBs
{
   int  id;
   int  segmentoCodigo;
   int  segmentoStack;
   int  cursorStack;
   int  indiceCodigo;
   int  indiceEtiquetas;
   int  programCounter;
   int  sizeContextoActual;
   int  sizeIndiceEtiquetas;
} PCB;

//deserializar

PCB desearilizar_PCB (char* estructura, int pos);
void deserializarDesplLong(char * msj,int despl, int longi);

//serializar

char* serializar_PCB (PCB prog);
void serCadena(char ** msj, char* agr);

//Obtener datos de conexión
int ObtenerPuerto(char* que);
char* ObtenerIP(char* que);

//Manejo de conexiones
int Enviar (int sRemoto, char * buffer);
void ConexionConSocket(int *Conec,int socketConec,struct sockaddr_in dest);
void Cerrar(int sRemoto);
int seguirConectado();
void AvisarDescAKernel();
void procesoTerminoQuantum(PCB prog);
int Recibir (int sRemoto, char* buffer);
int crearSocket(int socketConec);
struct sockaddr_in prepararDestino(struct sockaddr_in dest,int puerto,char* ip);


//Recibir-Enviar datos con el kernel
int RecibirProceso(PCB prog,int quantum,int sRemoto); //para recibir el pcb y el q
char* PedirSentencia(int indiceCodigo, int segCodigo, int progCounter, int sRemoto); //para recibir la instruccion


//Ejecutar
void parsearYejecutar (char* instr);
void salvarContextoProg();
void limpiarEstructuras();
void RecuperarContextoActual(PCB prog);

//Primitivas

void prim_wait(t_nombre_semaforo identificador_semaforo);
void prim_signal(t_nombre_semaforo identificador_semaforo);
void prim_asignar(t_puntero dir, t_valor_variable val);
t_valor_variable prim_obtenerValorCompartida(t_nombre_compartida var);  //listo
t_valor_variable prim_asignarValorCompartida(t_nombre_compartida var, t_valor_variable val);
void prim_llamarSinRetorno(t_nombre_etiqueta etiq);
void prim_llamarConRetorno(t_nombre_etiqueta etiq,
                                          t_puntero donde_ret);
void prim_finalizar(void);
void prim_retornar (t_valor_variable ret);
void prim_imprimir (t_valor_variable valor_mostrar);
t_valor_variable prim_dereferenciar(t_puntero dir_var);
void prim_irAlLabel(t_nombre_etiqueta etiq);
t_puntero prim_obtenerPosicionVariable(t_nombre_variable id_var);
t_puntero prim_definirVariable(t_nombre_variable id_var);
void prim_imprimirTexto(char* texto);
void prim_entradaSalida(t_nombre_dispositivo disp, int tiempo);









#endif /* CPU_H_ */


