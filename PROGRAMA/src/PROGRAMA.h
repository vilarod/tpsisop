/*
 * PROGRAMA.h
 *
 *  Created on: 26/04/2014
 *      Author: utnso
 */

#ifndef PROGRAMA_H_
#define PROGRAMA_H_
int ObtenerTamanioMemoriaConfig();
//obtener datos de conexion
int ObtenerPuertoKERNEL();
char* ObtenerIpKERNEL();
//conexion con el kernel
int ImprimirTrazaPorConsola = 1; //si es cero no imprime la traza
void traza(const char* mensaje, ...);
void Error(const char* mensaje, ...);
void errorFatal(char mensaje[], ...);
//void CerrarSocket(int socket);
int enviarDatos(int socket, void *buffer);
int recibirDatos(int socket, char *buffer);
int conexionConSocket(int puerto, char* IP);
int analizarRespuestaKERNEL(char *mensaje);
int hacerhandshakeKERNEL(int sockfd, char *programa);
void conectarAKERNEL(char *programa);
int enviarConfirmacionDeRecepcionDeDatos();
int analizarSiEsFinDeEjecucion(char *respuesta);
int imprimirRespuesta(char *texto);
void Error1(int code, char *err);

#endif /* PROGRAMA_H_ */
