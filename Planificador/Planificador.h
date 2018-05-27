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
t_queue *ready;
t_queue *ejecucion;
//ESTO DEBERIA ESTAR EN OTRO .H
struct parametrosConexion{
	int new_fd;
	struct node_t * colaProcesos;
};

// Uso de commons en Queue
typedef struct {
      int id;
      int fd;
}t_esi;


static t_esi * new_ESI(int id,int fd){
	t_esi *new = malloc(sizeof(t_esi));
	new->id = id;
	new->fd = fd;
	return new;
}

static void ESI_destroy (t_esi * self){
	free (self->id);
	free(self->fd);
	free (self);
}


// fin de uso de commons para queue
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
