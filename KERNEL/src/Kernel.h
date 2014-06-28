
//Sockets
int ConexionConSocket(int puerto, char* IP );
int EnviarDatos(int sRemoto, void *buffer);
int RecibirDatos(int sRemoto, char *buffer);
char* RecibirDatos2(int socket, char *buffer, int *bytesRecibidos);
void Cerrar (int sRemoto);
int hacerhandshakeUMV(int sockfd);
void conectarAUMV();
int analisarRespuestaUMV(char *mensaje);
void CerrarSocket(int socket);

//Config
char *ObtenerIPUMV();
int ObtenerPuertoConfig();
int ObtenerPuertoUMV();
int ObtenerPuertoPCPConfig();
int ObtenerQuamtum();
int ObtenerRetardo();
int ObtenerMulti();

//Hilos
void *PLP(void *arg);
void *PCP(void *arg);
void *IMPRIMIRConsola(void *arg);
void *FinEjecucion(void *arg);
void *HiloOrquestadorDeCPU();
void *moverEjecutar(void *arg);
void *hiloDispositivos(void *arg);
void *moverReadyDeNew(void *arg);
void *bloqueados_fnc(void *arg);

//Manejo de errores
void error(int code, char *err);
void ErrorFatal(char mensaje[], ...);
void Error(const char* mensaje, ...);
void Traza(const char* mensaje, ...);

/* Definición del pcb */
typedef struct PCBs {
	int id;
	int segmentoCodigo;
	int segmentoStack;
	int cursorStack;
	int indiceCodigo;
	int indiceEtiquetas;
	int programCounter;
	int sizeContextoActual;
	int sizeIndiceEtiquetas;
} PCB;

PCB PCB1;

//deserealizar
PCB* desearilizar_PCB(char* estructura, int* pos);
char* obtenerNombreMensaje(char* buffer, int pos);
char* obtenerParteDelMensaje(char* buffer, int* pos);

//serializar
char* serializar_PCB (PCB* prog);
void iniciarPCB(PCB* prog);

//Comando de mensajes
void comandoLiberar(int socket);
char ObtenerComandoCPU(char buffer[]);
int ObtenerComandoMSJ(char buffer[]);
void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente);
char* ComandoHandShake2(char *buffer, int *tipoCliente);
int ComandoRecibirPrograma(char *buffer, int id);
void comandoFinalQuamtum(char *buffer,int socket);
void comandoWait(char* buffer,int socket);
void comandoSignal(char* buffer,int socket);
void comandoFinalizar(int socket,char* buffer);
void comandoImprimir(char* buffer, int socket);
void comandoObtenerValorGlobar(char* buffer,int socket);
void comandoGrabarValorGlobar(char* buffer,int socket);
void comandoAbortar(char* buffer, int socket);

void crearEscucha();
int chartToInt(char x);
int posicionDeBufferAInt(char* buffer, int posicion);
char* RespuestaClienteOk(char *buffer);
void agregarNuevaCPU(int socket);

//Semaforo Contador
typedef struct{
	int n;
	pthread_mutex_t semdata;
	pthread_mutex_t sem_mutex;
} psem_t;

void semdestroy(psem_t *ps);
void semsig(psem_t *ps);
void semwait(psem_t *ps);
void seminit(psem_t *ps, int n);

psem_t newCont,readyCont,CPUCont,finalizarCont, imprimirCont;
psem_t multiCont; //limita la multiprogramacion

//Lista de CPU
typedef struct _t_CPU {
	int idCPU;
	PCB* idPCB;
	int libre;
} t_CPU;

static t_CPU *cpu_create(int idCPU)
{
	t_CPU *new = malloc(sizeof(t_CPU));
	new->idPCB = malloc(sizeof(PCB));
	new->idCPU = idCPU;
	new->libre = 0;
	return new;
}

static void cpu_destroy(t_CPU *self)
{	free(self->idPCB);
	free(self);
}

//Lista de semaforos del usuario
typedef struct _t_sem {
	char* nombre;
	int valor;
	t_list* listaSem;
} t_sem;

static t_sem *sem_create(char* nombre, int valor)
{
	t_sem *new = malloc(sizeof(t_sem));
	new->nombre = nombre;
	new->valor = valor;
	new->listaSem=list_create();
	return new;
}

//static void sem_destroy(t_sem *self)
//{
//	free(self->nombre);
//	free(self);
//}

//Variables globales
typedef struct _t_varGlobal{
	char* nombre;
	int valor;
}t_varGlobal;

static t_varGlobal *varGlobal_create(char* nombre, int valor )
{
	t_varGlobal*new = malloc(sizeof(t_varGlobal));
	new->nombre = nombre;
	new->valor = valor;
	return new;
}

//static void varGlobal_destroy(t_imp *self)
//{
//	free(self);
//}

//Lista de Final
typedef struct _t_Final {
	PCB* idPCB;
	int finalizo;	//0 para correcto 1 para incorrecto
	char* mensaje;
} t_Final;

static t_Final *final_create(PCB* pcb,int final, char* msj)
{
	t_Final*new = malloc(sizeof(t_Final));
	new->idPCB = pcb;
	new->finalizo = final;
	new->mensaje = msj;
	return new;
}

//static void final_destroy(t_Final *self)
//{	free(self->idPCB);
//	free(self);
//}

//Lista de imprimir
typedef struct _t_imp {
	int prog;
	char* mensaje;
} t_imp;

static t_imp *imp_create(int socket, char* msj)
{
	t_imp*new = malloc(sizeof(t_imp));
	new->prog = socket;
	new->mensaje = msj;
	return new;
}

static void imp_destroy(t_imp *self)
{
	free(self);
}

//Lista de Bloqueado por Dispositivo
typedef struct _t_bloqueado {
	PCB* idPCB;
	int tiempo;
} t_bloqueado;

static t_bloqueado *bloqueado_create(PCB* pcb,int tiempo)
{
	t_bloqueado*new = malloc(sizeof(t_bloqueado));
	new->idPCB = pcb;
	new->tiempo = tiempo;
	return new;
}

static void bloqueado_destroy(t_bloqueado *self)
{	free(self->idPCB);
	free(self);
}

//Lista de Hilos de dispositivos
typedef struct _t_hilos {
	pthread_t hilo;
} t_hilos;

static t_hilos *hilos_create()
{
	t_hilos* new = malloc(sizeof(t_hilos));
	return new;
}

static void hilos_destroy(t_hilos *self)
{
	free(self);
}

//Lista de dispositivos
typedef struct _t_HIO {
	char* nombre;
	int valor;
	t_list* listaBloqueados;
	psem_t bloqueadosCont;
	pthread_mutex_t mutexBloqueados;
} t_HIO;

static t_HIO *HIO_create(char* nombre, int valor)
{
	t_HIO *new = malloc(sizeof(t_HIO));
	new->nombre = nombre;
	new->valor = valor;
	new->listaBloqueados=list_create();
	seminit(&(new->bloqueadosCont), 0);
	pthread_mutex_init(&new->mutexBloqueados, NULL );
	return new;
}
//funciones de las listas de config
void llenarSemaforoConfig();
void llenarVarGlobConfig();
void llenarDispositConfig();
char* obtenerCadenaSem();
char* obtenerCadenaValSem();
char* obtenerCadenaDispositivos();
char* obtenerCadenaValDisp();
char* obtenerCadenaVarGlob();
char* obtenerCadenaValVarGlob();
//static void HIO_destroy(t_HIO *self)
//{
//	free(self->nombre);
//	free(self);
//}

t_CPU l_CPU;
bool encontrarInt(int actual, int expected);
t_CPU*  encontrarCPULibre();
t_CPU* encontrarCPU(int idcpu);
void eliminarCpu(int idcpu);
t_sem*  encontrarSemaforo(char *nombre);
t_HIO* encontrarDispositivo(char* nombre);
t_varGlobal* encontrarVarGlobal(char* nombre);
void borrarPCBenCPU(int idCPU);

//Globales
int socketumv;
int ImprimirTrazaPorConsola = 1;
int Puerto;
int PuertoPCP;
int UMV_PUERTO;
int Quamtum;
int Retardo;
int Multi;
char *UMV_IP;
t_list *listCPU, *listaNew, *listaReady, *listaDispositivos,*listaFin, *listaSemaforos;
t_list *listaImprimir, *listaVarGlobal;
pthread_mutex_t mutexNew = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexReady = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCPU = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexFIN = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexSemaforos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexDispositivos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexImprimir = PTHREAD_MUTEX_INITIALIZER;

int cantidadDigitos(int num);
