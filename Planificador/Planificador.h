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
	#include <commons/config.h>
/*
 * Coordinador.h
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */

#define ARCHIVO_CONFIGURACION "planificador.config"
/*
#define PORT 3491 // puerto al que vamos a conectar
#define PORT_COORDINADOR 3490 // puerto del coordinador, donde nos vamos a conectar
#define MAXDATASIZE 100 // máximo número de bytes que se pueden leer de una vez*/
#define TAMANIO_CLAVE 41 //Por enunciado la clave sera de 40 caracteres.
#define TAMANIO_NOMBREPROCESO 40

struct hostent *he;
struct sockaddr_in cord_addr; // información de la dirección del Coordinador
	t_queue *ready;
	t_queue *ejecucion;
   	t_queue *colaFinalizados;
   	t_queue *colaEspera;
    t_queue *colaBloq;
//ESTO DEBERIA ESTAR EN OTRO .H
struct parametrosConexion{
	int new_fd;
	struct node_t * colaProcesos;
};
//**varGlobales**
int flagOperar=1;
int PORT;
int PORT_COORDINADOR;
char* IP;
char* IPCO;
int MAXDATASIZE;
float EstimacionIni;
int Alfa;

// Uso de commons en Queue
typedef struct {
      int id;
      int fd;
      int cont;
      float Estimacion;
}t_esi;


static t_esi * new_ESI(int id,int fd){
	t_esi *new = malloc(sizeof(t_esi));
	new->id = id;
	new->fd = fd;
	new->cont = 0;
	new->Estimacion=EstimacionIni;
	return new;
}

static void ESI_destroy (t_esi * self){
	free (self->id);
	free(self->fd);
	free (self);
}
typedef enum{
	OK = 1,
	BLOQUEO = 2,
	ERROR = 3,
	DESBLOQUEO = 4,
	CHAU=0
}tResultadoOperacion;

typedef struct{
	tResultadoOperacion tipoNotificacion;
	char clave[TAMANIO_CLAVE];
	int pid;
}tNotificacionPlanificador;



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
	char nombreProceso[TAMANIO_NOMBREPROCESO];
}tHeader;


typedef struct{
	tResultadoOperacion tipoResultado;
	char clave[TAMANIO_CLAVE];
}__attribute__((packed)) tResultado;


int LeerArchivoDeConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
void ConectarAlCoordinador(int sockCord, struct sockaddr_in* cord_addr,struct hostent* he);
void *ejecutarConsola();
t_esi *buscarEsi(t_queue *lista,int id);
void destruirEsi(t_esi *unEsi);
void *gestionarConexion(int socket);
void *conexionESI(int *new_fd);

void ordenarEsis(t_queue *cola);