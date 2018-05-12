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
#include <sys/socket.h>

#define PORT_COORDINADOR 3490 // puerto del coordinador, donde nos vamos a conectar
#define PORT_PLANIFICADOR 3491 // puerto del planificador, donde nos vamos a conectar
#define TAMANIO_CLAVE 41 //Por enunciado la clave sera de 40 caracteres.


static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/ESI/configuracion.config";
struct hostent *he;
struct sockaddr_in their_addr; // información de la dirección de destino


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
	ERROR = 3
}tResultadoOperacion;



typedef struct{
	tResultadoOperacion resultado;
	char* clave;
}tResultado;



int main(int argc, char *argv[]);
int leerConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
int conectarmeYPresentarme(int port);
int enviarHeader(int sockfd);
int recibirMensaje(int sockfd);
int enviarMensaje(int sockfd, void* mensaje);
FILE *leerArchivo(char **argv);
int manejarArchivoConScripts(FILE * file, int socket_coordinador, int socket_planificador);
int recibirOrdenDeEjecucion(socket_planificador);
int recibirResultado(int socket_coordinador);
int manejarOperacionGET(int socket_coordinador, char clave[TAMANIO_CLAVE], OperacionAEnviar* operacion, OperaciontHeader * header);
int manejarOperacionSET(int socket_coordinador, char clave[TAMANIO_CLAVE], char *valor, OperacionAEnviar* operacion, OperaciontHeader *header);
int manejarOperacionSTORE(int socket_coordinador, char clave[TAMANIO_CLAVE], OperacionAEnviar* operacion, OperaciontHeader *header);

