	#include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <errno.h>
    #include <string.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
	#include <sys/queue.h>
	#include <commons/collections/list.h>
	#include <commons/collections/queue.h>
/*
 * Coordinador.h
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */


#define PORT 3491 // puerto al que vamos a conectar
#define PORT_COORDINADOR 3490 // puerto del coordinador, donde nos vamos a conectar
#define MAXDATASIZE 100 // máximo número de bytes que se pueden leer de una vez

struct hostent *he;
struct sockaddr_in cord_addr; // información de la dirección del Coordinador
t_queue *colaESIS;

//ESTO DEBERIA ESTAR EN OTRO .H
struct parametrosConexion{
	int new_fd;
	struct node_t * colaProcesos;
};

typedef struct Fifo {
      int pid;
      struct Fifo *sgt;
}Fifo;

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


void LeerArchivoDeConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
void ConectarAlCoordinador(int * sockCord, struct sockaddr_in* cord_addr,
		struct hostent* he);
void *ejecutarConsola();


void *gestionarConexion(int socket);
void *conexionESI(int *new_fd);
