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

    	InicializarListasYColas();

        EscucharConexiones(sockfd);

        close(sockfd);

        return 0;
    }

    int EscucharConexiones(int sockfd){
        int sin_size;
        int new_fd;
		sem_t * semaforoNuevo;
        struct sockaddr_in direccion_cliente; // información sobre la dirección del cliente

        //Inicializamos el mutex
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

        if (pthread_mutex_init(&mutex, NULL) == -1){
        	perror("error al crear mutex");
        	exit_gracefully(1);
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
    				if (sem_init(&semaforoNuevo,0,0) != 0){
    					perror("semaforo nuevo");
    		        	exit_gracefully(1);
    				} // Inicializo el semaforo en 0

    				parametrosConexion parametros = {new_fd,semaforoNuevo};
    				printf("Socket %d \n",new_fd);
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

		return 1;
    }

    int *gestionarConexion(parametrosConexion *parametros){ //(int* sockfd, int* new_fd)
        puts("Se disparo un hilo");
        int bytesRecibidos;
               tHeader *headerRecibido = malloc(sizeof(tHeader));

        puts("Se espera un identificador");

	   if ((bytesRecibidos = recv(parametros->new_fd, headerRecibido, sizeof(tHeader), 0)) <= 0){
		perror("recv");
		log_info(logger, "Mensaje: recv error");//process_get_thread_id()); //asienta error en logger y corta
		exit_gracefully(1);
					//exit(1);
	   }

	   if (headerRecibido->tipoMensaje == CONECTARSE){
		puts("Se procede a identificar el proceso");
		IdentificarProceso(headerRecibido, parametros);
		free(headerRecibido);
	   }

		return 1;
    }

    int IdentificarProceso(tHeader* headerRecibido, parametrosConexion* parametros) {
    	switch (headerRecibido->tipoProceso) {
    	case ESI:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
    		list_add(colaESIS,(void*)&parametros);
    		conexionESI(parametros);
    		break;
    	case PLANIFICADOR:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
    		conexionPlanificador(parametros);
    		planificador = parametros;
    		break;
    	case INSTANCIA:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
    		list_add(colaInstancias,(void*)&parametros);
    		conexionInstancia(parametros);
    		break;
    	default:
    		puts("Error al intentar conectar un proceso");
    		close(parametros->new_fd);
    	}
		return 1;
    }

    int *conexionESI(parametrosConexion* parametros){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("ESI conectandose");
        // todo RECIBIR UNA OPERACION DEL ESI APLICANDO EL PROTOCOLO (Esto deberia ir en un while para leer varias operaciones)
        while(1){

			OperaciontHeader * header = malloc(sizeof(OperaciontHeader));

			int recvHeader;
			if ((recvHeader = recv(parametros->new_fd, header, sizeof(OperaciontHeader), 0)) <= 0) {
				perror("recv");
				log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
				exit_gracefully(1);
			}
			// Podriamos recibir una estructura que nos indique el tipo y tamanio de los mensajes y despues recibir los mensajes por separado
			puts("INTENTO RECIBIR TAMANIO VALOR");
			int tamanioValor = header->tamanioValor;
			puts("RECIBI TAMANIO VALOR");
			OperacionAEnviar * operacion = malloc(sizeof(tTipoOperacion)+TAMANIO_CLAVE+tamanioValor);
			puts("EL MALLOC ANDUVO");

			// Segun el tipo de operacion que sea, cargamos el mensaje en una estructura
			AnalizarOperacion(tamanioValor, header, parametros, operacion);
			puts("El analizar operacion anduvo");

			// Debo avisar a una Instancia cuando recibo una operacion (QUE NO SEA UN GET)

			//Agregamos el mensaje a una cola en memoria
			pthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola
			list_add(colaMensajes,(void*)&operacion);
			pthread_mutex_unlock(&mutex);
			free(operacion);

			while(list_is_empty(colaInstancias)) ; // mientras la cola este vacia no puedo continuar

			pthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola

			parametrosConexion * instancia = list_take(colaInstancias,1); //saco la primer instancia de la cola pero luego tengo que deolverla a la cola
			sem_post(instancia->semaforo); // Le aviso al semaforo de la instancia de que puede operar (el semaforo es el tid)
			list_add(colaInstancias,(void*)&instancia);

			pthread_mutex_unlock(&mutex);

			// Semaforo por si llegaran a entrar mas de un ESI

			free(header);

			//esperamos el resultado para devolver
			while(list_is_empty(colaResultados)) ; // mientras la cola este vacia no puedo continuar
			tResultado * resultado = list_take(colaResultados,1);
			log_info(logger,resultado);
			int sendResultado;
			if ((sendResultado =send(parametros->new_fd, (void*)resultado->resultado, sizeof(resultado), 0)) <= 0){
				perror("send");
				exit_gracefully(1);
			}
        }
		close(parametros->new_fd);
		return 1;
    }

    int *conexionPlanificador(parametrosConexion* parametros){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("Planificador conectandose");
        int mensajeFalopa;
		if ((mensajeFalopa = send(parametros->new_fd, "Hola papa!\n", 14, 0)) <= 0)
			perror("send");
        int numbytes,tamanio_buffer=100;
        char buf[tamanio_buffer]; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(parametros->new_fd, buf, tamanio_buffer-1, 0)) <= 0) {
            perror("recv");
            exit_gracefully(1);
        }

        buf[numbytes] = '\0';
        printf("Received: %s\n",buf);
        free(buf[numbytes]);

        // Le avisamos al planificador cada clave que se bloquee y desbloquee
        while(1){
        	sem_wait(planificador->semaforo); // Me van a avisar si se produce algun bloqueo

        }

		close(parametros->new_fd);
		return 1;
    }

    int *conexionInstancia(parametrosConexion* parametros){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("Instancia conectandose");
        int handshake;
		if ((handshake = send(parametros->new_fd, "Hola papa!\n", 14, 0)) <= 0)
			perror("send");
        int numbytes,tamanio_buffer=100;
        char buf[tamanio_buffer]; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(parametros->new_fd, buf, tamanio_buffer-1, 0)) <= 0) {
            perror("recv");
            exit_gracefully(1);
        }

        buf[numbytes] = '\0';
        printf("Received: %s\n",buf);
        free(buf[numbytes]);

        sem_wait(parametros->semaforo); // Caundo me avisen que hay una operacion para enviar, la voy a levantar de la cola
        OperacionAEnviar * operacion = list_take(colaMensajes,1);
        int tamanioValor = strlen(operacion->valor);
        tTipoOperacion tipo = operacion->tipo;
        OperaciontHeader * header = malloc(sizeof(OperaciontHeader));  // Creo el header que le voy a enviar a la instancia para que identifique la operacion
        header->tipo = tipo;
        header->tamanioValor = tamanioValor;

        // Envio el header a la instancia
        int sendHeader;
		if ((sendHeader = send(parametros->new_fd, header, sizeof(header), 0)) <= 0){
			perror("send");
			exit_gracefully(1);
		}

		free(header);

		// Envio la clave
		int sendClave;
		if ((sendClave = send(parametros->new_fd, operacion->clave, TAMANIO_CLAVE, 0)) <= 0){
			perror("send");
			exit_gracefully(1);
		}


		if(tipo == OPERACION_SET){
			int sendSet;
			if ((sendSet = send(parametros->new_fd, operacion->valor, tamanioValor, 0)) <= 0)
				perror("send");
			exit_gracefully(1);
		}

		// Espero el resultado de la operacion
		tResultadoOperacion * resultado = malloc(sizeof(tResultadoOperacion));
		int resultadoRecv;
        if ((resultadoRecv = recv(parametros->new_fd, resultado, sizeof(tResultadoOperacion), 0)) <= 0) {
            perror("recv");
            exit_gracefully(1);
        }

        if (resultado == BLOQUEO){
        	tBloqueo esiBloqueado = {1,operacion->clave};
        	list_add(colaBloqueos,(void*)&esiBloqueado);
        	sem_post(planificador->semaforo);
        }

        tResultado * resultadoCompleto = {resultado,operacion->clave};

        free(resultado);

        //Debo avisarle al ESI que me invoco el resultado
        list_add(colaResultados,(void*)&resultadoCompleto);

		close(parametros->new_fd);
		return 1;
    }

    int AnalizarOperacion(int tamanioValor,
    			OperaciontHeader* header, parametrosConexion* parametros,
    			OperacionAEnviar* operacion) {
    		// Segun el tipo de operacion que sea, cargamos el mensaje en una estructura

			if(header->tipo == OPERACION_GET) puts("ES UN GET");
			else if(header->tipo == OPERACION_SET) puts("ES UN SET");
			else if(header->tipo == OPERACION_STORE) puts("ES UN STORE");

    		switch (header->tipo) {
    		case OPERACION_GET: // PARA EL GET NO HAY QUE ACCEDER A NINGUNA INSTANCIA
    			puts("MANEJO UN GET");
    			ManejarOperacionGET(parametros, operacion);
    			puts("YA MANEJE UN GET");
    			break;
    		case OPERACION_SET:
    			puts("MANEJO UN SET");
    			ManejarOperacionSET(tamanioValor, parametros, operacion);
    			puts("ya maneje un SET");
    			break;
    		case OPERACION_STORE:
    			puts("MANEJO UN STORE");
    			ManejarOperacionSTORE(parametros, operacion);
    			puts("YA MANEJE UN STORE");
    			break;
    		default:
    			puts("ENTRO POR EL DEFAULT");
    			break;
    		}
    		return 1;
    	}

	int ManejarOperacionGET(parametrosConexion* parametros, OperacionAEnviar* operacion) {

		char clave[TAMANIO_CLAVE];
		int recvClave;

		if ( (recvClave = recv(parametros->new_fd, clave, TAMANIO_CLAVE - 1, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			exit_gracefully(1);
		}

		clave[strlen(clave)+1] = '\0';
		printf("Recibi la clave: %s \n", clave);
		operacion->tipo = OPERACION_GET;
		operacion->clave = clave;
		operacion->valor = NULL;
		char* GetALoguear[sizeof(operacion)];
		strcpy(GetALoguear, "GET ");
		puts(GetALoguear);
		strcat(GetALoguear, clave);
		GetALoguear[4+strlen(clave)+1]='\0';
		puts(GetALoguear);

		log_info(logger, GetALoguear);

		return 1;
	}

	int ManejarOperacionSET(int tamanioValor, parametrosConexion* parametros, OperacionAEnviar* operacion) {

		char clave[TAMANIO_CLAVE];
		int recvClave, recvValor;
		if ((recvClave = recv(parametros->new_fd, clave, TAMANIO_CLAVE - 1, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			exit_gracefully(1);
		}
		clave[TAMANIO_CLAVE] = '\0';
		printf("Recibi la clave: %s\n", clave);
		char valor[tamanioValor];
		if (( recvValor = recv(parametros->new_fd, valor, tamanioValor - 1, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			exit_gracefully(1);
		}
		clave[tamanioValor] = '\0';
		printf("Recibi la clave: %s\n", valor);
		operacion->tipo = OPERACION_SET;
		operacion->clave = clave;
		operacion->valor = valor;
		char* SetALoguear[sizeof(operacion)];
		strncpy(SetALoguear, "SET ", 4);
		puts(SetALoguear);
		strncpy(SetALoguear, clave, strlen(clave));
		puts(SetALoguear);
		strncpy(SetALoguear, valor, strlen(valor));
		log_info(logger, SetALoguear);

		return 1;
	}

	int ManejarOperacionSTORE(parametrosConexion* parametros, OperacionAEnviar* operacion) {

		char clave[TAMANIO_CLAVE];
		int resultadoRecv;
		if ((resultadoRecv =recv(parametros->new_fd, clave, TAMANIO_CLAVE - 1, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			exit_gracefully(1);
		}
		clave[TAMANIO_CLAVE] = '\0';
		printf("Recibi la clave: %s\n", clave);
		char* StoreALoguear[sizeof(operacion)];
		strncpy(StoreALoguear, "STORE ", 6);
		puts(StoreALoguear);
		strncpy(StoreALoguear, clave, strlen(clave));
		puts(StoreALoguear);
		log_info(logger, StoreALoguear);

		return 1;
	}

	int InicializarListasYColas() {
		// Creamos una cola donde dejamos todas las instancias que se conectan con nosotros y otra para los mensajes recibidos de cualquier ESI
		colaInstancias = list_create();
		colaMensajes = list_create();
		colaResultados = list_create();
		colaESIS = list_create();
		colaBloqueos = list_create();

		return 1;
	}

    int exit_gracefully(int return_nr) {

       log_destroy(logger);
       exit(return_nr);

		return return_nr;
     }

    int configure_logger() {
      logger = log_create("Coordinador.log", "CORD", true, LOG_LEVEL_INFO);

      return 1;
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





