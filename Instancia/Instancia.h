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
	CONECTARSE = 1
}tTipoDeMensaje;


typedef struct{
	tTipoDeProceso tipoProceso;
	tTipoDeMensaje tipoMensaje;

}tHeader;
