	#include "Listas.h"
	#include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <errno.h>
    #include <string.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>

    struct hostent *he;
    struct sockaddr_in their_addr; // información de la dirección de destino

    int main(int argc, char *argv[])
    {

        t_list *tablaDeEntradas = list_create();
    	int socketCoordinador;
        char *buffer_mensaje_recibido;
        char *mensaje_coordinador = "Hola Coordinador, como va?";
        tMensaje *mensajeInstruccionRecibido = malloc(sizeof(tMensaje));


        verificarParametrosAlEjecutar(argc, argv);
        leerConfiguracion();

        socketCoordinador = conectarmeYPresentarme(PORT);
    	buffer_mensaje_recibido = recibirMensaje(socketCoordinador);
        printf("Received: %s \n",buffer_mensaje_recibido);
        free(buffer_mensaje_recibido);
        enviarMensaje(socketCoordinador, mensaje_coordinador);

        mensajeInstruccionRecibido = recibirInstruccion(socketCoordinador);
        procesarInstruccion(mensajeInstruccionRecibido, tablaDeEntradas);
        mostrarTabla(tablaDeEntradas);




        close(socketCoordinador);
        return 0;
    }

    int verificarParametrosAlEjecutar(int argc, char *argv[]){
    	if (argc != 2) {
    		puts("Error al ejecutar, te faltan parametros.");
    	    exit(1);
    	 }


    	 if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina
    	    puts("Error al obtener el hostname, te faltan parametros.");
    	    perror("gethostbyname");
    	    exit(1);
    	 }
    	 return 1;
    }

    int leerConfiguracion(){
    	char *token;
    	char *search = "=";
    	static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/Instancia/configuracion.config";
    	FILE *file = fopen ( filename, "r" );
    	if ( file != NULL )
    	{
    		puts("Leyendo archivo de configuracion");
    	    char line [ 128 ]; /* or other suitable maximum line size */
    	    while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */
    	    {
    	    // Token will point to the part before the =.
    	    	token = strtok(line, search);
    	    	puts(token);
    	    // Token will point to the part after the =.
    	    	token = strtok(NULL, search);
    	    	puts(token);
    	    }
    	    fclose ( file );
    	 }
    	 else
    		 puts("Archivo de configuracion vacio");

        	return 1;
        }

    int conectarmeYPresentarme(int port){
    	int socketCoordinador;
    	if ((socketCoordinador = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    		puts("Error al crear el socket");
    		perror("socket");
    		exit(1);
    	}

    	puts("El socket se creo correctamente\n");

    	their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina
    	their_addr.sin_port = htons(port);  // short, Ordenación de bytes de la red
    	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    	memset(&(their_addr.sin_zero),'\0', 8);  // poner a cero el resto de la estructura

    	if (connect(socketCoordinador, (struct sockaddr *)&their_addr,
    	                                              sizeof(struct sockaddr)) == -1) {
    		puts("Error al conectarme al servidor.");
    		perror("connect");
    		exit(1);
    	}

    	puts("Instancia conectada!\n");

    	enviarHeader(socketCoordinador);

    	return socketCoordinador;

    	}

    int enviarHeader(int socketCoordinador){
    	int pid = getpid(); //Los procesos podrian pasarle sus PID al coordinador para que los tenga identificados
    	printf("Mi ID es %d \n",pid);
        tHeader *header = malloc(sizeof(tHeader));
               header->tipoProceso = INSTANCIA;
               header->tipoMensaje = CONECTARSE;
               header->idProceso = pid;
			   if (send(socketCoordinador, header, sizeof(tHeader), 0) == -1){
				   puts("Error al enviar mi identificador");
				   perror("Send");
				   free(header);
				   exit(1);
			   }
			   puts("Se envió mi identificador");
			   free(header);
			   return 1;
    }

    char *recibirMensaje(int socketCoordinador){
    	int numbytes;
        int tamanio_buffer=100;
        char* buf = malloc(tamanio_buffer) ; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(socketCoordinador, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        *(buf + numbytes) = '\0';
        return buf;
    }

    int enviarMensaje(int socketCoordinador, char* mensaje){
    	int longitud_mensaje, bytes_enviados;

        longitud_mensaje = strlen(mensaje);
        if ((bytes_enviados=send(socketCoordinador, mensaje, longitud_mensaje , 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",mensaje);
        return 1;
    }

    tMensaje *recibirInstruccion(int socketCoordinador){
    	tMensaje *mensajeRecibido = malloc(sizeof(tMensaje));
    	int numBytes;

    	if ((numBytes = recv(socketCoordinador, mensajeRecibido, sizeof(tMensaje), 0)) == -1 ){
    		perror("Recv");
    		exit(1);
    	}
    	return mensajeRecibido;
    }

    void procesarInstruccion(tMensaje *unMensaje, t_list *tablaDeEntradas){
    	tEntrada *buffer = malloc(sizeof(tEntrada));
    	*buffer = unMensaje->entrada;
    	if (unMensaje->encabezado.tipoProceso != COORDINADOR){
    		puts("El mensaje no se recibio desde el tipo de proceso indicado");
    	}

    	switch(unMensaje->encabezado.tipoMensaje){
    	case SET:
    		list_add(tablaDeEntradas, buffer);
    		break;
    	default:
    		puts("Nada para hacer");
    	}

    	free(buffer);
    	return;
    }

    void mostrarTabla(t_list *tablaDeEntradas){

    	while(tablaDeEntradas->head->next){
    		puts("Clave:");
    		puts(tablaDeEntradas->head->info.clave);
    		puts("Valor:");
    		puts(tablaDeEntradas->head->info.valor);
    		tablaDeEntradas->head = tablaDeEntradas->head->next;
    	}

    	return;
    }
