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
    	// Leer archivo de configuracion con las commons
    	t_config *configuracion;
    	configuracion = config_create(ARCHIVO_CONFIGURACION);
    	PUERTO =  config_get_int_value(configuracion, "port");
    	IP = config_get_string_value(configuracion, "ip");
    	printf("Mi ip es: %s \n",IP);

    	configure_logger();

        int sockfd;  // Escuchar sobre sock_fd, nuevas conexiones sobre new_fd
        struct sockaddr_in mi_direccion;    // información sobre mi dirección
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

        sa.sa_handler = sigchld_handler; // Eliminar procesos muertos
        puts("Se eliminaron los procesos muertos");
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }

        puts("A trabajar");

        EscucharConexiones(sockfd);

        close(sockfd);

        return 0;
    }

    void EscucharConexiones(int sockfd){
        int sin_size, new_fd;
        struct sockaddr_in direccion_cliente; // información sobre la dirección del cliente

        // Creamos una cola donde dejamos todas las instancias que se conectan con nosotros y otra para los mensajes recibidos de cualquier ESI
        colaInstancias = queue_create();
        colaMensajes = 	queue_create();

        //Inicializamos el mutex
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

        if (pthread_mutex_init(&mutex, NULL) == -1){
        	perror("error al crear mutex");
        	exit(1);
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
    				struct parametrosConexion parametros = {new_fd,sem_init(tid)};//(&sockfd, &new_fd) --> no lo utilizo porque sockfd ya no se requiere

    	            int stat = pthread_create(&tid, NULL, (void*)gestionarConexion, (void*)&parametros);//(void*)&parametros -> parametros contendria todos los parametros que usa conexion
    				if (stat != 0){
    					puts("error al generar el hilo");
    					perror("thread");
    					//continue;
    				}
    				pthread_detach(tid); //Con esto decis que cuando el hilo termine libere sus recursos
    	        }

        //Tenemos que destruir el mutex
        pthread_mutex_destroy(&mutex);
    }

    void *gestionarConexion(struct parametrosConexion *parametros){ //(int* sockfd, int* new_fd)
        puts("Se disparo un hilo");
        int bytesRecibidos;
               tHeader *headerRecibido = malloc(sizeof(tHeader));

		   if ((bytesRecibidos = recv(parametros->new_fd, headerRecibido, sizeof(tHeader), 0)) == -1){
			perror("recv");
			log_info(logger, "Mensaje: recv error");//process_get_thread_id()); //asienta error en logger y corta
			exit_gracefully(1);
						//exit(1);
		   }

		   if (headerRecibido->tipoMensaje == CONECTARSE){
			IdentificarProceso(headerRecibido, parametros);
			free(headerRecibido);
		   }
    }

    void IdentificarProceso(tHeader* headerRecibido,
    		struct parametrosConexion* parametros) {
    	switch (headerRecibido->tipoProceso) {
    	case ESI:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
    		conexionESI(parametros->new_fd);
    		break;
    	case PLANIFICADOR:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
    		conexionPlanificador(parametros->new_fd);
    		break;
    	case INSTANCIA:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
    		queue_push(colaInstancias,(void*)&parametros);
    		conexionInstancia(parametros->new_fd);
    		break;
    	default:
    		puts("Error al intentar conectar un proceso");
    		close(parametros->new_fd);
    	}
    }


    void *conexionESI(int *new_fd){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("ESI conectandose");
		if (send(new_fd, "Hola papa!\n", 14, 0) == -1)
			perror("send");
        int numbytes,tamanio_buffer=200;
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


        // todo RECIBIR UNA OPERACION DEL ESI APLICANDO EL PROTOCOLO

        int tamanioOperacionHeader = malloc(sizeof(struct OperacionHeader));
        struct OperacionHeader * header = tamanioOperacionHeader;

        if ((tamanioOperacionHeader=recv(new_fd, header, tamanioOperacionHeader, 0)) == -1) {
            perror("recv");
            log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
            exit_gracefully(1);
        }

        // Podriamos recibir una estructura que nos indique el tipo y tamanio de los mensajes y despues recibir los mensajes por separado
        switch(header->tipo){
        case GET:

        	int tamanioClave = header->tamanioClave;
        	char clave[tamanioClave];
            if (recv(new_fd, clave, tamanioClave-1, 0) == -1) {
                perror("recv");
                log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
                exit_gracefully(1);
            }
            clave[tamanioClave] = '\0';
            printf("Recibi la clave: %s\n",clave);
            struct OperacionAEnviar operacionParaInstancia = malloc(sizeof(struct tTipoOperacion)+tamanioClave);
        	operacionParaInstancia->tipo = GET;
            operacionParaInstancia->clave = clave;
            operacionParaInstancia->valor = NULL;
            break;
        case SET:

        	int tamanioClave = header->tamanioClave;
        	char clave[tamanioClave];
            if (recv(new_fd, clave, tamanioClave-1, 0) == -1) {
                perror("recv");
                log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
                exit_gracefully(1);
            }
            clave[tamanioClave] = '\0';
            printf("Recibi la clave: %s\n",clave);

        	int tamanioValor = header->tamanioValor;
        	char valor[tamanioValor];
            if (recv(new_fd, valor, tamanioValor-1, 0) == -1) {
                perror("recv");
                log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
                exit_gracefully(1);
            }
            clave[tamanioValor] = '\0';
            printf("Recibi la clave: %s\n",valor);
            struct OperacionAEnviar operacionParaInstancia = malloc(sizeof(struct tTipoOperacion) +tamanioClave+tamanioValor);
        	operacionParaInstancia->tipo = SET;
            operacionParaInstancia->clave = clave;
            operacionParaInstancia->valor = valor;

            break;
        case STORE:
        	operacionParaInstancia->tipo = STORE;
        	int tamanioClave = header->tamanioClave;
        	char clave[tamanioClave];
            if (recv(new_fd, clave, tamanioClave-1, 0) == -1) {
                perror("recv");
                log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
                exit_gracefully(1);
            }
            clave[tamanioClave] = '\0';
            printf("Recibi la clave: %s\n",clave);
            struct OperacionAEnviar operacionParaInstancia = malloc(sizeof(struct tTipoOperacion)+tamanioClave);
        	operacionParaInstancia->tipo = STORE;
            operacionParaInstancia->clave = clave;
            operacionParaInstancia->valor = NULL;
            break;
        }
	    // Debo avisar a una Instancia cuando recibo una operacion

        while(queue_is_empty(colaInstancias)) ; // mientras la cola este vacia no puedo continuar

        pthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola

        struct operacionParaInstancia operacion = {1,sizeof(buf),buf};
        queue_push(colaMensajes,(void*)&operacionParaInstancia);

        struct parametrosConexion * instancia = queue_pop(colaInstancias); //saco la primer instancia de la cola pero luego tengo que deolverla a la cola
        sem_post(instancia->semaforo); // Le aviso al semaforo de la instancia de que puede operar (el semaforo es el tid)
        queue_push(colaInstancias,(void*)&instancia);

        pthread_mutex_unlock(&mutex);

        // Semaforo por si llegaran a entrar mas de un ESI

        free(tamanioOperacionHeader);

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

        sem_wait(pthread_self()); // Caundo me avisen que hay una operacion para enviar, la voy a levantar de la cola
        struct operacionParaInstancia * operacion = queue_pop(colaMensajes);

		close(new_fd);
    }

    void exit_gracefully(int return_nr) {

       log_destroy(logger);
       exit(return_nr);
     }

    void configure_logger() {
      logger = log_create("Coordinador.log", "CORD", true, LOG_LEVEL_INFO);
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





