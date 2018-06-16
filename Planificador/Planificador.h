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
	#include <semaphore.h>
	#include <signal.h>

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

static volatile int keepRunning = 1;
struct hostent *he;
struct sockaddr_in cord_addr; // información de la dirección del Coordinador
	t_queue *ready;
	t_queue *ejecucion;
   	t_queue *finalizados;
    t_queue *bloqueados;
    t_list *clavesBloqueadas;
	int sockCord;
	
	int PORT_COORDINADOR;
	char* IP;
	char* IPCO;
	int MAXDATASIZE;
	float estimacionIni;
	int Alfa;
//ESTO DEBERIA ESTAR EN OTRO .H
struct parametrosConexion{
	int new_fd;
	struct node_t * colaProcesos;
};
//**varGlobales**
int flagOperar=1;
int PORT;
int SOCKET_READ_TIMEOUT_SEC=4;
int tamanioValor;


typedef enum{
	FIFO=1,
	SJF=2,
	SJFD=3,
	HRRN=4
}tAlgoritmo;

tAlgoritmo algoritmo;
// Uso de commons en Queue
typedef struct {
      int id;
      int fd;
      int cont;
      float estimacion;
      float tasaTransf;
      int Espera;
	  char clave[TAMANIO_CLAVE];
}t_esi;

typedef enum{
	LISTAR = 1,
	BLOQUEAR = 2,
	DESBLOQUEAR = 3,
	KILL = 4,
	STATUS= 5
}tSolicitudesDeConsola;

typedef struct{
	tSolicitudesDeConsola solicitud;
}tSolicitudPlanificador;

typedef struct{
	int tamanioValor;
	char proceso[TAMANIO_NOMBREPROCESO];
}tStatusParaPlanificador;

static t_esi * new_ESI(int id,int fd,int esti,float tasa,int espera,char clave[TAMANIO_CLAVE]){
	t_esi *new = malloc(sizeof(t_esi));
	new->id = id;
	new->fd = fd;
	new->cont = 0;
	new->estimacion=esti;
	new->tasaTransf=tasa;
	new->Espera=espera;
	strcpy(new->clave,clave);	
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

typedef struct{
	int cantidadClavesBloqueadas;
}tClavesBloqueadas;


int LeerArchivoDeConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
int ConectarAlCoordinador(int sockCord, struct sockaddr_in* cord_addr,struct hostent* he);
void *ejecutarConsola();
t_esi *buscarEsi(t_queue *lista,char clave[TAMANIO_CLAVE],t_esi* esi);
void destruirEsi(t_esi *unEsi);
void *gestionarConexion(int socket);
void *conexionESI(int *new_fd);
void estimacionEsi (t_esi* esi);
void ordenarEsis(t_queue *cola);
int recibirResultado2(tResultado * resultado);
void obtenerBloqueados(char clave[TAMANIO_CLAVE]);
void bloquearEsi(int id,char clave[TAMANIO_CLAVE]);
void enviarClaveCoordinador(char clave[TAMANIO_CLAVE],tSolicitudesDeConsola *solicitud);
t_esi * buscarEsiPorId(t_queue *lista,int id,t_esi * esi);
void killEsi (int id);
void sumarEspera(t_queue *cola);
