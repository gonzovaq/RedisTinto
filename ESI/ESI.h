/*
 * Coordinador.h
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */

#define PORT_COORDINADOR 3490 // puerto del coordinador, donde nos vamos a conectar
#define PORT_PLANIFICADOR 3491 // puerto del planificador, donde nos vamos a conectar

static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/ESI/configuracion.config";

int main(int argc, char *argv[]);
int leerConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
int conectarSocket(int port);
int enviarHeader(int sockfd);
char* recibirMensaje(int sockfd);
int enviarMensaje(int sockfd, char* mensaje);



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
