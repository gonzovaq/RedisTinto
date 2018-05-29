/*
 * Coordinador.c
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */

#include "Coordinador.h"

    int main(void)
    {
    	// Leer archivo de configuracion con las commons
    	LeerArchivoDeConfiguracion();
    	printf("Mi ip es: %s \n",IP);

    	InicializarListasYColas();
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

    int EscucharConexiones(int sockfd){
        int sin_size;
        int new_fd;
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

    				//sem_t * semaforoNuevo = malloc(sizeof(sem_t));
    				char nombreProceso[TAMANIO_NOMBREPROCESO];

    	            //int ret;
    	            //int value;
    	            //int pshared;
    	            int entradasUsadas = 0;
    	            /* initialize a private semaphore */
    	            //pshared = 0;
    	            //value = 0;
    	            /*if ((ret = sem_init(&semaforoNuevo,pshared,value)) != 0){
    					perror("semaforo nuevo");
    		        	exit_gracefully(1);
    				} // Inicializo el semaforo en 0
    	            */
    				parametrosConexion parametros;//parametrosConexion parametros = {new_fd,malloc(sizeof(sem_t)),nombreProceso,ENTRADAS,entradasUsadas};
    				parametros.new_fd = new_fd;
    				strcpy(parametros.nombreProceso,nombreProceso);
    				parametros.cantidadEntradasMaximas = ENTRADAS;
    				parametros.entradasUsadas = 0;
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
		IdentificarProceso(headerRecibido, parametros);
		free(headerRecibido);
	   }

		return 1;
    }

    int IdentificarProceso(tHeader* headerRecibido, parametrosConexion* parametros) {
    	switch (headerRecibido->tipoProceso) {
    	case ESI:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
            strcpy(parametros->nombreProceso, headerRecibido->nombreProceso);
    		list_add(colaESIS,(void*)parametros);
    		conexionESI(parametros);
    		break;
    	case PLANIFICADOR:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
    		conexionPlanificador(parametros);
    		planificador = parametros;
    		break;
    	case INSTANCIA:
    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
            /*strcpy(parametros->nombreProceso, headerRecibido->nombreProceso);
            parametrosConexion * nuevaInstancia = malloc(sizeof(int)*2);
            nuevaInstancia->informacion = parametros;
            //memcpy(nuevaInstancia->informacion,parametros,sizeof(nuevaInstancia->informacion));
            nuevaInstancia->cantidadEntradasMaximas = ENTRADAS;
            nuevaInstancia->entradasUsadas=0;
            printf("Parametros instancia en direccion: %p\n", (void*)&(parametros));
            printf("parametrosConexion en direccion: %p\n", (void*)&(nuevaInstancia->informacion));
            */
    		sem_t * semaforo = malloc(sizeof(sem_t));
    		parametros->semaforo = semaforo;

            int ret;
            int value;
            int pshared;
            /* initialize a private semaphore */
            pshared = 0;
            value = 0;
            if ((ret = sem_init(parametros->semaforo,pshared,value)) != 0){
				perror("semaforo nuevo");
	        	exit_gracefully(1);
			} // Inicializo el semaforo en 0
			printf("Semaforo en direccion: %p\n", (void*)&(parametros->semaforo));
    		list_add(colaInstancias,(void*)parametros);
    		conexionInstancia(parametros);
    		break;
    	default:
    		puts("Error al intentar conectar un proceso");
    		close(parametros->new_fd);
    	}
		return 1;
    }

    int *conexionESI(parametrosConexion* parametros){
        puts("ESI conectandose");

        while(1){
        	int operacionValida;
        	operacionValida = verificarValidez(parametros->new_fd);
        	if (operacionValida == 2)
        		return EXIT_SUCCESS;
			OperaciontHeader * header = malloc(sizeof(OperaciontHeader));

			int recvHeader;
			if ((recvHeader = recv(parametros->new_fd, header, sizeof(OperaciontHeader), 0)) <= 0) {
				perror("recv");
				log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
				close(parametros->new_fd);
				//exit_gracefully(1);
			}

			puts("INTENTO RECIBIR TAMANIO VALOR");
			int tamanioValor = header->tamanioValor; // Si es un STORE o un GET, el ESI va a enviar 0
			puts("RECIBI TAMANIO VALOR");
			printf("Tamaño valor: %d \n",tamanioValor);
			OperacionAEnviar * operacion = malloc(sizeof(tTipoOperacion)+TAMANIO_CLAVE+tamanioValor);

			// Si la operacion devuelve 1 todo salio bien, si devuelve 2 hubo un bloqueo y le avisamos al ESI
			int resultadoOperacion;
			resultadoOperacion = AnalizarOperacion(tamanioValor, header, parametros, operacion);
			printf("%d",resultadoOperacion);
			if ( resultadoOperacion == 2){
				int sendBloqueo;
				if ((sendBloqueo =send(parametros->new_fd, BLOQUEO, sizeof(tResultadoOperacion), 0)) <= 0){
					perror("send");
					exit_gracefully(1);
				}
				free(header);
				free(operacion);
			}
			else {
				puts("El analizar operacion anduvo");

				// sleep(RETARDO);

				//Debo avisar a una Instancia cuando recibo una operacion (QUE NO SEA UN GET)
				//Agregamos el mensaje a una cola en memoria
				printf("ESI: clave de la operacion: %s \n",operacion->clave);
				printf("ESI: valor de la operacion: %s \n",operacion->valor);

				pthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola
				list_add(colaMensajes,(void*)operacion);
				pthread_mutex_unlock(&mutex);
				//free(operacion);

				puts("Voy a seleccionar la Instancia");
				SeleccionarInstancia(&CLAVE);
				puts("Se selecciono la Instancia");

				free(header);

				//esperamos el resultado para devolver
				puts("Vamos a ver si hay algun resultado en la cola");
				while(list_is_empty(colaResultados)) ; // mientras la cola este vacia no puedo continuar
				puts("No hay resultado en la cola");
				tResultado * resultado = malloc(sizeof(tResultado));
				resultado = list_take(colaResultados,1);
				log_info(logger,resultado);

				strcpy(resultado->clave,"PRUEBA");
				resultado->resultado = OK;

				int sendResultado;
				puts("Por enviar el resultado");
				if ((sendResultado =send(parametros->new_fd, resultado, sizeof(tResultado), 0)) <= 0){
					perror("send");
					exit_gracefully(1);
				}

				puts("Se envio el resultado");

				free(resultado);
			}
        }
		close(parametros->new_fd);
		return EXIT_SUCCESS;
    }

    int *conexionPlanificador(parametrosConexion* parametros){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("Planificador conectandose");
        while(1);
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
        	send(parametros->new_fd,notificacion,sizeof(notificacion),0);
        }

		close(parametros->new_fd);
		return 1;
    }

    int *conexionInstancia(parametrosConexion* parametros){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("Instancia conectandose");

        //while(1);
        puts("Instancia: Hago un sem_wait");
		printf("Semaforo en direccion: %p\n", (void*)&(parametros->semaforo));
        sem_wait(parametros->semaforo); // Caundo me avisen que hay una operacion para enviar, la voy a levantar de la cola
        puts("Instancia: Me hicieron un sem_post");
        OperacionAEnviar * operacion = list_get(colaMensajes,0);
        puts("Instancia: levante un mensaje de la cola de mensajes");
        printf("Instancia: la clave es %s \n",operacion->clave);
        printf("Instancia: el valor es %s \n",operacion->valor);
        int tamanioValor;
        if (operacion->valor == NULL){
        	tamanioValor=0;
        }
        else{
        tamanioValor = strlen(operacion->valor);
        }
        printf("Instancia: el tamanioValor es %d \n",tamanioValor);
        tTipoOperacion tipo = operacion->tipo;
        puts("Instancia: hago malloc de OperacionTHeader");
        OperaciontHeader * header = malloc(sizeof(OperaciontHeader));  // Creo el header que le voy a enviar a la instancia para que identifique la operacion
        header->tipo = tipo;
        header->tamanioValor = tamanioValor;

        puts("Instancia: Envio header a la instancia");
        // Envio el header a la instancia
        int sendHeader;
		if ((sendHeader = send(parametros->new_fd, header, sizeof(header), 0)) <= 0){
			perror("send");
			exit_gracefully(1);
		}

		free(header);

        puts("Instancia: Envio clave a la instancia");
		// Envio la clave
		int sendClave;
		if ((sendClave = send(parametros->new_fd, operacion->clave, TAMANIO_CLAVE, 0)) <= 0){
			perror("send");
			exit_gracefully(1);
		}


		if(tipo == OPERACION_SET){
			int sendSet;
	        puts("Instancia: Envio valor a la instancia");
	        if ((sendSet = send(parametros->new_fd, operacion->valor, tamanioValor, 0)) <= 0)
				perror("send");
			exit_gracefully(1);
		}

        puts("Instancia: Espero resuldato de la instancia");
		// Espero el resultado de la operacion
		tResultadoOperacion * resultado = malloc(sizeof(tResultadoOperacion));
		int resultadoRecv;
        if ((resultadoRecv = recv(parametros->new_fd, resultado, sizeof(tResultadoOperacion), 0)) <= 0) {
            perror("recv");
            exit_gracefully(1);
        }

        puts("Instancia: recibi el resultado de la instancia");

        if (resultado == BLOQUEO){
            puts("Instancia: el resultado de la instancia fue un bloqueo");
        	tBloqueo esiBloqueado = {1,operacion->clave};
        	list_add(colaBloqueos,(void*)&esiBloqueado);
        	sem_post(planificador->semaforo);
        }

        tResultado * resultadoCompleto = {resultado,operacion->clave};

        free(resultado);

        //Debo avisarle al ESI que me invoco el resultado
        list_add(colaResultados,(void*)resultadoCompleto);

		close(parametros->new_fd);
		return 1;
    }

    int AnalizarOperacion(int tamanioValor,
    			OperaciontHeader* header, parametrosConexion* parametros,
    			OperacionAEnviar* operacion) {

    	int resultadoOperacion;

    		switch (header->tipo) {
    		case OPERACION_GET: // PARA EL GET NO HAY QUE ACCEDER A NINGUNA INSTANCIA
    			puts("MANEJO UN GET");
    			resultadoOperacion = ManejarOperacionGET(parametros, operacion);
    			puts("YA MANEJE UN GET");
    			break;
    		case OPERACION_SET:
    			puts("MANEJO UN SET");
    			resultadoOperacion = ManejarOperacionSET(tamanioValor, parametros, operacion);
    			puts("ya maneje un SET");
    			break;
    		case OPERACION_STORE:
    			puts("MANEJO UN STORE");
    			resultadoOperacion = ManejarOperacionSTORE(parametros, operacion);
    			puts("YA MANEJE UN STORE");
    			break;
    		default:
    			puts("ENTRO POR EL DEFAULT");
    			break;
    		}
    		return resultadoOperacion;
    	}

	int ManejarOperacionGET(parametrosConexion* parametros, OperacionAEnviar* operacion) {

		char clave[TAMANIO_CLAVE];
		int recvClave;

		if ( (recvClave = recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			exit_gracefully(1);
		}

		clave[strlen(clave)+1] = '\0';
		printf("Recibi la clave: %s \n", clave);
		strcpy(CLAVE,clave);

		if (!list_is_empty(colaBloqueos) && EncontrarEnLista(colaBloqueos, &clave)){
			puts("La clave esta bloqueada");

			/*
			notificacion->tipoNotificacion=BLOQUEO;
			strcpy(notificacion->clave,clave);
			strcpy(notificacion->esi, parametros->nombreProceso);
			sem_post(planificador->semaforo);
			 */
			return 2;
		} // ACA HAY QUE AVISARLE AL PLANIFICDOR DEL BLOQUEO PARA QUE FRENE AL ESI

		operacion->tipo = OPERACION_GET;
		strcpy(operacion->clave,clave);
		operacion->valor = NULL;
		char* GetALoguear[4+strlen(clave)+1];
		strcpy(GetALoguear, "GET ");
		puts(GetALoguear);
		strcat(GetALoguear, clave);
		GetALoguear[4+strlen(clave)+1]='\0';
		puts(GetALoguear);

		tBloqueo *bloqueo = malloc(sizeof(tBloqueo));
		strcpy(bloqueo->clave,clave);
		strcpy(bloqueo->esi, parametros->nombreProceso);
		list_add(colaBloqueos,(void *)bloqueo); // TENGO QUE AVISARLE AL PLANIFICADOR QUE ESTA CLAVE ESTA BLOQUEADA POR ESTE ESI

		/* ACA HAY QUE AVISARLE AL PLANIFICDOR DEL BLOQUEO PARA QUE FRENE AL ESI
		notificacion->tipoNotificacion=BLOQUEO;
		strcpy(notificacion->clave,clave);
		strcpy(notificacion->esi, parametros->nombreProceso);
		sem_post(planificador->semaforo);
		*/

		log_info(logger, GetALoguear);

		return 1;
	}

	int ManejarOperacionSET(int tamanioValor, parametrosConexion* parametros, OperacionAEnviar* operacion) {

		char clave[TAMANIO_CLAVE];
		int recvClave, recvValor;
		if ((recvClave = recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			exit_gracefully(1);
		}
		clave[strlen(clave)+1] = '\0';
		printf("Recibi la clave: %s\n", clave);
		strcpy(CLAVE,clave);

		tBloqueo *bloqueo = malloc(sizeof(tBloqueo));
		strcpy(bloqueo->clave,clave);
		strcpy(bloqueo->esi, parametros->nombreProceso);

		if (!(!list_is_empty(colaBloqueos) && LePerteneceLaClave(colaBloqueos, bloqueo))){
			printf("No se puede realizar un SET sobre la clave: %s debido a que nunca se la solicito \n",clave);
		}
		free(bloqueo);

		char valor[tamanioValor];
		if (( recvValor = recv(parametros->new_fd, valor, tamanioValor + 1, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			exit_gracefully(1);
		}
		valor[tamanioValor] = '\0';
		printf("Recibi el valor: %s \n", valor);
		operacion->tipo = OPERACION_SET;
		strcpy(operacion->clave,clave);
		operacion->valor = valor;
		char* SetALoguear[5+strlen(clave)+strlen(valor)+1];
		strcpy(SetALoguear, "SET ");
		puts(SetALoguear);
		strcat(SetALoguear, clave);
		puts(SetALoguear);
		strcat(SetALoguear, " ");
		strcat(SetALoguear, valor);
		SetALoguear[5+strlen(clave)+strlen(valor)+1]='\0';
		puts(SetALoguear);

		log_info(logger, SetALoguear);
		//free(SetALoguear);

		return 1;
	}

	int ManejarOperacionSTORE(parametrosConexion* parametros, OperacionAEnviar* operacion) {

		char clave[TAMANIO_CLAVE];
		int resultadoRecv;

		if ((resultadoRecv =recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			exit_gracefully(1);
		}
		clave[strlen(clave)+1] = '\0';
		printf("Recibi la clave: %s\n", clave);
		strcpy(CLAVE,clave);

		tBloqueo *bloqueo = malloc(sizeof(tBloqueo));
		strcpy(bloqueo->clave,clave);
		strcpy(bloqueo->esi, parametros->nombreProceso);

		if (!list_is_empty(colaBloqueos) && LePerteneceLaClave(colaBloqueos, bloqueo)){
			printf("Se desbloqueo la clave: %s \n",clave);
			RemoverDeLaLista(colaBloqueos, &clave);

			/*
			notificacion->tipoNotificacion=BLOQUEO;
			strcpy(notificacion->clave,clave);
			strcpy(notificacion->esi, parametros->nombreProceso);
			sem_post(planificador->semaforo);
			*/
			// LE AVISO AL PLANIFICADOR QUE LA CLAVE SE DESBLOQUEO, DESPUES EL PODRA PLANIFICAR UN ESI BLOQUEADO POR ESTA
		}
		else{
			printf("No se puede realizar un STORE sobre la clave: %s debido a que nunca se la solicito \n",clave);
		}

		free(bloqueo);

		operacion->tipo = OPERACION_GET;
		strcpy(operacion->clave,clave);
		operacion->valor = NULL;

		char* StoreALoguear[6+strlen(clave)+1];
		strcpy(StoreALoguear, "STORE ");
		puts(StoreALoguear);
		strcat(StoreALoguear, clave);
		StoreALoguear[6+strlen(clave)+1]='\0';
		puts(StoreALoguear);

		log_info(logger, StoreALoguear);
		//free(StoreALoguear);

		return 1;
	}

	int InicializarListasYColas() {
		// Creamos una cola donde dejamos todas las instancias que se conectan con nosotros y otra para los mensajes recibidos de cualquier ESI
		int instanciasMaximas = 10;
		colaInstancias = list_create();
		//colaInstancias->head = malloc(sizeof(parametrosConexion) * instanciasMaximas);

		int mensajesMaximos = 5;
		colaMensajes = list_create();
		//colaMensajes->head = malloc(sizeof(OperacionAEnviar) * mensajesMaximos);

		int resultadosMaximos = 5;
		colaResultados = list_create();
		//colaResultados->head = malloc(sizeof(tResultado) * resultadosMaximos);

		int esisMaximos = 10;
		colaESIS = list_create();
		//colaESIS->head = malloc(sizeof(parametrosConexion) * esisMaximos);

		colaBloqueos = list_create();
		//colaBloqueos->head = malloc(TBLOQUEO * ENTRADAS);

		return 1;
	}

    int exit_gracefully(int return_nr) {

       log_destroy(logger);
       exit(return_nr);
       free(colaInstancias->head);
       free(colaMensajes->head);
       free(colaResultados->head);
       free(colaESIS->head);
       free(colaBloqueos->head);

		return return_nr;
     }

    int configure_logger() {
      logger = log_create("Coordinador.log", "CORD", true, LOG_LEVEL_INFO);

      return 1;
     }

    bool EncontrarEnLista(t_list * lista, char * claveABuscar){
		bool yaExisteLaClave(tBloqueo *bloqueo) {
			return string_equals_ignore_case(bloqueo->clave,claveABuscar);
		}
		return list_any_satisfy(lista,(void*) yaExisteLaClave);
    }

    bool LePerteneceLaClave(t_list * lista, tBloqueo * bloqueoBuscado){
		bool yaExisteLaClaveYPerteneceAlESI(tBloqueo *bloqueo) {
			return string_equals_ignore_case(bloqueo->esi,bloqueoBuscado->esi) && string_equals_ignore_case(bloqueo->clave,bloqueoBuscado->clave);
		}
		return list_any_satisfy(lista,(void*) yaExisteLaClaveYPerteneceAlESI);
    }

    parametrosConexion* BuscarInstanciaMenosUsada(t_list * lista){
    	int tamanioLista = list_size(lista);

    	parametrosConexion* instancia = list_get(lista,0);
    	for (int i = 0; i< tamanioLista; i++){
    		parametrosConexion* instanciaAComparar = malloc(sizeof(parametrosConexion));
        	instancia = list_get(lista,i);
        	if (instancia->entradasUsadas > instanciaAComparar->entradasUsadas)
        		instancia = instanciaAComparar;
    		free(instanciaAComparar);
    	}
    	MandarAlFinalDeLaLista(colaInstancias, &instancia);
    	return instancia;
    }

    int MandarAlFinalDeLaLista(t_list * lista, parametrosConexion * instancia){
    	void Notificar(parametrosConexion * instanciaAComparar){
    		if(instanciaAComparar->nombreProceso == instancia->nombreProceso){
    			//puts("Voy a hacer el sem_post a la Instancia seleccionada \n");
    			//printf("Semaforo en direccion: %p\n", (void*)&(instanciaAComparar->semaforo));
    			//sem_post(&(instanciaAComparar->semaforo));
    		}
    	}
    	bool Comparar(parametrosConexion * instanciaAComparar){
    		return instanciaAComparar->new_fd == instancia->new_fd;
    	}
    	t_list * listaNueva = malloc(sizeof(lista));
    	puts("Voy a iterar la lista");
    	list_iterate(lista,Notificar);
    	puts("Itere la lista");
    	listaNueva = list_take_and_remove(lista, 1);
    	list_add_all(lista,listaNueva);
    	free(listaNueva);
    	return 1;
    }

    void sigchld_handler(int s) // eliminar procesos muertos
    {
        while(wait(NULL) > 0);
    }

	int  LeerArchivoDeConfiguracion() {
		// Leer archivo de configuracion con las commons
		t_config* configuracion;
		configuracion = config_create(ARCHIVO_CONFIGURACION);
		PUERTO = config_get_int_value(configuracion, "port");
		IP = config_get_string_value(configuracion, "ip");
		ALGORITMO_CONFIG = config_get_string_value(configuracion,
				"algoritmo_distribucion");
		ENTRADAS = config_get_int_value(configuracion, "Cantidad de Entradas");
		TAMANIO_ENTRADAS = config_get_int_value(configuracion,
				"Tamanio de Entrada");
		RETARDO = config_get_int_value(configuracion, "Retardo");
		printf("Algoritmo de distribucion %s \n", ALGORITMO_CONFIG);
		if (strcmp(ALGORITMO_CONFIG, "EL") == 0)
			ALGORITMO = EL;
		else {
			if (strcmp(ALGORITMO_CONFIG, "KE") == 0)
				ALGORITMO = KE;
			else {
				if (strcmp(ALGORITMO_CONFIG, "LSU") == 0)
					ALGORITMO = LSU;
				else {
					puts("Error al cargar el algoritmo de distribucion");
					exit(1);
				}
			}
		}

		return 1;
	}

	int SeleccionarInstancia(char* clave) {
		while (list_is_empty(colaInstancias));

		// mientras la cola este vacia no puedo continuar

		pthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola
		switch (ALGORITMO){
		case EL: ; // Si no se deja un statement vacio rompe
			SeleccionarPorEquitativeLoad();
			break;
		case LSU:
			SeleccionarPorLeastSpaceUsed();
			break;
		case KE:
			SeleccionarPorKeyExplicit(&clave);
			break;
		default:
			puts("Hubo un problema al seleccionar la instancia correcta");
			exit_gracefully(1);
		}
		pthread_mutex_unlock(&mutex);
		//saco la primer instancia de la cola pero luego tengo que deolverla a la colasem_post(instancia->semaforo); // Le aviso al semaforo de la instancia de que puede operar (el semaforo es el tid)

		return 1;
	}

	int SeleccionarPorEquitativeLoad() {
		// mientras la cola este vacia no puedo continuarpthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola

		parametrosConexion * instancia = list_get(colaInstancias,0);
		printf("Semaforo de list_get en direccion: %p\n", (void*)&(instancia->semaforo));
		//list_remove_and_destroy_element(colaInstancias, 0,(void*)destruirInstancia);
		puts("Voy a hacer el sem_post a la Instancia seleccionada \n");
		sem_post(instancia->semaforo);
		//sem_post(instancia->informacion->semaforo);

		//list_add(colaInstancias,instancia);
		MandarAlFinalDeLaLista(colaInstancias,instancia);

		//free(instancia);
		return 1;
	}

	void destruirInstancia(parametrosConexion *self) {
	    free(self);
	}

	int SeleccionarPorLeastSpaceUsed(){
		parametrosConexion* instanciasMenosUsadas = BuscarInstanciaMenosUsada(colaInstancias); // Va a buscar la instancia que menos entradas tenga, desempata con fifo
		return 1;
	}

	int SeleccionarPorKeyExplicit(char* clave){
		int cantidadInstancias = list_size(colaInstancias);
		char primerCaracter = clave[0];
		int x = 0;
		while (clave[x] < 97 && clave[x] > 122){ // Busco el primer caracter en minuscula
			primerCaracter = clave[x];
			x++;
		}

		int posicionLetraEnASCII = primerCaracter - 97;
		int rango = KEYS_POSIBLES/cantidadInstancias;
		int restoRango = KEYS_POSIBLES%cantidadInstancias;
		int entradasUltimaInstancia;
		if (restoRango !=0){
			entradasUltimaInstancia = KEYS_POSIBLES - ((cantidadInstancias-1) * (rango + 1));
		}

		// Esto es si la clave no se encuentra en ninguna instancia
		for (int i = 0; i < cantidadInstancias; i++){
			if (i!= cantidadInstancias - 1 && restoRango == 0){    // Si no es la ultima instancia debo redondear hacia arriba
					if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + rango)){
						parametrosConexion * instancia = list_get(colaInstancias, i);
						sem_post(instancia->semaforo);
					}
				}
				else if (i!= cantidadInstancias - 1 && restoRango != 0){ // si es la ultima instancia debo recalcular y ver cuantas entradas puedo acepar
					if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + rango + 1)){
						parametrosConexion * instancia = list_get(colaInstancias, i);
						sem_post(instancia->semaforo);
					}else{
						if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + entradasUltimaInstancia)){
							parametrosConexion * instancia = list_get(colaInstancias, i);
							sem_post(instancia->semaforo);
					}
				}
			}
		}
		return 1;
	}

	static void destruirBloqueo(tBloqueo *bloqueo) {
		printf("Vor a borrar la clave %s \n",bloqueo->clave);
	    free(bloqueo);
	}

	void RemoverDeLaLista(t_list * lista, char * claveABuscar){
		bool yaExisteLaClave(tBloqueo *bloqueo){
			printf("Comparando la clave %s con %s \n", claveABuscar, bloqueo->clave);
			return string_equals_ignore_case(bloqueo->clave,claveABuscar);
		}
		list_remove_and_destroy_by_condition(colaBloqueos,yaExisteLaClave,(void*)destruirBloqueo);
	}

	int verificarValidez(int sockfd){
		tValidezOperacion *validez = malloc(sizeof(tValidezOperacion));
		if(recv(sockfd, validez, sizeof(tValidezOperacion), 0) <= 0){
			perror("Fallo el recibir validez");
			return 2;  //Aca salimos
		}

		if(validez == OPERACION_INVALIDA){
			close(sockfd);
			puts("Se cerró la conexión con el ESI debido a una OPERACION INVALIDA");
		}
		free(validez);
		return EXIT_SUCCESS;
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





