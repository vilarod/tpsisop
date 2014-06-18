/*
 ============================================================================
 Name        : CPU.c
 Author      : Romi
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

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

#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/CPU/src/config.cfg"
#define HandU 3
#define HandK 3
#define tCPUU 2
#define tCPUK 2
#define BUFFERSIZE 10000
#define TAM_INSTRUCCION 8
#define VAR_STACK 5

//Variables globales
int aux_conec_umv = 0;
int aux_conec_ker = 0;

int socketUMV = 0;
int socketKERNEL = 0;

int CONECTADO_UMV = 0;
int CONECTADO_KERNEL = 0;


t_dictionary* dicVariables;
t_dictionary* dicEtiquetas;
t_log* logger;

PCB* programa;
int quantum = 0;
int io = 0;
int up = 0;
int retardo = 0;
int g_ImprimirTrazaPorConsola = 1;


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
    Error("ERROR AL CONECTARME CON SOCKET: %d", socketConec);
  else{
    *Conec = 1;
    Traza("ME CONECTE CON SOCKET: %d", socketConec);}
}

void
Cerrar(int sRemoto)
{
  Traza("%s","TRAZA - SE CIERRA LA CONEXIÓN");
  close(sRemoto);
}


int
crearSocket(int socketConec)
{
  socketConec = socket(AF_INET, SOCK_STREAM, 0);
  if (socketConec == -1) //Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
    ErrorFatal("%s","ERROR - NO SE PUEDE CONECTAR EL SOCKET");
  return socketConec;
}

struct sockaddr_in
prepararDestino(struct sockaddr_in destino, int puerto, char* ip)
{
  struct hostent *he, *
  gethostbyname();
  he = gethostbyname(ip);
  Traza("TRAZA - PREPARO SOCKET DESTINO.PUERTO: %d IP: %s",puerto,ip);
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
  char *mensaje = "32";
  int aux = 0;
  Traza("%s", "TRAZA - VOY A ENVIAR HANDSHAKE");
  Enviar(sRemoto, mensaje);
  Recibir(sRemoto, respuesta);
  if (!(string_starts_with(respuesta, "1")))
    ErrorFatal("%s","ERROR - HANDSHAKE NO FUE EXITOSO");
  else aux=1;

  return aux;
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
    Traza("ENVIO DATOS. SOCKET %d, Buffer: %s", sRemoto, buffer);

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
    Traza("RECIBO DATOS. SOCKET %d, Buffer: %s", sRemoto, buffer);

  return bytecount;
}

int
seguirConectado()

{
  //consultare si he recibido la señal SIGUSR1,de ser asi, CHAU!
  return 1;
}

int
RecibirProceso()
{
  char* estructura = malloc(BUFFERSIZE * sizeof(char));
  char* sub = string_new();
  int i, aux;
  int indice = 1;
  int r=0;
  int inicio = 0;

  Traza("%s","TRAZA - ESTOY ESPERANDO RECIBIR UN QUANTUM + RETARDO + PCB");
  r = Recibir(socketKERNEL, estructura);

  if (r > 0)
    {
      if (string_starts_with(estructura, "1")) // ponele que dice que me esta enviando datos..
        {
          for (aux = 1; aux < 4; aux++)
            {
              for (i = 0; string_equals_ignore_case(sub, "-") == 0; i++)
                {
                  sub = string_substring(estructura, inicio, 1);
                  inicio++;
                }
              switch (aux)
                {
              case 1:
                {
                quantum = atoi(string_substring(estructura, indice, i));
                Traza("%s","TRAZA - EL QUANTUM QUE RECIBI ES: %d", quantum);
                }
                break;
              case 2:
                {
                retardo = atoi(string_substring(estructura, indice, i));
                Traza("%s","TRAZA - EL RETARDO QUE RECIBI ES: %d", retardo);
                }
                break;
              case 3:
                programa = desearilizar_PCB(estructura, indice);
                break;
                }
              sub = "";
              indice = inicio;
            }
        }
      else
        Error("%s","ERROR - NO RECIBI DATOS VÁLIDOS");
    }
  else
    ErrorFatal("%s","ERROR - EL SOCKET REMOTO SE HA DESCONECTADO");

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
Traza(const char* mensaje, ...)
{
  char* nuevo;
  va_list arguments;
  va_start(arguments, mensaje);
  nuevo = string_from_vformat(mensaje, arguments);

  log_trace(logger, "%s", nuevo);

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

  printf("El programa se cerrara. Presione ENTER para finalizar la ejecución.");
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
  Traza("%s","TRAZA - PREPARO PCB SERIALIZADO");

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

//serializar en cadena, a un mensaje le agrega algo mas y tamaño de algo mas
void
serCadena(char ** msj, char* agr)
{
  string_append(msj, string_itoa(strlen(agr)));
  string_append(msj, agr);
}




//DESERIALIZADO --------------------------------------------------------

void
deserializarDesplLong(char * msj, int despl, int longi)
{
  int tamanio1 = 0;
  int tamanio2 = 0;

  if (string_starts_with(msj, "1")) //si el mensaje es valido -> busca despl y longi
    {
      tamanio1 = atoi(string_substring(msj, 1, 1));
      despl = atoi(string_substring(msj, 2, tamanio1));
      tamanio2 = atoi(string_substring(msj, tamanio1 + 2, 1));
      longi = atoi(string_substring(msj, tamanio1 + 3, tamanio2));
    }
  else Error("%s","ERROR - EL MENSAJE NO PUEDE SER DESERIALIZADO");
}

PCB* desearilizar_PCB(char* estructura, int pos) {

        char* sub = string_new();
        PCB* est_prog;
        est_prog = (struct PCBs *) malloc(sizeof(PCB));
        int aux;
        int i;
        int cantguiones=0;
        int indice = pos;
        int inicio = pos;

        Traza("%s","TRAZA - DESERIALIZO EL PCB RECIBIDO");
        iniciarPCB(est_prog);

        for (aux = 1; aux < 10; aux++) {
                for (i = 0; string_equals_ignore_case(sub, "-") == 0; i++) {
                        sub = string_substring(estructura, inicio, 1);
                        inicio++;
                }
           if (string_equals_ignore_case(sub, "-"))
             cantguiones ++;

                switch (aux) {
                case 1:
                  {
                        est_prog->id = atoi(string_substring(estructura, indice, i));
                        Traza("TRAZA - EL ID DE PROCESO ES: %d",est_prog->id);
                        break;
                  }
                case 2:
                  {
                        est_prog->segmentoCodigo = atoi(
                                        string_substring(estructura, indice, i));
                        Traza("TRAZA - EL SEGMENTO DE CODIGO ES: %d",est_prog->segmentoCodigo);
                        break;
                  }
                case 3:
                  {
                        est_prog->segmentoStack = atoi(
                                        string_substring(estructura, indice, i));
                        Traza("TRAZA - EL SEGMENTO DE STACK ES: %d",est_prog->segmentoStack);
                        break;
                  }
                case 4:
                  {
                        est_prog->cursorStack = atoi(
                                        string_substring(estructura, indice, i));
                        Traza("TRAZA - EL CURSOR DE STACK ES: %d",est_prog->cursorStack);
                        break;
                  }
                case 5:
                  {
                        est_prog->indiceCodigo = atoi(
                                        string_substring(estructura, indice, i));
                        Traza("TRAZA - EL INDICE DE CODIGO ES: %d",est_prog->indiceCodigo);
                        break;
                  }
                case 6:
                  {
                        est_prog->indiceEtiquetas = atoi(
                                        string_substring(estructura, indice, i));
                        Traza("TRAZA - EL INDICE DE ETIQUETAS ES: %d",est_prog->indiceEtiquetas);
                        break;
                  }
                case 7:
                  {
                        est_prog->programCounter = atoi(
                                        string_substring(estructura, indice, i));
                        Traza("TRAZA - EL PROGRAM COUNTER ES: %d",est_prog->programCounter);
                        break;
                  }
                case 8:
                  {
                        est_prog->sizeContextoActual = atoi(
                                        string_substring(estructura, indice, i));
                        Traza("TRAZA - EL TAMAÑO DEL CONTEXTO ACTUAL ES: %d",est_prog->sizeContextoActual);
                        break;
                  }
                case 9:
                  {
                        est_prog->sizeIndiceEtiquetas = atoi(
                                        string_substring(estructura, indice, i));
                        Traza("TRAZA - EL TAMAÑO DEL INDICE DE ETIQUETAS ES: %d",est_prog->sizeIndiceEtiquetas);
                        break;
                  }
                }
                sub = "";
                indice = inicio;
        }
        pos = inicio;

        if (cantguiones==9)
          Traza("%s","TRAZA - RECIBI TODO EL PCB");
        else
          ErrorFatal("%s","ERROR - PCB INCORRECTO");

   return est_prog;
}



void
iniciarPCB(PCB* prog)
{
  Traza("%s","TRAZA - INICIALIZO UNA ESTRUCTURA PCB CON 0");
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

void AbortarProceso()
{
  //enviar "A" al kernel
}

//Mensajes frecuentes con la UMV ------------------------------------------------

char*
PedirSentencia()
{
  int despl = 0;
  int longi = 0;

  Traza("%s","TRAZA - SOLICITO SENTENCIA A LA UMV");
  char* instruccion = string_new();
  char* sentencia=string_new();
  instruccion = getUMV(programa->indiceCodigo, programa->programCounter * TAM_INSTRUCCION, TAM_INSTRUCCION);
  deserializarDesplLong(instruccion, despl, longi);

  if ((despl > (-1)) && (longi > 0))
    {
      instruccion = "";
      instruccion = getUMV(programa->segmentoCodigo, despl, longi);
    }
  else
    {
    Error("%s","ERROR - DESPLAZAMIENTO/OFFSET INVALIDOS");
    AbortarProceso();
    }
  if (string_starts_with(instruccion, "1")) //si comienza con 1 -> recibi un mensj valido
    sentencia=string_substring(instruccion, 1, (strlen(instruccion) - 1));
  else
    {
    Error("%s","ERROR - NO SE PUDO OBTENER LA INSTRUCCION");
    AbortarProceso();
    }

return sentencia;
}

char*
getUMV(int base, int dsp, int tam)
{
  char* instruccion = malloc(1 * sizeof(char));
  char* recibo = "";

  serCadena(&instruccion, string_itoa(base)); //base
  serCadena(&instruccion, string_itoa(dsp)); //desplazamiento
  serCadena(&instruccion, string_itoa(tam)); //longitud
  Traza("%s","TRAZA - SOLICITO DATOS A MEMORIA.BASE: %d DESPLAZAMIENTO: %d TAMAÑO: %d",base,dsp,tam);
  Enviar(socketUMV, instruccion);
  Recibir(socketUMV, recibo);
  if (!(string_starts_with(recibo, "1")))
    {
        Error("%s","ERROR - NO SE ENCONTRÓ VALOR EN ESA DIRECCION");
        Error("ERROR UMV: %s", string_substring(recibo,1,(strlen(recibo))-1));
        AbortarProceso();
        }

  free(instruccion);
  return recibo;
}

int
setUMV(int ptro, int dsp, int tam, char* valor)
{
  char* mensaje = "2";
  mensaje = malloc(1 * sizeof(char));
  char* recibo = "";

  serCadena(&mensaje, string_itoa(ptro));
  serCadena(&mensaje, string_itoa(dsp));
  serCadena(&mensaje, string_itoa(tam));
  serCadena(&mensaje, valor);
  Traza("%s","TRAZA - SOLICITO GRABAR EN MEMORIA.BASE: %d DESPLAZAMIENTO: %d TAMAÑO: %d VALOR: %s",ptro,dsp,tam,valor);
  Enviar(socketUMV, mensaje);
  Recibir(socketUMV, recibo);

  if (!(string_starts_with(recibo, "1")))
    {
        Error("%s","ERROR - NO SE PUDO GUARDAR VALOR EN ESA DIRECCION");
        Error("ERROR UMV: %s", string_substring(recibo,1,(strlen(recibo))-1));
        AbortarProceso();
    }
  free(mensaje);
  return 1;
}

void
CambioProcesoActivo()
{
  //aviso a la mem que voy a ejecutar tal programa
  Traza("TRAZA - INFORMO A UMV QUE MI PROCESO ACTIVO ES: %d", programa->id);

}

//Mensajes frecuentes al kernel ---------------------------------------------------------


void
AvisarDescAKernel()

{
  char* aviso = "7"; // kernel sabe que 7 es que me voy
  Traza("%s","TRAZA - AVISO AL KERNEL QUE LA CPU SE DESCONECTA");
  Enviar(socketKERNEL, aviso);
}

t_valor_variable
obtener_valor(t_nombre_compartida variable)
{
  t_valor_variable valor;
  char* pedido = "5"; //kernel sabe que 5 es obtener valor compartida
  char* recibo = "";
  pedido = malloc(1 * sizeof(char));

  string_append(&pedido, variable);
  Traza("TRAZA - SOLICITO VALOR DE LA VARIABLE COMPARTIDA: %s",variable);
  Enviar(socketKERNEL, pedido);  //envio PedidoVariable
  free(pedido);
  Recibir(socketKERNEL, recibo);  //recibo EstadoValor

  if (string_starts_with(recibo, "1")) //si comienza con 1 -> recibi un mensj valido
    {
    valor = atoi(string_substring(recibo, 1, (strlen(recibo) - 1)));
    Traza("TRAZA - EL VALOR DE LA VARIABLE ES: %d", valor);
    }
  else
    {
    Error("%s","ERROR - NO SE PUDO OBTENER EL VALOR DE LA VARIABLE COMPARTIDA");
    AbortarProceso();
    }

  return valor;
}

void
grabar_valor(t_nombre_compartida variable, t_valor_variable valor)
{
  char* pedido = "6"; // kernel sabe que 6 es grabar valor compartida
  pedido = malloc(1 * sizeof(char));
  char* msj=string_new();

  string_append(&pedido, variable);
  string_append(&pedido, string_itoa(valor));
  Traza("TRAZA - SOLICITO AL KERNEL ASIGNAR: %d A LA VARIABLE: %s",valor,variable);
  Enviar(socketKERNEL, pedido); //el mensaje que le mando es  PedidoVariableValor
  free(pedido);
  Recibir(socketKERNEL, msj);
  if (string_starts_with(msj, "1"))
    Traza("%s","TRAZA - KERNEL PROCESÓ OK EL PEDIDO");
  else
    {
    Error("%s","ERROR - KERNEL NO HA PODIDO PROCESAR EL PEDIDO");
    AbortarProceso();
    }
}

void
procesoTerminoQuantum(int que, char* donde, int cuanto)
{
  char* aviso = "4"; //  kernel sabe que 4 es fin de quantum;
  aviso = malloc(1 * sizeof(char));

  string_append(&aviso, serializar_PCB(programa));
  string_append(&aviso, string_itoa(que));
  string_append(&aviso, "-");
  string_append(&aviso, donde);
  string_append(&aviso, "-");
  string_append(&aviso, string_itoa(cuanto));
  Traza("TRAZA - INDICO AL KERNEL QUE EL PROCESO TERMINO EL QUANTUM CON MOTIVO : %d",que);
  Enviar(socketKERNEL, aviso);
  free(aviso);
}

//Enviar a parser --------------------------------------------------------------

void
parsearYejecutar(char* instr)
{
  Traza("TRAZA - LA SENTENCIA: %s SE ENVIARA AL PARSER", instr);
  analizadorLinea(instr, &funciones_p, &funciones_k);
}

void
esperarTiempoRetardo()
{
  Traza("TRAZA - TENGO UN TIEMPO DE ESPERA DE: %d MILISEGUNDOS", retardo);
  sleep(retardo);
}


//Manejo diccionarios ----------------------------------------------------------

void
limpiarEstructuras()
{
  Traza("%s","TRAZA - LIMPIO LOS DICCIONARIOS");
  dictionary_clean(dicVariables);
  dictionary_clean(dicEtiquetas);
}

void
destruirEstructuras()
{
  Traza("%s","TRAZA - DESTRUYO LOS DICCIONARIOS");
  dictionary_destroy(dicVariables);
  dictionary_destroy(dicEtiquetas);
}

void
RecuperarDicEtiquetas()
{
  Traza("%s", "TRAZA - VOY A RECUPERAR EL DICCIONARIO DE ETIQUETAS");
  /*
   t_puntero_instruccion primer_instr;
   char* ptr_etiquetas = "";
   t_size tam_etiquetas = 0;

   primer_instr = metadata_buscar_etiqueta(etiqueta, ptr_etiquetas,
   tam_etiquetas);*/

}

void
RecuperarDicVariables()
{
  //RECUPERAR ALGO
  //en sizeContextoActual tengo la cant de variables que debo leer
  //si es 0 -> programa nuevo
  //si es >0 -> cant de variables a leer desde seg stack en umv
  int i;
  int aux=0;
  char* variable = malloc(1 * sizeof(char));
  int* dato = 0;
  char* recibo;

  Traza("%s", "TRAZA - VOY A RECUPERAR EL DICCIONARIO DE VARIABLES");
  aux = programa->sizeContextoActual;
  Traza("%s", "TRAZA - CANTIDAD DE VARIABLES A RECUPERAR: %d",aux);

  if (aux > 0) //tengo variables a recuperar
    {
      for (i = 0; i < aux; i++) //voy de 0 a cantidad de variables en contexto actual
        {
          recibo = getUMV(aux, 0, 1);
          if (string_starts_with(recibo, "1"))
            {
              variable = string_substring(recibo, 1, strlen(recibo) - 1);
              *dato = aux + 1;
              dictionary_put(dicVariables, variable, dato);
              aux = aux + 5;
            }
          else
            {
            Error("%s", "ERROR - NO SE PUDO RECUPERAR LA TOTALIDAD DEL CONTEXTO");
            free(variable);
            AbortarProceso();
            }
        }
    }
  else Traza("%s", "TRAZA - ES UN PROGRAMA NUEVO, NO TENGO CONTEXTO QUE RECUPERAR");
  free(variable);
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

  int tengoProg = 0;
  char* sentencia = string_new();

  Traza("%s","TRAZA - INICIA LA CPU");
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
      Traza("%s", "TRAZA - UMV CONTESTO HANDSHAKE OK");
    }
  if (aux_conec_ker == saludar(HandK, tCPUK, socketKERNEL))
    {
      CONECTADO_KERNEL = 1;
      Traza("%s", "TRAZA - KERNEL CONTESTO HANDSHAKE OK");
    }

  //Creo los diccionarios
  dicVariables = dictionary_create();
  dicEtiquetas = dictionary_create();

  //voy a trabajar mientras este conectado tanto con kernel como umv
  while ((CONECTADO_KERNEL == 1) && (CONECTADO_UMV == 1))
    {
      Traza("%s", "TRAZA - ESTOY CONECTADO CON KERNEL Y UMV");
      Traza("CANTIDAD DE PROGRAMAS QUE TENGO: %d", tengoProg);

      while (tengoProg == 0) //me fijo si tengo un prog que ejecutar
        {
          tengoProg = RecibirProceso();
        }

      //Si salio del ciclo anterior es que ya tengo un programa
      CambioProcesoActivo();
      RecuperarDicEtiquetas();
      RecuperarDicVariables();

      while (quantum > 0) //mientras tengo quantum
        {
          programa->programCounter++; //Incremento el PC
          Traza("TRAZA - LA PROXIMA INSTRUCCION ES: %d", programa->programCounter);
          sentencia = PedirSentencia(); //le pido a la umv la sentencia a ejecutar
          parsearYejecutar(sentencia); //ejecuto sentencia
          esperarTiempoRetardo(); // espero X milisegundo para volver a ejecutar
          quantum--;
          Traza("TRAZA - EL QUANTUM QUE RESTA ES: %d", quantum);
        }

      limpiarEstructuras();
      if ((io == 0) && (up == 0))
        {
          procesoTerminoQuantum(0, "0", 0); //no necesita e/s ni wait
        }

      seguirConectado(); //aca controla si sigue conectada a kernel y umv
      CONECTADO_KERNEL = 0;
    }

  AvisarDescAKernel(); //avisar al kernel asi me saca de sus recursos
  destruirEstructuras();
  Cerrar(socketKERNEL);
  Cerrar(socketUMV);

  return EXIT_SUCCESS;
}

//Primitivas ---------------------------------------------------------------------

void
prim_asignar(t_puntero direccion_variable, t_valor_variable valor)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA ASIGNAR");
  int validar;
  validar = setUMV(direccion_variable, 1, 4, string_itoa(valor));
  if (validar == 1) //si es <=0 el set aborta el proceso
    Traza("%s", "TRAZA - ASIGNACIÓN EXITOSA");
}

t_valor_variable
prim_obtenerValorCompartida(t_nombre_compartida variable)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA ObtenerValorCompartida");
  return obtener_valor(variable); //devuelve el valor de la variable
}

t_valor_variable
prim_asignarValorCompartida(t_nombre_compartida variable,
    t_valor_variable valor)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA AsignarValorCompartida");
  grabar_valor(variable, valor);
  return valor; //devuelve el valor asignado
}

void
prim_llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA LlamarSinRetorno");
  //preservar el contexto de ejecucion actual y resetear estructuras. necesito un contexto vacio ahora

  dictionary_clean(dicVariables); //limpio el dic de variables

}

void
prim_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA LlamarConRetorno");
  //preservar el contexto actual para retornar al mismo

  dictionary_clean(dicVariables); //limpio el dic de variables
}

void
prim_finalizar(void)
{
  //recuperar pc y contexto apilados en stack
  Traza("%s","TRAZA - EJECUTO PRIMITIVA Finalizar");
  int aux = programa->cursorStack;
  limpiarEstructuras();
  programa->programCounter = atoi(getUMV(aux - VAR_STACK, 0, VAR_STACK));
  Traza("TRAZA - EL PROGRAM COUNTER ES: %d",programa->programCounter);
  programa->cursorStack = atoi(getUMV(aux - (VAR_STACK*2), 0, VAR_STACK));
  Traza("TRAZA - EL CURSOR STACK ES: %d",programa->cursorStack);
  programa->sizeContextoActual = (aux - programa->cursorStack) / VAR_STACK;
  Traza("TRAZA - EL TAMAÑO DEL CONTEXTO ACTUAL ES: %d",programa->sizeContextoActual);
  if (programa->sizeContextoActual > 0)
    RecuperarDicVariables();
  else
    {
      Traza("%s","TRAZA - EL PROGRAMA FINALIZO");
      char* pedido = "F"; // el kernel sabe que F es que finalizó el programa
      pedido = malloc(1 * sizeof(char));
      string_append(&pedido, serializar_PCB(programa));
      Enviar(socketKERNEL, pedido);
      quantum=0;
      free(pedido);
    }

}

void
prim_retornar(t_valor_variable retorno)
{  //acá tengo que volver a retorno
  Traza("%s","TRAZA - EJECUTO PRIMITIVA Retornar");
}

void
prim_imprimir(t_valor_variable valor_mostrar)
{
  //acá me conecto con el kernel y le paso el mensaje
  Traza("%s","TRAZA - EJECUTO PRIMITIVA Imprimir");
  char* pedido = "2"; // el kernel sabe que 1 es que finalizó el programa
  pedido = malloc(1 * sizeof(char));
  string_append(&pedido, string_itoa(valor_mostrar)); //por el momento muestra valor
  Traza("TRAZA - SOLICITO AL KERNEL IMPRIMIR: %d EN EL PROGRAMA EN EJECUCION", valor_mostrar);
  Enviar(socketKERNEL, pedido);
  free(pedido);
}

t_valor_variable
prim_dereferenciar(t_puntero direccion_variable)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA Dereferenciar");
  t_valor_variable valor = 0;
  char* mensaje = malloc(1 * sizeof(char));

  mensaje = getUMV((direccion_variable + 1), 0, 4);
  if (string_starts_with(mensaje, "1")) //si comienza con 1 -> recibi un mensj valido
    valor = atoi(string_substring(mensaje, 1, (strlen(mensaje) - 1)));
//si el msj no es valido, el get aborta el proceso
  Traza("TRAZA - EL VALOR EXISTENTE EN ESA POSICION ES: %d", valor);

  return valor;
}

void
prim_irAlLabel(t_nombre_etiqueta etiqueta)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA IrAlLabel");
  //int posicion;

  //posicion=dictionary_get(&dicEtiquetas,etiqueta);
  //aca tengo que asignarle la posicion al cursorstack?
}

t_puntero
prim_obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA ObtenerPosicionVariable");
  t_puntero posicion = 0;
  //busco la posicion de la variable
  //las variables y las posiciones respecto al stack estan en el dicVariables
  //posicion=dictionary_get(&dicVariables,identificador_variable);
  return posicion; //devuelvo la posicion
}

t_puntero
prim_definirVariable(t_nombre_variable identificador_variable)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA DefinirVariable");
  // reserva espacio para la variable,
  //la registra en el stack
  t_puntero pos_var_stack = programa->cursorStack
      + (programa->sizeContextoActual * 5);
  //dictionary_put(&dicVariables,identificador_variable,pos_var_stack);//la registro en el dicc de variables
  //setUMV();
  programa->sizeContextoActual++;

  return pos_var_stack; //devuelvo la pos en el stack
}

void
prim_imprimirTexto(char* texto)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA ImprimirTexto");
  char* pedido = "2";
  pedido = malloc(1 * sizeof(char));
  string_append(&pedido, texto);
  Traza("TRAZA - SOLICITO AL KERNEL IMPRIMIR: %s EN EL PROGRAMA EN EJECUCION", texto);
  Enviar(socketKERNEL, pedido);
  free(pedido);
}

void
prim_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA EntradaSalida");
  quantum = 0; //para que salga del ciclo
  io = 1; //señal de que paso por entrada y salida...ya le envio el pcb al kernel
  procesoTerminoQuantum(1, dispositivo, tiempo);
}

void
prim_wait(t_nombre_semaforo identificador_semaforo)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA Wait");
  int senial = 0;
  char* msj = "";
  char* pedido = "8"; //kernel sabe que 8 es wait
  pedido = malloc(1 * sizeof(char));

  //el mensaje que le mando es  PedidoSemaforo
  string_append(&pedido, identificador_semaforo);
  Traza("TRAZA - SOLICITO AL KERNEL EL SEMAFORO: %s", identificador_semaforo);
  Enviar(socketKERNEL, pedido);
  Recibir(socketKERNEL, msj);
  senial = atoi(string_substring(msj, 1, 1));

  if (senial == 0) //senial==1 -> consegui el sem, senial==0 -> proceso bloqueado por sem
    {
      Traza("%s","TRAZA - EL PROCESO QUEDÓ BLOQUEADO A LA ESPERA DEL SEMAFORO");
      up = 1;
      quantum = 0;
      procesoTerminoQuantum(2, identificador_semaforo, 0);
    }
  else
    Traza("%s","TRAZA - EL PROCESO OBTUVO EL SEMAFORO");
  free(pedido);
}

void
prim_signal(t_nombre_semaforo identificador_semaforo)
{
  Traza("%s","TRAZA - EJECUTO PRIMITIVA Signal");
  char* pedido = "9"; //kernel sabe que 9 es signal
  pedido = malloc(1 * sizeof(char));
  char* msj = "";
  //el mensaje que le mando es  PedidoSemaforo
  string_append(&pedido, identificador_semaforo);
  Traza("TRAZA - SOLICITO AL KERNEL LIBERAR UNA INSTANCIA DEL SEMAFORO: %s",identificador_semaforo);
  Enviar(socketKERNEL, pedido);
  Recibir(socketKERNEL, msj);
  if (string_equals_ignore_case(string_substring(msj,0,1), "1")) //si es -1 lo controla recibir
    {
      Traza("%s","TRAZA - LA SOLICITUD INGRESO CORRECTAMENTE");
    }
  else
    {
      Error("%s","NO SE PUDO LIBERAR EL SEMAFORO SOLICITADO");
      AbortarProceso();
    }
  free(pedido);
}
