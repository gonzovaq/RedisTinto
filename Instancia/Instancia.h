/*
 *
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <commons/collections/list.h>


#define PORT 3490 // puerto al que vamos a conectar

#define MAXDATASIZE 100 // máximo número de bytes que se pueden leer de una vez
#define TAMANIO_NOMBREPROCESO 40
#define TAMANIO_CLAVE 41



//ESTO DEBERIA ESTAR EN OTRO .H

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

typedef enum{
	OPERACION_GET = 1,
	OPERACION_SET = 2,
	OPERACION_STORE = 3
}tTipoOperacion;

typedef struct {
  tTipoOperacion tipo;
  int tamanioValor;
}__attribute__((packed)) OperaciontHeader;


typedef struct {
	tTipoOperacion tipo;
	char clave[TAMANIO_CLAVE];
	char* valor;
}__attribute__((packed)) operacionRecibida;



int main(int argc, char *argv[]);
int leerConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
int conectarSocket(int port);
int enviarHeader(int sockfd);
char* recibirMensaje(int sockfd, int bytesRecibidos);
int enviarMensaje(int sockfd, char* mensaje);
operacionRecibida *recibirOperacion(int socketCoordinador, int bytesValor);
void mostrarArray(char **arrayEntradas, int cantidadEntradas);
OperaciontHeader *recibirHeader(int socketCoordinador);

void agregarEntrada(operacionRecibida *unaOperacion, char ** arrayEntradas, int cantidadEntradas, int tamanioValor, t_list *tablaEntradas, int tamanioValorRecibido);
void procesarMensaje(tMensaje *unMensaje, t_list *tablaEntradas);
void mostrarLista(t_list *miTabla);
void procesarInstruccion(tMensaje *unMensaje, char **arrayEntradas, int cantidadEntradas, int tamanioValor, t_list *tablaEntradas, int socketCoordinador);
int calcularLongitudMaxValorBuscado(char *unaClave,t_list *tablaEntradas);
int calcularIteracion(int valorSobrePasado, int tamanioValor);
void agregarNodoAtabla(t_list *tablaEntradas, int nroEntradaStorage, int tamanioAlmacenado, char *nombreClave);
char *obtenerValor(int longitudMaximaValorBuscado, t_list *tablaEntradas, char *claveBuscada,char **arrayEntradas,int tamanioValor);

t_list* list_duplicate(t_list* self);
bool validarClaveExistente(char *unaClave, t_list *tablaEntradas);
t_list *filtrarLista(char *unaClave, t_list *tabla);
void destruirNodoDeTabla(tEntrada *unaEntrada);
void eliminarNodos(char *nombreClave, t_list *tablaEntradas);
