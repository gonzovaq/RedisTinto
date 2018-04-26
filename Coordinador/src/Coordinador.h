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
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "thpool.h"
#include <sys/queue.h>

#define MYPORT 3490    // Puerto al que conectarán los usuarios

#define BACKLOG 10     // Cuántas conexiones pendientes se mantienen en cola

struct parametrosConexion{
	//int sockfd; --> no se requiere para la conexion
	int new_fd;
	struct node_t * colaProcesos;
};

typedef enum{
	ESI = 1,
	PLANIFICADOR = 2,
	COORDINADOR = 3,
	INSTANCIA = 4

}tTipoDeProceso;

typedef enum{
	CONECTARSE = 1
}tTipoDeMensaje;

typedef struct{
	tTipoDeProceso tipoProceso;
	tTipoDeMensaje tipoMensaje;
	int idProceso;
}tHeader;

//var globales
t_log * logger;
t_queue *colaEsis;


void sigchld_handler(int s);
int main(void);

void configure_logger();
void exit_gracefully(int return_nr);
void *gestionarConexion(struct parametrosConexion *parametros);
void *conexionESI(int *new_fd);
void *conexionPlanificador(int *new_fd);
void *conexionInstancia(int *new_fd);

