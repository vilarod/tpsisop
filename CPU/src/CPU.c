/*
 ============================================================================
 Name        : CPU.c
 Author      : Romi
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

//Includes --------------------------------------------------------------
#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include "CPU.h"
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <errno.h>
#include <fcntl.h>
#include <resolv.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

//Defines -----------------------------------------------------------------
#define PATH_CONFIG "config.cfg"
#define HandU 3
#define HandK 3
#define tCPUU 2
#define tCPUK 2
#define BUFFERSIZE 40000
#define CAMBIO_PROCESO 4
#define FIN_QUANTUM 4
#define OBTENER_V_COMP 5
#define GRABAR_V_COMP 6
#define AVISO_DESC 7
#define SET_UMV 2
#define GET_UMV 1
#define AB_PROCESO "A"
#define FIN_PROCESO "F"
#define LIBRE "L-"
#define S_SIGNAL 9
#define S_WAIT 8
#define IMPRIMIR 2
#define separador "-"
#define bien "1"
#define mal "0"

//Variables globales ------------------------------------------------------
int aux_conec_umv = 0;
int aux_conec_ker = 0;

const int TAM_INSTRUCCION = 20;
const int DIG_VAL_VAR = 10;
const int DIG_NOM_VAR = sizeof(t_nombre_variable);
const int VAR_STACK = 11; //10 para el valor, 1 para el nombre


int socketUMV = 0;
int socketKERNEL = 0;

int CONECTADO_UMV = 0;
int CONECTADO_KERNEL = 0;

t_dictionary* dicVariables;
t_log* logger;

PCB* programa;
int quantum = 0;
int io = 0; //proceso bloqueado por entrada/salida
int down = 0; //proceso bloqueado por wait
int ab = 0; //proceso abortado
int f = 0;
int eti=0;
int retardo = 0; //tiempo que espero entre instruccion e instruccion
int g_ImprimirTrazaPorConsola = 1;
int senial_SIGUSR1 = 0; //seÃ±al kill
int tengoProg = 0;
char* motivo;
char* variable_ref; //guardara el nombre de la variable llamada por ObtenerDireccionVariable o ObtenerValorCompartida
char* etiquetas;


//Llamado a las funciones primitivas

AnSISOP_funciones funciones_p =
  { .AnSISOP_definirVariable = prim_definirVariable,
      .AnSISOP_obtenerPosicionVariable = prim_obtenerPosicionVariable,
      .AnSISOP_dereferenciar = prim_dereferenciar, .AnSISOP_asignar =
          prim_asignar, .AnSISOP_obtenerValorCompartida =
          prim_obtenerValorCompartida, .AnSISOP_asignarValorCompartida =
          prim_asignarValorCompartida, .AnSISOP_irAlLabel = prim_irAlLabel,
      .AnSISOP_llamarSinRetorno = prim_llamarSinRetorno,
      .AnSISOP_llamarConRetorno = prim_llamarConRetorno, .AnSISOP_finalizar =
          prim_finalizar, .AnSISOP_retornar = prim_retornar, .AnSISOP_imprimir =
          prim_imprimir, .AnSISOP_imprimirTexto = prim_imprimirTexto,
      .AnSISOP_entradaSalida = prim_entradaSalida, };

AnSISOP_kernel funciones_k =
  { .AnSISOP_signal = prim_signal, .AnSISOP_wait = prim_wait, };

//Manejo de conexiones --------------------------------------------------------

void
ConexionConSocket(int* Conec, int socketConec, struct sockaddr_in destino)
{
  if ((connect(socketConec, (struct sockaddr*) &destino,
      sizeof(struct sockaddr))) == -1)
    ErrorFatal("ERROR AL CONECTARME CON SOCKET: %d \n", socketConec);
  else
    {
      *Conec = 1;
      log_trace(logger, "ME CONECTE CON SOCKET: %d \n", socketConec);
    }
}

void
Cerrar(int sRemoto)
{
  log_trace(logger, "SE CIERRA LA CONEXION SOCKET: %d \n", sRemoto);
  close(sRemoto);
}

int
crearSocket(int socketConec)
{
  socketConec = socket(AF_INET, SOCK_STREAM, 0);
  if (socketConec == -1) //Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
    ErrorFatal("ERROR - NO SE PUEDE CONECTAR EL SOCKET: %d \n", socketConec);
  return socketConec;
}

struct sockaddr_in
prepararDestino(struct sockaddr_in destino, int puerto, char* ip)
{
  struct hostent *he, *
  gethostbyname();
  he = gethostbyname(ip);
  log_trace(logger, "TRAZA - PREPARO SOCKET DESTINO.PUERTO: %d IP: %s \n", puerto, ip);
  destino.sin_family = AF_INET;
  destino.sin_port = htons(puerto);
  bcopy(he->h_addr, &(destino.sin_addr.s_addr),he->h_length);
  memset(&(destino.sin_zero), '\0', 8);
  return destino;
}

int
saludar(int sld, int tipo, int sRemoto)
{
  char *respuesta=malloc(BUFFERSIZE*sizeof(char));
  char *mensaje = string_itoa(sld);
  string_append(&mensaje, string_itoa(tipo));
  int aux = 0;
  log_trace(logger, "%s", "TRAZA - VOY A ENVIAR HANDSHAKE \n");
  Enviar(sRemoto, mensaje);
  Recibir(sRemoto, respuesta);
  if (!(string_starts_with(respuesta, bien)))
    ErrorFatal("%s", "ERROR - HANDSHAKE NO FUE EXITOSO \n");
  else
    aux = 1;

  if (mensaje != NULL)
    free(mensaje);
  if (respuesta !=NULL)
    free(respuesta);

  return aux;
}

void
umvDesconectada()
{
  Error("%s", "ERROR - LA UMV SE HA DESCONECTADO \n");
  ab = 1;
  quantum = 0;
  motivo = "LA UMV SE HA DESCONECTADO";
  CONECTADO_UMV = 0;
}

void
kernelDesconectado()
{
  Error("%s", "ERROR - LA KERNEL SE HA DESCONECTADO \n");
  quantum = 0;
  CONECTADO_KERNEL = 0;
  f = 1;
}

//Leer archivo de configuracion ------------------------------------------------

int
ObtenerPuertoKernel()
{
  t_config* config = config_create(PATH_CONFIG);
  return config_get_int_value(config, "PUERTO_KERNEL");
}

char*
ObtenerIPKernel()
{
  t_config* config = config_create(PATH_CONFIG);
  return config_get_string_value(config, "IP_KERNEL");
}

int
ObtenerPuertoUmv()
{
  t_config* config = config_create(PATH_CONFIG);
  return config_get_int_value(config, "PUERTO_UMV");
}

char*
ObtenerIPUmv()
{
  t_config* config = config_create(PATH_CONFIG);
  return config_get_string_value(config, "IP_UMV");
}

// Enviar - Recibir datos con sockets --------------------------------------

int
Enviar(int sRemoto, char * buffer)
{
  int cantBytes;
  cantBytes = send(sRemoto, buffer, strlen(buffer), 0);
  if (cantBytes == -1)
    Error("ERROR ENVIO DATOS. SOCKET %d, Buffer: %s \n", sRemoto, buffer);
  else
   log_trace(logger, "ENVIO DATOS. SOCKET %d, Buffer: %s \n", sRemoto, buffer);

  return cantBytes;
}

int
Recibir(int sRemoto, char * buffer)
{
  int bytecount;
  memset(buffer, 0, BUFFERSIZE);
  if ((bytecount = recv(sRemoto, buffer, BUFFERSIZE, 0)) == -1)
    Error("ERROR RECIBO DATOS. SOCKET %d, Buffer: %s \n", sRemoto, buffer);
  else
  log_trace(logger, "RECIBO DATOS. SOCKET %d, Buffer: %s \n", sRemoto, buffer);

  return bytecount;
}

int
RecibirProceso()
{
  char* estructura = malloc(BUFFERSIZE * sizeof(char));
  memset(estructura,0,BUFFERSIZE);
  char* sub = string_new();
  int i, aux;
  int guiones = 0;
  int indice = 1;
  int r = 0;
  int inicio = 0;
  int control = 0;

  log_trace(logger, "%s", "TRAZA - ESTOY ESPERANDO RECIBIR UN QUANTUM + RETARDO + PCB \n");
  r = Recibir(socketKERNEL, estructura);

  if (r > 0)
    {
      if (string_starts_with(estructura, bien)) // ponele que dice que me esta enviando datos..
        {
          for (aux = 1; aux < 4; aux++)
            {
              for (i = 0;
                  ((string_equals_ignore_case(sub, separador) == 0)
                      && (inicio < (strlen(estructura) + 1))); i++)
                {
                  sub = string_substring(estructura, inicio, 1);
                  inicio++;
                  if (string_equals_ignore_case(sub, separador))
                    guiones = guiones + 1;
                }

              if (guiones < aux)
                aux = 5;

              switch (aux)
                {
              case 1:
                {
                  quantum = atoi(string_substring(estructura, indice, i));
                  log_trace(logger, "TRAZA - EL QUANTUM QUE RECIBI ES: %d \n", quantum);
                }
                break;
              case 2:
                {
                  retardo = atoi(string_substring(estructura, indice, i));
                  log_trace(logger, "TRAZA - EL RETARDO QUE RECIBI ES: %d \n", retardo);
                }
                break;
              case 3:
                programa = deserializar_PCB(estructura, indice, &control);
                break;
              case 5:
                {
                  Error("%s", "ERROR - HA OCURRIDO UN ERROR \n");
                  r = -2;
                }
                break;
                }
              sub = string_new();
              indice = inicio;
            }
        }
      else
        {
          Error("%s", "ERROR - NO RECIBI DATOS VALIDOS \n");
          r = -2;
        }
    }
  else
    {
      kernelDesconectado();
      r = -3;
    }

  if ((control < 9) || (quantum < 0) || (retardo < 0) || (guiones < 3))
    {
      Error("%s", "ERROR - NO RECIBI DATOS VALIDOS \n");
      r = -2;
      motivo = "ERROR NO RECIBI DATOS VALIDOS";
    }

  if (estructura != NULL)
    free(estructura);

  if (sub != NULL)
    free(sub);

  return r; //devuelve 0 si no tengo, -1 si fue error, >0 si recibi
}

// Manejo de log -------------------------------------------------------------

void
Error(const char* mensaje, ...)
{
  char* nuevo;
  va_list arguments;
  va_start(arguments, mensaje);
  nuevo = string_from_vformat(mensaje, arguments);
  log_error(logger, "%s", nuevo);
  va_end(arguments);
  free(nuevo);
}

void
ErrorFatal(const char* mensaje, ...)
{
  char* nuevo;
  va_list arguments;
  va_start(arguments, mensaje);
  nuevo = string_from_vformat(mensaje, arguments);
  log_error(logger, "\nERROR FATAL--> %s \n", nuevo);
  char fin;
  printf("El programa se cerrara. Presione ENTER para finalizar la ejecucion.");
  scanf("%c", &fin);
  va_end(arguments);
  free(nuevo);
  exit(EXIT_FAILURE);
}

//SERIALIZADO --------------------------------------------------------

char*
serializar_PCB(PCB* prog)
{
  char* cadena = string_new();

  log_trace(logger, "%s", "TRAZA - PREPARO PCB SERIALIZADO \n");


  string_append(&cadena, string_itoa(prog->id));
  string_append(&cadena, separador);
  string_append(&cadena, string_itoa(prog->segmentoCodigo));
  string_append(&cadena, separador);
  string_append(&cadena, string_itoa(prog->segmentoStack));
  string_append(&cadena, separador);
  string_append(&cadena, string_itoa(prog->cursorStack));
  string_append(&cadena, separador);
  string_append(&cadena, string_itoa(prog->indiceCodigo));
  string_append(&cadena, separador);
  string_append(&cadena, string_itoa(prog->indiceEtiquetas));
  string_append(&cadena, separador);
  string_append(&cadena, string_itoa(prog->programCounter));
  string_append(&cadena, separador);
  string_append(&cadena, string_itoa(prog->sizeContextoActual));
  string_append(&cadena, separador);
  string_append(&cadena, string_itoa(prog->sizeIndiceEtiquetas));
  string_append(&cadena, separador);
  return cadena;

}

//serializar en cadena, a un mensaje le agrega algo mas y tamaÃ±o de algo mas
void
serCadena(char ** msj, char* agr)
{
  string_append(msj, string_itoa(strlen(agr)));
  string_append(msj, agr);
}

//DESERIALIZADO --------------------------------------------------------

void
deserializarDesplLong(char * msj, int* despl, int* longi)
{
  int tamanio = 10;

  log_trace(logger, "TRAZA - DESERIALIZO DESPLAZAMIENTO Y LONGITUD DE: %s \n", msj);
  if (string_starts_with(msj, bien)) //si el mensaje es valido -> busca despl y longi
    {

      *despl = atoi(string_substring(msj, 1, tamanio));
      log_trace(logger, "TRAZA - DESPLAZAMIENTO: %d \n", *despl);

      *longi = atoi(string_substring(msj, tamanio + 1, tamanio));

      log_trace(logger, "TRAZA - LONGITUD: %d \n", *longi);
    }
  else
    Error("%s", "ERROR - EL MENSAJE NO PUEDE SER DESERIALIZADO \n");
}

PCB*
deserializar_PCB(char* estructura, int pos, int* cantguiones)
{
  char* sub = string_new();
  PCB* est_prog;
  est_prog = (struct PCBs *) malloc(sizeof(PCB));
  int aux;
  int i;
  int indice = pos;
  int inicio = pos;

  log_trace(logger, "%s", "TRAZA - DESERIALIZO EL PCB RECIBIDO \n");
  iniciarPCB(est_prog);

  for (aux = 1; aux < 10; aux++)
    {
      for (i = 0; string_equals_ignore_case(sub, separador) == 0; i++)
        {
          sub = string_substring(estructura, inicio, 1);
          inicio++;
          if (string_equals_ignore_case(sub, separador))
            *cantguiones = *cantguiones + 1;
        }
      if (*cantguiones == aux)
        {
          switch (aux)
            {
          case 1:
            {
              est_prog->id = atoi(string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL ID DE PROCESO ES: %d \n", est_prog->id);
              break;
            }
          case 2:
            {
              est_prog->segmentoCodigo = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL SEGMENTO DE CODIGO ES: %d \n",
                  est_prog->segmentoCodigo);
              break;
            }
          case 3:
            {
              est_prog->segmentoStack = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL SEGMENTO DE STACK ES: %d \n",
                  est_prog->segmentoStack);
              break;
            }
          case 4:
            {
              est_prog->cursorStack = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL CURSOR DE STACK ES: %d \n", est_prog->cursorStack);
              break;
            }
          case 5:
            {
              est_prog->indiceCodigo = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL INDICE DE CODIGO ES: %d \n",
                  est_prog->indiceCodigo);
              break;
            }
          case 6:
            {
              est_prog->indiceEtiquetas = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL INDICE DE ETIQUETAS ES: %d \n",
                  est_prog->indiceEtiquetas);
              break;
            }
          case 7:
            {
              est_prog->programCounter = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL PROGRAM COUNTER ES: %d \n",
                  est_prog->programCounter);
              break;
            }
          case 8:
            {
              est_prog->sizeContextoActual = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d \n",
                  est_prog->sizeContextoActual);
              break;
            }
          case 9:
            {
              est_prog->sizeIndiceEtiquetas = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL SIZE DEL INDICE DE ETIQUETAS ES: %d \n",
                  est_prog->sizeIndiceEtiquetas);
              break;
            }
            }
          sub = string_new();
          indice = inicio;
        }
      else
        {
          Error("%s", "ERROR - PCB INCORRECTO \n");
          aux = 11;
        }
    }

  if (*cantguiones == 9)
    log_trace(logger, "%s", "TRAZA - RECIBI TODO EL PCB \n");


  if (sub != NULL)
    free(sub);

  return est_prog;
}

void
iniciarPCB(PCB* prog)
{
  log_trace(logger, "%s", "TRAZA - INICIALIZO UNA ESTRUCTURA PCB CON 0 \n");
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

//Manejo de errores ------------------------------------------------------------

void
AbortarProceso()
{
  if (CONECTADO_KERNEL == 1)
    {
      //enviar "A" al kernel
      char *mensaje = string_new();
      char *respuesta=malloc(BUFFERSIZE*sizeof(char));
      string_append(&mensaje, AB_PROCESO);
      string_append(&mensaje, motivo);
      string_append(&mensaje, separador);
      log_trace(logger, "%s", "TRAZA - SE ABORTARA EL PROCESO \n");
      Enviar(socketKERNEL, mensaje);

      Recibir(socketKERNEL,respuesta);
            if  (string_starts_with(respuesta, mal))
                Error("ERROR - KERNEL NO RECIBIO MENSAJE \n");
            else
              {
                if ((string_starts_with(respuesta, bien)) == false)
                  kernelDesconectado();
              }

            if (mensaje != NULL)
              free(mensaje);
            if (respuesta != NULL)
              free(respuesta);

      motivo = string_new();
      quantum=0;
    }

}

//Manejo de seÃ±al SIGUSR1---------------------------------------------------------------

void
AtenderSenial(int s)
{
  switch (s)
    {
  case SIGUSR1:
    {
      log_trace(logger, "%s", "TRAZA - RECIBI LA SENIAL SIGUSR1 \n");

      if (quantum > 0)
        {
      senial_SIGUSR1 = 1; // marco que recibi la seÃ±al
      log_trace(logger, "%s", "Una vez finalizado el quantum, la CPU se desconectara. \n");
        }
      else
        {
          log_trace(logger, "%s","CPU sin programa en ejecucion. Se desconectara. \n");
          exit(EXIT_FAILURE);
        }
      break;
    }

    }
}

//Mensajes frecuentes con la UMV ------------------------------------------------

int
PedirSentencia(char** sentencia)
{
  int despl = 0;
  int longi = 0;
  int aux = 0;

  log_trace(logger, "%s", "TRAZA - SOLICITO SENTENCIA A LA UMV \n");
  char* instruccion = malloc(BUFFERSIZE * sizeof(char));
  memset(instruccion,0,BUFFERSIZE);

  instruccion = getUMV(programa->indiceCodigo,
      programa->programCounter * TAM_INSTRUCCION, TAM_INSTRUCCION);

  if ((string_starts_with(instruccion, bien)
      && (strlen(string_substring(instruccion, 1, TAM_INSTRUCCION)) == TAM_INSTRUCCION)))
    {
      deserializarDesplLong(instruccion, &despl, &longi);
      instruccion = string_new();
      if ((despl > (-1)) && (longi > 0))
        {
          instruccion = getUMV(programa->segmentoCodigo, despl, longi);
          if (string_starts_with(instruccion,bien))
            {
              *sentencia = string_substring(instruccion, 1, longi);
              aux = 1;
            }
          else
            mensajeAbortar("NO RECIBI CANTIDAD DE CARACTERES VALIDOS");
        }
      else
        {
          mensajeAbortar(
              string_substring(instruccion, 1, (strlen(instruccion)) - 1));
          instruccion = bien;
        }
    }
  if (string_starts_with(instruccion, mal))
    mensajeAbortar(string_substring(instruccion, 1, (strlen(instruccion)) - 1));
  else
    {
      if (!(string_starts_with(instruccion,bien)))
        umvDesconectada();
    }

  if (instruccion != NULL)
    free(instruccion);

  return aux;
}

char*
getUMV(int base, int dsp, int tam)
{
  char* respuesta = malloc(BUFFERSIZE * sizeof(char));
  memset(respuesta,0,BUFFERSIZE);
  char *mensaje = string_itoa(GET_UMV);

  serCadena(&mensaje, string_itoa(base)); //base
  serCadena(&mensaje, string_itoa(dsp)); //desplazamiento
  serCadena(&mensaje, string_itoa(tam)); //longitud
  log_trace(logger,
      "TRAZA - SOLICITO DATOS A MEMORIA.BASE: %d DESPLAZAMIENTO: %d TAMANIO: %d \n",
      base, dsp, tam);
  Enviar(socketUMV, mensaje);
  Recibir(socketUMV, respuesta);
  if (string_starts_with(respuesta, mal))
    Error("ERROR UMV: %s \n",
        string_substring(respuesta, 1, (strlen(respuesta)) - 1));
  else
    {
      if (!(string_starts_with(respuesta, bien)))
        umvDesconectada();
    }

  if (mensaje != NULL)
      free(mensaje);


  return respuesta;
}

int cantidadDigitos(int num) {
        int contador = 1;

        while (num / 10 > 0) {
                num = num / 10;
                contador++;
        }

        return contador;
}

int
setUMV(int ptro, int dsp, int tam, char* valor)
{
  char *respuesta = malloc(BUFFERSIZE * sizeof(char));
  char *mensaje = string_itoa(SET_UMV);
  int validar = 1;

  serCadena(&mensaje, string_itoa(ptro));
  serCadena(&mensaje, string_itoa(dsp));
  serCadena(&mensaje, string_itoa(tam));
  string_append(&mensaje, valor);
  log_trace(logger,
      "TRAZA - SOLICITO GRABAR EN MEMORIA.BASE: %d DESPLAZAMIENTO: %d TAMANIO: %d VALOR: %s \n",
      ptro, dsp, tam, valor);
  Enviar(socketUMV, mensaje);
  Recibir(socketUMV, respuesta);

  if (string_starts_with(respuesta, mal))
    {
      Error("%s", "ERROR - NO SE PUDO GUARDAR VALOR EN ESA DIRECCION \n");
      mensajeAbortar(string_substring(respuesta, 1, (strlen(respuesta) - 1)));
      validar = 0;
    }
  else
    {
      if (!(string_starts_with(respuesta,bien)))
        {
          umvDesconectada();
          validar = 0;
        }
    }

  if (respuesta != NULL)
    free(respuesta);
  if (mensaje != NULL)
    free(mensaje);

  return validar;

}

void
CambioProcesoActivo()
{
  if ((ab == 0) && (f == 0) && (quantum > 0))
    {
      char *respuesta=malloc(BUFFERSIZE*sizeof(char));
      char *mensaje = string_itoa(CAMBIO_PROCESO);
      serCadena(&mensaje, string_itoa(programa->id));
      log_trace(logger, "TRAZA - INFORMO A UMV QUE MI PROCESO ACTIVO ES: %d \n", programa->id);
      Enviar(socketUMV, mensaje);
      Recibir(socketUMV, respuesta);
      if (string_starts_with(respuesta, bien))
        log_trace(logger, "%s",
            "TRAZA - SE INFORMO CORRECTAMENTE EL CAMBIO DE PROCESO ACTIVO \n");
      else
        {
          if (string_starts_with(respuesta, mal))
            {
              Error("%s",
                  "ERROR - OCURRIO UN ERROR AL REALIZAR CAMBIO DE CONTEXTO \n");
              mensajeAbortar(
                  string_substring(respuesta, 1, (strlen(respuesta) - 1)));
            }
          else
            umvDesconectada();
        }

      if (mensaje != NULL)
        free(mensaje);
      if (respuesta != NULL)
        free(respuesta);
    }

}

//Mensajes frecuentes al kernel ---------------------------------------------------------

void
CPULibre()
{
  Enviar(socketKERNEL, LIBRE);
}

void
AvisarDescAKernel()

{
  if (CONECTADO_KERNEL == 1) //hace esto solo si el kernel sigue conectado
    {
      char *mensaje = string_itoa(AVISO_DESC);
      string_append(&mensaje, separador);
      log_trace(logger, "%s", "TRAZA - AVISO AL KERNEL QUE LA CPU SE DESCONECTA \n");
      Enviar(socketKERNEL, mensaje);

      if (mensaje != NULL)
        free(mensaje);
    }
}

t_valor_variable
obtener_valor(t_nombre_compartida variable)
{
  t_valor_variable valor;

  char *respuesta= malloc(BUFFERSIZE+sizeof(char));
  char *mensaje = string_itoa(OBTENER_V_COMP);

  string_append(&mensaje, variable);
  string_append(&mensaje, separador);
  log_trace(logger, "TRAZA - SOLICITO VALOR DE LA VARIABLE COMPARTIDA: %s \n", variable);
  Enviar(socketKERNEL, mensaje);  //envio PedidoVariable

  Recibir(socketKERNEL, respuesta);  //recibo EstadoValor

  if (string_starts_with(respuesta, bien)) //si comienza con 1 -> recibi un mensj valido
    {
      valor = atoi(string_substring(respuesta, 1, (strlen(respuesta) - 1)));
      log_trace(logger, "TRAZA - EL VALOR DE LA VARIABLE ES: %d \n", valor);
      variable_ref = string_new();
      variable_ref = variable;
    }
  analizarRtaKernel(respuesta);

  if (mensaje != NULL)
  free(mensaje);

  if(respuesta != NULL)
    free(respuesta);

  return valor;
}

void
grabar_valor(t_nombre_compartida variable, t_valor_variable valor)
{
  char *respuesta=malloc(BUFFERSIZE*sizeof(char));
  char *mensaje = string_itoa(GRABAR_V_COMP);

  string_append(&mensaje, variable);
  string_append(&mensaje, separador);
  string_append(&mensaje, string_itoa(valor));
  string_append(&mensaje, separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL ASIGNAR: %d A LA VARIABLE: %s \n", valor,
      variable);
  Enviar(socketKERNEL, mensaje); //el mensaje que le mando es  PedidoVariableValor

  if (mensaje != NULL)
    free(mensaje);

  Recibir(socketKERNEL, respuesta);
  analizarRtaKernel(respuesta);

  if (respuesta != NULL)
      free(respuesta);

}

void
procesoTerminoQuantum(int que, char* donde, int cuanto)
{
  if (CONECTADO_KERNEL == 1)
    {
      imprimirContextoActual();
      char *mensaje = string_itoa(FIN_QUANTUM);
      char *respuesta= malloc(BUFFERSIZE*sizeof(char));

      string_append(&mensaje, serializar_PCB(programa));
      string_append(&mensaje, string_itoa(que));
      string_append(&mensaje, separador);
      string_append(&mensaje, donde);
      string_append(&mensaje, separador);
      string_append(&mensaje, string_itoa(cuanto));
      string_append(&mensaje, separador);
      log_trace(logger,
          "TRAZA - INDICO AL KERNEL QUE EL PROCESO TERMINO EL QUANTUM CON MOTIVO : %d \n",
          que);
      Enviar(socketKERNEL, mensaje);

      if (mensaje != NULL)
        free(mensaje);

      Recibir(socketKERNEL,respuesta);

      analizarRtaKernel(respuesta);
      if(respuesta != NULL)
        free(respuesta);

    }
}

void analizarRtaKernel (char *respuesta)
{
  if (string_starts_with(respuesta, bien))
            log_trace(logger, "%s", "TRAZA - KERNEL PROCESO OK EL PEDIDO \n");
          else
            {
             if (string_starts_with(respuesta, AB_PROCESO) || string_starts_with(respuesta, mal))
               Error("ERROR - KERNEL NO RECIBIO MENSAJE \n");
              else{
                 if (atoi(string_substring(respuesta, 0,1)) > 1)
                    mensajeAbortar("FORMATO MENSAJE KERNEL NO VALIDO");
                 else
                     kernelDesconectado();
                   }
            }
}



void
finalizarProceso()
{
  log_trace(logger, "%s", "TRAZA - EL PROGRAMA FINALIZO \n");
  imprimirContextoActual();
  char *respuesta= malloc(BUFFERSIZE*sizeof(char));
  char *mensaje = malloc(BUFFERSIZE * sizeof(char));
  memset(mensaje,0,BUFFERSIZE);
  string_append(&mensaje, FIN_PROCESO);
  string_append(&mensaje, serializar_PCB(programa));
  log_trace(logger, "TRAZA - EL MENSAJE QUE LE ENVIO AL KERNEL ES: %s \n", mensaje);
  Enviar(socketKERNEL, mensaje);

  if (mensaje !=NULL)
    free(mensaje);
  Recibir(socketKERNEL,respuesta);

  analizarRtaKernel(respuesta);

  if (respuesta !=NULL)
      free(respuesta);
  ab = 0;
  f = 1;
  quantum = 0;
}

void
mensajeAbortar(char* mensaje)
{
  Error("ERROR: %s \n", mensaje);
  ab = 1;
  quantum = 0;
  motivo = mensaje;
}

//Enviar a parser --------------------------------------------------------------

void
parsearYejecutar(char* instr)
{
  log_trace(logger, "TRAZA - LA SENTENCIA: %s SE ENVIARA AL PARSER \n", instr);

  analizadorLinea(instr, &funciones_p, &funciones_k);
}

void
esperarTiempoRetardo()
{
  log_trace(logger, "TRAZA - TENGO UN TIEMPO DE ESPERA DE: %d MILISEGUNDOS \n", retardo);
  usleep(retardo); //retardo en milisegundos
}

//Manejo diccionarios ----------------------------------------------------------

void
limpiarEstructuras()
{
  log_trace(logger, "%s", "TRAZA - LIMPIO ESTRUCTURAS AUXILIARES \n");
  dictionary_clean(dicVariables);
}

void
destruirEstructuras()
{
  log_trace(logger, "%s", "TRAZA - DESTRUYO ESTRUCTURAS AUXILIARES \n");
  dictionary_destroy(dicVariables);
}

void
RecuperarEtiquetas()
{
  if ((ab == 0) && (f == 0) && (quantum > 0) && ((programa->sizeIndiceEtiquetas) > 0)) // Solo buscarÃ¡ los datos si el programa no se abortÃ³
    {
      log_trace(logger, "%s", "TRAZA - VOY A RECUPERAR EL INDICE DE ETIQUETAS \n");
      etiquetas=string_new();
      char* respuesta = malloc(BUFFERSIZE * sizeof(char));
      memset(respuesta,0,BUFFERSIZE);
      respuesta = getUMV(programa->indiceEtiquetas, 0,
          programa->sizeIndiceEtiquetas);

      if (string_starts_with(respuesta, bien))
         // && ((strlen(respuesta) - 1) == programa->sizeIndiceEtiquetas))
        {
          etiquetas = string_substring(respuesta, 1, strlen(respuesta) - 1);
          int x;
          for(x=0; x < programa->sizeIndiceEtiquetas; x ++)
            {
              if (*(etiquetas + x) == '!')
                *(etiquetas + x)='\0';
            }
        }
      else
        analizarRtaUMV(respuesta,"NO SE PUDO RECUPERAR LAS ETIQUETAS DEL PROCESO");

      if (respuesta != NULL)
        free(respuesta);
    }
}
void analizarRtaUMV (char *respuesta, char* mensaje)
{
          if (string_starts_with(respuesta, mal))
            mensajeAbortar(mensaje);
          else
            {
              if (atoi(string_substring(respuesta, 0,1)) > 1)
                mensajeAbortar("FORMATO MENSAJE UMV NO VALIDO");
                else
                  umvDesconectada();
            }
}

void
RecuperarDicVariables()
{
  //RECUPERAR ALGO
  //en sizeContextoActual tengo la cant de variables que debo leer
  //si es 0 -> programa nuevo
  //si es >0 -> cant de variables a leer desde seg stack en umv

  if ((ab == 0) && (f == 0) && (quantum > 0)) // si el programa no fue abortado antes de entrar aca
    {
      int i;
      int aux = 0;
      int ptr_posicion = 0;
      int pos = 0;

      char* respuesta = malloc(BUFFERSIZE * sizeof(char));
      memset(respuesta,0,BUFFERSIZE);
      char* var = string_new();
      char* variables = malloc(BUFFERSIZE * sizeof(char));
      memset(variables,0,BUFFERSIZE);

      log_trace(logger, "%s", "TRAZA - VOY A RECUPERAR EL DICCIONARIO DE VARIABLES \n");
      aux = programa->sizeContextoActual;
      log_trace(logger, "TRAZA - CANTIDAD DE VARIABLES A RECUPERAR: %d \n", aux);
      ptr_posicion = programa->cursorStack - programa->segmentoStack;


      if (aux > 0) //tengo variables a recuperar
        {
          respuesta = getUMV(programa->segmentoStack, ptr_posicion, (VAR_STACK * aux));
          if (string_starts_with(respuesta, bien))
            {
              variables = string_substring(respuesta, 1, strlen(respuesta) - 1);
              for (i = 0; i < aux; i++) //voy de 0 a cantidad de variables en contexto actual
                {
                  var = string_substring(variables, pos, 1);
                  dictionary_put(dicVariables, var, (void*) ptr_posicion);
                  ptr_posicion = ptr_posicion + VAR_STACK;
                  pos = pos + VAR_STACK;
                }
            }
          else
            analizarRtaUMV(respuesta,"NO SE PUDO RECUPERAR LA TOTALIDAD DEL CONTEXTO");
        }
      else
        log_trace(logger, "%s",
            "TRAZA - ES UN PROGRAMA NUEVO, NO TENGO CONTEXTO QUE RECUPERAR \n");

      if (respuesta != NULL)
      free(respuesta);
      if (variables != NULL)
      free(variables);
      if (var !=NULL)
      free(var);
    }
}

void
imprimirContextoActual()
{
  char *mensaje =  malloc(BUFFERSIZE * sizeof(char));
  memset(mensaje,0,BUFFERSIZE);
  mensaje = string_itoa(IMPRIMIR);
  char *rtaKERNEL=malloc(BUFFERSIZE * sizeof(char));

  int i;
  int aux = 0;
  int ptr_posicion = 0;
  int pos = 0;

  char* respuesta = malloc(BUFFERSIZE * sizeof(char));
  memset(respuesta,0,BUFFERSIZE);
  char* variables = malloc(BUFFERSIZE * sizeof(char));
  memset(variables,0,BUFFERSIZE);
  aux = programa->sizeContextoActual;
  ptr_posicion = (programa->cursorStack - programa->segmentoStack);

  if (aux > 0) //tengo variables a recuperar
    {
      respuesta = getUMV(programa->segmentoStack, ptr_posicion, (VAR_STACK * aux));
      if (string_starts_with(respuesta, bien))
        {
          variables = string_substring(respuesta, 1, strlen(respuesta) - 1);

          for (i = 0; i < aux; i++)
            {

              string_append(&mensaje, "VARIABLE ");
              string_append(&mensaje, string_substring(variables, pos, DIG_NOM_VAR));
              string_append(&mensaje, ":");
              string_append(&mensaje,
                  string_itoa(atoi(string_substring(variables, (pos + 1), VAR_STACK - DIG_NOM_VAR))));
              string_append(&mensaje, " ");
              pos = pos + VAR_STACK;
            }
        }
      else
       analizarRtaUMV(respuesta,"ERROR AL RECUPERAR VALORES VARIABLES");
    }
  else
    string_append(&mensaje, "NO EXISTEN VARIABLES EN EL CONTEXTO ACTUAL");

  string_append(&mensaje,separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL IMPRIMIR: %s EN EL PROGRAMA EN EJECUCION \n",
      mensaje);
  Enviar(socketKERNEL, mensaje);

  Recibir(socketKERNEL,rtaKERNEL);
  analizarRtaKernel(rtaKERNEL);

  if (respuesta != NULL)
    free(respuesta);
  if (variables != NULL)
    free(variables);
  if (mensaje != NULL)
    free(mensaje);
  if (rtaKERNEL != NULL)
      free(rtaKERNEL);

}

void
inciarVariables()
{
  quantum = 0;
  retardo = 0;
  io = 0;
  down = 0;
  ab = 0;
  tengoProg = 0;
  f = 0;
  eti=0;
}

int
estoyConectado()
{
  if ((CONECTADO_UMV == 1) && (CONECTADO_KERNEL == 1) && (senial_SIGUSR1 == 0))
    return 1;
  else
    return 0;
}

//Hilo que atiende SIGUSR1 -----------------------
void *
SENIAL(void *arg)
{
  log_trace(logger, "%s", "TRAZA - HOT PLUG ACTIVO \n");
  signal(SIGUSR1, AtenderSenial);

  return NULL ;
}

//Main --------------------------------------------------------------------------

int
main(void)


{

  //Variables locales
  int UMV_PUERTO = ObtenerPuertoUmv();
  int KERNEL_PUERTO = ObtenerPuertoKernel();
  char* UMV_IP = ObtenerIPUmv();
  char* KERNEL_IP = ObtenerIPKernel();
  char* temp_file = tmpnam(NULL );
  logger = log_create(temp_file, "CPU", g_ImprimirTrazaPorConsola,
      LOG_LEVEL_TRACE);

  char* sentencia = string_new();

  int sent = 0;

  log_trace(logger, "%s", "TRAZA - INICIA LA CPU \n");

  //Creacion del hilo senial
  pthread_t senial;
  pthread_create(&senial, NULL, SENIAL, NULL );
  pthread_join(senial, NULL );

  socketUMV = crearSocket(socketUMV);
  socketKERNEL = crearSocket(socketKERNEL);

  struct sockaddr_in dest_UMV = prepararDestino(dest_UMV, UMV_PUERTO, UMV_IP);
  struct sockaddr_in dest_KERNEL = prepararDestino(dest_KERNEL, KERNEL_PUERTO,
      KERNEL_IP);

  //Ahora que se donde estan, me quiero conectar con los dos
  ConexionConSocket(&aux_conec_umv, socketUMV, dest_UMV);

  ConexionConSocket(&aux_conec_ker, socketKERNEL, dest_KERNEL);

  if (aux_conec_umv == saludar(HandU, tCPUU, socketUMV))
    {
      CONECTADO_UMV = 1;
      log_trace(logger, "%s", "TRAZA - UMV CONTESTO HANDSHAKE OK \n");
    }
  if (aux_conec_ker == saludar(HandK, tCPUK, socketKERNEL))
    {
      CONECTADO_KERNEL = 1;
      log_trace(logger, "%s", "TRAZA - KERNEL CONTESTO HANDSHAKE OK \n");
    }

  //Creo los diccionarios
  dicVariables = dictionary_create();

  //voy a trabajar mientras este conectado tanto con kernel como umv
  while (estoyConectado() == 1)
    {
      inciarVariables();
      CPULibre();
      log_trace(logger, "%s", "TRAZA - ESTOY CONECTADO CON KERNEL Y UMV \n");
      log_trace(logger, "CANTIDAD DE PROGRAMAS QUE TENGO: %d \n", tengoProg);

      while (tengoProg == 0) //me fijo si tengo un prog que ejecutar
        {
          tengoProg = RecibirProceso();
          if (tengoProg == (-2))
            {ab=1;
            quantum=0;}

          if (tengoProg == (-3))
            ab = 0;
        }

      //Si salio del ciclo anterior es que ya tengo un programa
      CambioProcesoActivo();
      RecuperarEtiquetas();
      RecuperarDicVariables();

      while (quantum > 0) //mientras tengo quantum
        {
          eti=0;
          log_trace(logger, "TRAZA - EL QUANTUM ES: %d \n", quantum);
          log_trace(logger, "TRAZA - LA INSTRUCCION A EJECUTAR ES: %d \n",programa->programCounter);
          sent = PedirSentencia(&sentencia);
          if (sent == 1) //la sentencia es valida
            {
              parsearYejecutar(sentencia); //ejecuto sentencia
              esperarTiempoRetardo(); // espero X milisegundo para volver a ejecutar
              quantum--;
              if ((f==0) && (eti==0))
                programa->programCounter++; //Incremento el PC
            }
          else
            {
              Error("%s", "ERROR - NO SE PUDO LEER LA INSTRUCCION \n");
              quantum = 0;
              ab = 1;
            }
        }

      if ((io == 0) && (down == 0) && (ab == 0) && (f == 0))
        procesoTerminoQuantum(0, "0", 0); //no necesita e/s ni wait ni fue abortado

      if (ab == 1)
        AbortarProceso(); //proceso abortado por errores varios
      limpiarEstructuras();

      if (sentencia != NULL)
        free(sentencia);
      if (programa != NULL)
        free(programa);
      if (etiquetas != NULL)
        free(etiquetas);
    }

  AvisarDescAKernel(); //avisar al kernel asi me saca de sus recursos
  destruirEstructuras();
  Cerrar(socketKERNEL);
  Cerrar(socketUMV);

  return EXIT_SUCCESS;
}

void rellenarConCeros(int cuanto,char **mensaje)
{
  int j=0;
  while (j != cuanto)
    {
      string_append(mensaje, "0");
      j++;
    }
}

//Primitivas ---------------------------------------------------------------------

void
prim_asignar(t_puntero direccion_variable, t_valor_variable valor)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA ASIGNAR \n");
  int validar;

   char* mensaje=string_new();
   int ceros = ((VAR_STACK - DIG_NOM_VAR) - cantidadDigitos(valor));
   rellenarConCeros(ceros,&mensaje);
   string_append(&mensaje, string_itoa(valor));

  validar = setUMV(programa->segmentoStack, direccion_variable + 1, DIG_VAL_VAR, mensaje);
  if (validar == 1) //si es <=0 el set aborta el proceso
    log_trace(logger, "%s", "TRAZA - ASIGNACION EXITOSA \n");

  if (mensaje != NULL)
    free(mensaje);
}

t_valor_variable
prim_obtenerValorCompartida(t_nombre_compartida variable)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA ObtenerValorCompartida \n");
  return obtener_valor(variable); //devuelve el valor de la variable
}

t_valor_variable
prim_asignarValorCompartida(t_nombre_compartida variable,
    t_valor_variable valor)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA AsignarValorCompartida \n");
  grabar_valor(variable, valor);
  return valor; //devuelve el valor asignado
}

void
prim_llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA LlamarSinRetorno \n");
  //preservar el contexto de ejecucion actual y resetear estructuras. necesito un contexto vacio ahora
  int aux;

  char* mensaje=string_new();
  int ceros = (VAR_STACK - cantidadDigitos(programa->cursorStack));
  rellenarConCeros(ceros,&mensaje);
  string_append(&mensaje, string_itoa(programa->cursorStack));

  aux = (VAR_STACK * programa->sizeContextoActual);
  int despl=(programa->cursorStack - programa->segmentoStack) + aux;


  if (setUMV(programa->segmentoStack, despl, VAR_STACK, mensaje) == 1)
    {

      if (mensaje != NULL)
        free(mensaje);
      mensaje=string_new();

      ceros = (VAR_STACK - cantidadDigitos(programa->programCounter));
      rellenarConCeros(ceros,&mensaje);
      string_append(&mensaje, string_itoa(programa->programCounter));
      despl=despl + VAR_STACK;
      if (setUMV(programa->segmentoStack, despl, VAR_STACK,mensaje) == 1)
        {
          programa->cursorStack = programa->segmentoStack + despl + VAR_STACK;
          log_trace(logger, "TRAZA - EL CURSOR STACK APUNTA A: %d \n", programa->cursorStack);
          programa->sizeContextoActual = 0;
          prim_irAlLabel(etiqueta);
          log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d \n",
              programa->sizeContextoActual);
          dictionary_clean(dicVariables); //limpio el dic de variables
        }
    }


  if (mensaje != NULL)
    free(mensaje);
}

void
prim_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA LlamarConRetorno \n");
  //preservar el contexto actual para retornar al mismo

  int aux;

  char* mensaje=string_new();
  int ceros = (VAR_STACK - cantidadDigitos(programa->cursorStack));
  rellenarConCeros(ceros,&mensaje);
  string_append(&mensaje, string_itoa(programa->cursorStack));

  aux = (VAR_STACK * programa->sizeContextoActual);
  int despl=(programa->cursorStack - programa->segmentoStack) + aux;

  if (setUMV(programa->segmentoStack, despl, VAR_STACK, mensaje) == 1)
    {

      if (mensaje != NULL)
        free(mensaje);
      mensaje=string_new();
      ceros = (VAR_STACK - cantidadDigitos(programa->programCounter));
      rellenarConCeros(ceros,&mensaje);
      string_append(&mensaje,string_itoa(programa->programCounter));

      despl=despl + VAR_STACK;

      if (setUMV(programa->segmentoStack, despl, VAR_STACK,mensaje) == 1)
        {

          if (mensaje != NULL)
            free(mensaje);
          mensaje=string_new();
          ceros = (VAR_STACK - cantidadDigitos(donde_retornar));
          rellenarConCeros(ceros,&mensaje);
          string_append(&mensaje,string_itoa(donde_retornar));

          despl=despl + VAR_STACK;

          if (setUMV(programa->segmentoStack, despl, VAR_STACK,mensaje) == 1)
            {
              log_trace(logger, "TRAZA - LA DIRECCION POR DONDE RETORNAR ES: %d \n",donde_retornar);
              programa->cursorStack = programa->segmentoStack + despl + VAR_STACK;
              log_trace(logger, "TRAZA - EL CURSOR STACK APUNTA A: %d \n",programa->cursorStack);
              programa->sizeContextoActual = 0;
              prim_irAlLabel(etiqueta);
              log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d \n",programa->sizeContextoActual);
              dictionary_clean(dicVariables); //limpio el dic de variables
            }
        }
    }

  if (mensaje != NULL)
    free(mensaje);
}

void
prim_finalizar(void)
{
  //recuperar pc y contexto apilados en stack
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Finalizar \n");
  int aux = programa->cursorStack; //tengo que leer desde la base del stack anterior hacia abajo

  if (aux > programa->segmentoStack)
    {
      aux = (aux - programa->segmentoStack) - VAR_STACK;
      char *pedido = malloc(BUFFERSIZE * sizeof(char));
      memset(pedido,0,BUFFERSIZE);
      pedido = getUMV(programa->segmentoStack, aux, VAR_STACK);

      if (string_starts_with(pedido, bien))
        {
          programa->programCounter = atoi(string_substring(pedido, 1, strlen(pedido) - 1));

          log_trace(logger, "TRAZA - EL PROGRAM COUNTER ES: %d \n", programa->programCounter);
          aux = aux - VAR_STACK;

          if (pedido != NULL)
            free(pedido);
          pedido=string_new();
          pedido = getUMV(programa->segmentoStack, aux, VAR_STACK);
          if (string_starts_with(pedido, bien))
            {
              aux = aux - VAR_STACK;
              programa->cursorStack = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
              log_trace(logger, "TRAZA - EL CURSOR STACK ES: %d \n", programa->cursorStack);
              programa->sizeContextoActual = ((aux - (programa->cursorStack - programa->segmentoStack)) / VAR_STACK) + 1;
              log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d \n",programa->sizeContextoActual);
              if (programa->sizeContextoActual > 0)
                {
                  dictionary_clean(dicVariables); //limpio el dic de variables
                  RecuperarDicVariables();
                }
              else
                  finalizarProceso();
            }
          else
              analizarRtaUMV(pedido,"NO SE PUDO RECUPERAR CURSOR STACK");
        }
      else
        analizarRtaUMV(pedido,"NO SE PUDO RECUPERAR PROGRAM COUNTER");

      if (pedido != NULL)
        free(pedido);
    }
  else
    finalizarProceso();

}

void
prim_retornar(t_valor_variable retorno)
{  //acÃ¡ tengo que volver a retorno
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Retornar \n");

  int retor;
  int aux = programa->cursorStack; //tengo que leer desde la base del stack anterior hacia abajo
  aux = (aux - programa->segmentoStack) - VAR_STACK;
  char *pedido = malloc(BUFFERSIZE * sizeof(char));
  memset(pedido,0,BUFFERSIZE);
  pedido = getUMV(programa->segmentoStack, aux, VAR_STACK);

  char* mensj=string_new();
    int ceros = ((VAR_STACK - DIG_NOM_VAR) - cantidadDigitos(retorno));
    rellenarConCeros(ceros,&mensj);
    string_append(&mensj, string_itoa(retorno));

  if (string_starts_with(pedido,bien))
    {
      retor = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
      log_trace(logger, "TRAZA - LA DIRECCION DONDE RETORNAR ES: %d \n", retor);


      if (setUMV(programa->segmentoStack, (retor + 1), DIG_VAL_VAR, mensj) == 1)
        {
          aux = aux - VAR_STACK;
          pedido = getUMV(programa->segmentoStack, aux, VAR_STACK);

          if (string_starts_with(pedido, bien))
            {
              programa->programCounter = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
              log_trace(logger, "TRAZA - EL PROGRAM COUNTER ES: %d \n",programa->programCounter);
              aux = aux - VAR_STACK;
              pedido = getUMV(programa->segmentoStack,aux, VAR_STACK);


              if (string_starts_with(pedido, bien))
                {
                  aux=aux - VAR_STACK;
                  programa->cursorStack = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
                  log_trace(logger, "TRAZA - EL CURSOR DEL STACK ES: %d \n",programa->cursorStack);
                  programa->sizeContextoActual =((aux - (programa->cursorStack - programa->segmentoStack)) / VAR_STACK) + 1;
                  log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d \n",programa->sizeContextoActual);

                  if (programa->sizeContextoActual > 0)
                    {
                      dictionary_clean(dicVariables); //limpio el dic de variables
                      RecuperarDicVariables();
                    }
                }
              else
                analizarRtaUMV(pedido,"NO SE PUDO RECUPERAR CURSOR STACK");
            }
          else
            analizarRtaUMV(pedido,"NO SE PUDO RECUPERAR PROGRAM COUNTER");
        }
      else
        analizarRtaUMV(pedido,"NO SE PUDO RETORNAR EL VALOR A LA VARIABLE");
    }else
      analizarRtaUMV(pedido,"NO SE PUDO RECUPERAR LA DIRECCION DE RETORNO");

  if (pedido != NULL)
    free(pedido);

}

void
prim_imprimir(t_valor_variable valor_mostrar)
{
  //acÃ¡ me conecto con el kernel y le paso el mensaje
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Imprimir \n");

  char *mensaje = malloc(BUFFERSIZE*sizeof(char));
  memset(mensaje,0,BUFFERSIZE);
  mensaje=string_itoa(IMPRIMIR);
  string_append(&mensaje, "VARIABLE ");
  string_append(&mensaje, variable_ref);
  string_append(&mensaje, ":");
  string_append(&mensaje, string_itoa(valor_mostrar)); //por el momento muestra valor
  string_append(&mensaje, separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL IMPRIMIR: %d EN EL PROGRAMA EN EJECUCION \n",valor_mostrar);
  Enviar(socketKERNEL, mensaje);
  if (mensaje != NULL)
    free(mensaje);

char *rtaKERNEL = malloc(BUFFERSIZE*sizeof(char));
  memset(rtaKERNEL,0,BUFFERSIZE);

Recibir(socketKERNEL,rtaKERNEL);

analizarRtaKernel(rtaKERNEL);

if (rtaKERNEL != NULL)
free(rtaKERNEL);

if (variable_ref != NULL)
  free(variable_ref);

}

t_valor_variable
prim_dereferenciar(t_puntero direccion_variable)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Dereferenciar \n");
  t_valor_variable valor = 0;
  char* mensaje = malloc(1 * sizeof(char));

  mensaje = getUMV(programa->segmentoStack,direccion_variable + 1, DIG_VAL_VAR);
  if (string_starts_with(mensaje, bien)) //si comienza con 1 -> recibi un mensj valido
    {
    valor = atoi(string_substring(mensaje, 1, (strlen(mensaje) - 1)));
    log_trace(logger, "TRAZA - EL VALOR EXISTENTE EN ESA POSICION ES: %d \n", valor);
    }
  else
    mensajeAbortar("ERROR AL LEER DIRECCION MEMORIA");

  if (mensaje !=NULL)
    free(mensaje);

  return valor;
}

void
prim_irAlLabel(t_nombre_etiqueta etiqueta)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA IrAlLabel \n");

  programa->programCounter = metadata_buscar_etiqueta(etiqueta, etiquetas,programa->sizeIndiceEtiquetas); //asigno la primer instruccion ejecutable de etiqueta al PC
  //programa->programCounter ++ ;
  log_trace(logger, "TRAZA - EL VALOR DEL PROGRAM COUNTER ES: %d \n",programa->programCounter);
  eti=1;
}

t_puntero
prim_obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA ObtenerPosicionVariable \n");
  t_puntero posicion = 0;


  char* var = string_new();
  sprintf(var,"%c",identificador_variable);

  //busco la posicion de la variable
  //las variables y las posiciones respecto al stack estan en el dicVariables
  if (dictionary_has_key(dicVariables, var) == true)
    {
      int* aux = dictionary_get(dicVariables, var);
      posicion = (t_puntero) aux;
      log_trace(logger, "TRAZA - ENCONTRE LA VARIABLE: %s, POSICION: %d \n", var, aux);

      variable_ref=string_new();
      variable_ref = var;
    }
  else
    {
    mensajeAbortar("LA VARIABLE NO EXISTE EN EL CONTEXTO DE EJECUCION");
    posicion= -1;
    }
  if (var !=NULL)
  free(var);
  return posicion; //devuelvo la posicion
}

t_puntero
prim_definirVariable(t_nombre_variable identificador_variable)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA DefinirVariable \n");
  // reserva espacio para la variable,
  //la registra en el stack
  t_puntero pos_var_stack;

  char* var = malloc(DIG_NOM_VAR);
  var[0] = identificador_variable;

  log_trace(logger, "TRAZA - LA VARIABLE QUE SE QUIERE DEFINIR ES: %s \n",string_substring(var, 0, 1));
  pos_var_stack = (programa->cursorStack - programa->segmentoStack) + programa->sizeContextoActual * VAR_STACK;

  log_trace(logger, "TRAZA - LA POSICION DONDE SE QUIERE DEFINIR LA VARIABLE ES: %d \n",pos_var_stack);

  if ((dictionary_has_key(dicVariables, string_substring(var, 0, 1))) == false)
    {
      if ((setUMV(programa->segmentoStack, pos_var_stack, 1, string_substring(var, 0, 1))) > 0)
        {
          dictionary_put(dicVariables, string_substring(var, 0, 1),(void*) pos_var_stack); //la registro en el dicc de variables
          programa->sizeContextoActual++;
          log_trace(logger, "%s","TRAZA - SE DEFINIO CORRECTAMENTE LA VARIABLE \n");
        }
      else
        {
        mensajeAbortar("NO SE PUDO INGRESAR LA VARIABLE EN EL STACK");
        }
    }
  else
    {
   mensajeAbortar("LA VARIABLE YA SE ENCUENTRA EN EL CONTEXTO ACTUAL");
    }
  if (var != NULL)
  free(var);

  return pos_var_stack; //devuelvo la pos en el stack
}

void
prim_imprimirTexto(char* texto)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA ImprimirTexto \n");

  char *mensaje = malloc(BUFFERSIZE*sizeof(char));
  memset(mensaje,0,BUFFERSIZE);
  mensaje=string_itoa(IMPRIMIR);
  string_append(&mensaje, texto);
  string_append(&mensaje, separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL IMPRIMIR: %s EN EL PROGRAMA EN EJECUCION \n",texto);
  Enviar(socketKERNEL, mensaje);
  if (mensaje != NULL)
  free(mensaje);

  char *rtaKERNEL = malloc(BUFFERSIZE*sizeof(char));
    memset(rtaKERNEL,0,BUFFERSIZE);

  Recibir(socketKERNEL,rtaKERNEL);
  analizarRtaKernel(rtaKERNEL);
  if (rtaKERNEL !=NULL)
    free(rtaKERNEL);
}

void
prim_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA EntradaSalida \n");
  log_trace(logger,"TRAZA - DISPOSITIVO QUE SOLICITA: %s \n",dispositivo);
  quantum = 0; //para que salga del ciclo
  io = 1; //seÃ±al de que paso por entrada y salida...ya le envio el pcb al kernel
  programa->programCounter ++; //cuando vuelva, que siga la proxima instruccion
  procesoTerminoQuantum(1, dispositivo, tiempo);
}

void
prim_wait(t_nombre_semaforo identificador_semaforo)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Wait \n");
  char* senial;
  char *respuesta= malloc(BUFFERSIZE*sizeof(char));
  char *mensaje = string_itoa(S_WAIT);

  //el mensaje que le mando es  PedidoSemaforo
  string_append(&mensaje, identificador_semaforo);
  string_append(&mensaje,separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL EL SEMAFORO: %s \n", identificador_semaforo);
  Enviar(socketKERNEL, mensaje);
  Recibir(socketKERNEL, respuesta);
  senial = string_substring(respuesta, 0, 1);

  if (string_equals_ignore_case(senial, AB_PROCESO))
   mensajeAbortar(string_substring(respuesta, 1, strlen(respuesta)));
  else
    {
      if (string_equals_ignore_case(senial, mal)) //senial==1 -> consegui el sem, senial==0 -> proceso bloqueado por sem
        {
          log_trace(logger, "%s","TRAZA - EL PROCESO QUEDO BLOQUEADO A LA ESPERA DEL SEMAFORO \n");
          down = 1;
          quantum = 0;
          tengoProg = 0;
          programa->programCounter ++; //cuando vuelva, que siga la proxima instruccion
          procesoTerminoQuantum(2, identificador_semaforo, 0);
        }
      else
        {
          if (!(string_equals_ignore_case(senial, bien)))
            kernelDesconectado();
          else
            log_trace(logger, "%s", "TRAZA - EL PROCESO OBTUVO EL SEMAFORO \n");
        }
    }

  if (mensaje !=NULL)
    free(mensaje);
  if (respuesta!=NULL)
    free(respuesta);
  if(senial !=NULL)
    free(senial);
}

void
prim_signal(t_nombre_semaforo identificador_semaforo)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Signal \n");
  char *respuesta=malloc(BUFFERSIZE*sizeof(char));
  char *mensaje = string_itoa(S_SIGNAL);

  //el mensaje que le mando es  PedidoSemaforo
  string_append(&mensaje, identificador_semaforo);
  string_append(&mensaje,separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL LIBERAR UNA INSTANCIA DEL SEMAFORO: %s \n",identificador_semaforo);
  Enviar(socketKERNEL, mensaje);
  Recibir(socketKERNEL, respuesta);
  if (string_starts_with(respuesta, bien)) //si es -1 lo controla recibir
      log_trace(logger, "%s", "TRAZA - LA SOLICITUD INGRESO CORRECTAMENTE \n");
  else
    {
      if (string_starts_with(respuesta, bien))
       mensajeAbortar("NO SE PUDO LIBERAR EL SEMAFORO SOLICITADO");
      else
        {
        if (atoi(string_substring(respuesta, 0, 1)) > 1)
          mensajeAbortar("FORMATO MENSAJE KERNEL NO VALIDO");
        else
        kernelDesconectado();
        }
    }
}
