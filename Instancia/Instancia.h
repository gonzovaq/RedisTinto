/*
 *
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <semaphore.h>

#define ARCHIVO_CONFIGURACION "Instancia.config"
#define MAXDATASIZE 100 // máximo número de bytes que se pueden leer de una vez
#define TAMANIO_NOMBREPROCESO 40
#define TAMANIO_CLAVE 41
#define SLEEP_DUMP 5 //Le puse 5, no tengo idea cada cuanto tiene que ejecutar el Dump

// Enums

typedef enum{
	ESI = 1,
	PLANIFICADOR = 2,
	COORDINADOR = 3,
	INSTANCIA = 4

}tTipoDeProceso;

typedef enum{
	CONECTARSE = 1,
	SET = 2,
	GET = 3,
	STORE = 4
}tTipoDeMensaje;

typedef enum{
	OPERACION_GET = 1,
	OPERACION_SET = 2,
	OPERACION_STORE = 3
}tTipoOperacion;

typedef enum{
	CIRC = 1,
	LRU = 2,
	BSU = 3
}tAlgoritmoReemplazo;

// Structs

typedef struct{
	tTipoDeProceso tipoProceso;
	tTipoDeMensaje tipoMensaje;
	int idProceso;
	char nombreProceso[TAMANIO_NOMBREPROCESO];
}tHeader;

typedef struct{
	char clave[TAMANIO_CLAVE];
	int numeroEntrada;
	int tamanioAlmacenado;
}tEntrada;

typedef enum{
	OK = 1,
	BLOQUEO = 2,
	ERROR = 3,
	DESBLOQUEO = 4
}tResultadoOperacion;

typedef enum{
	SOLICITAR_VALOR = 1,
	OPERAR = 2,
	COMPACTAR = 3
}tOperacionInstancia;

typedef struct{
	tOperacionInstancia operacion;
}__attribute__((packed)) tOperacionInstanciaStruct;

typedef struct{
	tResultadoOperacion resultado;
	int tamanioValor;
}__attribute__((packed)) tResultadoInstancia;

typedef struct{
	char clave[41];
	char valor[40];
}__attribute__((packed))
tInstruccion;

typedef struct{
	tHeader encabezado;
	tInstruccion instruccion;
}__attribute__((packed))
tMensaje;

typedef struct {
  tTipoOperacion tipo;
  int tamanioValor;
}__attribute__((packed)) OperaciontHeader;


typedef struct {
	tTipoOperacion tipo;
	char clave[TAMANIO_CLAVE];
	char* valor;
}__attribute__((packed)) operacionRecibida;

typedef struct{
	int entradas;
	int tamanioEntradas;
	int cantidadClaves;
}tInformacionParaLaInstancia;

typedef struct{
	int entradasUsadas;
}tEntradasUsadas;

// Var Globales

	//Variables para el archivo de configuración
		char* IP;
		int PORTCO;
		char *Algoritmo;
		char *PuntoMontaje;
		char *nombreInstancia;
		int Intervalo;

	//Sockets
    	struct hostent *he;
    	struct sockaddr_in their_addr; // información de la dirección de destino

	//Otras
    	char **arrayEntradas;
    	int cantidadEntradas = 0;
	    int tamanioValor = 0;
	    int posicionPunteroCirc = 0;
	    t_list *tablaEntradas;
	    t_list *claves;
	    tAlgoritmoReemplazo algoritmoReemplazo;
	    int cantidadClavesEnTabla = 0;
	    sem_t *semaforo;


// Prototipos de las funciones

int main(int argc, char *argv[]);
int LeerArchivoDeConfiguracion(char *argv[]);
int verificarParametrosAlEjecutar(int argc, char *argv[]);
int conectarSocket(int port);
int enviarHeader(int sockfd);
char* recibirMensaje(int sockfd, int bytesRecibidos);
int enviarMensaje(int sockfd, char* mensaje);
operacionRecibida *recibirOperacion(int socketCoordinador, int bytesValor);
void mostrarArray(char **arrayEntradas, int cantidadEntradas);
OperaciontHeader *recibirHeader(int socketCoordinador);
int recibirConfiguracion(int sockeCoordinador);

void agregarEntrada(operacionRecibida *unaOperacion, int tamanioValorRecibido);
void procesarMensaje(tMensaje *unMensaje, t_list *tablaEntradas);
void mostrarLista(t_list *miTabla);
void procesarInstruccion(tMensaje *unMensaje, char **arrayEntradas, int cantidadEntradas, int tamanioValor, t_list *tablaEntradas, int socketCoordinador);
int calcularLongitudMaxValorBuscado(char *unaClave,t_list *tablaEntradas);
int calcularEntradasNecesarias(int valorSobrePasado, int tamanioValor);
void agregarNodoAtabla(t_list *tablaEntradas, int nroEntradaStorage, int tamanioAlmacenado, char *nombreClave);
char *obtenerValor(int longitudMaximaValorBuscado, t_list *tablaEntradas, char *claveBuscada,char **arrayEntradas,int tamanioValor);

t_list* list_duplicate(t_list* self);
bool validarClaveExistente(char *unaClave, t_list *tablaEntradas);
t_list *filtrarLista(char *unaClave, t_list *tabla);
void destruirNodoDeTabla(tEntrada *unaEntrada);
void eliminarNodos(char *nombreClave, t_list *tablaEntradas);
bool validarEntradaLibre(char **arrayEntradas, int cantidadEntradas);
void eliminarEntradasStorageCircular(char **arrayEntradas, int cantidadEntradasABorrar);
void eliminarNodoPorIndex(t_list *tablaEntradas, int index);
int calcularEntradasABorrar(char **arrayEntradas, int entradasNecesarias);

int Dump(char **arrayEntradas);

void guardarUnArchivo(char *unaClave, char *valorArchivo);
void guardarTodasMisClaves(t_list *tablaEntradas, char **arrayEntradas);
int enviarEntradasUsadas(int socketCoordinador,t_list *tablaEntradas, char *bufferClave);
void eliminarEntradasStorageLRU(char **arrayEntradas, int cantidadEntradasABorrar);
void eliminarEntradasStorageBSU(char **arrayEntradas, t_list *entradasABorrar);
t_list *obtenerTablaParaBSU(t_list *tablaEntradas, int cantidadEntradasPendientes);

int EnviarAvisoDeQueEstoyViva(int socketCoordinador);
int RecibirClavesPrevias(int socketCoordinador);
FILE *leerArchivo(char *archivo);
