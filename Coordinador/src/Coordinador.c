/*
 * Coordinador.c
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */

#include "Coordinador.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <errno.h>
    #include <string.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/wait.h>
    #include <signal.h>

    void sigchld_handler(int s) // eliminar procesos muertos
    {
        while(wait(NULL) > 0);
    }

    int main(void)
    {
        int sockfd, new_fd;  // Escuchar sobre sock_fd, nuevas conexiones sobre new_fd
        struct sockaddr_in mi_direccion;    // información sobre mi dirección
        struct sockaddr_in direccion_cliente; // información sobre la dirección del cliente
        int sin_size;
        struct sigaction sa;
        int yes=1;

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        mi_direccion.sin_family = AF_INET;         // Ordenación de bytes de la máquina
        mi_direccion.sin_port = htons(MYPORT);     // short, Ordenación de bytes de la red
        mi_direccion.sin_addr.s_addr = INADDR_ANY; // Rellenar con mi dirección IP
        memset(&(mi_direccion.sin_zero), '\0', 8); // Poner a cero el resto de la estructura

        // si escucho (listen()) debo conocer los puertos y aceptar conexiones y bindearlas (bind())

        if (bind(sockfd, (struct sockaddr *)&mi_direccion, sizeof(struct sockaddr))
                                                                       == -1) {
            perror("bind");
            exit(1);
        }

        if (listen(sockfd, BACKLOG) == -1) {
            perror("listen");
            exit(1);
        }
        puts("Escuchamos");

        sa.sa_handler = sigchld_handler; // Eliminar procesos muertos
        puts("Se eliminaron los procesos muertos");
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }

		// CERRAR CONEXIONES
		// close() y shutdown()
		// close(sockfd); -> cierra la conexion en ambos sentidos
		// int shutdown(int sockfd, int how); -> permite mas control al cerrar la conexion
		// how -> 0(no se permite recibir mas datos), 1(no se permite enviar mas datos), 2(no se permite ni recibir ni enviar mas datos, lo mismo que close())
		// shutdown devuelve 0 si es un exito, -1 si da error

		// getpeername()
		// te dice quien esta del otro lado de un socket
		// int getpeername(int sockfd, struct sockaddr *addr, int *addrlen);
		// sockfd es el descriptor del socket de flujo conectado, addr es un puntero a una struct de sockadd que guarda la info acerca del otro lado de la conexion, y addrlen es un puntero a un int que se debe inicializar a sizeof(struct sockaddr)
		// devuelve -1 en caso de error

		// gethostname() -> devuelve el nombre del ordenador sobre el que esta corriendo el programa
		// gethostbyname() -> obtiene la direccion ip de mi maquina local
		// int gethostbyname(char *hostname, size_t size);
		// hostname es un puntero a una cadena de caracteres donde se almacena el nombre de la maquina cuando la funcion retorne y size es la longitud de bytes de esa cadena de caracteres
		// 0 -> ok, -1 -> error

        puts("A trabajar");
        // El coordinador va a disparar hilos!
        while(1) {  // main accept() loop
            sin_size = sizeof(struct sockaddr_in);
            if ((new_fd = accept(sockfd, (struct sockaddr *)&direccion_cliente,
                                                           &sin_size)) == -1) {
                perror("accept");
                continue;
            }
            printf("server: recibi una conexion de  %s\n",
                                               inet_ntoa(direccion_cliente.sin_addr));
            if (!fork()) { // Este es el proceso hijo
                close(sockfd); // El hijo no necesita este descriptor
                if (send(new_fd, "Hola papa!\n", 14, 0) == -1)
                    perror("send");
                //close(new_fd);
                exit(0);
            }
            //close(new_fd);  // El proceso padre no lo necesita
        }

        return 0;
    }

