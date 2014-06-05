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
  char* cadena = "";
  char* aux = "";

  aux = string_itoa(prog.id);
  strcat(cadena, aux);
  strcat(cadena, "-");

  return cadena;
}

int
posicionDeCadAInt(char* buffer, int posicion)
{
  int logitudBuffer = 0;
  logitudBuffer = strlen(buffer);

  if (logitudBuffer <= posicion)
    return 0;
  else
    return chartToInt(buffer[posicion]);
}

int
chartToInt(char x)
{
  int numero = 0;
  char* aux = malloc(1 * sizeof(char));
  sprintf(aux, "%c", x);
  numero = strtol(aux, (char **) NULL, 10);

  free(aux);
  return numero;
}

PCB
desearilizar_PCB(char* estructura, int pos)
{

  PCB est_prog;
  int aux;
  int i,h=0;
  int indice = 0;
  char* sub = "";
  int inicio = 0;


  est_prog.id = 0;
  est_prog.segmentoCodigo = 0;
  est_prog.segmentoStack = 0;
  est_prog.cursorStack = 0;
  est_prog.indiceCodigo = 0;
  est_prog.indiceEtiquetas = 0;
  est_prog.programCounter = 0;
  est_prog.sizeContextoActual = 0;
  est_prog.sizeIndiceEtiquetas = 0;

  for (aux=1; aux < 10; aux ++)
    {

      for (i = 0; string_equals_ignore_case(sub, "-") == 0; i++)
        {

          sub = string_substring(estructura, inicio, 1);
          inicio ++;
          h++;
        }

      indice = pos + inicio;

      switch (aux)
        {
      case 1:
        est_prog.id = atoi(string_substring(estructura, pos, h));

        break;
      case 2:
        est_prog.segmentoCodigo = atoi(string_substring(estructura, indice, h));
        break;
      case 3:
        est_prog.segmentoStack = atoi(string_substring(estructura, indice, h));
        break;
      case 4:
        est_prog.cursorStack = atoi(string_substring(estructura, indice, h));
        break;
      case 5:
        est_prog.indiceCodigo = atoi(string_substring(estructura, indice, h));
        break;
      case 6:
        est_prog.indiceEtiquetas = atoi(string_substring(estructura, indice, h));
        break;
      case 7:
        est_prog.programCounter = atoi(string_substring(estructura, indice, h));
        break;
      case 8:
        est_prog.sizeContextoActual = atoi(string_substring(estructura, indice, h));
        break;
      case 9:
        est_prog.sizeIndiceEtiquetas = atoi(string_substring(estructura, indice, h));
        break;

        }
      //inicio++;
      sub = "";
      //indice = 0;
      h=0;
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
          quantum = atoi(estructura);
          prog = desearilizar_PCB(estructura, 2);
        }
      else
        puts("No recibi datos validos");
    }
  else
    perror("No recibi datos validos");

  return r; //devuelve 0 si no tengo, -1 si fue error, >0 si recibi
}

int
PedirSentencia(int indiceCodigo, int sRemoto, char* sentencia)
{
  //aca hare algo para enviarle el pedido a la umv y recibir lo solicitado

  char* pedido = "1"; //lo inicializo en 1=getbyte
  char aux[20];

  //esto hay que modificarlo cuando corrija el pcb
  sprintf(aux, "%d", indiceCodigo);

  /* sprintf(aux,"%d",indiceCodigo.desplInicio);
   * serCadena(pedido,aux);
   * aux="";
   * sprintf(aux,"%d",indiceCodigo.desplfin);
   * serCadena(pedido,aux);
   *
   */
  string_append(&pedido, aux);
  Enviar(sRemoto, pedido);
  int r = (Recibir(sRemoto, sentencia) > 0);
  return r; //devuelve 0 si no tengo, -1 si fue error, >0 si recibi

}

//serializar en cadena, a un mensaje le agrega algo mas y tamaño de algo mas
void
serCadena(char * msj, char* agr)
{
  char* taux = "";
  taux = string_itoa(strlen(agr));
  string_append(&msj, taux);
  string_append(&msj, agr);
}

//primitivas al kernel

t_valor_variable
obtener_valor(t_nombre_compartida variable)
{
  t_valor_variable valor;
  char* pedido = "1"; //o el numero que sea que interprete el kernel
  char recibo[10];
  strcat(pedido, variable);
  Enviar(socketKERNEL, pedido);
  Recibir(socketKERNEL, recibo);
  valor = atoi(recibo);
  return valor;
}

t_valor_variable
grabar_valor(t_nombre_compartida variable)
{
  return 1;
}

void
procesoTerminoQuantum()
{
  //le aviso al kernel que el proceso termino el Q
}

void
parsearYejecutar(char* instr)
{

  AnSISOP_funciones * funciones = 0; //primitivas...pero, que onda??
  //por lo que vi, tendria que empaquetar todas las primitivas en un tipo
  //ya que el parser hace  funciones.AnSISOP_lafuncionquesea

  AnSISOP_kernel * f_kernel = 0; //esto es wait y signal

  //aca hay que invocar al parser y ejecutar las primitivas que
  //estan mas abajito

  analizadorLinea(instr, funciones, f_kernel);

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
RecuperarContextoActual(PCB prog)
{
  //RECUPERAR ALGO
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
  char* sentencia = "";

  PCB prueba;
  char* eje = "166532-23-33-4-57-6722-7-8-999-";

  prueba = desearilizar_PCB(eje,0);

  printf("/n %d", prueba.id);
  printf("/n %d", prueba.segmentoCodigo);
  printf("/n %d", prueba.segmentoStack);
  printf("/n %d", prueba.cursorStack);
  printf("/n %d", prueba.indiceCodigo);
  printf("/n %d", prueba.indiceEtiquetas);
  printf("/n %d", prueba.programCounter);
  printf("/n %d", prueba.sizeContextoActual);
  printf("/n %d", prueba.sizeIndiceEtiquetas);

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

      //tengo que recuperar el contexto actual del programa que recibi
      RecuperarContextoActual(programa);

      while (quantum > 0) //mientras tengo quantum
        {
          programa.programCounter++; //Incremento el PC
          PedirSentencia(programa.indiceCodigo, socketUMV, sentencia); //le pido a la umv la sentencia a ejecutar
          parsearYejecutar(sentencia); //ejecuto sentencia
          quantum--; //decremento el quantum
        }

      //una vez que el proceso termino el quantum
      //quiero salvar el contexto y limpiar estructuras auxiliares
      salvarContextoProg();
      limpiarEstructuras();
      procesoTerminoQuantum(); //ahora le aviso al kernel que el proceso
                               //ha finalizado
      //el enunciado dice que cuando finalizo la ejecucion de un programa
      //tengo que destruir las estructuras correspondientes
      //cuando termino la ejecución de ese
      CONECTADO_UMV = seguirConectado();

      //me voy a desconectar, asi que... antes le tengo que
      //avisar al kernel asi me saca de sus recursos

      //AvisarDescAKernel();

    }

  Cerrar(socketKERNEL); //cierro el socket
  Cerrar(socketUMV);

  return EXIT_SUCCESS;

}

// Primitivas
//esto es solamente un bosquejo para ir armandolas

void
asignar(t_puntero direccion_variable, t_valor_variable valor)
{
  //direccion_variable=valor;
}

t_valor_variable
obtenerValorCompartida(t_nombre_compartida variable)
{
  return obtener_valor(variable); //devuelve el valor de la variable
}

t_valor_variable
asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor)
{

  //como hago para identificar variable - valor? tengo que crear una estructura

  return grabar_valor(variable); //devuelve el valor asignado
}

t_puntero_instruccion
llamarSinRetorno(t_nombre_etiqueta etiqueta)
{
  t_puntero_instruccion instr;
  instr = 1; //inicializo de prueba

  //preservar el contexto de ejecucion actual y resetear estructuras. necesito un contexto vacio ahora

  return instr;
}

t_puntero_instruccion
llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{
  t_puntero_instruccion instr;
  instr = 1; //inicializo de prueba

  //preservar el contexto actual para retornar al mismo

  return instr;
}

t_puntero_instruccion
finalizar(void)
{
  t_puntero_instruccion instr;
  instr = 1; //inicializo de prueba
  //recuperar pc y contexto apilados en stack.Si finalizo, devolver -1

  return instr;
}

t_puntero_instruccion
retornar(t_valor_variable retorno)
{
  t_puntero_instruccion instr;
  instr = 1; //inicializo de prueba
  //acá tengo que volver a retorno

  return instr;
}

int
imprimir(t_valor_variable valor_mostrar)
{
  int cant_caracteres;
  cant_caracteres = 1; //inicializo de prueba

  //acá me conecto con el kernel y le paso el mensaje

  return cant_caracteres;
}

t_valor_variable
dereferenciar(t_puntero direccion_variable)
{
  t_valor_variable valor;
  valor = 1; //inicializo de prueba

  //tiene que leer el valor que se guarda en esa posición

  return valor; //devuelve el valor encontrado
}

t_puntero_instruccion
irAlLabel(t_nombre_etiqueta etiqueta)
{
  t_puntero_instruccion primer_instr;
  char* ptr_etiquetas = "";
  t_size tam_etiquetas = 0;

  primer_instr = metadata_buscar_etiqueta(etiqueta, ptr_etiquetas,
      tam_etiquetas);

  //busco la primer instruccion ejecutable

  return primer_instr; //devuelve la instruccion encontrada
}

t_puntero
obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
  t_puntero posicion;
  posicion = 1; //inicializo de prueba
  //busco la posicion de la variable

  return posicion; //devuelvo la posicion
}

t_puntero
definirVariable(t_nombre_variable identificador_variable)
{
  t_puntero pos_var_stack;
  pos_var_stack = 1; //inicializo de prueba
  // reserva espacio para la variable,
  //la registra en el stack y en el diccionario de variables

  return pos_var_stack; //devuelvo la pos en el stack
}

int
imprimirTexto(char* texto)
{
  int cant_caracteres;
  cant_caracteres = 1; //inicializo de prueba

  //acá me conecto con el kernel y le paso el mensaje

  return cant_caracteres;
}

int
entradaSalida(t_nombre_dispositivo dispositivo, int tiempo)
{
  int tiempo2;
  tiempo2 = 1; //inicializo de prueba

  //acá pido por sys call a kernel
  // entrada_salida(dispositivo,tiempo);

  return tiempo2;
}

int
a_wait(t_nombre_semaforo identificador_semaforo)
{
  int bloquea;
  bloquea = 0;        //incializo de prueba

  //sys call con kernel
  // wait(identificador_semaforo);

  return bloquea;
}

int
a_signal(t_nombre_semaforo identificador_semaforo)
{
  int desbloquea;
  desbloquea = 1; //inicializo de prueba

  //sys call con kernel
  // signal(identificador_semaforo);

  return desbloquea;
}
