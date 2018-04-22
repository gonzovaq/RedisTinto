	#include "Instancia.h"
	#include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <errno.h>
    #include <string.h>
    #include <netdb.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <sys/socket.h>


    int main(int argc, char *argv[])
    {
        int socketCoordinador, numbytes;
        char buf[MAXDATASIZE];
        struct hostent *he;
        struct sockaddr_in their_addr; // información de la dirección de destino

        if (argc != 2) {
            fprintf(stderr,"usage: client hostname\n");
            exit(1);
        }

        if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina
            perror("gethostbyname");
            exit(1);
        }

        if ((socketCoordinador = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

        their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina
        their_addr.sin_port = htons(PORT);  // short, Ordenación de bytes de la red
        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
        memset(&(their_addr.sin_zero),'\0', 8);  // poner a cero el resto de la estructura

        if (connect(socketCoordinador, (struct sockaddr *)&their_addr,
                                              sizeof(struct sockaddr)) == -1) {
            perror("connect");
            exit(1);
        }

//        //Me identifico con el coordinador
//        char *mensaje = "i";
//        int longitud_mensaje = strlen(mensaje);
//        if (send(socketCoordinador, mensaje, longitud_mensaje, 0) == -1) {
//        	puts("Error al enviar el mensaje.");
//        	perror("send");
//            exit(1);
//        }

        tHeader *header = malloc(sizeof(tHeader));
        header->tipoProceso = INSTANCIA;
        header->tipoMensaje = CONECTARSE;
            if (send(socketCoordinador, header, sizeof(tHeader), 0) == -1){
           	   puts("Error al enviar mi identificador");
           	   perror("Send");
           	   exit(1);
           	   free(header);
              }


        if ((numbytes=recv(socketCoordinador, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';

        printf("Received: %s",buf);


        close(socketCoordinador);

        return 0;
    }
