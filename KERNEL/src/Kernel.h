void ConexionConSocket();
int ObtenerPuertoUMV();
int Enviar (int sRemoto, void * buffer);
void Cerrar (int sRemoto);
void *PLP(void *arg);
void *PCP(void *arg);
void *escuchaPLP(void *arg);
void error(int code, char *err);
