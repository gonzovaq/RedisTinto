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
#include <pthread.h>

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

        while(1) {  // main accept() loop
            sin_size = sizeof(struct sockaddr_in);
            if ((new_fd = accept(sockfd, (struct sockaddr *)&direccion_cliente,
                                                           &sin_size)) == -1) {
                perror("accept");
                //continue;
            }
            printf("server: recibi una conexion de  %s\n",
                                               inet_ntoa(direccion_cliente.sin_addr));

            //if (!fork()) { // Este es el proceso hijo
            //Pruebo con hilos en vez de generar procesos hijos

            //Para hilos debo crear una estructura de parametros de la funcion que quiera llamar
			pthread_t tid;
			//struct parametrosConexion parametros = {sockfd,new_fd};//(&sockfd, &new_fd) --> no lo utilizo porque sockfd ya no se requiere

			int stat = pthread_create(&tid, NULL, (void*)gestionarConexion, (void*)&new_fd);//(void*)&parametros -> parametros contendria todos los parametros que usa conexion
			if (stat != 0){
				puts("error al generar el hilo");
				perror("thread");
				//continue;
			}

            	//gestionarConexion(&sockfd, &new_fd);
            	//pthread_join(tid, NULL); sinonimo de wait()
            //}
            //close(new_fd);  // El proceso padre no lo necesita
        }
        close(sockfd);

        return 0;
    }

    void *gestionarConexion(int *new_fd){ //(int* sockfd, int* new_fd)
        puts("Se disparo un hilo");

        int bytesRecibidos,bytesIdentificador = sizeof(char);
        //Handshake --> El coordinador debe recibir el primer caracter del proceso que se conecte para manejar la comunicacion adecuandamente
        char identificador[bytesIdentificador];
        if ((bytesRecibidos =recv(*new_fd,identificador,bytesIdentificador-1,NULL)) == -1){
            perror("recv");
            exit(1);
        }

        puts(identificador);
        puts(new_fd);

        switch (identificador[0])
        {
			case 'e':
				conexionESI(&new_fd);
				break;
			case 'p':
				conexionPlanificador(&new_fd);
				break;
			case 'i':
				conexionInstancia(&new_fd);
				break;
			default :
				close(new_fd);
        }
        free(identificador);
    }

    void *conexionESI(int *new_fd){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts(new_fd);
		if (send(new_fd, "Hola papa!\n", 14, 0) == -1)
			perror("send");
        int numbytes,tamanio_buffer=100;
        char buf[tamanio_buffer]; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(new_fd, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        printf("Received: %s\n",buf);
        free(buf[numbytes]);
		close(new_fd);
    }

    void *conexionPlanificador(int *new_fd){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts(new_fd);
		if (send(new_fd, "Hola papa!\n", 14, 0) == -1)
			perror("send");
        int numbytes,tamanio_buffer=100;
        char buf[tamanio_buffer]; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(new_fd, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        printf("Received: %s\n",buf);
        free(buf[numbytes]);
		close(new_fd);
    }

    void *conexionInstancia(int *new_fd){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts(new_fd);
		if (send(new_fd, "Hola papa!\n", 14, 0) == -1)
			perror("send");
        int numbytes,tamanio_buffer=100;
        char buf[tamanio_buffer]; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(new_fd, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        printf("Received: %s\n",buf);
        free(buf[numbytes]);
		close(new_fd);
    }

