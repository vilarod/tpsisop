/*
 * CPU.h
 *
 *  Created on: 26/04/2014
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_
#include<parser/parser.h>

int ObtenerPuertoUMV();
char* ObtenerIPUMV();
int Enviar (int sRemoto, void * buffer);
void ConexionConSocket();
void Cerrar();


//Primitivas

void asignar(t_puntero dir, t_valor_variable val);
t_valor_variable obtenerValorCompartida(t_nombre_compartida var);
t_valor_variable asignarValorCompartida(t_nombre_compartida var, t_valor_variable val);
t_puntero_instruccion llamarSinRetorno(t_nombre_etiqueta etiq);
t_puntero_instruccion llamarConRetorno(t_nombre_etiqueta etiq,
					  t_puntero donde_ret);
t_puntero_instruccion finalizar(void);
t_puntero_instruccion retornar (t_valor_variable ret);
int imprimir (t_valor_variable valor_mostrar);
t_valor_variable dereferenciar(t_puntero dir_var);
t_puntero_instruccion irAlLabel(t_nombre_etiqueta etiq);
t_puntero obtenerPosicionVariable(t_nombre_variable id_var);
t_puntero definirVariable(t_nombre_variable id_var);
int imprimirTexto(char* texto);
int entradaSalida(t_nombre_dispositivo disp, int tiempo);
/* int wait(t_nombre_semaforo id_sem);
int signal(t_nombre_semaforo id_sem); */



#endif /* CPU_H_ */


