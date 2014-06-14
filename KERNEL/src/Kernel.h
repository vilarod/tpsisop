
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

//Manejo de errores
void error(int code, char *err);
void ErrorFatal(char mensaje[], ...);
void Error(const char* mensaje, ...);
void Traza(const char* mensaje, ...);

void crearEscucha();
int AtiendeCliente(int sockete);
int ObtenerComandoMSJ(char buffer[]);
void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente);
char* ComandoHandShake2(char *buffer, int *tipoCliente);
int chartToInt(char x);
void ComandoRecibirPrograma(char *buffer, int id);
int pedirMemoriaUMV(int socketumv, PCB programa);
int AtiendeClienteCPU(void * arg);
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

psem_t readyCont,CPUCont,finalizarCont, imprimirCont;

//Lista de CPU
typedef struct _t_CPU {
	int idCPU;
	int idPrograma;
	int libre;
} t_CPU;

static t_CPU *cpu_create(int idCPU)
{
	t_CPU *new = malloc(sizeof(t_CPU));
	new->idPrograma = 0;
	new->idCPU = idCPU;
	new->libre = 0;
	return new;
}

static void cpu_destroy(t_CPU *self)
{
	free(self);
}


t_CPU l_CPU;
bool encontrarInt(int actual, int expected);
t_CPU*  encontrarCPULibre();

//Globales
int ImprimirTrazaPorConsola = 1;
int Puerto;
int PuertoPCP;
int UMV_PUERTO;
int Quamtum;
int Retardo;
int Multi;
char *UMV_IP;
t_list *listCPU;

