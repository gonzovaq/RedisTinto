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


    int main(int argc, char *argv[])
    {
        int sockfd, bytes_enviados, longitud_mensaje;
        struct hostent *he;
        struct sockaddr_in their_addr; // información de la dirección de destino
        char *mensaje = "Prueba mensaje para enviar";
        int numbytes,tamanio_buffer=100;
        char buf[tamanio_buffer]; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.


        if (argc != 2) {
        	puts("Error al ejecutar, te faltan parametros.");
            fprintf(stderr,"usage: client hostname \n");
            exit(1);
        }

        if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina
        	puts("Error al obtener el hostname, te faltan parametros.");
        	perror("gethostbyname");
            exit(1);
        }

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        	puts("Error al crear el socket");
        	perror("socket");
            exit(1);
        }
        puts("El socket se creo correctamente\n");

        their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina
        their_addr.sin_port = htons(PORT);  // short, Ordenación de bytes de la red
        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
        memset(&(their_addr.sin_zero),'\0', 8);  // poner a cero el resto de la estructura

        if (connect(sockfd, (struct sockaddr *)&their_addr,
                                              sizeof(struct sockaddr)) == -1) {
        	puts("Error al conectarme al servidor.");
        	perror("connect");
            exit(1);
        }
        puts("ESI conectado!\n");

        if ((numbytes=recv(sockfd, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        printf("Received: %s",buf);


        longitud_mensaje = strlen(mensaje);

        if ((bytes_enviados=send(sockfd, mensaje, longitud_mensaje , 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",mensaje);

        close(sockfd);
        return 0;
    }
