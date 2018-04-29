/*
 * Coordinador.h
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */


#define PORT 3490 // puerto al que vamos a conectar

#define MAXDATASIZE 100 // máximo número de bytes que se pueden leer de una vez





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
	GET = 3
}tTipoDeMensaje;


typedef struct{
	tTipoDeProceso tipoProceso;
	tTipoDeMensaje tipoMensaje;
	int idProceso;
}tHeader;

typedef struct{
	char clave[40];
	char valor[40];
//	int numeroEntrada;
//	int tamanioAlmacenado;
}tEntrada;

typedef struct{
	tHeader encabezado;
	tEntrada entrada;
}tMensaje;

struct link_element{
	tEntrada info;
	struct link_element *next;
};
typedef struct link_element t_link_element;

//Prototipos de funciones

int main(int argc, char *argv[]);
int leerConfiguracion();
int verificarParametrosAlEjecutar(int argc, char *argv[]);
int conectarSocket(int port);
int enviarHeader(int sockfd);
char* recibirMensaje(int sockfd);
int enviarMensaje(int sockfd, char* mensaje);
tMensaje *recibirInstruccion(int sockfd);
