#include <commons/txt.h>
#include <parsi/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/config.h>
#include <sys/socket.h>

//#define PORT_COORDINADOR 3490 // puerto del coordinador, donde nos vamos a conectar
//#define PORT_PLANIFICADOR 3491 // puerto del planificador, donde nos vamos a conectar
#define TAMANIO_CLAVE 41 //Por enunciado la clave sera de 40 caracteres.
#define TAMANIO_NOMBREPROCESO 40
#define ARCHIVO_CONFIGURACION "configuracion.config"
int PORT_COORDINADOR;
int PORT_PLANIFICADOR;
char* IPPL;
char* IPCO;
static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/ESI/configuracion.config";
struct hostent *he;
struct sockaddr_in their_addr; // información de la dirección de destino
char MINOMBRE[40];


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

typedef enum{
	OPERACION_GET = 1,
	OPERACION_SET = 2,
	OPERACION_STORE = 3
}tTipoOperacion;

typedef struct {
	tTipoOperacion tipo;
	int tamanioValor;
}OperaciontHeader; // Header que vamos a recibir de parte del ESI para identificar la operacion

typedef struct {
	tTipoOperacion tipo;
	char* clave;
	char* valor;
}OperacionAEnviar; // Operacion que vamos a enviar a la instancia

typedef enum{
	OK = 1,
	BLOQUEO = 2,
	ERROR = 3,
	CHAU = 0
}tResultadoOperacion;

typedef enum{
	OPERACION_VALIDA = 1,
	OPERACION_INVALIDA= -1
}tValidezOperacion;


typedef struct{
	tResultadoOperacion tipoResultado;
	char clave[TAMANIO_CLAVE];
}__attribute__((packed)) tResultado;



int main(int argc, char *argv[]);
int leerConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
int conectarmeYPresentarme(int port);
int enviarHeader(int sockfd);
int recibirMensaje(int sockfd);
int enviarMensaje(int sockfd, char* mensaje);
int enviarOperaciontHeader(int sockfd, OperaciontHeader* header);
FILE *leerArchivo(char **argv);
int manejarArchivoConScripts(FILE * file, int socket_coordinador, int socket_planificador);
int manejarOperacionGET(int socket_coordinador, char clave[TAMANIO_CLAVE], OperacionAEnviar* operacion, OperaciontHeader * header);
int manejarOperacionSET(int socket_coordinador, char clave[TAMANIO_CLAVE], char *valor, OperacionAEnviar* operacion, OperaciontHeader *header);
int manejarOperacionSTORE(int socket_coordinador, char clave[TAMANIO_CLAVE], OperacionAEnviar* operacion, OperaciontHeader *header);
int enviarValor(int sockfd, char* valor);
int enviarValidez(int sockfd, tValidezOperacion *validez);
int recibirResultadoDelCoordinador(int sockfd, tResultado * resultado);
int recibirResultado(int socket_coordinador, tResultado * resultado);
int recibirOrdenDeEjecucion(int socket_planificador);
