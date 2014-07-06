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
#define BUFFERSIZE 10000
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

const int TAM_INSTRUCCION = sizeof(t_intructions);
const int DIG_VAL_VAR = sizeof(t_valor_variable);
const int DIG_NOM_VAR = sizeof(t_nombre_variable);
const int VAR_STACK = (sizeof(t_valor_variable) + sizeof(t_nombre_variable));


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
    ErrorFatal("ERROR AL CONECTARME CON SOCKET: %d", socketConec);
  else
    {
      *Conec = 1;
      log_trace(logger, "ME CONECTE CON SOCKET: %d", socketConec);
    }
}

void
Cerrar(int sRemoto)
{
  log_trace(logger, "TRAZA - SE CIERRA LA CONEXION SOCKET: %d", sRemoto);
  close(sRemoto);
}

int
crearSocket(int socketConec)
{
  socketConec = socket(AF_INET, SOCK_STREAM, 0);
  if (socketConec == -1) //Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
    ErrorFatal("ERROR - NO SE PUEDE CONECTAR EL SOCKET: %d", socketConec);
  return socketConec;
}

struct sockaddr_in
prepararDestino(struct sockaddr_in destino, int puerto, char* ip)
{
  struct hostent *he, *
  gethostbyname();
  he = gethostbyname(ip);
  log_trace(logger, "TRAZA - PREPARO SOCKET DESTINO.PUERTO: %d IP: %s", puerto, ip);
  destino.sin_family = AF_INET;
  destino.sin_port = htons(puerto);
  bcopy(he->h_addr, &(destino.sin_addr.s_addr),he->h_length);
  memset(&(destino.sin_zero), '\0', 8);
  return destino;
}

int
saludar(int sld, int tipo, int sRemoto)
{
  char respuesta[BUFFERSIZE];
  char *mensaje = string_itoa(sld);
  string_append(&mensaje, string_itoa(tipo));
  int aux = 0;
  log_trace(logger, "%s", "TRAZA - VOY A ENVIAR HANDSHAKE");
  Enviar(sRemoto, mensaje);
  Recibir(sRemoto, respuesta);
  if (!(string_starts_with(respuesta, "1")))
    ErrorFatal("%s", "ERROR - HANDSHAKE NO FUE EXITOSO");
  else
    aux = 1;

  return aux;
}

void
umvDesconectada()
{
  Error("%s", "ERROR - LA UMV SE HA DESCONECTADO");
  ab = 1;
  quantum = 0;
  motivo = "LA UMV SE HA DESCONECTADO";
  CONECTADO_UMV = 0;
}

void
kernelDesconectado()
{
  Error("%s", "ERROR - LA KERNEL SE HA DESCONECTADO");
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
    Error("ERROR ENVIO DATOS. SOCKET %d, Buffer: %s", sRemoto, buffer);
  else
    //log_trace(logger, "ENVIO DATOS. SOCKET %d, Buffer: %s", sRemoto, buffer);

  printf("ENVIO DATOS. SOCKET %d, Buffer: %s", sRemoto, buffer);
  return cantBytes;
}

int
Recibir(int sRemoto, char * buffer)
{
  int bytecount;
  memset(buffer, 0, BUFFERSIZE);
  if ((bytecount = recv(sRemoto, buffer, BUFFERSIZE, 0)) == -1)
    Error("ERROR RECIBO DATOS. SOCKET %d, Buffer: %s", sRemoto, buffer);
  else
 //   log_trace(logger, "RECIBO DATOS. SOCKET %d, Buffer: %s", sRemoto, buffer);


  printf("RECIBO DATOS. SOCKET %d, Buffer: %s", sRemoto, buffer);
  return bytecount;
}

int
RecibirProceso()
{
  char* estructura = malloc(BUFFERSIZE * sizeof(char));
  char* sub = string_new();
  int i, aux;
  int guiones = 0;
  int indice = 1;
  int r = 0;
  int inicio = 0;
  int control = 0;

  log_trace(logger, "%s", "TRAZA - ESTOY ESPERANDO RECIBIR UN QUANTUM + RETARDO + PCB");
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
                  log_trace(logger, "TRAZA - EL QUANTUM QUE RECIBI ES: %d", quantum);
                }
                break;
              case 2:
                {
                  retardo = atoi(string_substring(estructura, indice, i));
                  log_trace(logger, "TRAZA - EL RETARDO QUE RECIBI ES: %d", retardo);
                }
                break;
              case 3:
                programa = deserializar_PCB(estructura, indice, &control);
                break;
              case 5:
                {
                  Error("%s", "ERROR - HA OCURRIDO UN ERROR");
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
          Error("%s", "ERROR - NO RECIBI DATOS VALIDOS");
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
      Error("%s", "ERROR - NO RECIBI DATOS VALIDOS");
      r = -2;
      motivo = "ERROR NO RECIBI DATOS VALIDOS";
    }

  free(estructura);
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
  //log_trace(logger, "%s", "TRAZA - PREPARO PCB SERIALIZADO");

  log_trace(logger, "%s", "TRAZA - PREPARO PCB SERIALIZADO");


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
  int tamanio1 = sizeof(t_puntero_instruccion);
  int tamanio2 = sizeof(t_size);

  //log_trace(logger, "TRAZA - DESERIALIZO DESPLAZAMIENTO Y LONGITUD DE: %s", msj);

  log_trace(logger, "TRAZA - DESERIALIZO DESPLAZAMIENTO Y LONGITUD DE: %s", msj);
  if (string_starts_with(msj, bien)) //si el mensaje es valido -> busca despl y longi
    {
      *despl = atoi(string_substring(msj, 1, tamanio1));
      log_trace(logger, "TRAZA - DESPLAZAMIENTO: %d", *despl);

      *longi = atoi(string_substring(msj, tamanio1 + 1, tamanio2));
      log_trace(logger, "TRAZA - LONGITUD: %d", *longi);
    }
  else
    Error("%s", "ERROR - EL MENSAJE NO PUEDE SER DESERIALIZADO");
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

  log_trace(logger, "%s", "TRAZA - DESERIALIZO EL PCB RECIBIDO");
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
              log_trace(logger, "TRAZA - EL ID DE PROCESO ES: %d", est_prog->id);
              break;
            }
          case 2:
            {
              est_prog->segmentoCodigo = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL SEGMENTO DE CODIGO ES: %d",
                  est_prog->segmentoCodigo);
              break;
            }
          case 3:
            {
              est_prog->segmentoStack = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL SEGMENTO DE STACK ES: %d",
                  est_prog->segmentoStack);
              break;
            }
          case 4:
            {
              est_prog->cursorStack = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL CURSOR DE STACK ES: %d", est_prog->cursorStack);
              break;
            }
          case 5:
            {
              est_prog->indiceCodigo = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL INDICE DE CODIGO ES: %d",
                  est_prog->indiceCodigo);
              break;
            }
          case 6:
            {
              est_prog->indiceEtiquetas = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL INDICE DE ETIQUETAS ES: %d",
                  est_prog->indiceEtiquetas);
              break;
            }
          case 7:
            {
              est_prog->programCounter = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL PROGRAM COUNTER ES: %d",
                  est_prog->programCounter);
              break;
            }
          case 8:
            {
              est_prog->sizeContextoActual = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d",
                  est_prog->sizeContextoActual);
              break;
            }
          case 9:
            {
              est_prog->sizeIndiceEtiquetas = atoi(
                  string_substring(estructura, indice, i));
              log_trace(logger, "TRAZA - EL SIZE DEL INDICE DE ETIQUETAS ES: %d",
                  est_prog->sizeIndiceEtiquetas);
              break;
            }
            }
          sub = string_new();
          indice = inicio;
        }
      else
        {
          Error("%s", "ERROR - PCB INCORRECTO");
          aux = 11;
        }
    }

  if (*cantguiones == 9)
    log_trace(logger, "%s", "TRAZA - RECIBI TODO EL PCB");

  return est_prog;
}

void
iniciarPCB(PCB* prog)
{
  log_trace(logger, "%s", "TRAZA - INICIALIZO UNA ESTRUCTURA PCB CON 0");
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
      char* respuesta=string_new();
      string_append(&mensaje, AB_PROCESO);
      string_append(&mensaje, motivo);
      string_append(&mensaje, separador);
      log_trace(logger, "%s", "TRAZA - SE ABORTARA EL PROCESO");
      Enviar(socketKERNEL, mensaje);
      free(mensaje);
      Recibir(socketKERNEL,respuesta);
            if  (string_starts_with(respuesta, mal))
                Error("ERROR - KERNEL NO RECIBIO MENSAJE");
            else
              {
                if ((string_starts_with(respuesta, bien)) == false)
                  kernelDesconectado();
              }

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
      log_trace(logger, "%s", "TRAZA - RECIBI LA SENIAL SIGUSR1");
      senial_SIGUSR1 = 1; // marco que recibi la seÃ±al
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

  log_trace(logger, "%s", "TRAZA - SOLICITO SENTENCIA A LA UMV");
  char* instruccion = malloc(BUFFERSIZE * sizeof(char));

  instruccion = getUMV(programa->indiceCodigo,
      programa->programCounter * TAM_INSTRUCCION, TAM_INSTRUCCION);

  if (string_starts_with(instruccion, bien)
      && ((strlen(instruccion) - 1) == TAM_INSTRUCCION))
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

  return aux;
}

char*
getUMV(int base, int dsp, int tam)
{
  char* respuesta = malloc(BUFFERSIZE * sizeof(char));
  char *mensaje = string_itoa(GET_UMV);

  serCadena(&mensaje, string_itoa(base)); //base
  serCadena(&mensaje, string_itoa(dsp)); //desplazamiento
  serCadena(&mensaje, string_itoa(tam)); //longitud
  log_trace(logger,
      "TRAZA - SOLICITO DATOS A MEMORIA.BASE: %d DESPLAZAMIENTO: %d TAMANIO: %d",
      base, dsp, tam);
  Enviar(socketUMV, mensaje);
  Recibir(socketUMV, respuesta);
  if (string_starts_with(respuesta, mal))
    Error("ERROR UMV: %s",
        string_substring(respuesta, 1, (strlen(respuesta)) - 1));
  else
    {
      if (!(string_starts_with(respuesta, bien)))
        umvDesconectada();
    }

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
  char respuesta[BUFFERSIZE];
  char *mensaje = string_itoa(SET_UMV);
  int validar = 1;

  serCadena(&mensaje, string_itoa(ptro));
  serCadena(&mensaje, string_itoa(dsp));
  serCadena(&mensaje, string_itoa(tam));
  string_append(&mensaje, valor);
  log_trace(logger,
      "TRAZA - SOLICITO GRABAR EN MEMORIA.BASE: %d DESPLAZAMIENTO: %d TAMANIO: %d VALOR: %s",
      ptro, dsp, tam, valor);
  Enviar(socketUMV, mensaje);
  Recibir(socketUMV, respuesta);

  if (string_starts_with(respuesta, mal))
    {
      Error("%s", "ERROR - NO SE PUDO GUARDAR VALOR EN ESA DIRECCION");
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

  return validar;

}

void
CambioProcesoActivo()
{
  if ((ab == 0) && (f == 0) && (quantum > 0))
    {
      char respuesta[BUFFERSIZE];
      char *mensaje = string_itoa(CAMBIO_PROCESO);
      serCadena(&mensaje, string_itoa(programa->id));
      log_trace(logger, "TRAZA - INFORMO A UMV QUE MI PROCESO ACTIVO ES: %d", programa->id);
      Enviar(socketUMV, mensaje);
      Recibir(socketUMV, respuesta);
      if (string_starts_with(respuesta, bien))
        log_trace(logger, "%s",
            "TRAZA - SE INFORMO CORRECTAMENTE EL CAMBIO DE PROCESO ACTIVO");
      else
        {
          if (string_starts_with(respuesta, mal))
            {
              Error("%s",
                  "ERROR - OCURRIO UN ERROR AL REALIZAR CAMBIO DE CONTEXTO");
              mensajeAbortar(
                  string_substring(respuesta, 1, (strlen(respuesta) - 1)));
            }
          else
            umvDesconectada();
        }
    }

}

//Mensajes frecuentes al kernel ---------------------------------------------------------

void
CPULibre()
{
  char* respuesta=string_new();
  Enviar(socketKERNEL, LIBRE);
  Recibir(socketKERNEL,respuesta);
  if ((string_starts_with(respuesta, bien)))

      printf("bien!!! %s",respuesta);
  else
    {

        if  (string_starts_with(respuesta, mal))
            Error("ERROR - KERNEL NO RECIBIO MENSAJE");
        else

              kernelDesconectado();

    }

}

void
AvisarDescAKernel()

{
  if (CONECTADO_KERNEL == 1) //hace esto solo si el kernel sigue conectado
    {
      char *mensaje = string_itoa(AVISO_DESC);
      string_append(&mensaje, separador);
      log_trace(logger, "%s", "TRAZA - AVISO AL KERNEL QUE LA CPU SE DESCONECTA");
      Enviar(socketKERNEL, mensaje);
    }
}

t_valor_variable
obtener_valor(t_nombre_compartida variable)
{
  t_valor_variable valor;

  char respuesta[BUFFERSIZE];
  char *mensaje = string_itoa(OBTENER_V_COMP);

  string_append(&mensaje, variable);
  string_append(&mensaje, separador);
  log_trace(logger, "TRAZA - SOLICITO VALOR DE LA VARIABLE COMPARTIDA: %s", variable);
  Enviar(socketKERNEL, mensaje);  //envio PedidoVariable

  Recibir(socketKERNEL, respuesta);  //recibo EstadoValor

  if (string_starts_with(respuesta, bien)) //si comienza con 1 -> recibi un mensj valido
    {
      valor = atoi(string_substring(respuesta, 1, (strlen(respuesta) - 1)));
      log_trace(logger, "TRAZA - EL VALOR DE LA VARIABLE ES: %d", valor);
      variable_ref = string_new();
      variable_ref = variable;
    }
  else
    {
      if (string_starts_with(respuesta, mal))
        mensajeAbortar(string_substring(respuesta, 1, (strlen(respuesta) - 1)));
      else
        kernelDesconectado();
    }

  free(mensaje);
  return valor;
}

void
grabar_valor(t_nombre_compartida variable, t_valor_variable valor)
{
  char respuesta[BUFFERSIZE];
  char *mensaje = string_itoa(GRABAR_V_COMP);

  string_append(&mensaje, variable);
  string_append(&mensaje, separador);
  string_append(&mensaje, string_itoa(valor));
  string_append(&mensaje, separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL ASIGNAR: %d A LA VARIABLE: %s", valor,
      variable);
  Enviar(socketKERNEL, mensaje); //el mensaje que le mando es  PedidoVariableValor

  Recibir(socketKERNEL, respuesta);
  if (string_starts_with(respuesta, bien))
    log_trace(logger, "%s", "TRAZA - KERNEL PROCESO OK EL PEDIDO");
  else
    {
      if (string_starts_with(respuesta, mal))
        mensajeAbortar(string_substring(respuesta, 1, (strlen(respuesta) - 1)));
      else
        kernelDesconectado();
    }
  free(mensaje);
}

void
procesoTerminoQuantum(int que, char* donde, int cuanto)
{
  if (CONECTADO_KERNEL == 1)
    {
      imprimirContextoActual();
      char *mensaje = string_itoa(FIN_QUANTUM);
      char *respuesta=string_new();

      string_append(&mensaje, serializar_PCB(programa));
      string_append(&mensaje, string_itoa(que));
      string_append(&mensaje, separador);
      string_append(&mensaje, donde);
      string_append(&mensaje, separador);
      string_append(&mensaje, string_itoa(cuanto));
      string_append(&mensaje, separador);
      log_trace(logger,
          "TRAZA - INDICO AL KERNEL QUE EL PROCESO TERMINO EL QUANTUM CON MOTIVO : %d",
          que);
      Enviar(socketKERNEL, mensaje);
      free(mensaje);
      Recibir(socketKERNEL,respuesta);
      if  (string_starts_with(respuesta, mal))
          Error("ERROR - KERNEL NO RECIBIO MENSAJE");
      else
        {
          if ((string_starts_with(respuesta, bien)) == false)
            kernelDesconectado();
        }

    }
}

void
finalizarProceso()
{
  log_trace(logger, "%s", "TRAZA - EL PROGRAMA FINALIZO");
  imprimirContextoActual();
  char* respuesta=string_new();
  char *mensaje = malloc(BUFFERSIZE * sizeof(char));
  string_append(&mensaje, FIN_PROCESO);
  string_append(&mensaje, serializar_PCB(programa));
  log_trace(logger, "TRAZA - EL MENSAJE QUE LE ENVIO AL KERNEL ES: %s", mensaje);
  Enviar(socketKERNEL, mensaje);
  Recibir(socketKERNEL,respuesta);
        if  (string_starts_with(respuesta, mal))
            Error("ERROR - KERNEL NO RECIBIO MENSAJE");
        else
          {
            if ((string_starts_with(respuesta, bien)) == false)
              kernelDesconectado();
          }

  free(mensaje);
  ab = 0;
  f = 1;
  quantum = 0;
}

void
mensajeAbortar(char* mensaje)
{
  Error("ERROR: %s", mensaje);
  ab = 1;
  quantum = 0;
  motivo = mensaje;
}

//Enviar a parser --------------------------------------------------------------

void
parsearYejecutar(char* instr)
{
  log_trace(logger, "TRAZA - LA SENTENCIA: %s SE ENVIARA AL PARSER", instr);

  analizadorLinea(instr, &funciones_p, &funciones_k);
}

void
esperarTiempoRetardo()
{
  log_trace(logger, "TRAZA - TENGO UN TIEMPO DE ESPERA DE: %d MILISEGUNDOS", retardo);
  usleep(retardo); //retardo en milisegundos
}

//Manejo diccionarios ----------------------------------------------------------

void
limpiarEstructuras()
{
  log_trace(logger, "%s", "TRAZA - LIMPIO ESTRUCTURAS AUXILIARES");
  dictionary_clean(dicVariables);
}

void
destruirEstructuras()
{
  log_trace(logger, "%s", "TRAZA - DESTRUYO ESTRUCTURAS AUXILIARES");
  dictionary_destroy(dicVariables);
}

void
RecuperarEtiquetas()
{
  if ((ab == 0) && (f == 0) && (quantum > 0) && ((programa->sizeIndiceEtiquetas) > 0)) // Solo buscarÃ¡ los datos si el programa no se abortÃ³
    {
      log_trace(logger, "%s", "TRAZA - VOY A RECUPERAR EL INDICE DE ETIQUETAS");

      char* respuesta = malloc(BUFFERSIZE * sizeof(char));

      respuesta = getUMV(programa->indiceEtiquetas, 0,
          programa->sizeIndiceEtiquetas);

      if (string_starts_with(respuesta, bien)
          && ((strlen(respuesta) - 1) == programa->sizeIndiceEtiquetas))
        {
          etiquetas = string_substring(respuesta, 1, strlen(respuesta) - 1);
          log_trace(logger, "TRAZA - INDICE DE ETIQUETAS OBTENIDO: %s", etiquetas);
        }
      else
        {
          if (string_starts_with(respuesta, mal))
            mensajeAbortar("NO SE PUDO RECUPERAR LAS ETIQUETAS DEL PROCESO");
          else
            umvDesconectada();
        }
      free(respuesta);
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
      char* var = string_new();
      char* variables = malloc(BUFFERSIZE * sizeof(char));

      log_trace(logger, "%s", "TRAZA - VOY A RECUPERAR EL DICCIONARIO DE VARIABLES");
      aux = programa->sizeContextoActual;
      log_trace(logger, "TRAZA - CANTIDAD DE VARIABLES A RECUPERAR: %d", aux);
      ptr_posicion = programa->cursorStack;

      if (aux > 0) //tengo variables a recuperar
        {
          respuesta = getUMV(ptr_posicion, 0, (VAR_STACK * aux));
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
            {
              if (string_starts_with(respuesta, mal))
                {
                  mensajeAbortar(
                      "NO SE PUDO RECUPERAR LA TOTALIDAD DEL CONTEXTO");
                }
              else
                umvDesconectada();
            }
        }
      else
        log_trace(logger, "%s",
            "TRAZA - ES UN PROGRAMA NUEVO, NO TENGO CONTEXTO QUE RECUPERAR");

      free(respuesta);
      free(variables);
      free(var);
    }
}

void
imprimirContextoActual()
{
  char *mensaje = string_itoa(IMPRIMIR);

  int i;
  int aux = 0;
  int ptr_posicion = 0;
  int pos = 0;

  char* respuesta = malloc(BUFFERSIZE * sizeof(char));
  char* variables = malloc(BUFFERSIZE * sizeof(char));

  aux = programa->sizeContextoActual;
  ptr_posicion = programa->cursorStack;

  if (aux > 0) //tengo variables a recuperar
    {
      respuesta = getUMV(ptr_posicion, 0, (VAR_STACK * aux));
      if (string_starts_with(respuesta, bien))
        {
          variables = string_substring(respuesta, 1, strlen(respuesta) - 1);
          for (i = 0; i < aux; i++)
            {

              string_append(&mensaje, "VARIABLE ");
              string_append(&mensaje, string_substring(variables, pos, 1));
              string_append(&mensaje, ":");
              string_append(&mensaje,
                  string_itoa(atoi(string_substring(variables, (pos + 1), 4))));
              string_append(&mensaje, " ");
              pos = pos + VAR_STACK;
            }
        }
      else
        {
          string_append(&mensaje, "ERROR AL RECUPERAR VALORES VARIABLES");
          if (!(string_starts_with(respuesta, mal)))
            {
              umvDesconectada();
              ab = 0;
            }
        }
    }
  else
    string_append(&mensaje, "NO EXISTEN VARIABLES EN EL CONTEXTO ACTUAL");

  string_append(&mensaje,separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL IMPRIMIR: %s EN EL PROGRAMA EN EJECUCION",
      mensaje);
  Enviar(socketKERNEL, mensaje);

  free(respuesta);
  free(variables);
  free(mensaje);
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
  motivo = string_new();
  variable_ref = string_new();
  etiquetas = string_new();
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
  log_trace(logger, "%s", "TRAZA - HOT PLUG ACTIVO");
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

  log_trace(logger, "%s", "TRAZA - INICIA LA CPU");

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
      log_trace(logger, "%s", "TRAZA - UMV CONTESTO HANDSHAKE OK");
    }
  if (aux_conec_ker == saludar(HandK, tCPUK, socketKERNEL))
    {
      CONECTADO_KERNEL = 1;
      log_trace(logger, "%s", "TRAZA - KERNEL CONTESTO HANDSHAKE OK");
    }

  //Creo los diccionarios
  dicVariables = dictionary_create();

  //voy a trabajar mientras este conectado tanto con kernel como umv
  while (estoyConectado() == 1)
    {
      inciarVariables();
      CPULibre();
      log_trace(logger, "%s", "TRAZA - ESTOY CONECTADO CON KERNEL Y UMV");
     log_trace(logger, "CANTIDAD DE PROGRAMAS QUE TENGO: %d", tengoProg);

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
          log_trace(logger, "TRAZA - EL QUANTUM ES: %d", quantum);
          sent = PedirSentencia(&sentencia);
          if (sent == 1) //la sentencia es valida
            {
              parsearYejecutar(sentencia); //ejecuto sentencia
              esperarTiempoRetardo(); // espero X milisegundo para volver a ejecutar
              quantum--;
              if (f==0)
                {
              programa->programCounter++; //Incremento el PC
              log_trace(logger, "TRAZA - LA PROXIMA INSTRUCCION ES: %d",
                  programa->programCounter);
                }
            }
          else
            {
              Error("%s", "ERROR - NO SE PUDO LEER LA INSTRUCCION");
              quantum = 0;
              ab = 1;
            }
        }

      if ((io == 0) && (down == 0) && (ab == 0) && (f == 0))
        procesoTerminoQuantum(0, "0", 0); //no necesita e/s ni wait ni fue abortado

      if (ab == 1)
        AbortarProceso(); //proceso abortado por errores varios
      limpiarEstructuras();

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
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA ASIGNAR");
  int validar;
  char* mensaje=string_new();
  int ceros = (DIG_VAL_VAR - cantidadDigitos(valor));
  rellenarConCeros(ceros,&mensaje);
  string_append(&mensaje, string_itoa(valor));

  int despl=((programa->segmentoStack - programa->cursorStack)/VAR_STACK) + direccion_variable + DIG_NOM_VAR;
  validar = setUMV(programa->segmentoStack, despl, DIG_VAL_VAR, mensaje);
  if (validar == 1) //si es <=0 el set aborta el proceso
    log_trace(logger, "%s", "TRAZA - ASIGNACION EXITOSA");

}

t_valor_variable
prim_obtenerValorCompartida(t_nombre_compartida variable)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA ObtenerValorCompartida");
  return obtener_valor(variable); //devuelve el valor de la variable
}

t_valor_variable
prim_asignarValorCompartida(t_nombre_compartida variable,
    t_valor_variable valor)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA AsignarValorCompartida");
  grabar_valor(variable, valor);
  return valor; //devuelve el valor asignado
}

void
prim_llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA LlamarSinRetorno");
  //preservar el contexto de ejecucion actual y resetear estructuras. necesito un contexto vacio ahora
  int aux;

  char* mensaje=string_new();
  int ceros = (VAR_STACK - cantidadDigitos(programa->cursorStack));
  rellenarConCeros(ceros,&mensaje);
  string_append(&mensaje, string_itoa(programa->cursorStack));

  aux = programa->cursorStack + (VAR_STACK * programa->sizeContextoActual);

  if (setUMV(aux, 0, VAR_STACK, mensaje) == 1)
    {
      mensaje=string_new();
      ceros = (VAR_STACK - cantidadDigitos(programa->programCounter));
      rellenarConCeros(ceros,&mensaje);
      string_append(&mensaje, string_itoa(programa->programCounter));

      if (setUMV((aux + VAR_STACK), 0, VAR_STACK,mensaje) == 1)
        {
          programa->cursorStack = aux + (VAR_STACK * 2);
          log_trace(logger, "TRAZA - EL CURSOR STACK APUNTA A: %d", programa->cursorStack);
          programa->sizeContextoActual = 0;
          prim_irAlLabel(etiqueta);
          log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d",
              programa->sizeContextoActual);
          dictionary_clean(dicVariables); //limpio el dic de variables
        }
    }
}

void
prim_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA LlamarConRetorno");
  //preservar el contexto actual para retornar al mismo

  int aux;

  char* mensaje=string_new();
  int ceros = (VAR_STACK - cantidadDigitos(programa->cursorStack));
  rellenarConCeros(ceros,&mensaje);
  string_append(&mensaje, string_itoa(programa->cursorStack));

  aux = programa->cursorStack + (VAR_STACK * programa->sizeContextoActual);

  if (setUMV(aux, 0, VAR_STACK, mensaje) == 1)
    {
      mensaje=string_new();
      ceros = (VAR_STACK - cantidadDigitos(programa->programCounter));
      rellenarConCeros(ceros,&mensaje);
      string_append(&mensaje,string_itoa(programa->programCounter));

      if (setUMV((aux + VAR_STACK), 0, VAR_STACK,mensaje) == 1)
        {
          mensaje=string_new();
          ceros = (VAR_STACK - cantidadDigitos(donde_retornar));
          rellenarConCeros(ceros,&mensaje);
          string_append(&mensaje,string_itoa(donde_retornar));

          if (setUMV((aux + (VAR_STACK * 2)), 0, VAR_STACK,mensaje) == 1)
            {
              log_trace(logger, "TRAZA - LA DIRECCION POR DONDE RETORNAR ES: %d",donde_retornar);
              programa->cursorStack = aux + (VAR_STACK * 3);
              log_trace(logger, "TRAZA - EL CURSOR STACK APUNTA A: %d",programa->cursorStack);
              programa->sizeContextoActual = 0;
              prim_irAlLabel(etiqueta);
              log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d",programa->sizeContextoActual);
              dictionary_clean(dicVariables); //limpio el dic de variables
            }
        }
    }
}

void
prim_finalizar(void)
{
  //recuperar pc y contexto apilados en stack
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Finalizar");
  int aux = programa->cursorStack; //tengo que leer desde la base del stack anterior hacia abajo

  if (aux > programa->segmentoStack)
    {
      aux = aux - VAR_STACK;
      char *pedido = malloc(BUFFERSIZE * sizeof(char));
      pedido = getUMV(aux, 0, VAR_STACK);

      if (string_starts_with(pedido, bien))
        {
          programa->programCounter = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
          log_trace(logger, "TRAZA - EL PROGRAM COUNTER ES: %d", programa->programCounter);
          aux = aux - (VAR_STACK * 2);
          pedido = getUMV(aux, 0, VAR_STACK);
          if (string_starts_with(pedido, bien))
            {
              programa->cursorStack = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
              log_trace(logger, "TRAZA - EL CURSOR STACK ES: %d", programa->cursorStack);
              programa->sizeContextoActual = ((aux - (programa->cursorStack))/ VAR_STACK);
              log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d",programa->sizeContextoActual);
              if (programa->sizeContextoActual > 0)
                {
                  dictionary_clean(dicVariables); //limpio el dic de variables
                  RecuperarDicVariables();
                }
              else
                  finalizarProceso();
            }
          else
              mensajeAbortar(string_substring(pedido, 1, strlen(pedido) - 1));
        }
      else
          mensajeAbortar(string_substring(pedido, 1, strlen(pedido) - 1));

      free(pedido);
    }
  else
    finalizarProceso();

}

void
prim_retornar(t_valor_variable retorno)
{  //acÃ¡ tengo que volver a retorno
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Retornar");

  int retor;
  int aux = programa->cursorStack; //tengo que leer desde la base del stack anterior hacia abajo
  aux = aux - VAR_STACK;
  char *pedido = malloc(BUFFERSIZE * sizeof(char));
  pedido = getUMV(aux, 0, VAR_STACK);

  if (string_starts_with(pedido,bien))
    {
      retor = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
      log_trace(logger, "TRAZA - LA DIRECCION DONDE RETORNAR ES: %d", retor);

      if (setUMV(retor, 0, (VAR_STACK - DIG_NOM_VAR), string_itoa(retorno)) == 1)
        {
          aux = aux - (VAR_STACK * 2);
          pedido = getUMV(aux, 0, VAR_STACK);

          if (string_starts_with(pedido, bien))
            {
              programa->programCounter = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
              log_trace(logger, "TRAZA - EL PROGRAM COUNTER ES: %d",programa->programCounter);
              aux = aux - (VAR_STACK * 3);
              pedido = getUMV(aux, 0, VAR_STACK);

              if (string_starts_with(pedido, bien))
                {
                  programa->cursorStack = atoi(string_substring(pedido, 1, strlen(pedido) - 1));
                  log_trace(logger, "TRAZA - EL CURSOR DEL STACK ES: %d",programa->cursorStack);
                  programa->sizeContextoActual =((aux - (programa->cursorStack)) / VAR_STACK);
                  log_trace(logger, "TRAZA - EL SIZE DEL CONTEXTO ACTUAL ES: %d",programa->sizeContextoActual);

                  if (programa->sizeContextoActual > 0)
                    {
                      dictionary_clean(dicVariables); //limpio el dic de variables
                      RecuperarDicVariables();
                    }
                }
              else
                mensajeAbortar(string_substring(pedido, 1, strlen(pedido) - 1));
            }
          else
            mensajeAbortar(string_substring(pedido, 1, strlen(pedido) - 1));
        }
      else
        mensajeAbortar(string_substring(pedido, 1, strlen(pedido) - 1));
    }

  free(pedido);

}

void
prim_imprimir(t_valor_variable valor_mostrar)
{
  //acÃ¡ me conecto con el kernel y le paso el mensaje
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Imprimir");

  char *mensaje = string_itoa(IMPRIMIR);
  string_append(&mensaje, "VARIABLE ");
  string_append(&mensaje, variable_ref);
  string_append(&mensaje, ":");
  string_append(&mensaje, string_itoa(valor_mostrar)); //por el momento muestra valor
  string_append(&mensaje, separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL IMPRIMIR: %d EN EL PROGRAMA EN EJECUCION",valor_mostrar);
  Enviar(socketKERNEL, mensaje);

  variable_ref = string_new();
}

t_valor_variable
prim_dereferenciar(t_puntero direccion_variable)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Dereferenciar");
  t_valor_variable valor = 0;
  char* mensaje = malloc(1 * sizeof(char));

  mensaje = getUMV(direccion_variable, 1, DIG_VAL_VAR);
  if (string_starts_with(mensaje, bien)) //si comienza con 1 -> recibi un mensj valido
    {
    valor = atoi(string_substring(mensaje, 1, (strlen(mensaje) - 1)));
    log_trace(logger, "TRAZA - EL VALOR EXISTENTE EN ESA POSICION ES: %d", valor);
    }
  else
    mensajeAbortar("ERROR AL LEER DIRECCION MEMORIA");

  return valor;
}

void
prim_irAlLabel(t_nombre_etiqueta etiqueta)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA IrAlLabel");

  programa->programCounter = metadata_buscar_etiqueta(etiqueta, etiquetas,programa->sizeIndiceEtiquetas); //asigno la primer instruccion ejecutable de etiqueta al PC
  log_trace(logger, "TRAZA - EL VALOR DEL PROGRAM COUNTER ES: %d",programa->programCounter);
}

t_puntero
prim_obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA ObtenerPosicionVariable");
  t_puntero posicion = 0;

  variable_ref = string_new();

  char* var = string_new();
  sprintf(var,"%c",identificador_variable);

  //busco la posicion de la variable
  //las variables y las posiciones respecto al stack estan en el dicVariables
  if (dictionary_has_key(dicVariables, var) == true)
    {
      int* aux = dictionary_get(dicVariables, var);
      posicion = (t_puntero) aux;
      log_trace(logger, "TRAZA - ENCONTRE LA VARIABLE: %s, POSICION: %d", var, aux);
      variable_ref = var;
    }
  else
    {
    mensajeAbortar("LA VARIABLE NO EXISTE EN EL CONTEXTO DE EJECUCION");
    }

  free(var);
  return posicion; //devuelvo la posicion
}

t_puntero
prim_definirVariable(t_nombre_variable identificador_variable)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA DefinirVariable");
  // reserva espacio para la variable,
  //la registra en el stack
  t_puntero pos_var_stack;

  char* var = malloc(DIG_NOM_VAR);
  var[0] = identificador_variable;

  log_trace(logger, "TRAZA - LA VARIABLE QUE SE QUIERE DEFINIR ES: %s",string_substring(var, 0, 1));
  pos_var_stack = programa->sizeContextoActual * VAR_STACK;

  log_trace(logger, "TRAZA - LA POSICION DONDE SE QUIERE DEFINIR LA VARIABLE ES: %d",pos_var_stack);

  void algo (char* key, t_puntero puntero)
  {
    log_trace(logger, "key: %s, pos: %d",key,puntero);
  }
  dictionary_iterator(dicVariables,(void*)algo);

  if ((dictionary_has_key(dicVariables, string_substring(var, 0, 1))) == false)
    {
      if ((setUMV(programa->cursorStack, pos_var_stack, 1, string_substring(var, 0, 1))) > 0)
        {
          dictionary_put(dicVariables, string_substring(var, 0, 1),(void*) pos_var_stack); //la registro en el dicc de variables
          programa->sizeContextoActual++;
          log_trace(logger, "%s","TRAZA - SE DEFINIO CORRECTAMENTE LA VARIABLE");
        }
      else
        mensajeAbortar("NO SE PUDO INGRESAR LA VARIABLE EN EL STACK");
    }
  else
   mensajeAbortar("LA VARIABLE YA SE ENCUENTRA EN EL CONTEXTO ACTUAL");

  free(var);
  return pos_var_stack; //devuelvo la pos en el stack
}

void
prim_imprimirTexto(char* texto)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA ImprimirTexto");
  char *mensaje = string_itoa(IMPRIMIR);
  string_append(&mensaje, texto);
  string_append(&mensaje, separador);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL IMPRIMIR: %s EN EL PROGRAMA EN EJECUCION",texto);
  Enviar(socketKERNEL, mensaje);
}

void
prim_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA EntradaSalida");
  quantum = 0; //para que salga del ciclo
  io = 1; //seÃ±al de que paso por entrada y salida...ya le envio el pcb al kernel
  procesoTerminoQuantum(1, dispositivo, tiempo);
}

void
prim_wait(t_nombre_semaforo identificador_semaforo)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Wait");
  char* senial;
  char respuesta[BUFFERSIZE];
  char *mensaje = string_itoa(S_WAIT);

  //el mensaje que le mando es  PedidoSemaforo
  string_append(&mensaje, identificador_semaforo);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL EL SEMAFORO: %s", identificador_semaforo);
  Enviar(socketKERNEL, mensaje);
  Recibir(socketKERNEL, respuesta);
  senial = string_substring(respuesta, 0, 1);

  if (string_equals_ignore_case(senial, "A"))
   mensajeAbortar(string_substring(respuesta, 1, strlen(respuesta)));
  else
    {
      if (string_equals_ignore_case(senial, mal)) //senial==1 -> consegui el sem, senial==0 -> proceso bloqueado por sem
        {
          log_trace(logger, "%s","TRAZA - EL PROCESO QUEDO BLOQUEADO A LA ESPERA DEL SEMAFORO");
          down = 1;
          quantum = 0;
          tengoProg = 0;
          procesoTerminoQuantum(2, identificador_semaforo, 0);
        }
      else
        {
          if (!(string_equals_ignore_case(senial, bien)))
            kernelDesconectado();
          else
            log_trace(logger, "%s", "TRAZA - EL PROCESO OBTUVO EL SEMAFORO");
        }
    }
}

void
prim_signal(t_nombre_semaforo identificador_semaforo)
{
  log_trace(logger, "%s", "TRAZA - EJECUTO PRIMITIVA Signal");
  char respuesta[BUFFERSIZE];
  char *mensaje = string_itoa(S_SIGNAL);

  //el mensaje que le mando es  PedidoSemaforo
  string_append(&mensaje, identificador_semaforo);
  log_trace(logger, "TRAZA - SOLICITO AL KERNEL LIBERAR UNA INSTANCIA DEL SEMAFORO: %s",identificador_semaforo);
  Enviar(socketKERNEL, mensaje);
  Recibir(socketKERNEL, respuesta);
  if (string_equals_ignore_case(string_substring(respuesta, 0, 1), bien)) //si es -1 lo controla recibir
      log_trace(logger, "%s", "TRAZA - LA SOLICITUD INGRESO CORRECTAMENTE");
  else
    {
      if (string_equals_ignore_case(string_substring(respuesta, 0, 1), mal))
       mensajeAbortar("NO SE PUDO LIBERAR EL SEMAFORO SOLICITADO");
      else
        kernelDesconectado();
    }
}
