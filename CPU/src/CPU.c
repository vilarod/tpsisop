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

//Ruta del config
#define PATH_CONFIG "/home/utnso/tp-2014-1c-garras/CPU/src/config.cfg"
#define HandU 3
#define HandK 1
#define tCPUU 2
#define tCPUK 1

#define BUFFERSIZE 1024
int socketUMV = 0;
int socketKERNEL = 0;

int CONECTADO_UMV = 0;
int CONECTADO_KERNEL = 0;


t_dictionary* dicVariables;
t_dictionary* dicEtiquetas;


PCB programa;
int quantum=0;
int io=0;
int up=0;
int retardo=0;

//Llamado a las funciones primitivas

AnSISOP_funciones funciones_p =
  { .AnSISOP_definirVariable = prim_definirVariable,
      .AnSISOP_obtenerPosicionVariable = prim_obtenerPosicionVariable,
      .AnSISOP_dereferenciar = prim_dereferenciar,
      .AnSISOP_asignar = prim_asignar,
      .AnSISOP_obtenerValorCompartida = prim_obtenerValorCompartida,
      .AnSISOP_asignarValorCompartida =
          prim_asignarValorCompartida,
      .AnSISOP_irAlLabel = prim_irAlLabel,
      .AnSISOP_llamarSinRetorno = prim_llamarSinRetorno,
      .AnSISOP_llamarConRetorno = prim_llamarConRetorno, .AnSISOP_finalizar =
          prim_finalizar, .AnSISOP_retornar = prim_retornar, .AnSISOP_imprimir =
          prim_imprimir, .AnSISOP_imprimirTexto = prim_imprimirTexto,
      .AnSISOP_entradaSalida = prim_entradaSalida, };

AnSISOP_kernel funciones_k =
  { .AnSISOP_signal = prim_signal, .AnSISOP_wait = prim_wait, };


void
ConexionConSocket(int Conec, int socketConec, struct sockaddr_in destino)
{
  if ((connect(socketConec, (struct sockaddr*) &destino,
      sizeof(struct sockaddr))) == -1) //Controlo si puedo conectarme
    perror("No me puedo conectar con servidor!");
  else
    Conec = 1;
}

//Para enviar datos

int
Enviar(int sRemoto, char * buffer)

{
  int cantBytes;
  cantBytes = send(sRemoto, buffer, strlen(buffer), 0);
  if (cantBytes == -1)
    perror("No lo puedo enviar todo junto!");
  return cantBytes;
}

//Para recibir datos

int
Recibir(int sRemoto, char * buffer)
{
  int numbytes;
  char bufferAux[BUFFERSIZE];
  buffer = realloc(buffer, 1 * sizeof(char)); //--> el buffer ocupa 0 lugares (todavia no sabemos que tamaño tendra)
  memset(buffer, 0, 1* sizeof(char));
  memset(bufferAux, 0, BUFFERSIZE* sizeof(char));
  numbytes = recv(sRemoto, bufferAux, BUFFERSIZE, 0);
  buffer = realloc(buffer, numbytes * sizeof(char));
  sprintf(buffer, "%s%s", (char*) buffer, bufferAux);
  return numbytes;
}

//Cerrar conexión

void
Cerrar(int sRemoto)
{
  close(sRemoto);
}

//Para leer desde el archivo de configuracion

int
ObtenerPuerto(char* que)
{
  t_config* config = config_create(PATH_CONFIG);
  return config_get_int_value(config, que);
}

char*
ObtenerIP(char* que)
{
  t_config* config = config_create(PATH_CONFIG);
  return config_get_string_value(config, que);
}

//serializar pcb

char*
serializar_PCB(PCB prog)
{
  char* cadena=string_new();

  string_append(&cadena, string_itoa(prog.id));
  string_append(&cadena, "-");
  string_append(&cadena, string_itoa(prog.segmentoCodigo));
  string_append(&cadena, "-");
  string_append(&cadena, string_itoa(prog.segmentoStack));
  string_append(&cadena, "-");
  string_append(&cadena, string_itoa(prog.cursorStack));
  string_append(&cadena, "-");
  string_append(&cadena, string_itoa(prog.indiceCodigo));
  string_append(&cadena, "-");
  string_append(&cadena, string_itoa(prog.indiceEtiquetas));
  string_append(&cadena, "-");
  string_append(&cadena, string_itoa(prog.programCounter));
  string_append(&cadena, "-");
  string_append(&cadena, string_itoa(prog.sizeContextoActual));
  string_append(&cadena, "-");
  string_append(&cadena, string_itoa(prog.sizeIndiceEtiquetas));
  string_append(&cadena, "-");
  return cadena;

}

void deserializarDesplLong(char * msj,int despl, int longi)
{

  int tamanio1=0;
  int tamanio2=0;

  if (string_starts_with(msj,"1")) //si el mensaje es valido -> busca despl y longi
      {
      tamanio1= atoi(string_substring(msj,1,1));
      despl=atoi(string_substring(msj,2, tamanio1));
      tamanio2= atoi(string_substring(msj,tamanio1 + 2,1));
      longi=atoi(string_substring(msj,tamanio1 + 3, tamanio2));
      }
}

void iniciarPCB(PCB prog)
{
  prog.id = 0;
  prog.segmentoCodigo = 0;
  prog.segmentoStack = 0;
  prog.cursorStack = 0;
  prog.indiceCodigo = 0;
  prog.indiceEtiquetas = 0;
  prog.programCounter = 0;
  prog.sizeContextoActual = 0;
  prog.sizeIndiceEtiquetas = 0;
}

PCB
desearilizar_PCB(char* estructura, int pos)
{

  char* sub=string_new();;

  PCB est_prog;
  int aux;
  int i;
  int indice = pos;
  int inicio = 0;

  iniciarPCB(est_prog);

  for (aux = 1; aux < 11; aux++)
    {
      for (i = 0; string_equals_ignore_case(sub, "-") == 0; i++)
        {
          sub = string_substring(estructura, inicio, 1);
          inicio++;
        }
      switch (aux)
        {
      case 1:
        est_prog.id = atoi(string_substring(estructura, indice, i));
        break;
      case 2:
        est_prog.segmentoCodigo = atoi(string_substring(estructura, indice, i));
        break;
      case 3:
        est_prog.segmentoStack = atoi(string_substring(estructura, indice, i));
        break;
      case 4:
        est_prog.cursorStack = atoi(string_substring(estructura, indice, i));
        break;
      case 5:
        est_prog.indiceCodigo = atoi(string_substring(estructura, indice, i));
        break;
      case 6:
        est_prog.indiceEtiquetas = atoi(string_substring(estructura, indice, i));
        break;
      case 7:
        est_prog.programCounter = atoi(string_substring(estructura, indice, i));
        break;
      case 8:
        est_prog.sizeContextoActual = atoi(string_substring(estructura, indice, i));
        break;
      case 9:
        est_prog.sizeIndiceEtiquetas = atoi(string_substring(estructura, indice, i));
        break;
        }
      sub = "";
      indice = pos + inicio;
    }
  return est_prog;
}

int
RecibirProceso()
{
  char* estructura = string_new();
  char* sub= string_new();
  int i,aux;
  int indice = 1;
  int inicio = 0;
  int r = Recibir(socketUMV, estructura);
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
                  quantum = atoi(string_substring(estructura, indice, i));
                  break;
                case 2:
                  retardo = atoi(string_substring(estructura, indice, i));
                  break;
                case 3:
                  programa = desearilizar_PCB(estructura, indice);
                  break;
                  }
          sub = "";
          indice=inicio;
        }
        }
      else
        puts("No recibi datos validos");
    }
  else
    perror("No recibi datos validos");
  return r; //devuelve 0 si no tengo, -1 si fue error, >0 si recibi
}

char* PedirSentencia()
{
  //aca hare algo para enviarle el pedido a la umv y recibir lo solicitado
  int despl=0;
  int longi=0;

  char* instruccion = malloc(1 * sizeof(char));
  instruccion=getUMV(programa.indiceCodigo,programa.programCounter*8,8);
  deserializarDesplLong(instruccion,despl,longi); //tengo que saber que me mando

  if ((despl !=0) && (longi !=0)) //si son igual 0->no recibi nada :S
    {
      instruccion="";
      instruccion=getUMV(programa.segmentoCodigo,despl,longi);
    } else instruccion="0"; // si no recibi nada -> el mensaje no comienza con 1
  if (string_starts_with(instruccion,"1")) //si comienza con 1 -> recibi un mensj valido
    return string_substring(instruccion,1,(strlen(instruccion)-1));
     else return "ERROR"; //sino, no sirve lo que recibi...tengo que ver lib errores!!!!

}

//serializar en cadena, a un mensaje le agrega algo mas y tamaño de algo mas
void
serCadena(char ** msj, char* agr)
{
  string_append(msj, string_itoa(strlen(agr)));
  string_append(msj, agr);
}

//primitivas al kernel

t_valor_variable
obtener_valor(t_nombre_compartida variable)
{
  t_valor_variable valor;
  char* pedido = "5"; //kernel sabe que 5 es obtener valor compartida
  char* recibo="";
  pedido = malloc(1 * sizeof(char));

  string_append(&pedido, variable);
  Enviar(socketKERNEL, pedido);  //envio PedidoVariable
  Recibir(socketKERNEL, recibo);  //recibo EstadoValor

  if (string_starts_with(recibo,"1")) //si comienza con 1 -> recibi un mensj valido
    valor= atoi(string_substring(recibo,1,(strlen(recibo)-1)));
   else printf("ERROR"); //aca en verdad tendria que devolver un valor por default a convenir

  free(pedido);
  return valor;
}

void
grabar_valor(t_nombre_compartida variable,t_valor_variable valor)
{
  char* pedido = "6"; // kernel sabe que 6 es grabar valor compartida
  pedido = malloc(1 * sizeof(char));

  string_append(&pedido, variable);
  string_append(&pedido, string_itoa(valor));
  Enviar(socketKERNEL, pedido); //el mensaje que le mando es  PedidoVariableValor
  free(pedido);
}

void
procesoTerminoQuantum(int que, char* donde, int cuanto)
{
  char* aviso="4"; //  kernel sabe que 4 es fin de quantum;
  aviso = malloc(1 * sizeof(char));

  string_append(&aviso, serializar_PCB(programa));
  string_append(&aviso, string_itoa(que));
  string_append(&aviso, "-");
  string_append(&aviso, donde);
  string_append(&aviso, "-");
  string_append(&aviso, string_itoa(cuanto));
  Enviar(socketKERNEL,aviso);
  free(aviso);
}

void
parsearYejecutar(char* instr)
{
  analizadorLinea(instr, &funciones_p, &funciones_k);
}

void
salvarContextoProg()
{
  //necesito actualizar los segmentos del programa en la UMV
  //actualizar el PC en el PCB
}

void
limpiarEstructuras()
{
  //limpio dicVariables y dicEtiquetas
  dictionary_clean(dicVariables);
  dictionary_clean(dicEtiquetas);
}

void
destruirEstructuras()
{
  //destruyo dicVariables y dicEtiquetas
  dictionary_destroy(dicVariables);
  dictionary_destroy(dicEtiquetas);
}

int
seguirConectado()

{
  //consultare si he recibido la señal SIGUSR1,de ser asi, CHAU!
  return 1;
}

void
AvisarDescAKernel()

{
  char* aviso="7"; // kernel sabe que 7 es que me voy
  Enviar(socketKERNEL,aviso);
}

int
crearSocket(int socketConec)
{
  socketConec = socket(AF_INET, SOCK_STREAM, 0);
  if (socketConec == -1)  //Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
    perror("Este socket tiene errores!");
  return socketConec;
}

struct sockaddr_in
prepararDestino(struct sockaddr_in destino, int puerto, char* ip)
{
  struct hostent *he, *gethostbyname();
  he = gethostbyname(ip);
  //Con esto me quiero conectar con UMV o KERNEL
  destino.sin_family = AF_INET;
  destino.sin_port = htons(puerto);
  bcopy(he->h_addr, &(destino.sin_addr.s_addr),he->h_length);
  memset(&(destino.sin_zero), '\0', 8);
  return destino;
}

void
RecuperarDicEtiquetas()
{
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
int i,aux;
char* variable=malloc(1 * sizeof(char));
int* dato=0;
char* recibo;

aux=programa.sizeContextoActual;

  if (aux > 0) //tengo variables a recuperar
    {
      for (i=0; i < aux; i ++) //voy de 0 a cantidad de variables en contexto actual
        {
         recibo=getUMV(aux,0,1);
         if (string_starts_with(recibo,"1"))
           {
          variable= string_substring(recibo,1, strlen(recibo) -1);
          *dato=aux + 1;
          dictionary_put(dicVariables,variable,dato);
          aux=aux + 5;
           }
         else printf("ERROR");
        }
    } //si no tengo variables que recuperar, no hago nada
free(variable);
}

int saludar(int sld,int tipo, int sRemoto)
{
  int aux=0;
  char* recibo=string_new();
  char* msj=string_new();
  msj=string_itoa(sld);

  string_append(&msj, string_itoa(tipo));
  Enviar(sRemoto,msj);
  Recibir(sRemoto,recibo);
  if (string_starts_with(recibo,"1"))
    aux= 1;
    else aux=0;
  printf("saludo");
  return aux;
}



char* getUMV(int base, int dsp, int tam)
{
char* instruccion= malloc(1*sizeof(char));
char* recibo="";

serCadena(&instruccion,string_itoa(base)); //base
serCadena(&instruccion, string_itoa(dsp)); //desplazamiento
serCadena(&instruccion, string_itoa(tam)); //longitud
Enviar(socketUMV,instruccion);
Recibir(socketUMV,recibo);
free(instruccion);
return recibo;
}


int setUMV(int ptro, int dsp, int tam, char* valor)
{
 int resultado=0;
 char* mensaje="2";
 mensaje= malloc(1 * sizeof(char));
 char* recibo= "";

   serCadena(&mensaje,string_itoa(ptro)); //concateno con la direccion a donde apunta aux
   serCadena(&mensaje,string_itoa(dsp));//no tengo desplazamiento
   serCadena(&mensaje,string_itoa(tam));//uso un tamaño de 5 bytes
   serCadena(&mensaje,valor); //que guarde la direccion del cursor de stack actual
   Enviar(socketUMV,mensaje);
   Recibir(socketUMV,recibo);

   if (string_starts_with(recibo,"1"))
     resultado= 1;

   free(mensaje);
  return resultado;
}

void esperarTiempoRetardo()
{
  sleep(retardo);
}

void CambioProcesoActivo()
{
 //aviso a la mem que voy a ejecutar tal programa
}

int
main(void)

{
  //Llegué y quiero leer los datos de conexión
  //donde esta el kernel?donde esta la umv?
  int aux_conec_umv=0;
  int aux_conec_ker=0;
  int UMV_PUERTO = ObtenerPuerto("PUERTO_UMV");
  int KERNEL_PUERTO = ObtenerPuerto("PUERTO_KERNEL");
  char* UMV_IP = ObtenerIP("IP_UMV");
  char* KERNEL_IP = ObtenerIP("IP_KERNEL");

  socketUMV = crearSocket(socketUMV);
  socketKERNEL = crearSocket(socketKERNEL);

  struct sockaddr_in dest_UMV = prepararDestino(dest_UMV, UMV_PUERTO, UMV_IP);
  struct sockaddr_in dest_KERNEL = prepararDestino(dest_KERNEL, KERNEL_PUERTO,
      KERNEL_IP);

  //Ahora que se donde estan, me quiero conectar con los dos
  ConexionConSocket(aux_conec_umv, socketUMV, dest_UMV);
  ConexionConSocket(aux_conec_ker, socketKERNEL, dest_KERNEL);

  //Control programa
  int tengoProg = 0;
  char* sentencia="";

  if (aux_conec_umv == saludar(HandU,tCPUU,socketUMV))
    CONECTADO_UMV= 1;
  else CONECTADO_UMV= 0;

  if (aux_conec_ker == saludar(HandK,tCPUK,socketKERNEL))
    CONECTADO_KERNEL=1 ;
  else CONECTADO_KERNEL=0 ;

  dicVariables=dictionary_create();
  dicEtiquetas=dictionary_create();

  //voy a trabajar mientras este conectado tanto con kernel como umv
  while ((CONECTADO_KERNEL == 1) && (CONECTADO_UMV == 1))
    {
      //Mientras estoy conectado al Kernel y no tengo programa que
      //ejecutar...solo tengo que esperar a recibir uno
      while (tengoProg < 1) //me fijo si tengo un prog que ejecutar
        {
          tengoProg = RecibirProceso();
        }
      //Si salio del ciclo anterior es que ya tengo un programa
      RecuperarDicEtiquetas();
      RecuperarDicVariables();
      CambioProcesoActivo();
      while (quantum > 0) //mientras tengo quantum
        {
          programa.programCounter++; //Incremento el PC
          sentencia=PedirSentencia(); //le pido a la umv la sentencia a ejecutar
          parsearYejecutar(sentencia); //ejecuto sentencia
          esperarTiempoRetardo(); // espero X milisegundo para volver a ejecutar
          quantum--;
        }
      //una vez que el proceso termino el quantum
      //quiero salvar el contexto y limpiar estructuras auxiliares
      salvarContextoProg();
      limpiarEstructuras();
      if ((io==0)&&(up==0))
        {
          procesoTerminoQuantum(0,"0",0);//no necesita e/s ni wait
        }

      seguirConectado(); //aca controla si sigue conectada a kernel y umv
      }

  AvisarDescAKernel(); //avisar al kernel asi me saca de sus recursos
  destruirEstructuras();
  Cerrar(socketKERNEL);
  Cerrar(socketUMV);

  return EXIT_SUCCESS;
}


// Primitivas

void
prim_asignar(t_puntero direccion_variable, t_valor_variable valor)
{
int validar;
validar=setUMV(direccion_variable,1,4,string_itoa(valor));
if (validar==0)
 printf("ERROR");

}

t_valor_variable
prim_obtenerValorCompartida(t_nombre_compartida variable)
{
  return obtener_valor(variable); //devuelve el valor de la variable
}

t_valor_variable
prim_asignarValorCompartida(t_nombre_compartida variable,
    t_valor_variable valor)
{
  grabar_valor(variable,valor);
  return valor; //devuelve el valor asignado
}

void
prim_llamarSinRetorno(t_nombre_etiqueta etiqueta)
{

  //preservar el contexto de ejecucion actual y resetear estructuras. necesito un contexto vacio ahora

 dictionary_clean(dicVariables);//limpio el dic de variables

}

void
prim_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{

  //preservar el contexto actual para retornar al mismo

  dictionary_clean(dicVariables);//limpio el dic de variables
}

void
prim_finalizar(void)
{
  //recuperar pc y contexto apilados en stack

int aux=programa.cursorStack;
limpiarEstructuras();
programa.programCounter=atoi(getUMV(aux-5, 0,5));
programa.cursorStack=atoi(getUMV(aux-10, 0,5));
programa.sizeContextoActual=(aux-programa.cursorStack)/5;
if (programa.sizeContextoActual > 0)
RecuperarDicVariables();
 else {
     char* pedido="1"; // el kernel sabe que 1 es que finalizó el programa
     pedido=malloc(1*sizeof(char));
     string_append(&pedido,serializar_PCB(programa));
     Enviar(socketKERNEL,pedido);
     free(pedido);
 }

}

void
prim_retornar(t_valor_variable retorno)
{  //acá tengo que volver a retorno

}

void
prim_imprimir(t_valor_variable valor_mostrar)
{
  //acá me conecto con el kernel y le paso el mensaje
  char* pedido="2"; // el kernel sabe que 1 es que finalizó el programa
  pedido=malloc(1*sizeof(char));
  string_append(&pedido, string_itoa(valor_mostrar)); //por el momento muestra valor
  Enviar(socketKERNEL,pedido);
  free(pedido);
}

t_valor_variable
prim_dereferenciar(t_puntero direccion_variable)
{
  t_valor_variable valor=0;
 char* mensaje= malloc(1 * sizeof(char));
 
  mensaje=getUMV((direccion_variable + 1), 0 , 4);
  if (string_starts_with(mensaje,"1"))//si comienza con 1 -> recibi un mensj valido
  valor= atoi(string_substring(mensaje,1,(strlen(mensaje)-1)));
  else
    printf("ERROR");; //aca en verdad tendria que devolver un valor por default a convenir-o mensaje de error

  
  return valor;
}

void
prim_irAlLabel(t_nombre_etiqueta etiqueta)
{
  //int posicion;

  //posicion=dictionary_get(&dicEtiquetas,etiqueta);
  //aca tengo que asignarle la posicion al cursorstack?

  
}

t_puntero
prim_obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
  t_puntero posicion=0;
  //busco la posicion de la variable
  //las variables y las posiciones respecto al stack estan en el dicVariables
 //posicion=dictionary_get(&dicVariables,identificador_variable);
  return posicion; //devuelvo la posicion
}

t_puntero
prim_definirVariable(t_nombre_variable identificador_variable)
{
  // reserva espacio para la variable,
    //la registra en el stack
  t_puntero pos_var_stack= programa.cursorStack + (programa.sizeContextoActual*5);
  //dictionary_put(&dicVariables,identificador_variable,pos_var_stack);//la registro en el dicc de variables
  //setUMV();
  programa.sizeContextoActual ++;

  return pos_var_stack; //devuelvo la pos en el stack
}

void
prim_imprimirTexto(char* texto)
{
  //acá me conecto con el kernel y le paso el mensaje
   char* pedido="2"; // el kernel sabe que 1 es que finalizó el programa
   pedido=malloc(1*sizeof(char));
   string_append(&pedido, texto);
   Enviar(socketKERNEL,pedido);
   free(pedido);
}

void
prim_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
  quantum=0; //para que salga del ciclo
  io=1; //señal de que paso por entrada y salida...ya le envio el pcb al kernel
  procesoTerminoQuantum(1,dispositivo,tiempo);
}

void
prim_wait(t_nombre_semaforo identificador_semaforo)
{
  int senial=0;
  char* msj="";
  char* pedido = "8"; //kernel sabe que 8 es wait
  pedido = malloc(1 * sizeof(char));

    //el mensaje que le mando es  PedidoSemaforo
    string_append(&pedido, identificador_semaforo);
    Enviar(socketKERNEL,pedido);
    Recibir(socketKERNEL,msj);
    senial=atoi(string_substring(msj,1,1));

    if (senial==0) //senial==1 -> consegui el sem, senial==0 -> proceso bloqueado por sem
      {
        up=1;
        quantum=0;
        procesoTerminoQuantum(2,identificador_semaforo,0);
      }
  free(pedido);
}

void
prim_signal(t_nombre_semaforo identificador_semaforo)
{
  char* pedido = "9"; //kernel sabe que 9 es signal
  pedido = malloc(1 * sizeof(char));
  char* msj="";
    //el mensaje que le mando es  PedidoSemaforo
    string_append(&pedido, identificador_semaforo);
    Enviar(socketKERNEL,pedido);
    if (Recibir(socketKERNEL,msj)<1)
      printf("ERROR");
  free(pedido);
}
