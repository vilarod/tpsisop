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

//hasta ver que hago con las primitivas..las uso globales:
int socketUMV = 0;
int socketKERNEL = 0;



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
ConexionConSocket(int * Conec, int socketConec, struct sockaddr_in destino)
{
  //Controlo si puedo conectarme
  if ((connect(socketConec, (struct sockaddr*) &destino,
      sizeof(struct sockaddr))) == -1)
    perror("No me puedo conectar con servidor!");
  else
    *Conec = 1;

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

  numbytes = recv(sRemoto, buffer, 99, 0);
  buffer[numbytes] = '\0';

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

  char* cadena;
  cadena = malloc(1 * sizeof(char));

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

PCB
desearilizar_PCB(char* estructura, int pos)
{

  char* sub;
  sub = malloc(1 * sizeof(char));

  PCB est_prog;
  int aux = 0;
  int i, h = 0;
  int indice = 0;
  int inicio = 0;

  sub = "";
  est_prog.id = 0;
  est_prog.segmentoCodigo = 0;
  est_prog.segmentoStack = 0;
  est_prog.cursorStack = 0;
  est_prog.indiceCodigo = 0;
  est_prog.indiceEtiquetas = 0;
  est_prog.programCounter = 0;
  est_prog.sizeContextoActual = 0;
  est_prog.sizeIndiceEtiquetas = 0;

  indice = pos;

  for (aux = 1; aux < 11; aux++)
    {

      for (i = 0; string_equals_ignore_case(sub, "-") == 0; i++)
        {

          sub = string_substring(estructura, inicio, 1);
          inicio++;
          h++;
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
      h = 0;
    }

  return est_prog;
}

int
RecibirProceso(PCB prog, int quantum, int sRemoto)
{
  char* estructura = "";
  int r = Recibir(sRemoto, estructura);
  if (r > 0)
    {
      if (string_starts_with(estructura, "1")) // ponele que dice que me esta enviando datos..
        {
          quantum = atoi(string_substring(estructura,1,1));
          prog = desearilizar_PCB(estructura, 2);
        }
      else
        puts("No recibi datos validos");
    }
  else
    perror("No recibi datos validos");

  return r; //devuelve 0 si no tengo, -1 si fue error, >0 si recibi
}

char* PedirSentencia(int indiceCodigo, int segCodigo, int progCounter, int sRemoto)
{
  //aca hare algo para enviarle el pedido a la umv y recibir lo solicitado

  char* pedido = "1"; //lo inicializo en 1=getbyte
  char* mensaje;
  int despl=0;
  int longi=0;

  char* instruccion;
  instruccion = malloc(1 * sizeof(char));

  mensaje = malloc(1 * sizeof(char));

  string_append(&instruccion, pedido);
  serCadena(&instruccion,string_itoa(indiceCodigo)); //base
  serCadena(&instruccion, string_itoa(progCounter*8)); //desplazamiento
  serCadena(&instruccion, string_itoa(8)); //longitud

  Enviar(sRemoto, instruccion); //envio el pedido a la umv
  Recibir(sRemoto, mensaje);

  deserializarDesplLong(mensaje,despl,longi); //tengo que saber que me mando

  if ((despl !=0) && (longi !=0)) //si son igual 0->no recibi nada :S
    {
  instruccion="";
  mensaje="";

  string_append(&instruccion, pedido);
  serCadena(&instruccion,string_itoa(segCodigo));
  serCadena(&instruccion,string_itoa(despl));
  serCadena(&instruccion,string_itoa(longi));


  Enviar(sRemoto, instruccion); //envio el pedido a la umv
  Recibir(sRemoto, mensaje);
    } else {mensaje="0"; // si no recibi nada -> el mensaje no comienza con 1
}


  if (string_starts_with(mensaje,"1")) //si comienza con 1 -> recibi un mensj valido
    {
      return string_substring(mensaje,1,(strlen(mensaje)-1));
    } else {
      return "ERROR"; //sino, no sirve lo que recibi...tengo que ver lib errores!!!!
    }

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
  char* pedido = "1"; //o el numero que sea que interprete el kernel
  char* recibo;

  pedido = malloc(1 * sizeof(char));
  recibo = malloc(1 * sizeof(char));

  //el mensaje que le mando es PedidoVariable
  string_append(&pedido, variable);
  Enviar(socketKERNEL, pedido);
  //recibo EstadoValor
  Recibir(socketKERNEL, recibo);

  if (string_starts_with(recibo,"1")) //si comienza con 1 -> recibi un mensj valido
    {
      valor= atoi(string_substring(recibo,1,(strlen(recibo)-1)));
    } else {
      valor=0; //aca en verdad tendria que devolver un valor por default a convenir
    }

  return valor;
}


void
grabar_valor(t_nombre_compartida variable,t_valor_variable valor)
{
  char* pedido = "1";
  pedido = malloc(1 * sizeof(char));

  //el mensaje que le mando es  PedidoVariableValor
  string_append(&pedido, variable);
  string_append(&pedido, string_itoa(valor));
  Enviar(socketKERNEL, pedido);

}

void
procesoTerminoQuantum(PCB prog)
{

  char* aviso;
  aviso = malloc(1 * sizeof(char));

  aviso="2"; // el numero que el kernel interprete como fin programa

  string_append(&aviso, serializar_PCB(prog));
  Enviar(socketKERNEL,aviso);

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
  //destruyo estructuras auxiliares
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
  //aviso al kernel que me voy :(
  char* aviso="3";
  aviso = malloc(1 * sizeof(char));

  string_append(&aviso, "CHAU"); //como interpreta el kernel que me voy?
  Enviar(socketKERNEL,aviso);

}

int
crearSocket(int socketConec)
{

  socketConec = socket(AF_INET, SOCK_STREAM, 0);
  //Si al crear el socket devuelve -1 quiere decir que no lo puedo usar
  if (socketConec == -1)
    perror("Este socket tiene errores!");
  return socketConec;
}

struct sockaddr_in
prepararDestino(struct sockaddr_in destino, int puerto, char* ip)
{

  struct hostent *he, *
  gethostbyname();
  he = gethostbyname(ip);

  //Con esto me quiero conectar con UMV o KERNEL
  destino.sin_family = AF_INET;
  destino.sin_port = htons(puerto);
  bcopy(he->h_addr, &(destino.sin_addr.s_addr),he->h_length);
  memset(&(destino.sin_zero), '\0', 8);

  return destino;
}


void
RecuperarDicEtiquetas(int indice, int tamanio, t_dictionary dicc)
{

 /* int i;
  int inicio;
  char* funcion;

  for (i=1; i< (tamanio + 1);i ++)
    {
      //aca tengo que ver como obtener la funcion y el inicio
      //dictionary_put(dicc,funcion,inicio);
    }*/

}
void
RecuperarContextoActual(PCB prog, t_dictionary dicc)
{
  //RECUPERAR ALGO
  //en sizeContextoActual tengo la cant de variables que debo leer
  //si es 0 -> programa nuevo
  //si es >0 -> cant de variables a leer desde seg stack en umv

  int i;
char* variable;
void* dato;

  if (prog.sizeContextoActual > 0) //tengo variables a recuperar
    {
      //voy de 0 a cantidad de variables en contexto actual
      for (i=0; i < prog.sizeContextoActual; i ++)
        {
         variable="";
         dato=0;
         //aca tengo que ver como obtener la variable y el dato
         dictionary_put(&dicc,variable,dato);

        }


    }

}

int
main(void)

{

  int CONECTADO_UMV = 0;
  int CONECTADO_KERNEL = 0;

  //Llegué y quiero leer los datos de conexión
  //donde esta el kernel?donde esta la umv?

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
  ConexionConSocket(&CONECTADO_UMV, socketUMV, dest_UMV);
  ConexionConSocket(&CONECTADO_KERNEL, socketKERNEL, dest_KERNEL);

  //Control programa
  int tengoProg = 0;
  PCB programa;
  int quantum;
  char* sentencia;
  sentencia = malloc(1 * sizeof(char));

  t_dictionary dicVariables;
  t_dictionary dicEtiquetas;


  //solo pruebas
  /*
  sentencia= PedirSentencia(687, 200,5, 2);
  PCB prueba;
  char* eje = "1143-242-33-44-55-6609-77-88-9900-";

  prueba = desearilizar_PCB(eje, 0);

  printf("/n %d", prueba.id);
  printf("/n %d", prueba.segmentoCodigo);
  printf("/n %d", prueba.segmentoStack);
  printf("/n %d", prueba.cursorStack);
  printf("/n %d", prueba.indiceCodigo);
  printf("/n %d", prueba.indiceEtiquetas);
  printf("/n %d", prueba.programCounter);
  printf("/n %d", prueba.sizeContextoActual);
  printf("/n %d", prueba.sizeIndiceEtiquetas);

  printf("cadena %s", serializar_PCB(prueba));

  analizadorLinea("a = b + 3", &funciones_p, &funciones_k);*/

  //voy a trabajar mientras este conectado tanto con kernel como umv
  while ((CONECTADO_KERNEL == 1) && (CONECTADO_UMV == 1))
    {
      //Mientras estoy conectado al Kernel y no tengo programa que
      //ejecutar...solo tengo que esperar a recibir uno

      while (tengoProg < 1) //me fijo si tengo un prog que ejecutar
        {
          tengoProg = RecibirProceso(programa, quantum, socketKERNEL);
        }
      //Si salio del ciclo anterior es que ya tengo un programa


      RecuperarDicEtiquetas(programa.indiceEtiquetas,
                            programa.sizeIndiceEtiquetas, dicEtiquetas);
      RecuperarContextoActual(programa,dicVariables);

      while (quantum > 0) //mientras tengo quantum
        {

          //tengo que recuperar el contexto actual del programa que recibi


          programa.programCounter++; //Incremento el PC
          sentencia=PedirSentencia(programa.indiceCodigo,
                                    programa.segmentoCodigo,
                                    programa.programCounter,socketUMV); //le pido a la umv la sentencia a ejecutar
          parsearYejecutar(sentencia); //ejecuto sentencia
          quantum--; //decremento el quantum

          // aca me conviene poner un control, si ya terminó de ejecutar y uso menos
          //quantum que el previsto -> quantum=0 para que salga
        }

      //una vez que el proceso termino el quantum
      //quiero salvar el contexto y limpiar estructuras auxiliares
      salvarContextoProg();
      limpiarEstructuras();
      procesoTerminoQuantum(programa); //ahora le aviso al kernel que el proceso
                               //ha finalizado
      //el enunciado dice que cuando finalizo la ejecucion de un programa
      //tengo que destruir las estructuras correspondientes
      //cuando termino la ejecución de ese
      CONECTADO_UMV = seguirConectado();
      CONECTADO_KERNEL=CONECTADO_UMV;
    }
  //me voy a desconectar, asi que... antes le tengo que
      //avisar al kernel asi me saca de sus recursos

  AvisarDescAKernel();

  Cerrar(socketKERNEL); //cierro el socket
  Cerrar(socketUMV);

  return EXIT_SUCCESS;

}

// Primitivas

void
prim_asignar(t_puntero direccion_variable, t_valor_variable valor)
{

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

}

void
prim_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{

  //preservar el contexto actual para retornar al mismo

}

void
prim_finalizar(void)
{
  //recuperar pc y contexto apilados en stack.Si finalizo, devolver -1

}

void
prim_retornar(t_valor_variable retorno)
{  //acá tengo que volver a retorno

}

void
prim_imprimir(t_valor_variable valor_mostrar)
{

  //acá me conecto con el kernel y le paso el mensaje

}

t_valor_variable
prim_dereferenciar(t_puntero direccion_variable)
{
  t_valor_variable valor;
  valor = 1; //inicializo de prueba

  //tiene que leer el valor que se guarda en esa posición

  return valor; //devuelve el valor encontrado
}

void
prim_irAlLabel(t_nombre_etiqueta etiqueta)
{
  /*t_puntero_instruccion primer_instr;
   char* ptr_etiquetas = "";
   t_size tam_etiquetas = 0;

   primer_instr = metadata_buscar_etiqueta(etiqueta, ptr_etiquetas,
   tam_etiquetas);*/

  //busco la primer instruccion ejecutable
}

t_puntero
prim_obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
  t_puntero posicion;
  posicion = 1; //inicializo de prueba
  //busco la posicion de la variable

  printf("hola!!!!!");
  return posicion; //devuelvo la posicion
}

t_puntero
prim_definirVariable(t_nombre_variable identificador_variable)
{
  t_puntero pos_var_stack=0;
  // reserva espacio para la variable,
  //la registra en el stack

  //la registro en el dicc de variables

  //dictionary_put(dicVariables,identificador_variable,pos_var_stack);
  //aumentarContextoAct();  acá de algún modo tengo que modificar el tmñ del contexto actual de ejecucion


  return pos_var_stack; //devuelvo la pos en el stack
}

void
prim_imprimirTexto(char* texto)
{
  /*int cant_caracteres;
   cant_caracteres = 1; //inicializo de prueba

   //acá me conecto con el kernel y le paso el mensaje
   */
}

void
prim_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
  /* int tiempo2;
   tiempo2 = 1; //inicializo de prueba

   //acá pido por sys call a kernel
   // entrada_salida(dispositivo,tiempo);
   */
}

void
prim_wait(t_nombre_semaforo identificador_semaforo)
{
  //sys call con kernel
  // wait(identificador_semaforo);
  int senial=0;
  char* msj="";
  char* pedido = "5"; //lo que identifique como wait
  pedido = malloc(1 * sizeof(char));

    //el mensaje que le mando es  PedidoSemaforo
    string_append(&pedido, identificador_semaforo);
    Enviar(socketKERNEL,pedido);

    //aca algo tengo que hacer si lo toma o no...
    //pienso quizas..que el kernel me responda y salir del ciclo
    //sólo si consegui el semaforo

    while (senial==0) //solo va a salir del ciclo cuando senial=1 ->me dio el semaforo
      {
        Recibir(socketKERNEL,msj);
        senial=atoi(string_substring(msj,1,1)); //donde este la señal
      }
}

void
prim_signal(t_nombre_semaforo identificador_semaforo)
{
   //sys call con kernel
   // signal(identificador_semaforo);

  char* pedido = "6"; //lo que identifique como wait
  pedido = malloc(1 * sizeof(char));

    //el mensaje que le mando es  PedidoSemaforo
    string_append(&pedido, identificador_semaforo);
    Enviar(socketKERNEL,pedido);
    //aca no me importa que haga el kernel, el se va a fijar si desbloquea a alguien
}
