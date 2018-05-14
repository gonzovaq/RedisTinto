/*
 * Coordinador.h
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <sys/queue.h>
#include <commons/txt.h>
#include <netdb.h>
#include <sys/socket.h>

///// DEFINES

#define MYPORT 3490    // Puerto al que conectarán los usuarios
#define ARCHIVO_CONFIGURACION "configuracion.config"
#define BACKLOG 10     // Cuántas conexiones pendientes se mantienen en cola
#define TAMANIO_CLAVE 41


// ESTRUCTURAS Y ENUMS

struct parametrosConexion{
	int new_fd;
	int semaforo;
	struct node_t * colaProcesos;
};  // Aqui dejamos los descriptores y un semaforo para los hilos que lo necesiten


typedef enum{
	ESI = 1,
	PLANIFICADOR = 2,
	COORDINADOR = 3,
	INSTANCIA = 4

}tTipoDeProceso;

typedef enum{
	CONECTARSE = 1
}tTipoDeMensaje;

typedef enum{
	OPERACION_GET = 1,
	OPERACION_SET = 2,
	OPERACION_STORE = 3
}tTipoOperacion;

typedef enum{
	OK = 1,
	BLOQUEO = 2,
	ERROR = 3
}tResultadoOperacion;



typedef struct{
	tResultadoOperacion resultado;
	char* clave;
}tResultado;

typedef struct{
	tTipoDeProceso tipoProceso;
	tTipoDeMensaje tipoMensaje;
	int idProceso;
}tHeader; // Header que recibimos de los procesos para identificarlos

typedef struct {
  tTipoOperacion tipo;
  int tamanioValor;
}OperaciontHeader; // Header que vamos a recibir de parte del ESI para identificar la operacion

typedef struct {
	tTipoOperacion tipo;
	char* clave;
	char* valor;
}OperacionAEnviar; // Operacion que vamos a enviar a la instancia

typedef struct {
	int ESIPID;
	char* clave;
}tBloqueo;


// VARIABLES GLOBALES

int PUERTO;
char* IP;
t_log * logger;

struct parametrosConexion * planificador;
t_list* colaInstancias;
t_list* colaESIS;
t_list* colaMensajes;
t_list* colaResultados;
t_list* colaBloqueos;

pthread_mutex_t mutex;


// FUNCIONES

void sigchld_handler(int s);
int main(void);
int EscucharConexiones(int sockfd);
int IdentificarProceso(tHeader* headerRecibido, struct parametrosConexion* parametros);
int configure_logger();
int exit_gracefully(int return_nr);
int *gestionarConexion(struct parametrosConexion *parametros);
int *conexionESI(struct parametrosConexion* parametros);
int *conexionPlanificador(struct parametrosConexion* parametros);
int *conexionInstancia(struct parametrosConexion* parametros);
int AnalizarOperacion(int tamanioValor,OperaciontHeader* header, struct parametrosConexion* parametros,
		OperacionAEnviar* operacion);
int ManejarOperacionGET(struct parametrosConexion* parametros, OperacionAEnviar* operacion);
int ManejarOperacionSET(int tamanioValor, struct parametrosConexion* parametros, OperacionAEnviar* operacion);
int ManejarOperacionSTORE(struct parametrosConexion* parametros, OperacionAEnviar* operacion);
int InicializarListasYColas();

