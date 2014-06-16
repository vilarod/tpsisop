
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
void *IMPRIMIRYFIN(void *arg);
void *HiloOrquestadorDeCPU();
void *moverEjecutar(void *arg);
void *moverReady(void *arg);
void *moverReadyDeNew(void *arg);

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

//serializar
char* serializar_PCB (PCB* prog);
void iniciarPCB(PCB* prog);

//Comando de mensajes
void comandoLiberar(int socket);
char ObtenerComandoCPU(char buffer[]);
int ObtenerComandoMSJ(char buffer[]);
void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente);
char* ComandoHandShake2(char *buffer, int *tipoCliente);
void ComandoRecibirPrograma(char *buffer, int id);


void crearEscucha();
int AtiendeCliente(int sockete);
int chartToInt(char x);
//int pedirMemoriaUMV(int socketumv, PCB programa);
int pedirMemoriaUMV(int socketumv);
int posicionDeBufferAInt(char* buffer, int posicion);
char* RespuestaClienteOk(char *buffer);
void agregarNuevaCPU(int socket);
//int AtiendeClienteCPU(void * arg);

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
	PCB *nuevo = malloc(sizeof(PCB));
	new->idPCB = nuevo;
	new->idCPU = idCPU;
	new->libre = 0;
	return new;
}

static void cpu_destroy(t_CPU *self)
{	free(self->idPCB);
	free(self);
}


t_CPU l_CPU;
bool encontrarInt(int actual, int expected);
t_CPU*  encontrarCPULibre();
t_CPU* encontrarCPU(int idcpu);
void eliminarCpu(int idcpu);

//Globales
int ImprimirTrazaPorConsola = 1;
int Puerto;
int PuertoPCP;
int UMV_PUERTO;
int Quamtum;
int Retardo;
int Multi;
char *UMV_IP;
t_list *listCPU, *listaNew, *listaReady, *listaBloqueados;

