
int AtiendeCliente(int socket, struct sockaddr_in addr);
int DemasiadosClientes(int socket, struct sockaddr_in addr);
void error(int code, char *err);
void reloj(int loop);
void HiloOrquestadorDeConexiones();
void HiloConsola();
int RecibirDatos(int socket, void *buffer);
int ObtenerComandoMSJ(char buffer[]);
void LevantarConfig();
//Comandos
void ComandoGetBytes(char *buffer, int idProg);
void ComandoSetBytes(char *buffer, int idProg);
void ComandoHandShake(char *buffer, int *idProg, int *tipoCliente );
void ComandoCambioProceso(char *buffer, int *idProg);
void ComandoCrearSegmento(char *buffer, int idProg);
void ComandoDestruirSegmento(char *buffer, int idProg);
