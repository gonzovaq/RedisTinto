/*
 * Coordinador.c
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */

#include "Coordinador.h"


    void sigchld_handler(int s) // eliminar procesos muertos
    {
        while(wait(NULL) > 0);
    }

    int main(void)
    {
    	// Ejemplo para leer archivo de configuracion en formato clave=valor por linea
    	char *token;
    	char *search = "=";
    	 static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/Coordinador/src/configuracion.config";
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


    	configure_logger();
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

        //Armo una cola para las instancias vacia
        struct node_t * head = NULL;
        head = malloc(sizeof(node_t));
        if (head == NULL) {
            return 1;
        }

        while(1) {  // main accept() loop
            sin_size = sizeof(struct sockaddr_in);
            if ((new_fd = accept(sockfd, (struct sockaddr *)&direccion_cliente,
                                                           &sin_size)) == -1) {
                perror("accept");
                //continue;
            }
            printf("server: recibi una conexion de  %s\n",
                                               inet_ntoa(direccion_cliente.sin_addr));

            //Para hilos debo crear una estructura de parametros de la funcion que quiera llamar
			pthread_t tid;
			struct parametrosConexion parametros = {new_fd,head};//(&sockfd, &new_fd) --> no lo utilizo porque sockfd ya no se requiere

            //Con Pooltread !
			/* No se aplica porque pierde memoria
            threadpool thpool = thpool_init(4);
            thpool_add_work(thpool,(void*)gestionarConexion, (void*)&parametros);
			*/

			// Sin Poolthread !
            int stat = pthread_create(&tid, NULL, (void*)gestionarConexion, (void*)&parametros);//(void*)&parametros -> parametros contendria todos los parametros que usa conexion
			if (stat != 0){
				puts("error al generar el hilo");
				perror("thread");
				//continue;
			}
			pthread_detach(tid); //Con esto decis que cuando el hilo termine libere sus recursos
        }

        close(sockfd);

        return 0;
    }

    void *gestionarConexion(struct parametrosConexion *parametros){ //(int* sockfd, int* new_fd)
        puts("Se disparo un hilo");

//        int bytesRecibidos,bytesIdentificador = 50;
//        //Handshake --> El coordinador debe recibir el primer caracter del proceso que se conecte para manejar la comunicacion adecuandamente
//        char identificador[bytesIdentificador];
//        if ((bytesRecibidos =recv(parametros->new_fd,identificador,bytesIdentificador-1,NULL)) == -1){
//            perror("recv");
//            exit(1);
//        }
//
//        puts(identificador);
//
//
//        switch (identificador[0])
//        {
//			case 'e':
//				conexionESI(parametros->new_fd);
//				break;
//			case 'p':
//				conexionPlanificador(parametros->new_fd);
//				break;
//			case 'i':
//				conexionInstancia(parametros->new_fd);
//				break;
//			default :
//				close(parametros->new_fd);
//        }
//        //free(identificador);
        int bytesRecibidos;
               tHeader *headerRecibido = malloc(sizeof(tHeader));

               if ((bytesRecibidos = recv(parametros->new_fd, headerRecibido, sizeof(tHeader), 0)) == -1){
               	perror("recv");
               	log_info(logger, "Mensaje: recv error");//process_get_thread_id()); //asienta error en logger y corta
               	exit_gracefully(1);
               	            //exit(1);
               }
               if (headerRecibido->tipoMensaje == CONECTARSE){
               	switch(headerRecibido->tipoProceso){
               				case ESI:
               					printf("Se conecto el proceso %d \n",headerRecibido->idProceso);
               					conexionESI(parametros->new_fd);
       							break;
               				case PLANIFICADOR:
               					printf("Se conecto el proceso %d \n",headerRecibido->idProceso);
               					conexionPlanificador(parametros->new_fd);
               					break;
               	        	case INSTANCIA:
               					printf("Se conecto el proceso %d \n",headerRecibido->idProceso);
               					push(&parametros->colaProcesos,&headerRecibido);
               	        		conexionInstancia(parametros->new_fd);
               	        		break;
               	        	default:
               	        		puts("Error al intentar conectar un proceso");
               	        		close(parametros->new_fd);
               	        }
               	free(headerRecibido);
               }
    }

    void *conexionESI(int *new_fd){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("ESI conectandose");
		if (send(new_fd, "Hola papa!\n", 14, 0) == -1)
			perror("send");
        int numbytes,tamanio_buffer=100;
        char buf[tamanio_buffer]; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(new_fd, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
            exit_gracefully(1);
        }

        buf[numbytes] = '\0';
        printf("Received: %s\n",buf);
        log_info(logger, "TID %d  Mensaje:  %s",process_get_thread_id(),buf); // que onda con pthread_self()?
        free(buf[numbytes]);
		close(new_fd);
    }

    void *conexionPlanificador(int *new_fd){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("Planificador conectandose");
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
        puts("Instancia conectandose");
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

    //logger

    void exit_gracefully(int return_nr) {

       log_destroy(logger);
       exit(return_nr);
     }

    void configure_logger() {
      logger = log_create("Coordinador.log", "CORD", true, LOG_LEVEL_INFO);
     }

    void push(node_t * head, tHeader * proceso) {
    	//Si la cola esta vacia agrego el primer proceso
    	if (head->proceso == NULL){
    		head->proceso = proceso;
    		return;
    	}

        node_t * current = head;
        while (current->next != NULL) {
            current = current->next;
        }

        /* Agrego nuevo proceso a la cola */
        current->next = malloc(sizeof(node_t));
        current->next->proceso = proceso;
        current->next->next = NULL;
    }




