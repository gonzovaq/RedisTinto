/*
 * Coordinador.c
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */

#include "Coordinador.h"

	void intHandler(int dummy) { // para atajar ctrl c
		keepRunning = 0;
		sleep(4);
		exit_gracefully(EXIT_SUCCESS);
	}

    int main(int argc, char *argv[])
    {
    	signal(SIGINT, intHandler);

    	while (keepRunning) {
    	verificarParametrosAlEjecutar(argc, argv);

    	// Leer archivo de configuracion con las commons
    	LeerArchivoDeConfiguracion(argv);

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
        mi_direccion.sin_port = htons(PUERTO);     // short, Ordenación de bytes de la red
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

        // Inicializo un semaforo general para la Instancia
        int ret;
        int value;
        int pshared;
        /* initialize a private semaphore */
        pshared = 0;
        value= 1;
        if ((ret = sem_init(&semaforoInstancia,pshared,value)) != 0){
			perror("Semaforo para instancia");
        	exit_gracefully(1);
		} // Inicializo el semaforo en 0


        EscucharConexiones(sockfd);

        close(sockfd);

    	}
    	exit_gracefully(EXIT_SUCCESS);
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
        	exit_gracefully(EXIT_SUCCESS);
        }

    	while(keepRunning) {  // main accept() loop
    	            sin_size = sizeof(struct sockaddr_in);
    	            if ((new_fd = accept(sockfd, (struct sockaddr *)&direccion_cliente,
    	                                                           &sin_size)) == -1) {
    	                perror("accept");

    	            }
    	            printf("server: recibi una conexion de  %s\n",
    	                                               inet_ntoa(direccion_cliente.sin_addr));

    	            //Para hilos debo crear una estructura de parametros de la funcion que quiera llamar
    				pthread_t tid;

    				char nombreProceso[TAMANIO_NOMBREPROCESO];

    	            int entradasUsadas = 0;

    				parametrosConexion parametros;//parametrosConexion parametros = {new_fd,malloc(sizeof(sem_t)),nombreProceso,ENTRADAS,entradasUsadas};
    				parametros.new_fd = new_fd;
    				strcpy(parametros.nombreProceso,nombreProceso);
    				parametros.cantidadEntradasMaximas = ENTRADAS;
    				parametros.entradasUsadas = 0;

    				parametros.pid = 0;

    				printf("Socket %d \n",new_fd);
    	            int stat = pthread_create(&tid, NULL, (void*)gestionarConexion, (void*)&parametros);//(void*)&parametros -> parametros contendria todos los parametros que usa conexion
    				if (stat != 0){
    					puts("error al generar el hilo");
    					perror("thread");

    				}
    				pthread_detach(tid); //Con esto decis que cuando el hilo termine libere sus recursos
    	        }

        //Tenemos que destruir el mutex
        pthread_mutex_destroy(&mutex);

		return 1;
    }

    int *gestionarConexion(parametrosConexion *parametros){ //(int* sockfd, int* new_fd)
	   tHeader *headerRecibido = malloc(sizeof(tHeader));
	   parametrosConexion *parametrosNuevos=  malloc(sizeof(parametrosConexion));

	   parametrosNuevos->new_fd = parametros->new_fd;

	   if ((recv(parametrosNuevos->new_fd, headerRecibido, sizeof(tHeader), 0)) <= 0){
			perror("recv");
			log_info(logger, "Mensaje: recv error");//process_get_thread_id()); //asienta error en logger y corta
	   }

	   // Realizo una copia de los parametros que contiene la informacion de la conexion que acabo de recibir
	   strcpy(parametrosNuevos->nombreProceso, headerRecibido->nombreProceso);
	   parametrosNuevos-> cantidadEntradasMaximas = parametros->cantidadEntradasMaximas;
	   parametrosNuevos-> entradasUsadas = parametros->entradasUsadas;
	   parametrosNuevos->conectada = 1;

	   if (headerRecibido->tipoMensaje == CONECTARSE){
		   parametrosNuevos->pid = headerRecibido->idProceso;
		   IdentificarProceso(headerRecibido, parametrosNuevos);
	   }

	   free(parametrosNuevos);
	   free(headerRecibido);

	   return EXIT_SUCCESS;
    }

    int IdentificarProceso(tHeader* headerRecibido, parametrosConexion* parametros) {
    	switch (headerRecibido->tipoProceso) {
    	case ESI:
    		printf("ESI: Se conecto el proceso %d \n", headerRecibido->idProceso);

    		sem_t * semaforoESI = malloc(sizeof(sem_t));
    		parametros->semaforo = semaforoESI;

            int retESI;
            int valueESI;
            int psharedESI;
            /* initialize a private semaphore */
            psharedESI = 0;
            valueESI = 0;
            if ((retESI = sem_init(parametros->semaforo,psharedESI,valueESI)) != 0){
				perror("ESI: semaforo nuevo");
	        	exit_gracefully(1);
			} // Inicializo el semaforo en 0
			printf("ESI: Semaforo en direccion: %p\n", (void*)&(parametros->semaforo));

            parametros->pid = headerRecibido->idProceso;
            parametros->claves = list_create();
    		list_add(colaESIS,(void*)parametros);
    		conexionESI(parametros);
    		break;
    	case PLANIFICADOR:
    		printf("Planificador: Se conecto el proceso %d \n", headerRecibido->idProceso);
    		sem_t * semaforoPlanif = malloc(sizeof(sem_t));
    		parametros->semaforo = semaforoPlanif;

            int retPlanif;
            int valuePlanif;
            int psharedPlanif;
            /* initialize a private semaphore */
            psharedPlanif = 0;
            valuePlanif = 0;
            if ((retPlanif = sem_init(parametros->semaforo,psharedPlanif,valuePlanif)) != 0){
				perror("semaforo nuevo");
	        	exit_gracefully(1);
			} // Inicializo el semaforo en 0
			printf("Planificador: Semaforo en direccion: %p\n", (void*)&(parametros->semaforo));

    		planificador = parametros;
    		conexionPlanificador(parametros);
    		free(semaforoPlanif);
    		break;
    	case INSTANCIA:
    		printf("Instancia: Se conecto el proceso %d con nombre %s \n", parametros->pid ,headerRecibido->nombreProceso);
    		printf("Instancia: Socket de la instancia: %d\n", parametros->new_fd);

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
			printf("Instancia: Semaforo en direccion: %p\n", (void*)&(parametros->semaforo));
			parametros->claves = list_create();

			//TODO: Debo revisar si la instancia se esta reincorporando !!!!
			BuscarSiLaInstanciaSeEstaReincorporando(parametros);
    		list_add(colaInstancias,(void*)parametros);
    		int cantidadInstancias = list_size(colaInstancias);
    		printf("Instancia: Ahora tengo %d instancias \n",cantidadInstancias);
    		conexionInstancia(parametros);
    		free(semaforo);
    		break;
    	default:
    		puts("Error al intentar conectar un proceso");
    		close(parametros->new_fd);
    	}
		return 1;
    }

    int BuscarSiLaInstanciaSeEstaReincorporando(parametrosConexion * parametros){
    	bool EsLaInstanciaDesconectada(parametrosConexion * parametrosAComparar){
    		printf("Instancia: Comparo instancia %s, con %s \n",parametros->nombreProceso,parametrosAComparar->nombreProceso);
    		if(string_equals_ignore_case(parametros->nombreProceso,parametrosAComparar->nombreProceso) == true){

    						puts("Instancia: La Instancia se esta reincorporando");
    						return true;
    		}
    		return false;
    	}
    	parametrosConexion * instancia = list_remove_by_condition(colaInstancias,(void*)EsLaInstanciaDesconectada);
    	if (instancia == NULL){
    		puts("Instancia: Es una nueva Instancia");
    		return EXIT_SUCCESS;
    	}

    	puts("Instancia: Voy a actualizar la informacion de la Instancia");
    	parametros->entradasUsadas = instancia->entradasUsadas;
    	parametros->conectada = 1;
    	list_add_all(parametros->claves,instancia->claves);
    	puts("Instancia: Actualice la informacion de la Instancia");

    	return EXIT_SUCCESS;
    }

    int *conexionESI(parametrosConexion* parametros){
        puts("ESI conectandose");

        while(keepRunning){
        	int operacionValida;
        	operacionValida = verificarValidez(parametros->new_fd);
        	if (operacionValida == 2)
        		return EXIT_SUCCESS;
			OperaciontHeader * header = malloc(sizeof(OperaciontHeader));

			int recvHeader;
			if ((recvHeader = recv(parametros->new_fd, header, sizeof(OperaciontHeader), 0)) <= 0) {
				perror("recv");
				log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
				// Liberamos sus claves
				LiberarLasClavesDelESI(parametros);
				close(parametros->new_fd);
				free(header);
				return EXIT_SUCCESS;

			}

			puts("ESI: INTENTO RECIBIR TAMANIO VALOR");
			int tamanioValor = header->tamanioValor; // Si es un STORE o un GET, el ESI va a enviar 0
			puts("ESI: RECIBI TAMANIO VALOR");
			printf("ESI: Tamaño valor: %d \n",tamanioValor);
			OperacionAEnviar * operacion = malloc(sizeof(OperacionAEnviar));//+TAMANIO_CLAVE+tamanioValor);
			printf("ESI: clave operacion en direccion: %p\n", (void*)&(operacion->clave));

			// Si la operacion devuelve OK(1) todo salio bien, si devuelve BLOQUEO(2) hubo un bloqueo y le avisamos al ESI Y puede ser ERROR(3)
			int resultadoOperacion;
			resultadoOperacion = AnalizarOperacion(tamanioValor, header, parametros, operacion);
			printf("ESI: El resultado de analizar operacion fue: %d \n",resultadoOperacion);
			printf("ESI: El socket del ESI es: %d \n",parametros->new_fd);
			switch (resultadoOperacion){
			case OK:
				puts("ESI: El analizar operacion anduvo");

				free(header);
				sleep(RETARDO/100);

				//Debo avisar a una Instancia cuando recibo una operacion (QUE NO SEA UN GET)
				//Agregamos el mensaje a una cola en memoria
				ConexionESISinBloqueo(operacion,parametros);

				break;
			case BLOQUEO: ;// Que hacemos si hay bloqueo? Debemos avisarle al planificador

				tResultado *  resultado = malloc(sizeof(tResultado));
				resultado->resultado= BLOQUEO;
				strcpy(resultado->clave,&CLAVE);
				printf("ESI: Le envio al esi que se bloqueo con la clave %s \n", resultado->clave);
				if ((send(parametros->new_fd, resultado, sizeof(tResultado), 0)) <= 0){
					perror("send");
				}
				free(header);

				break;
			case ERROR:
				puts("ESI: Error en la soliciud");

				if ((send(parametros->new_fd, ERROR, sizeof(tResultadoOperacion), 0)) <= 0){
					perror("send");
				}

				LiberarLasClavesDelESI(parametros);

				free(header);

				break;
			default:
				puts("ESI Fallo al ver el resultado de la operacion");
				break;
			}
			if(operacion->tipo==OPERACION_SET)
				free(operacion->valor);
			free(operacion);
        }
		close(parametros->new_fd);
		return EXIT_SUCCESS;
    }

    int LiberarLasClavesDelESI(parametrosConexion * parametros){

    	for(int i = 0; i< list_size(parametros->claves); i++){

    		EliminarClaveDeClavesTomadas(list_remove(parametros->claves,i));
    	}

    	void LiberarClave(char * clave){
    		printf("ESI: Se va a eliminar la clave %s del ESI de id %d \n",clave,parametros->pid);
    		free(clave);
    	}

    	list_clean_and_destroy_elements(parametros->claves,(void*)LiberarClave);

    	return EXIT_SUCCESS;
    }

    int EliminarClaveDeClavesTomadas(char * claveABorrar){
    	bool EsLaMismaClave(char * claveAComparar){
    		return string_equals_ignore_case(claveABorrar, claveAComparar);
    	}
    	void BorrarClave(char * clave){
    		RemoverClaveDeLaListaBloqueos(clave);
    		printf("ESI: Voy a borrar la clave %s de las clavesTomadas",clave);
    		free(clave);
    	}
    	list_remove_and_destroy_by_condition(clavesTomadas,(void*)EsLaMismaClave,(void*)BorrarClave);

    	return EXIT_SUCCESS;
    }

    int *conexionPlanificador(parametrosConexion* parametros){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("Planificador conectandose");

        RecibirClavesBloqueadas(parametros);

        //Abro otro hilo para escuchar sus comandos solicitados por consola
        pthread_t tid;
        int stat = pthread_create(&tid, NULL, (void*)escucharMensajesDelPlanificador, (void*)parametros);
		if (stat != 0){
			puts("Planificador: error al generar el hilo");
			perror("thread");
			return 1;
		}
		pthread_detach(tid); //Con esto decis que cuando el hilo termine libere sus recursos

		printf("Planificador: Manejo la conexion en el socket %d \n",parametros->new_fd);

        // Le avisamos al planificador cada clave que se bloquee y desbloquee
        while(keepRunning){
        	puts("Planificador: Espero a que me avisen!");
        	sem_wait(planificador->semaforo); // Me van a avisar si se produce algun bloqueo
        	puts("Planificador: Hay algo para avisarle al planificador");
        	printf("Planificador: Te voy a enviar la notificacion: %d a la clave: %s\n",notificacion->tipoNotificacion,notificacion->clave);
        	//tNotificacionPlanificador* resultado = list_remove(colaMensajesParaPlanificador, 0);
        	if (send(parametros->new_fd,notificacion,sizeof(tNotificacionPlanificador),0) <= 0){
        		puts("Planificador: Fallo al enviar mensaje al planificador");
        		perror("send planificador");
        	}
        	puts("Planificador: le envie algo al planificador");
        }

		close(parametros->new_fd);
		return 1;
    }

	int AgregarClaveBloqueada(parametrosConexion* parametros) {

		char * clave = malloc(TAMANIO_CLAVE);
		if ((recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());

		} else
		{
			clave[TAMANIO_CLAVE-1] = '\0';
			printf("Planificador: Agrego la clave %s a la cola de bloqueadas \n", clave);

			char* claveCopia = malloc(strlen(clave)+1);
			strcpy(claveCopia,clave);
			pthread_mutex_lock(&mutex);
			list_add(clavesTomadas, (char *)claveCopia);
			pthread_mutex_unlock(&mutex);
		}
		free(clave);
		return EXIT_SUCCESS;
	}

    int RecibirClavesBloqueadas(parametrosConexion* parametros){
    	puts("Planificador: Voy a recibir cuantas claves hay inicialmente bloqueadas");
    	tClavesBloqueadas * clavesBloqueadas = malloc(sizeof(tClavesBloqueadas));
    	if ((recv(parametros->new_fd, clavesBloqueadas, sizeof(tClavesBloqueadas), 0)) <= 0){
    		puts("Planificador: Fallo el recibir la cantidad de claves iniciales");
    		return -1;
    	}

    	printf("Planificador: Tengo %d claves inicialmente bloqueadas \n",clavesBloqueadas->cantidadClavesBloqueadas);

    	for(int i = 0; i <clavesBloqueadas->cantidadClavesBloqueadas; i++){
    		AgregarClaveBloqueada(parametros);
    	}

    	return EXIT_SUCCESS;
    }

    int *escucharMensajesDelPlanificador(parametrosConexion* parametros){
    	while(keepRunning){
    		// Aca vamos a Escuchar todos los mensajes que solicite el planificador, hay que ver cuales son y vemos que hacemos
    		printf("Planificador: Espero alguna solicitud del socket %d \n",parametros->new_fd);
    		tSolicitudPlanificador * solicitud = malloc(sizeof(tSolicitudPlanificador));
			if ((recv(parametros->new_fd, solicitud, sizeof(tSolicitudPlanificador), 0)) <= 0) {

				close(parametros->new_fd);
				return 2;
				//exit_gracefully(1);
			}
			printf("Planificador: Recibi una solicitud de tipo %d \n", *solicitud);
			char clave[TAMANIO_CLAVE];
			char idS[4];
			switch(solicitud->solicitud){
			case BLOQUEAR:
				if ( (recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
					perror("recv");
					log_info(logger, "TID %d  Mensaje: ERROR en ESI",
							process_get_thread_id());
					return ERROR;

				}
				else{
					char* claveCopia = malloc(strlen(clave)+1);
					strcpy(claveCopia,clave);
					printf("Planificador: voy a agregar a las bloqueadas a la clave %s \n",claveCopia);
					pthread_mutex_lock(&mutex);
					list_add(clavesTomadas,claveCopia); //Estaba &clave, cambie por clave para probar.
					pthread_mutex_unlock(&mutex);
				}
				break;
			case DESBLOQUEAR:

				if ( (recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
					perror("recv");
					log_info(logger, "TID %d  Mensaje: ERROR en ESI",
							process_get_thread_id());
					return ERROR;

				}
				else{
					printf("Planificador: voy a remover de las bloqueadas a la clave %s \n",clave);
					printf("Planificador: Size de la lista clavesTomadas antes de remover: %d\n", list_size(clavesTomadas));
					RemoverClaveDeClavesTomadas(clave);//Aca habia un &clave, pruebo sacandoselo
					printf("Planificador: Size de la lista clavesTomadas despues de remover: %d\n", list_size(clavesTomadas));
				break;
				}
			case KILL:
				if ( (recv(parametros->new_fd,idS,sizeof(idS), 0)) <= 0) {
					perror("Planificador: recv");
					/*log_info(logger, "TID %d  Mensaje: ERROR en ESI",
							process_get_thread_id());*/
					return ERROR;


				}
				else{
					printf("Planificador: id del esi KILLED a liberar todas sus claves %s \n",idS);
					int id=atoi(idS);
					printf("Planificador: En int %d \n",id);
					EncontrarAlESIYEliminarlo(id);
				}
				break;
			case STATUS:
			if ( (recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
					perror("recv");
					log_info(logger, "TID %d  Mensaje: ERROR en ESI",
							process_get_thread_id());
					return ERROR;

				}
				else{
					printf("Planificador: recibi la clave %s y voy a mostrar la instancia \n",clave);

					sem_wait(&semaforoInstancia);
					BuscarClaveEnInstanciaYEnviar(&clave);
					sem_post(&semaforoInstancia);
				}
				break;

			case LISTAR:
				/*
				char clave[TAMANIO_CLAVE];
				if ((recv(parametros->new_fd, clave, sizeof(TAMANIO_CLAVE), 0)) <= 0) {
					perror("recv");
					log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
					close(parametros->new_fd);
				}
				int cantidadDeESIS;
				t_list * ESISQueSolicitaronLaClave;
				ESISQueSolicitaronLaClave = list_create();
				cantidadDeESIS = list_size(colaESIS);
				parametrosConexion* parametrosESI;
				for(int i = 0;i<cantidadDeESIS;i++){
					parametrosESI = list_get(colaESIS,i);
				}
				*/
				break;
			default:
				puts("Planificador: Me envio cualquier cosa");
				break;
			}

			free(solicitud);
		}

    	return EXIT_SUCCESS;
    }

    int BuscarClaveEnInstanciaYEnviar(char * clave){

    	bool EsLaMismaClave(char * claveAComparar){
    		return string_equals_ignore_case(clave, claveAComparar);
    	}

    	bool TieneLaClave(parametrosConexion * parametros){
    		return list_any_satisfy(parametros->claves,(void *)EsLaMismaClave);
    	}

    	parametrosConexion * instancia = list_find(colaInstancias,(void*)TieneLaClave);

    	if (instancia != NULL){


			tEntradasUsadas *estasConecatada = malloc(sizeof(tEntradasUsadas));
			if ((recv(instancia->new_fd, estasConecatada, sizeof(tEntradasUsadas), 0)) <= 0) {
				puts("Instancia: Fallo al enviar el tipo de operacion");
				instancia->conectada = 0;

				free(estasConecatada);
				sem_destroy(instancia->semaforo);
				close(instancia->new_fd);
				return ERROR;
			}
			free(estasConecatada);


			tOperacionInstanciaStruct * operacionInstancia = malloc(sizeof(tOperacionInstanciaStruct));
			operacionInstancia->operacion= SOLICITAR_VALOR;
			if ((send(instancia->new_fd, operacionInstancia, sizeof(tOperacionInstanciaStruct), 0)) // Le informamos que quiero hacer!
							<= 0) {
						puts("Instancia: Fallo al enviar el tipo de operacion");
						perror("send");

						RemoverInstanciaDeLaLista(instancia);
						close(instancia->new_fd);
						return ERROR;
					}
			free(operacionInstancia);

			tEntradasUsadas *estasConecatada2 = malloc(sizeof(tEntradasUsadas));
			if ((recv(instancia->new_fd, estasConecatada2, sizeof(tEntradasUsadas), 0)) <= 0) {
				perror("Instancia: se desconecto!!!");
				instancia->conectada = 0;
				free(estasConecatada2);
				return OK;
			}

			free(estasConecatada2);

			if ((send(instancia->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
				puts("Planificador: Error al enviar la clave a la Instancia");
				perror("Planificador: send");
				RemoverInstanciaDeLaLista(instancia);
				close(instancia->new_fd);
				return ERROR;
			}

			tEntradasUsadas * tamanioValor = malloc(sizeof(tEntradasUsadas));
			if((recv(instancia->new_fd, tamanioValor, sizeof(tEntradasUsadas), 0)) <= 0){
				perror("Planificador: Fallo al recibir el tamanio valor");
				perror("Instancia: se desconecto!!!");
				instancia->conectada = 0;
				free(estasConecatada2);
				return OK;
			}

			char * valor = malloc(tamanioValor+1);
			if((recv(instancia->new_fd, valor, tamanioValor+1, 0)) <= 0){
				perror("Planificador: Fallo al recibir el valor");
				perror("Instancia: se desconecto!!!");
				instancia->conectada = 0;
				free(estasConecatada2);
				return OK;
			}

			printf("Planificador: La Instancia que tiene la clave %s es %s \n",clave,instancia->nombreProceso);

			tStatusParaPlanificador * status = malloc(sizeof(tStatusParaPlanificador));
			strcpy(status,instancia->nombreProceso);
			status->tamanioValor = tamanioValor->entradasUsadas;

			free(tamanioValor);

			if ((send(planificador->new_fd,status,sizeof(tStatusParaPlanificador),0) <= 0)){
				perror("Planificador: Error al enviarle el nombre de la Instancia y tamanioValor");
				free(status);
				free(valor);
				return ERROR;
			}
			free(status);

			if ((send(planificador->new_fd,valor,tamanioValor+1,0) <= 0)){
				perror("Planificador: Error al enviarle el valor");
				free(valor);

				return ERROR;
			}
			free(valor);
    	}
    	else{
    		puts("Planificador: No hay ninguna Instancia que posea la clave, simulemos buscar una");

    		char * nombreInstancia;
    		switch (ALGORITMO){
    		case EL: ; // Si no se deja un statement vacio rompe
    			nombreInstancia = SimulacionSeleccionarPorEquitativeLoad(&clave);
    			break;
    		case LSU:
    			nombreInstancia = SimulacionSeleccionarPorLeastSpaceUsed(&clave);
    			break;
    		case KE:
    			nombreInstancia = SimulacionSeleccionarPorKeyExplicit(&clave);
    			break;
    		default:
    			puts("Planificador: Hubo un problema al seleccionar la instancia correcta");
    			exit_gracefully(1);
    		}

			printf("Planificador: La Instancia que tendria la clave %s es %s \n",clave,nombreInstancia);

			tNotificacionPlanificador * notif = malloc(sizeof(tNotificacionPlanificador));

			strcpy(notif->clave,clave);
			notif->tipoNotificacion = STATUS;
        	if (send(planificador->new_fd,notif,sizeof(tNotificacionPlanificador),0) <= 0){
        		puts("Planificador: Fallo al enviar mensaje al planificador");
        		perror("send planificador");
        	}
        	free(notif);

			tStatusParaPlanificador * status = malloc(sizeof(tStatusParaPlanificador));
			if(nombreInstancia==NULL){
				strcpy(status->proceso,"No hay instancias disponibles \0");
			}else
			{
				strcpy(status->proceso,nombreInstancia);
			}
			status->tamanioValor = 0;

			if ((send(planificador->new_fd,status,sizeof(tStatusParaPlanificador),0) <= 0)){
				perror("Planificador: Error al enviarle el nombre de la Instancia y tamanioValor");
				free(status);
				close(instancia->new_fd);
				return ERROR;
			}
			free(status);
    	}

    	return EXIT_SUCCESS;
    }

    int EncontrarAlESIYEliminarlo(int id){

    	bool EsElESIaBorrar(parametrosConexion* parametrosAComparar){
    		return parametrosAComparar->pid == id;
    	}

    	void LiberarParametros(parametrosConexion * parametros){
    		printf("Planificador: Se va a Eliminar el ESI de id %d \n",parametros->pid);
    		LiberarLasClavesDelESI(parametros);
    		//free(parametros);
    	}
    	list_remove_and_destroy_by_condition(colaESIS,(void*)EsElESIaBorrar,(void*)LiberarParametros);

    	return EXIT_SUCCESS;
    }

    int RemoverClaveDeClavesTomadas(char * clave){
		bool compararClaveParaDesbloquear(char *claveABuscar){
			printf("Planificador: Comparando la clave %s con %s \n", claveABuscar, clave);
			if(string_equals_ignore_case(clave,claveABuscar) == true){
				puts("Planificador: Las claves son iguales");
				printf("Vamos a eliminar la clave %s de la lista de Claves Tomadas\n",claveABuscar);
				return true;
			}
			puts("Planificador: Las claves son distintas");
			return false;
		}
		pthread_mutex_lock(&mutex);
		list_remove_and_destroy_by_condition(clavesTomadas,(void*)compararClaveParaDesbloquear,(void*)borrarClave);
		pthread_mutex_unlock(&mutex);


		return EXIT_SUCCESS;
    }

    int RemoverClaveDeLaListaBloqueos(char * claveABuscar){
    		bool yaExisteLaClave(tBloqueo *bloqueo){
    			printf("Comparando la clave %s con %s \n", claveABuscar, bloqueo->clave);
    			if(string_equals_ignore_case(bloqueo->clave,claveABuscar) == true){
    				printf("Vamos a eliminar la clave %s de la listaBloqueos\n",claveABuscar);
    				return true;
    			}
    			return false;
    		}
    		pthread_mutex_lock(&mutex);
    		list_remove_and_destroy_by_condition(listaBloqueos,yaExisteLaClave,(void*)destruirBloqueo);
    		pthread_mutex_unlock(&mutex);
    		return EXIT_SUCCESS;
    }

    int RemoverClaveDeClavesPropias(char * clave, parametrosConexion *parametros){
		bool compararClaveParaDesbloquear(char *claveABuscar){
			printf("Planificador: Comparando la clave %s con %s \n", claveABuscar, clave);
			if(string_equals_ignore_case(clave,claveABuscar) == true){
				puts("Planificador: Las claves son iguales");
				printf("Vamos a eliminar la clave %s de la lista de Claves Tomadas\n",claveABuscar);
				return true;
			}
			puts("Planificador: Las claves son distintas");
			return false;
		}
		pthread_mutex_lock(&mutex);
		list_remove_and_destroy_by_condition(parametros->claves,(void*)compararClaveParaDesbloquear,(void*)borrarClave);
		pthread_mutex_unlock(&mutex);


		return EXIT_SUCCESS;
    }


    int *conexionInstancia(parametrosConexion* parametros){

        printf("Instancia conectandose de id %d \n",parametros->pid);

        int clavesPrevias = list_size(parametros->claves);

        tInformacionParaLaInstancia * informacion = malloc(sizeof(tInformacionParaLaInstancia));
        informacion->entradas = ENTRADAS;
        informacion->tamanioEntradas = TAMANIO_ENTRADAS;
        informacion->cantidadClaves = clavesPrevias;
		if ((send(parametros->new_fd, informacion, sizeof(tInformacionParaLaInstancia), 0))
				<= 0) {
			perror("Instancia: send informacion");
		}
		free(informacion);
		printf("Instancia: Tiene %d claves previas \n",clavesPrevias);

		for(int i = 0; i < clavesPrevias; i++){
			puts("Instancia: Se envio una clave previa a la instancia");
			if ((send(parametros->new_fd, list_get(parametros->claves,i), TAMANIO_CLAVE, 0))
					<= 0) {
				perror("Instancia: send clave previa");
			}
		}

        while(keepRunning){ // Debo atajar cuando una instancia se me desconecta

        	puts("Instancia: Recibo un aviso de que la Instancia sigue viva");
			tEntradasUsadas *estasConecatada1 = malloc(sizeof(tEntradasUsadas));
			if ((recv(parametros->new_fd, estasConecatada1, sizeof(tEntradasUsadas), 0)) <= 0) {
				perror("Instancia: se desconecto!!!");
				parametros->conectada = 0;
				sem_destroy(parametros->semaforo);
				return OK;
			}
			puts("Instancia: La Instancia sigue viva");

			free(estasConecatada1);

			puts("Instancia: Hago un sem_wait");

			printf("Semaforo en direccion: %p\n", (void*)&(parametros->semaforo));
			sem_wait(parametros->semaforo); // Caundo me avisen que hay una operacion para enviar, la voy a levantar de la cola
			printf("Instancia: Me hicieron un sem_post y tengo de id %d \n",parametros->pid);
			sem_wait(&semaforoInstancia);
			printf("Instancia: Accedi a la seccion de operacion con id %d \n",parametros->pid);
			OperacionAEnviar * operacion = list_remove(colaMensajes,0); //hay que borrar esa operacion


			tOperacionInstanciaStruct * operacionInstancia = malloc(sizeof(tOperacionInstanciaStruct));
			operacionInstancia->operacion = OPERAR;
			if ((send(parametros->new_fd, operacionInstancia, sizeof(tOperacionInstanciaStruct), 0)) // Le informamos que quiero hacer!
							<= 0) {
						puts("Instancia: Fallo al enviar el tipo de operacion");
						perror("send");
						sem_post(&semaforoInstancia);

						RemoverInstanciaDeLaLista(parametros);
						close(parametros->new_fd);
						return 2;
					}
			free(operacionInstancia);
			puts("Instancia: Envie la operacion y voy a recibir respuesta de la Instancia");

			tEntradasUsadas *estasConecatada2 = malloc(sizeof(tEntradasUsadas));
			if ((recv(parametros->new_fd, estasConecatada2, sizeof(tEntradasUsadas), 0)) <= 0) {
				perror("Instancia: se desconecto!!!");
				sem_post(&semaforoInstancia);
				parametros->conectada = 0;
				tResultado * resultadoCompleto = malloc(sizeof(tResultado));
				resultadoCompleto->resultado = ERROR;
				sem_destroy(parametros->semaforo);
				strcpy(resultadoCompleto->clave,operacion->clave);
				pthread_mutex_lock(&mutex);
				list_add(colaResultados,(void*)resultadoCompleto);
				pthread_mutex_unlock(&mutex);
				sem_post(ESIActual->semaforo);
				free(estasConecatada2);
				return OK;
			}

			free(estasConecatada2);

			puts("Instancia: La instancia me respondio, asi que podemos operar");

			if (operacion->tipo != OPERACION_GET){

				printf("Instancia: clave operacion en direccion: %p\n", (void*)&(operacion->clave));
				puts("Instancia: levante un mensaje de la cola de mensajes");
				printf("Instancia: la clave es %s \n",operacion->clave);
				printf("Instancia: el valor es %s \n",operacion->valor);
				printf("Instancia: el tipo de operacion es %d \n",operacion->tipo);
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

				pthread_mutex_lock(&mutex);
				int conexion = EnviarClaveYValorAInstancia(tipo, tamanioValor, parametros, header,	operacion);
				pthread_mutex_unlock(&mutex);

				if (conexion==2){
					parametros->conectada = 0;
					puts("Desaparecio una Instancia");
					free(header);
					sem_post(&semaforoInstancia);
					sem_destroy(parametros->semaforo);
					RemoverInstanciaDeLaLista(parametros);
					return OK;
				}

				//puts("Instancia: Espero resultado de la instancia");
				// Espero el resultado de la operacion
				//tResultadoOperacion * resultado = malloc(sizeof(tResultadoOperacion));


				tEntradasUsadas *buffer = malloc(sizeof(tEntradasUsadas));
				if ((recv(parametros->new_fd, buffer, sizeof(tEntradasUsadas), 0)) <= 0) {
					perror("recv");
					parametros->conectada = 0;
					free(header);
					sem_post(&semaforoInstancia);
					sem_destroy(parametros->semaforo);
					free(buffer);
					return 1;
				}

				puts("Instancia: recibi el resultado de la instancia");

				parametros->entradasUsadas = buffer->entradasUsadas;

				free(buffer);

				puts("Preparamos el resultado");
				tResultado * resultadoCompleto = malloc(sizeof(tResultado));
				resultadoCompleto->resultado = OK;
				strcpy(resultadoCompleto->clave,operacion->clave);

				if(operacion->tipo == OPERACION_STORE){ // Si es STORE la instancia no tiene mas la clave
					bool CompararClaves(char * claveAComprar){
						return string_equals_ignore_case(operacion->clave,claveAComprar);
					}
					void DestruirClaveDeInstancia(char * clave){
						free(clave);
					}
					list_remove_and_destroy_by_condition(parametros->claves,(void *)CompararClaves,(void *)DestruirClaveDeInstancia);
				}

				//Debo avisarle al ESI que me invoco el resultado
				pthread_mutex_lock(&mutex);
				list_add(colaResultados,(void*)resultadoCompleto);
				pthread_mutex_unlock(&mutex);

			}
			else{
				OperaciontHeader * headerGET = malloc(sizeof(OperaciontHeader));  // Creo el header que le voy a enviar a la instancia para que identifique la operacion
				headerGET->tipo = operacion->tipo;
				headerGET->tamanioValor = 0;

				int sendHeader;
				if ((sendHeader = send(parametros->new_fd, headerGET, sizeof(OperaciontHeader), 0))
						<= 0) {
					puts("Fallo al enviar el header");
					perror("send");
					sem_post(&semaforoInstancia);
					RemoverInstanciaDeLaLista(parametros);
					close(parametros->new_fd);
					return 2;
				}
			}

			sem_post(ESIActual->semaforo); // SE HACE AFUERA PORQUE EL GET TAMBIEN DEBE TENER SU POST
			sem_post(&semaforoInstancia);
		}
	close(parametros->new_fd);
			return 1;
	}

    int AnalizarOperacion(int tamanioValor,
    			OperaciontHeader* header, parametrosConexion* parametros,
    			OperacionAEnviar* operacion) {

    	int resultadoOperacion;

    		switch (header->tipo) {
    		case OPERACION_GET: // PARA EL GET NO HAY QUE ACCEDER A NINGUNA INSTANCIA
    			puts("ESI: MANEJO UN GET");
    			resultadoOperacion = ManejarOperacionGET(parametros, operacion);
    			puts("ESI: YA MANEJE UN GET");
    			break;
    		case OPERACION_SET:
    			puts("ESI: MANEJO UN SET");
    			resultadoOperacion = ManejarOperacionSET(tamanioValor, parametros, operacion);
    			puts("ESI: ya maneje un SET");
    			break;
    		case OPERACION_STORE:
    			puts("ESI: MANEJO UN STORE");
    			resultadoOperacion = ManejarOperacionSTORE(parametros, operacion);
    			puts("ESI: YA MANEJE UN STORE");
    			break;
    		default:
    			puts("ESI: ENTRO POR EL DEFAULT");
    			break;
    		}
    		return resultadoOperacion;
    	}

    bool EncontrarClaveEnClavesBloqueadas(t_list * lista, char * claveABuscar){
		bool yaExisteLaClaveEnLasBloqueadas(char* clave) {
			printf("ESI: Comparando la clave %s con %s \n",claveABuscar, clave);
			if (string_equals_ignore_case(clave,claveABuscar) == true){
				puts("ESI: Las claves son iguales");
				return true;
			}
			puts("ESI: Las claves son distintas");
			return false;
		}
		return list_any_satisfy(lista,(void*) yaExisteLaClaveEnLasBloqueadas);
    }

	int ManejarOperacionGET(parametrosConexion* parametros, OperacionAEnviar* operacion) {

		OPERACION_ACTUAL = OPERACION_GET;

		char clave[TAMANIO_CLAVE];
		int recvClave;

		if ( (recvClave = recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			return ERROR;

		}

		clave[strlen(clave)+1] = '\0';
		printf("ESI: Recibi la clave: %s \n", clave);
		strcpy(CLAVE,clave);

		puts("ESI: Verifico en los ESIS si alguno tiene la clave");
		if (((!list_is_empty(listaBloqueos)) && EncontrarEnLista(listaBloqueos, &clave))){
			puts("ESI: La clave esta bloqueada por un ESI");

			/* No es necesario avisarle el bloqueo porque se lo esta avisando el ESI
			notificacion->tipoNotificacion=BLOQUEO;
			strcpy(notificacion->clave,clave);
			notificacion->pid = parametros->pid;
			printf("ESI: le voy a avisar al planificador que se bloqueo la clave: %s \n",clave);
			sem_post(planificador->semaforo);
			puts("ESI: Ya le avise al planificador que se bloqueo la clave");
			*/
			return BLOQUEO;
		} // ACA HAY QUE AVISARLE AL PLANIFICDOR DEL BLOQUEO PARA QUE FRENE AL ESI

		puts("ESI: Verifico en las claves bloqueadas por el planificador");
		if(EncontrarEnLista(clavesTomadas,&clave)){
			puts("ESI: La clave esta bloqueada por consola del planificador");
			return BLOQUEO;
		}

		//list_add(clavesTomadas,&clave);
		char * claveCopia1 = malloc(strlen(clave)+1);
		char * claveCopia2 = malloc(strlen(clave)+1);
		strcpy(claveCopia1,clave);
		strcpy(claveCopia2,clave);
		pthread_mutex_lock(&mutex);
		list_add(clavesTomadas,claveCopia1);
		list_add(parametros->claves,claveCopia2);
		pthread_mutex_unlock(&mutex);

		operacion->tipo = OPERACION_GET;
		strcpy(operacion->clave,clave);
		operacion->valor = NULL;
		char GetALoguear[4+strlen(clave)+1];
		strcpy(GetALoguear, "GET ");
		puts(GetALoguear);
		strcat(GetALoguear, clave);
		GetALoguear[4+strlen(clave)]='\0';
		puts(GetALoguear);

		tBloqueo *bloqueo = malloc(sizeof(tBloqueo));
		strcpy(bloqueo->clave,clave);
		bloqueo->pid = parametros->pid;

		list_add(listaBloqueos,(void *)bloqueo);
		// No le aviso al planificador que este ESI logro tomar x clave, se lo va a avisar el ESI, aunque guardo esta informacion en una lista


		log_info(logger, GetALoguear);

		return OK;
	}

	int ManejarOperacionSET(int tamanioValor, parametrosConexion* parametros, OperacionAEnviar* operacion) {

		OPERACION_ACTUAL = OPERACION_SET;

		char clave[TAMANIO_CLAVE];
		int recvClave, recvValor;
		if ((recvClave = recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			return ERROR;

		}
		clave[strlen(clave)+1] = '\0';
		printf("ESI: Recibi la clave: %s\n", clave);

		if (!laClaveTuvoUnGETPrevio(&clave,parametros)){
			puts("ESI: La clave nunca tuvo un GET");

			notificacion->tipoNotificacion=ERROR;
			strcpy(notificacion->clave,clave);
			notificacion->pid = parametros->pid;
			printf("ESI: le voy a avisar al planificador que hay que abortar al ESI por no hacer GET sobre: %s \n",clave);
			sem_post(planificador->semaforo);
			puts("ESI: Ya le avise al planificador que aborte el ESI");

			return ERROR;
		}


		strcpy(CLAVE,clave);

		tBloqueo *bloqueo = malloc(sizeof(tBloqueo));
		strcpy(bloqueo->clave,clave);
		bloqueo->pid = parametros->pid;

		if (!(!list_is_empty(listaBloqueos) && LePerteneceLaClave(listaBloqueos, bloqueo))){
			printf("ESI: No se puede realizar un SET sobre la clave: %s debido a que nunca se la solicito \n",clave);
		}
		free(bloqueo);

		char *valor = malloc(tamanioValor+1);

		if (( recvValor = recv(parametros->new_fd, valor, tamanioValor+1, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			return ERROR;

		}
		valor[tamanioValor] = '\0';
		printf("ESI: Recibi el valor: %s \n", valor);
		operacion->tipo = OPERACION_SET;
		strcpy(operacion->clave,clave);
		operacion->valor= valor;
		printf("ESI: El valor dentro de operacion es %s \n",operacion->valor);
		char SetALoguear[5+strlen(clave)+strlen(valor)+1]; // SET jugador TraemeLaCopa da 24
		strcpy(SetALoguear, "SET ");
		puts(SetALoguear);
		strcat(SetALoguear, clave);
		puts(SetALoguear);
		strcat(SetALoguear, " ");
		strcat(SetALoguear, valor);
		SetALoguear[5+strlen(clave)+strlen(valor)]='\0';
		puts(SetALoguear);

		log_info(logger, SetALoguear);

		return OK;
	}

	int ManejarOperacionSTORE(parametrosConexion* parametros, OperacionAEnviar* operacion) {

		OPERACION_ACTUAL = OPERACION_STORE;

		char clave[TAMANIO_CLAVE];
		int resultadoRecv;

		if ((resultadoRecv =recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());

			return ERROR;
		}
		clave[strlen(clave)+1] = '\0';
		printf("ESI: Recibi la clave: %s\n", clave);

		if (!laClaveTuvoUnGETPrevio(clave,parametros)){
			puts("ESI: La clave nunca tuvo un GET");

			notificacion->tipoNotificacion=ERROR;
			strcpy(notificacion->clave,clave);
			notificacion->pid = parametros->pid;
			printf("ESI: le voy a avisar al planificador que hay que abortar al ESI por no hacer GET sobre: %s \n",clave);
			sem_post(planificador->semaforo);
			puts("ESI: Ya le avise al planificador que aborte el ESI");

			return ERROR;
		}
		strcpy(CLAVE,clave);

		tBloqueo *bloqueo = malloc(sizeof(tBloqueo));
		strcpy(bloqueo->clave,clave);
		bloqueo->pid = parametros->pid;

		if (!list_is_empty(listaBloqueos) && LePerteneceLaClave(listaBloqueos, bloqueo)){
			printf("ESI: Se desbloqueo la clave: %s \n",clave);

			notificacion->tipoNotificacion=DESBLOQUEO;
			strcpy(notificacion->clave,clave);
			notificacion->pid = parametros->pid;

			// TODO: Las 2 primeras veces que elimino debo solamente remover sin destruir para que en el resto no quede basura

			printf("ESI: Size de la listaBloqueos antes de remover: %d\n", list_size(listaBloqueos));
			RemoverClaveDeLaListaBloqueos(clave);
			printf("ESI: Size de la listaBloqueos despues de remover: %d\n", list_size(listaBloqueos));

			printf("ESI: Size de la lista clavesTomadas antes de remover: %d\n", list_size(clavesTomadas));
			RemoverClaveDeClavesTomadas(clave);
			printf("ESI: Size de la lista clavesTomadas despues de remover: %d\n", list_size(clavesTomadas));

			printf("ESI: Size de las clavesPropias antes de remover: %d\n", list_size(parametros->claves));
			RemoverClaveDeClavesPropias(clave, parametros);
			printf("ESI: Size de las clavesPropias despues de remover: %d\n", list_size(parametros->claves));

			printf("ESI: le voy a avisar al planificador que se desbloqueo la clave: %s \n",clave);
			sem_post(planificador->semaforo);
			puts("ESI: Ya le avise al planificador del desbloqueo");

			// LE AVISO AL PLANIFICADOR QUE LA CLAVE SE DESBLOQUEO, DESPUES EL PODRA PLANIFICAR UN ESI BLOQUEADO POR ESTA
		}
		else{
			printf("ESI: No se puede realizar un STORE sobre la clave: %s debido a que nunca se la solicito \n",clave);
		}

		free(bloqueo);

		operacion->tipo = OPERACION_STORE;
		strcpy(operacion->clave,clave);
		operacion->valor = NULL;

		char StoreALoguear[6+strlen(clave)+1];
		strcpy(StoreALoguear, "STORE ");
		puts(StoreALoguear);
		strcat(StoreALoguear, clave);
		StoreALoguear[6+strlen(clave)]='\0';
		puts(StoreALoguear);

		log_info(logger, StoreALoguear);

		return OK;
	}

	int ConexionESISinBloqueo(OperacionAEnviar* operacion,
			parametrosConexion* parametros) {
		//Debo avisar a una Instancia cuando recibo una operacion (QUE NO SEA UN GET)
		//Agregamos el mensaje a una cola en memoria
		printf("ESI: clave de la operacion: %s \n", operacion->clave);
		printf("ESI: La operacion es de tipo: %d\n",operacion->tipo);


		//free(operacion);

		puts("ESI: Vamos a chequear el tipo");

		//if(operacion->tipo != OPERACION_GET){
			pthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola
			printf("ESI: clave operacion en direccion: %p\n", (void*)&(operacion->clave));
			list_add(colaMensajes, (void*) operacion);
			pthread_mutex_unlock(&mutex);
		//}

		ESIActual = parametros;

		puts("ESI: Voy a seleccionar la Instancia");
		int seleccionInstancia = SeleccionarInstancia(&CLAVE);
		puts("ESI: Se selecciono la Instancia");

		if(seleccionInstancia == ERROR){
			tResultado* resultadoError = malloc(sizeof(tResultado));
			resultadoError->resultado = ERROR;
			strcpy(resultadoError->clave,operacion->clave);

			if ((send(parametros->new_fd, resultadoError, sizeof(tResultado),
					0)) <= 0) {
				close(parametros->new_fd);
				perror("send");
				//exit_gracefully(1);
				return 1;
			}
			puts("ESI: Se envio el resultado");
			free(resultadoError);
			LiberarLasClavesDelESI(parametros);
			return ERROR;
		}

		if(operacion->tipo != OPERACION_GET){
			printf("ESI: valor de la operacion: %s \n", operacion->valor);

			//esperamos el resultado para devolver
			puts("ESI: Vamos a ver si hay algun resultado en la cola");

			// ---------------------------------
			// while (list_is_empty(colaResultados)); // TODO ESO ES UNA ESPERA ACTIVA
			sem_wait(ESIActual->semaforo);
			// ----------------------------------
			puts("ESI: Tenemos un resultado en la cola");

			// mientras la cola este vacia no puedo continuarputs("ESI: Hay resultado en la cola");
			pthread_mutex_lock(&mutex);
			tResultado* resultado = list_remove(colaResultados, 0);
			pthread_mutex_unlock(&mutex);

			int sendResultado;
			printf("ESI: Por enviar el resultado de la clave %s \n",resultado->clave);
			if ((sendResultado = send(parametros->new_fd, resultado, sizeof(tResultado),
					0)) <= 0) {
				close(parametros->new_fd);
				perror("send");
				//exit_gracefully(1);
				return 1;
			}
			puts("ESI: Se envio el resultado");
			free(resultado);
		}
		else {
			sem_wait(ESIActual->semaforo);
			if(seleccionInstancia==2){
				tResultado* resultado = malloc(sizeof(tResultado));

				strcpy(resultado->clave, operacion->clave);
				resultado->resultado = ERROR;
				int sendResultado;
				puts("ESI: Por enviar el resultado");
				if ((sendResultado = send(parametros->new_fd, resultado, sizeof(tResultado),
						0)) <= 0) {
					perror("send");
					close(parametros->new_fd);
					//exit_gracefully(1); todo: que pasa aca?
					return 1;
				}
				puts("ESI: Se envio el resultado");
				free(resultado);
			}
			else{
				tResultado* resultado = malloc(sizeof(tResultado));

				strcpy(resultado->clave, operacion->clave);
				resultado->resultado = OK;
				int sendResultado;
				puts("ESI: Por enviar el resultado");
				if ((sendResultado = send(parametros->new_fd, resultado, sizeof(tResultado),
						0)) <= 0) {
					perror("send");
					close(parametros->new_fd);
					//exit_gracefully(1); todo: que pasa aca?
					return 1;
				}
				puts("ESI: Se envio el resultado");
				free(resultado);
			}
		}

		return EXIT_SUCCESS;
	}

	int InicializarListasYColas() {

		notificacion = malloc(sizeof(tNotificacionPlanificador));

		// Creamos una cola donde dejamos todas las instancias que se conectan con nosotros y otra para los mensajes recibidos de cualquier ESI
		colaInstancias = list_create();

		colaMensajes = list_create();

		colaResultados = list_create();

		colaESIS = list_create();

		listaBloqueos = list_create();

		clavesTomadas = list_create();

		//colaMensajesParaPlanificador = list_create();

		return EXIT_SUCCESS;
	}

    int exit_gracefully(int return_nr) {

    	puts("ENTRE AL EXIT GRACEFULLY ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR");
       log_destroy(logger);
       // list_clean_and_destroy_elements(colaInstancias,(void *)destruirInstancia); // TIRA DOUBLE FREE CORRUPTION
       list_clean_and_destroy_elements(colaMensajes,(void *)destruirOperacionAEnviar);
       list_clean_and_destroy_elements(colaResultados,(void *)destruirResultado);
       // list_clean_and_destroy_elements(colaESIS,(void *)destruirInstancia);// TIRA DOUBLE FREE CORRUPTION (hay que adaptar a ESIS)
       list_clean_and_destroy_elements(listaBloqueos,(void *)destruirBloqueo);
       free(notificacion);
       list_clean_and_destroy_elements(clavesTomadas,(void *)borrarClave);
       pthread_mutex_destroy(&mutex);
       sem_destroy(&semaforoInstancia);
       exit(return_nr);

		return return_nr;
     }

    int configure_logger() {
      logger = log_create("Coordinador.log", "CORD", true, LOG_LEVEL_INFO);

      return EXIT_SUCCESS;
     }

    bool EncontrarEnLista(t_list * lista, char * claveABuscar){
		bool yaExisteLaClave(tBloqueo *bloqueo) {
			printf("ESI: Claves Tomadas -- Comparando la clave %s con %s \n",claveABuscar, bloqueo->clave);
			if (string_equals_ignore_case(bloqueo->clave,claveABuscar) == true){
				puts("ESI: Las claves son iguales");
				return true;
			}
			puts("ESI: Las claves son distintas");
			return false;
		}
		return list_any_satisfy(lista,(void*) yaExisteLaClave);
    }

    bool LePerteneceLaClave(t_list * lista, tBloqueo * bloqueoBuscado){
		bool yaExisteLaClaveYPerteneceAlESI(tBloqueo *bloqueo) {
			return ((bloqueo->pid == bloqueoBuscado->pid) && (string_equals_ignore_case(bloqueo->clave,bloqueoBuscado->clave)) == true);
		}
		return list_any_satisfy(lista,(void*) yaExisteLaClaveYPerteneceAlESI);
    }

    bool laClaveTuvoUnGETPrevio(char * clave, parametrosConexion * parametros){
    	bool existeLaClave(char * claveAComparar){
    		printf("ESI: Comparo la clave %s con %s \n", clave,claveAComparar);
    		return string_equals_ignore_case(clave,claveAComparar);
    	}
    	if(list_is_empty(parametros->claves) == true){
    		puts("ESI: la lista de las claves esta vacia");
    		return false;
    	}
    	puts("ESI: la lista de las claves no esta vacia");
    	return list_any_satisfy(parametros->claves,existeLaClave);
    }

    parametrosConexion* BuscarInstanciaQuePoseeLaClave(char * clave){
    	bool tieneLaClave(char * claveAComparar){
    		printf("ESI: Buscando instancia, comparando la clave %s con %s \n", clave, claveAComparar);
    		if(string_equals_ignore_case(clave,claveAComparar) == true){
    			puts("ESI: Las claves coincidieron");
    			return true;
    		}
    		puts("ESI: Las claves no coincidieron");
    		return false;
    	}
    	bool lePerteneceLaClave(parametrosConexion * parametros){
    		printf("ESI: Busco una instancia que tenga la clave: %s \n", clave);
    		if(list_any_satisfy(parametros->claves,tieneLaClave) == true){
    			printf("ESI: Encontre la instancia que posee la clave: %d \n",parametros->pid);
    			return true;
    		}
    		printf("ESI: La instancia %d no posee la clave \n",parametros->pid);
    		return false;
    	}
    	return list_find(colaInstancias,(void*) lePerteneceLaClave);
    }

    parametrosConexion* BuscarInstanciaMenosUsada(){
    	int tamanioLista = list_size(colaInstancias);

    	parametrosConexion* instancia = list_get(colaInstancias,0);
    	for (int i = 0; i< tamanioLista; i++){
    		parametrosConexion* instanciaAComparar = malloc(sizeof(parametrosConexion));
        	instancia = list_get(colaInstancias,i);
        	if (instancia->conectada == 0)
        		instancia = instanciaAComparar;
        	if (instancia->entradasUsadas > instanciaAComparar->entradasUsadas && instanciaAComparar->conectada == 1)
        		instancia = instanciaAComparar;
    		free(instanciaAComparar);
    	}

    	MandarAlFinalDeLaLista(colaInstancias, &instancia);
    	return instancia;
    }

    int MandarAlFinalDeLaLista(t_list * lista, parametrosConexion * instancia){
    	bool Comparar(parametrosConexion * instanciaAComparar){
    		return instanciaAComparar->new_fd == instancia->new_fd;
    	}
    	//t_list * listaNueva = malloc(sizeof(lista));

    	t_list * listaNueva = list_take_and_remove(lista, 1);
    	list_add_all(lista,listaNueva);
    	//free(listaNueva);
    	return 1;
    }

    void sigchld_handler(int s) // eliminar procesos muertos
    {
        while(wait(NULL) > 0);
    }

	int  LeerArchivoDeConfiguracion(char *argv[]) {
		// Leer archivo de configuracion con las commons

		t_config* configuracion;
		configuracion = config_create(argv[1]);
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

	int RemoverInstanciaDeLaLista(parametrosConexion* parametros){
		bool sonLasMismasnstancias(parametrosConexion * parametrosAComparar){
			return parametrosAComparar->pid == parametros->pid;
		}
		list_remove_and_destroy_by_condition(colaInstancias,(void*)sonLasMismasnstancias,(void*)destruirInstancia);
		return 1;
	}

	int EnviarClaveYValorAInstancia(tTipoOperacion tipo, int tamanioValor,
			parametrosConexion* parametros, OperaciontHeader* header,
			OperacionAEnviar* operacion) {

		if (parametros->conectada == 0)
			return 2;

		puts("Instancia: Envio header a la instancia");
		// Envio el header a la instancia


		int sendHeader;
		if ((sendHeader = send(parametros->new_fd, header, sizeof(OperaciontHeader), 0))
				<= 0) {
			puts("Fallo al enviar el header");
			perror("send");

			close(parametros->new_fd);
			return 2;
		}

		if (parametros->conectada == 0)
			return 2;

		printf("Instancia: Voy a enviarle la operacion de tipo: %d\n",header->tipo);
		free(header);
		printf("Instancia: Envio clave %s a la instancia\n", operacion->clave);
		// Envio la clave
		int sendClave;
		if ((sendClave = send(parametros->new_fd, operacion->clave, TAMANIO_CLAVE,0)) <= 0) {
			puts("Fallo al enviar la clave");
			perror("send");

			close(parametros->new_fd);
			return 2;
		}

		if (parametros->conectada == 0)
			return 2;

		if (tipo == OPERACION_SET) {
			int sendSet;
			printf("Instancia: Envio valor %s a la instancia\n", operacion->valor);
			if ((sendSet = send(parametros->new_fd, operacion->valor, tamanioValor, 0)) <= 0){
				perror("send");

				close(parametros->new_fd);
				return 2;
			}
		}

		return EXIT_SUCCESS;
	}

	int SeleccionarInstancia(char* clave) {
		int resultado;
		// mientras la cola este vacia no puedo continuar

		pthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola
		switch (ALGORITMO){
		case EL: ; // Si no se deja un statement vacio rompe
		resultado = SeleccionarPorEquitativeLoad(clave);
			break;
		case LSU:
			resultado = SeleccionarPorLeastSpaceUsed(clave);
			break;
		case KE:
			resultado = SeleccionarPorKeyExplicit(clave);
			break;
		default:
			puts("ESI: Hubo un problema al seleccionar la instancia correcta");
			exit_gracefully(1);
		}
		pthread_mutex_unlock(&mutex);
		//saco la primer instancia de la cola pero luego tengo que deolverla a la colasem_post(instancia->semaforo); // Le aviso al semaforo de la instancia de que puede operar (el semaforo es el tid)

		return resultado;
	}

	int SeleccionarPorEquitativeLoad(char* clave) {
		// mientras la cola este vacia no puedo continuarpthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola
		parametrosConexion * instancia;
		int conectada = 1;

		if(OPERACION_ACTUAL == OPERACION_GET){
				while(conectada){

				bool NoEstaConectada(parametrosConexion * parametros){
					printf("ESI: La instancia %s con pid %d tiene el flag conectada en %d",parametros->nombreProceso,parametros->pid,parametros->conectada);
					return parametros->conectada != 1;
				}

				if (list_all_satisfy(colaInstancias,(void*)NoEstaConectada) == true){
					puts("ESI: No hay Instancias conectadas para operar, se debe abortar");
					return ERROR;
				}

				instancia = list_remove(colaInstancias,0);
				if(instancia == NULL){
					puts("ESI: No hay Instancia para operar, se debe abortar");
					return ERROR;
				}
				if (instancia->conectada == 1){
					printf("ESI: Agrego la clave %s a la instancia %d \n",clave,instancia->pid);
					char * claveCopia = malloc(TAMANIO_CLAVE);
					strcpy(claveCopia,clave);
					printf("ESI: Agrego la copia clave %s a la instancia %d \n",claveCopia,instancia->pid);
					list_add(instancia->claves,claveCopia);
					//MandarAlFinalDeLaLista(colaInstancias,instancia);
					printf("ESI: Le hago el sem_post al semaforo en direccion %p \n",(void *)&(instancia->semaforo));
					sem_post(instancia->semaforo);
					conectada = 0;
				}
				list_add(colaInstancias, instancia);
			}
		}
		else{
			instancia = BuscarInstanciaQuePoseeLaClave(clave);
			printf("ESI: La Instancia %s tiene el flag conectada en %d \n",instancia->nombreProceso,instancia->conectada);

			if ((instancia == NULL) || (instancia->conectada != 1)){ //Hay que ver si devuelve NULL, esto es en caso de que se desconecte la instancia
				puts("ESI: Se desconecto la instancia");
				return ERROR;
			}

			printf("ESI: Semaforo de list_get en direccion: %p\n", (void*)&(instancia->semaforo));
			//list_remove_and_destroy_element(colaInstancias, 0,(void*)destruirInstancia);
			puts("ESI: Voy a hacer el sem_post a la Instancia seleccionada \n");
			sem_post(instancia->semaforo);
			//sem_post(instancia->informacion->semaforo);

			//list_add(colaInstancias,instancia);
		}

		//free(instancia);
		return 1;
	}

	int SeleccionarPorLeastSpaceUsed(char * clave){
		parametrosConexion* instanciaMenosUsada;

		if(OPERACION_ACTUAL == OPERACION_GET){
			instanciaMenosUsada = BuscarInstanciaMenosUsada(clave); // Va a buscar la instancia que menos entradas tenga, desempata con fifo
			printf("ESI: Agrego la clave %s a la instancia %d \n",clave,instanciaMenosUsada->pid);
			char * claveCopia = malloc(TAMANIO_CLAVE);
			strcpy(claveCopia,clave);
			printf("ESI: Agrego la copia clave %s a la instancia %d \n",claveCopia,instanciaMenosUsada->pid);
			sem_post(instanciaMenosUsada->semaforo);
			list_add(instanciaMenosUsada->claves,claveCopia);
		}
		else{
			instanciaMenosUsada = BuscarInstanciaQuePoseeLaClave(clave);

			if (instanciaMenosUsada == NULL || instanciaMenosUsada->conectada != 1){ //Hay que ver si devuelve NULL, esto es en caso de que se desconecte la instancia
				puts("ESI: Se desconecto la instancia");
				return ERROR;
			}

			printf("ESI: Semaforo de list_get en direccion: %p\n", (void*)&(instanciaMenosUsada->semaforo));
			//list_remove_and_destroy_element(colaInstancias, 0,(void*)destruirInstancia);
			puts("ESI: Voy a hacer el sem_post a la Instancia seleccionada \n");
			sem_post(instanciaMenosUsada->semaforo);
		}
		return 1;
	}

	int SeleccionarPorKeyExplicit(char* clave){
		parametrosConexion * instancia;
		if(OPERACION_ACTUAL == OPERACION_GET){
			// TODO: Debo filtrar las instancias que no esten conectadas para no contar con ellas!
			t_list * listaFiltrada;
			listaFiltrada = list_filter(colaInstancias,(void*)EstaConectada);

			int cantidadInstancias = list_size(listaFiltrada);
			char primerCaracter = clave[0];
			int x = 0;
			while ((clave[x] < 97 && clave[x] > 122) && (clave[x] < 65 && clave[x] > 90)){ // Busco el primer caracter en minuscula/mayuscula
				primerCaracter = clave[x];
				x++;
			}
			int posicionLetraEnASCII;
			if (primerCaracter >= 97)
				posicionLetraEnASCII = primerCaracter - 97;
			else
				posicionLetraEnASCII = primerCaracter - 65;
			int rango = KEYS_POSIBLES/cantidadInstancias;
			int restoRango = KEYS_POSIBLES%cantidadInstancias;
			int entradasUltimaInstancia;
			if (restoRango !=0){
				entradasUltimaInstancia = KEYS_POSIBLES - ((cantidadInstancias-1) * (rango + 1));
			}

			for (int i = 0; i < cantidadInstancias; i++){
				if (i!= cantidadInstancias - 1 && restoRango == 0){    // Si no es la ultima instancia debo redondear hacia arriba
						if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + rango)){
							instancia = list_get(listaFiltrada, i);
							//sem_post(instancia->semaforo);
						}
					}
					else if (i!= cantidadInstancias - 1 && restoRango != 0){ // si es la ultima instancia debo recalcular y ver cuantas entradas puedo aceptar
						if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + rango + 1)){
							instancia = list_get(listaFiltrada, i);
							//sem_post(instancia->semaforo);
						}else{
							if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + entradasUltimaInstancia)){
								instancia = list_get(listaFiltrada, i);
								//sem_post(instancia->semaforo);
						}
					}
				}
			}

			printf("ESI: Agrego la clave %s a la instancia %d \n",clave,instancia->pid);
			char * claveCopia = malloc(TAMANIO_CLAVE);
			strcpy(claveCopia,clave);
			printf("ESI: Agrego la copia clave %s a la instancia %d \n",claveCopia,instancia->pid);
			list_add(instancia->claves,claveCopia);
		}
		else {
			instancia = BuscarInstanciaQuePoseeLaClave(clave);

			if (instancia == NULL || instancia->conectada != 1){ //Hay que ver si devuelve NULL, esto es en caso de que se desconecte la instancia
				puts("ESI: Se desconecto la instancia");
				return ERROR;
			}

			printf("ESI: Semaforo de list_get en direccion: %p\n", (void*)&(instancia->semaforo));
			//list_remove_and_destroy_element(colaInstancias, 0,(void*)destruirInstancia);
			puts("ESI: Voy a hacer el sem_post a la Instancia seleccionada \n");
			sem_post(instancia->semaforo);
		}
		return 1;
	}

	static void destruirOperacionAEnviar(OperacionAEnviar * operacion){
		free(operacion);
	}

	static void destruirResultado(tResultado * resultado){
		free(resultado);
	}

	static void destruirBloqueo(tBloqueo *bloqueo) {
		printf("Vor a borrar la clave %s \n",bloqueo->clave);
	    free(bloqueo);
	    puts("logre borrar la clave");
	}

	static void destruirInstancia(parametrosConexion * parametros){
		puts("Elimino una instancia");
		free(parametros);
		puts("Falle al eliminar una instanca");
	}

	static void borrarClave(char * clave){
		free(clave);
	}

	int verificarValidez(int sockfd){
		tValidezOperacion *validez = malloc(sizeof(tValidezOperacion));
		if(recv(sockfd, validez, sizeof(tValidezOperacion), 0) <= 0){
			puts("Se desconecto el ESI (?");
			return 2;  //Aca salimos
		}

		if(validez == OPERACION_INVALIDA){
			close(sockfd);
			puts("ESI: Se cerró la conexión con el ESI debido a una OPERACION INVALIDA");
		}
		free(validez);
		return EXIT_SUCCESS;
	}

    int verificarParametrosAlEjecutar(int argc, char *argv[]){

        if (argc != 2) {//argc es la cantidad de parametros que recibe el main.
        	puts("Error al ejecutar, para correr este proceso deberias ejecutar: ./Coordinador \"nombreArchivo\"");
            exit(1);
        }

        return EXIT_SUCCESS;
    }

    bool EstaConectada(parametrosConexion * instancia){
    	return instancia->conectada == 1;
    }


    char * SimulacionSeleccionarPorEquitativeLoad(char* clave) {
    		// mientras la cola este vacia no puedo continuarpthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola
    		parametrosConexion * instancia;

    		int cantidadInstancias = list_size(colaInstancias);
    		printf("Planificador: Hay %d instancias (conectadas y no conectadas) \n",cantidadInstancias);
    		if(cantidadInstancias == 0)
    			return NULL;

    		if((list_any_satisfy(colaInstancias,(void*)EstaConectada)) == false)
    			return NULL;

			instancia = list_find(colaInstancias,(void*)EstaConectada);

			if(instancia == NULL)
				return NULL;

			printf("Planificador: El nombre de la Instancia que tendria el planificador es %s \n",instancia->nombreProceso);

			return instancia->nombreProceso;

    	}

    	char * SimulacionSeleccionarPorLeastSpaceUsed(char * clave){
    		parametrosConexion* instanciaMenosUsada;

			instanciaMenosUsada = BuscarInstanciaMenosUsada(clave); // Va a buscar la instancia que menos entradas tenga, desempata con fifo

			if (instanciaMenosUsada == NULL)
				return NULL;

			printf("Planificador: El nombre de la Instancia que tendria el planificador es %s \n",instanciaMenosUsada->nombreProceso);

			return instanciaMenosUsada->nombreProceso;
    	}

    	char *  SimulacionSeleccionarPorKeyExplicit(char* clave){
    		parametrosConexion * instancia;

			t_list * listaFiltrada;
			listaFiltrada = list_filter(colaInstancias,(void*)EstaConectada);

			int cantidadInstancias = list_size(listaFiltrada);
			char primerCaracter = clave[0];
			int x = 0;
			while ((clave[x] < 97 && clave[x] > 122) && (clave[x] < 65 && clave[x] > 90)){ // Busco el primer caracter en minuscula/mayuscula
				primerCaracter = clave[x];
				x++;
			}
			int posicionLetraEnASCII;
			if (primerCaracter >= 97)
				posicionLetraEnASCII = primerCaracter - 97;
			else
				posicionLetraEnASCII = primerCaracter - 65;
			int rango = KEYS_POSIBLES/cantidadInstancias;
			int restoRango = KEYS_POSIBLES%cantidadInstancias;
			int entradasUltimaInstancia;
			if (restoRango !=0){
				entradasUltimaInstancia = KEYS_POSIBLES - ((cantidadInstancias-1) * (rango + 1));
			}

			for (int i = 0; i < cantidadInstancias; i++){
				if (i!= cantidadInstancias - 1 && restoRango == 0){    // Si no es la ultima instancia debo redondear hacia arriba
						if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + rango)){
							instancia = list_get(listaFiltrada, i);
							//sem_post(instancia->semaforo);
						}
					}
					else if (i!= cantidadInstancias - 1 && restoRango != 0){ // si es la ultima instancia debo recalcular y ver cuantas entradas puedo aceptar
						if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + rango + 1)){
							instancia = list_get(listaFiltrada, i);
							//sem_post(instancia->semaforo);
						}else{
							if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + entradasUltimaInstancia)){
								instancia = list_get(listaFiltrada, i);
								//sem_post(instancia->semaforo);
						}
					}
				}
			}

			if(instancia == NULL)
				return NULL;

			printf("Planificador: El nombre de la Instancia que tendria el planificador es %s \n",instancia->nombreProceso);

			return instancia->nombreProceso;
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





