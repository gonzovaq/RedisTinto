	#include "ESI.h"
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
        int socket_coordinador, socket_planificador;
        char *mensaje_coordinador = "Hola Coordinador, como va?";
        char *mensaje_planificador = "Que onda Planificador?";

        char *buffer_mensaje_recibido;


    	verificarParametrosAlEjecutar(argc, argv);
    	leerConfiguracion();

    	socket_coordinador = conectarSocket(PORT_COORDINADOR);
    	buffer_mensaje_recibido = recibirMensaje(socket_coordinador);
        printf("Received: %s \n",buffer_mensaje_recibido);
        // TODO: Antes de hacer el return esta bien el mensaje, pero cuando la funcion devuelve el char*, devuelve (null). Hay que corregirlo

    	enviarMensaje(socket_coordinador, mensaje_coordinador);

    	socket_planificador = conectarSocket(PORT_PLANIFICADOR);
    	enviarMensaje(socket_planificador, mensaje_planificador);

        close(socket_coordinador);
        close(socket_planificador);
        return 0;
    }


    int leerConfiguracion(){
    	char *token;
    	char *search = "=";
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
    	else {
    		puts("Archivo de configuracion vacio");
    	}

    	return 1;
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

    int conectarSocket(int port){
        int sockfd;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        	puts("Error al crear el socket");
        	perror("socket");
            exit(1);
        }
        puts("El socket se creo correctamente\n");

        their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina
        their_addr.sin_port = htons(port);  // short, Ordenación de bytes de la red
        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
        memset(&(their_addr.sin_zero),'\0', 8);  // poner a cero el resto de la estructura

        if (connect(sockfd, (struct sockaddr *)&their_addr,
                                              sizeof(struct sockaddr)) == -1) {
        	puts("Error al conectarme al servidor.");
        	perror("connect");
            exit(1);
        }
        puts("ESI conectado!\n");

        enviarHeader(sockfd);

        return sockfd;

    }

    int enviarHeader(int sockfd){
    	int pid = getpid(); //Los procesos podrian pasarle sus PID al coordinador para que los tenga identificados
    	printf("Mi ID es %d \n",pid);
        tHeader *header = malloc(sizeof(tHeader));
               header->tipoProceso = ESI;
               header->tipoMensaje = CONECTARSE;
               header->idProceso = pid;
			   if (send(sockfd, header, sizeof(tHeader), 0) == -1){
				   puts("Error al enviar mi identificador");
				   perror("Send");
				   free(header);
				   exit(1);
			   }
			   puts("Se envió mi identificador");
			   free(header);
			   return 1;
    }

    char* recibirMensaje(int sockfd){
    	int numbytes;
        int tamanio_buffer=100;
        char buf[tamanio_buffer]; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(sockfd, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        return buf;
    }

    int enviarMensaje(int sockfd, char* mensaje){
    	int longitud_mensaje, bytes_enviados;

        longitud_mensaje = strlen(mensaje);
        if ((bytes_enviados=send(sockfd, mensaje, longitud_mensaje , 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",mensaje);
        return 1;
    }
