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

static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/ESI/configuracion.config";
struct hostent *he;
struct sockaddr_in their_addr; // información de la dirección de destino

int main(int argc, char *argv[]);
int leerConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
int conectarSocket(int port);
int enviarHeader(int sockfd);
char* recibirMensaje(int sockfd);
int enviarMensaje(int sockfd, char* mensaje);
int leerArchivoEImprimirloEnConsola(char **argv);


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
