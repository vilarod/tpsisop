void ConexionConSocket();
int Recibir (int sRemoto, char * buffer);
int ObtenerPuertoUMV();
int Enviar (int sRemoto, char * buffer);
void Cerrar (int sRemoto);
void *PLP(void *arg);
void *PCP(void *arg);
void *escuchaPLP(void *arg);
void error(int code, char *err);
