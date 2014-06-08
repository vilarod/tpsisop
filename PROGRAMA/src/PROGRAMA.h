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
void Traza(const char* mensaje, ...);
void Error(const char* mensaje, ...);
void ErrorFatal(char mensaje[], ...);
void CerrarSocket(int socket);
int EnviarDatos(int socket, void *buffer);
int RecibirDatos(int socket, char *buffer);
int ConexionConSocket(int puerto, char* IP);
int analizarRespuestaKERNEL(char *mensaje);
int hacerhandshakeKERNEL(int sockfd, char *prueba);//agrego como parametro prueba que el programa
void conectarAKERNEL(char *programa);//agrego como parametro prueba que es el programa
int soquete;



#endif /* PROGRAMA_H_ */
