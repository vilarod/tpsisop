
// ESTRUCTURAS //
struct _t_segmento;
typedef struct _t_segmento t_segmento;

// METODOS PARA CONSOLA //
void HiloConsola();
int ObtenerComandoConsola(char buffer[]);
void ConsolaComandoHelp();
void ConsolaComandoLeerMemoria();
void ConsolaComandoEscribirMemoria();
void ConsolaComandoCrearSegmento();
void ConsolaComandoDestruirSegmento();
void ConsolaComandoDefinirAlgoritmo();
void ConsolaComandoDefinirRetardo();
void ConsolaComandoCompactarMemoria();
void ConsolaComandoDumpEstructuras();
void ConsolaComandoDumpMemoriaPrincipal();
void ConsolaComandoDumpContenidoMemoriaPrincipal();
void ConsolaComandoGrabaSiempreArchivo();
void ConsolaComandoActivarTraza();

// METODOS QUE IMPRIMEN //
void ImprimirResuladoDeLeerMemoria(int ok, int idPrograma, int base, int desplazamiento, int cantidadBytes, char* buffer, int imprimirArchivo);
void ImprimirListadoSegmentosDeProgramaTSeg(int imprimirArchivo, t_segmento *Programa);
void ImprimirListadoSegmentosDePrograma(int imprimirArchivo, int idPrograma);
void ImprimirEncabezadoDeListadoSegmentos(int imprimirArchivo);
void ImprimirResuladoDeCrearSegmento(int idPrograma, int idSegmento, int tamanio, int imprimirArchivo);
void ImprimirBaseMemoria(int imprimirArchivo);
void ImprimirResumenUsoMemoria(int imprimirArchivo);
void ImprimirResuladoDeDestruirSegmento(int idPrograma, int ok, int imprimirArchivo);
void ImprimirResuladoDeEscribirMemoria(int ok, int idPrograma, int base, int desplazamiento, int cantidadBytes, char* buffer, int imprimirArchivo);
void ImprimirListadoSegmentos(int imprimirArchivo);
void Imprimir(int ImprimirArchivo, const char* mensaje, ...);
void ImprimirMemoriaSegmentosDePrograma(int imprimirArchivo, int idPrograma);
void ImprimirMemoriaSegmentosDeProgramaTSeg(int imprimirArchivo, t_segmento *seg);
void ImprimirMemoriaSegmentos(int imprimirArchivo);

// METODOS MANEJO DE ERRORES //
void Error(const char* mensaje, ...);
void Traza(const char* mensaje, ...);
void SetearErrorGlobal(const char* mensaje, ...);
void ErrorFatal(const char* mensaje, ...);

// METODOS SOCKETS //
void HiloOrquestadorDeConexiones();
char* RecibirDatos(int socket, char *buffer, int* bytesRecibidos);
int EnviarDatos(int socket, char *buffer);
void CerrarSocket(int socket);

// METODOS CONFIGURACION //
void LevantarConfig();

// METODOS MANEJO MEMORIA //
void InstanciarTablaSegmentos();
void reservarMemoriaPrincipal();
void AgregarSegmentoALista(int idPrograma, int idSegmento, int inicio, int tamanio, char* ubicacionMP);
char* CalcularUbicacionMP(int tamanioSegmento);
int CrearSegmento(int idPrograma, int tamanio);
int CalcularIdSegmento(int idPrograma);
int CalcularInicioSegmento(int idPrograma);
char* CalcularUbicacionMP_FirstFit(int tamanioRequeridoSegmento);
char* CalcularUbicacionMP_WorstFit(int tamanioRequeridoSegmento);
int ObtenerCantidadSegmentos();
int DestruirSegmentos(int idPrograma);
t_segmento* ObtenerInfoSegmento(int idPrograma, int idSegmento);
int obtenerTotalMemoriaEnUso();
void CompactarMemoria();
char * ObtenerUbicacionMPEnBaseAUbicacionVirtual(int idPrograma, int base);
int VerificarAccesoMemoria(int idPrograma, int base, int desplazamiento, int cantidadBytes);
int EscribirMemoria(int idPrograma, int base, int desplazamiento, int cantidadBytes, char* buffer);
int LeerMemoria(int idPrograma, int base, int desplazamiento, int cantidadBytes, char* buffer);

// METODOS ATENDER CLIENTE //
int AtiendeCliente(void * arg);
int ObtenerComandoMSJ(char buffer[]);
char* ComandoGetBytes(char *buffer, int idProg, int tipocliente);
char* ComandoSetBytes(char *buffer, int idProg, int tipocliente);
char* ComandoHandShake(char *buffer, int *tipoCliente);
char* ComandoCambioProceso(char *buffer, int *idProg);
char* ComandoCrearSegmento(char *buffer, int idProg);
char* ComandoDestruirSegmento(char *buffer, int idProg);
char* RespuestaClienteOk(char *buffer);
char* RespuestaClienteError(char *buffer, char *msj);

// OTROS METODOS //
int chartToInt(char x);
char* NombreDeAlgoritmo(char idAlgorit);
int ValidarCodigoAlgoritmo(char idAlgorit);
int TraducirSiNo(char caracter);
int numeroAleatorio(int desde, int hasta);
int cantidadCaracteres(char* buffer);
int EsTipoClienteValido(int tipoCliente);
int cantidadDigitos(int num);
int subCadenaAInt(char* text, int start, int length);
int posicionDeBufferAInt(char* buffer, int posicion);










