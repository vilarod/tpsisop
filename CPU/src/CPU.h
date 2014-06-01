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


//Obtener datos de conexión
int ObtenerPuertoUMV();
int ObtenerPuertoKERNEL();
char* ObtenerIPUMV();

//Manejo de conexiones
int Enviar (int sRemoto, char * buffer);
void ConexionConSocket(int *Conec,int socketConec,struct sockaddr_in dest);
void Cerrar(int sRemoto);
int seguirConectado();
void AvisarDescAKernel();
void procesoTerminoQuantum();
int Recibir (int sRemoto, char* buffer);
int crearSocket(int socketConec);
struct sockaddr_in prepararDestino(struct sockaddr_in dest,int puerto,int ip);


//Recibir-Enviar datos con el kernel
int RecibirProceso(PCB prog,int quantum,int sRemoto); //para recibir el pcb y el q
int PedirSentencia(int indiceCodigo, int sRemoto, char* sentencia); //para recibir la instruccion


//Ejecutar
void parsearYejecutar (char* instr);
void salvarContextoProg();
void limpiarEstructuras();


//Primitivas

void AnSISOP_asignar(t_puntero dir, t_valor_variable val);
t_valor_variable AnSISOP_obtenerValorCompartida(t_nombre_compartida var);
t_valor_variable AnSISOP_asignarValorCompartida(t_nombre_compartida var, t_valor_variable val);
t_puntero_instruccion AnSISOP_llamarSinRetorno(t_nombre_etiqueta etiq);
t_puntero_instruccion AnSISOP_llamarConRetorno(t_nombre_etiqueta etiq,
					  t_puntero donde_ret);
t_puntero_instruccion AnSISOP_finalizar(void);
t_puntero_instruccion AnSISOP_retornar (t_valor_variable ret);
int AnSISOP_imprimir (t_valor_variable valor_mostrar);
t_valor_variable AnSISOP_dereferenciar(t_puntero dir_var);
t_puntero_instruccion AnSISOP_irAlLabel(t_nombre_etiqueta etiq);
t_puntero AnSISOP_obtenerPosicionVariable(t_nombre_variable id_var);
t_puntero AnSISOP_definirVariable(t_nombre_variable id_var);
int AnSISOP_imprimirTexto(char* texto);
int AnSISOP_entradaSalida(t_nombre_dispositivo disp, int tiempo);
int AnSISOP_wait(t_nombre_semaforo id_sem);
int AnSISOP_signal(t_nombre_semaforo id_sem);



#endif /* CPU_H_ */


