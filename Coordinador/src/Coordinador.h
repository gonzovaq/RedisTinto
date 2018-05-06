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

///// DEFINES

#define MYPORT 3490    // Puerto al que conectarán los usuarios
#define ARCHIVO_CONFIGURACION "configuracion.config"
#define BACKLOG 10     // Cuántas conexiones pendientes se mantienen en cola
#define TAMANIO_CLAVE 41
#define TAMANIO_VALOR 41


// ESTRUCTURAS Y ENUMS

struct parametrosConexion{
	//int sockfd; --> no se requiere para la conexion
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
	GET = 1,
	SET = 2,
	STORE = 3
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


// VARIABLES GLOBALES

int PUERTO;
char* IP;
t_log * logger;
t_queue *colaInstancias;
t_queue *colaESIS;
t_queue *colaMensajes;
t_queue *colaResultados;
pthread_mutex_t mutex;


// FUNCIONES

void sigchld_handler(int s);
int main(void);
void configure_logger();
void exit_gracefully(int return_nr);
void *gestionarConexion(struct parametrosConexion *parametros);
void *conexionESI(struct parametrosConexion* parametros);
void *conexionPlanificador(struct parametrosConexion* parametros);
void *conexionInstancia(struct parametrosConexion* parametros);
void AnalizarOperacion(char clave[TAMANIO_CLAVE], int tamanioValor,
		OperaciontHeader* header, struct parametrosConexion* parametros,
		OperacionAEnviar* operacion);
void ManejarOperacionGET(char clave[TAMANIO_CLAVE],
		struct parametrosConexion* parametros, OperacionAEnviar* operacion);
void ManejarOperacionSET(clave, tamanioValor, parametros, operacion);
void ManejarOperacionSTORE(clave, parametros, operacion);

