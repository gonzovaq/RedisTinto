/*
 * Coordinador.c
 *
 *  Created on: 4 abr. 2018
 *      Author: utnso
 */

#include "Coordinador.h"

    int main(int argc, char *argv[])
    {
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

    				parametros.pid = 0;

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
	   tHeader *headerRecibido = malloc(sizeof(tHeader));
	   parametrosConexion *parametrosNuevos=  malloc(sizeof(parametrosConexion));
	   strcpy(parametrosNuevos->nombreProceso, parametros->nombreProceso);
	   parametrosNuevos->new_fd = parametros->new_fd;
	   parametrosNuevos-> cantidadEntradasMaximas = parametros->cantidadEntradasMaximas;
	   parametrosNuevos-> entradasUsadas = parametros->entradasUsadas;

	   if ((recv(parametrosNuevos->new_fd, headerRecibido, sizeof(tHeader), 0)) <= 0){

		perror("recv");
		log_info(logger, "Mensaje: recv error");//process_get_thread_id()); //asienta error en logger y corta
		//exit_gracefully(1);
					//exit(1);
	   }

	   if (headerRecibido->tipoMensaje == CONECTARSE){
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
    		printf("Instancia: Se conecto el proceso %d \n", headerRecibido->idProceso);
    		printf("Socket de la instancia: %d\n", parametros->new_fd);
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
			printf("Instancia: Semaforo en direccion: %p\n", (void*)&(parametros->semaforo));
			parametros->claves = list_create();
    		list_add(colaInstancias,(void*)parametros);
    		conexionInstancia(parametros);
    		free(semaforo);
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
				free(header);
				return 1;
				//exit_gracefully(1);
			}

			puts("ESI: INTENTO RECIBIR TAMANIO VALOR");
			int tamanioValor = header->tamanioValor; // Si es un STORE o un GET, el ESI va a enviar 0
			puts("ESI: RECIBI TAMANIO VALOR");
			printf("ESI: Tamaño valor: %d \n",tamanioValor);
			OperacionAEnviar * operacion = malloc(sizeof(OperacionAEnviar));//+TAMANIO_CLAVE+tamanioValor);

			// Si la operacion devuelve 1 todo salio bien, si devuelve 2 hubo un bloqueo y le avisamos al ESI
			int resultadoOperacion;
			resultadoOperacion = AnalizarOperacion(tamanioValor, header, parametros, operacion);
			printf("%d",resultadoOperacion);
			switch (resultadoOperacion){
			case 1:
				puts("ESI: El analizar operacion anduvo");

				free(header);
				sleep(RETARDO/100);

				//Debo avisar a una Instancia cuando recibo una operacion (QUE NO SEA UN GET)
				//Agregamos el mensaje a una cola en memoria
				ConexionESISinBloqueo(operacion,parametros);

				break;
			case 2: // Que hacemos si hay bloqueo? Debemos avisarle al planificador

				if ((send(parametros->new_fd, BLOQUEO, sizeof(tResultadoOperacion), 0)) <= 0){
					perror("send");
					//exit_gracefully(1);
				}
				free(header);

				break;
			case 3:
				puts("ESI: Error en la soliciud");

				if ((send(parametros->new_fd, ERROR, sizeof(tResultadoOperacion), 0)) <= 0){
					perror("send");
					//exit_gracefully(1);
				}
				free(header);

				break;
			default:
				puts("ESI Fallo al ver el resultado de la operacion");
				break;
			}
			free(operacion);
        }
		close(parametros->new_fd);
		return EXIT_SUCCESS;
    }

    int *conexionPlanificador(parametrosConexion* parametros){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("Planificador conectandose");

        RecibirClavesBloqueadas(parametros);

        //Abro otro hilo para escuchar sus comandos solicitados por consola
        pthread_t tid;
        int stat = pthread_create(&tid, NULL, (void*)escucharMensajesDelPlanificador, (void*)&parametros);
		if (stat != 0){
			puts("Planificador: error al generar el hilo");
			perror("thread");
			return 1;
			//continue;
		}
		pthread_detach(tid); //Con esto decis que cuando el hilo termine libere sus recursos

        // Le avisamos al planificador cada clave que se bloquee y desbloquee
        while(1){
        	puts("Planificador: Espero a que me avisen!");
        	sem_wait(planificador->semaforo); // Me van a avisar si se produce algun bloqueo
        	puts("Planificador: Hay algo para avisarle al planificador");
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
		char clave[TAMANIO_CLAVE];
		if ((recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			//exit_gracefully(1);
		} else
			printf("Planificador: Agrego la clave %s a la cola de bloqueadas \n", clave);
			list_add(clavesTomadas, &clave);

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
    	while(1){
    		// Aca vamos a Escuchar todos los mensajes que solicite el planificador, hay que ver cuales son y vemos que hacemos
    		tSolicitudesDeConsola * solicitud = malloc(sizeof(tSolicitudesDeConsola));
			if ((recv(parametros->new_fd, solicitud, sizeof(tSolicitudesDeConsola), 0)) <= 0) {
				//perror("recv");
				//log_info(logger, "TID %d  Mensaje: ERROR en ESI",process_get_thread_id());
				close(parametros->new_fd);
				//exit_gracefully(1);
			}
			char clave[TAMANIO_CLAVE];
			switch(*solicitud){
			case BLOQUEAR:
				if ( (recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
					perror("recv");
					log_info(logger, "TID %d  Mensaje: ERROR en ESI",
							process_get_thread_id());
					return 3;
					//exit_gracefully(1);
				}
				else
					list_add(clavesTomadas,&clave);
				break;
			case DESBLOQUEAR:

				if ( (recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
					perror("recv");
					log_info(logger, "TID %d  Mensaje: ERROR en ESI",
							process_get_thread_id());
					return 3;
					//exit_gracefully(1);
				}
				else ;
					RemoverClaveDeBloqueados(&clave);
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
				//puts("Planificador: Me envio cualquier cosa");
				break;
			}

			free(solicitud);
		}

    	return EXIT_SUCCESS;
    }

    int RemoverClaveDeBloqueados(char * clave){
		bool compararClaveParaDesbloquear(char *claveABuscar){
			printf("Planificador: Comparando la clave %s con %s \n", claveABuscar, clave);
			return string_equals_ignore_case(clave,claveABuscar) == true;
		}
		list_remove_and_destroy_by_condition(clavesTomadas,(void*)compararClaveParaDesbloquear,(void*)borrarClave);
		return EXIT_SUCCESS;
    }


    int *conexionInstancia(parametrosConexion* parametros){
    	//close(new_fd->sockfd); // El hijo no necesita este descriptor aca -- Esto era cuando lo haciamos con fork
        puts("Instancia conectandose");

        tInformacionParaLaInstancia * informacion = malloc(sizeof(tInformacionParaLaInstancia));
        informacion->entradas = ENTRADAS;
        informacion->tamanioEntradas = TAMANIO_ENTRADAS;
		if ((send(parametros->new_fd, informacion, sizeof(tInformacionParaLaInstancia), 0))
				<= 0) {
			perror("send");
			//exit_gracefully(1);
		}
		free(informacion);

        while(1){ // Debo atajar cuando una instancia se me desconecta

			puts("Instancia: Hago un sem_wait");
			printf("Semaforo en direccion: %p\n", (void*)&(parametros->semaforo));
			sem_wait(parametros->semaforo); // Caundo me avisen que hay una operacion para enviar, la voy a levantar de la cola
			puts("Instancia: Me hicieron un sem_post");
			OperacionAEnviar * operacion = list_remove(colaMensajes,0); //hay que borrar esa operacion
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

			int conexion = EnviarClaveYValorAInstancia(tipo, tamanioValor, parametros, header,	operacion);
			if (conexion==2){
				puts("Desaparecio una Instancia");
				return 1;
			}

			//puts("Instancia: Espero resultado de la instancia");
			// Espero el resultado de la operacion
			//tResultadoOperacion * resultado = malloc(sizeof(tResultadoOperacion));


			tEntradasUsadas *buffer = malloc(sizeof(tEntradasUsadas));
			if ((recv(parametros->new_fd, buffer, sizeof(tEntradasUsadas), 0)) <= 0) {
				perror("recv");
				//exit_gracefully(1);
				return 1;
			}

			puts("Instancia: recibi el resultado de la instancia");

			parametros->entradasUsadas = buffer->entradasUsadas;

			free(buffer);

			/*
			if (resultado == BLOQUEO){
=======
        switch(resultado){

        	case BLOQUEO:
>>>>>>> Stashed changes
				puts("Instancia: el resultado de la instancia fue un bloqueo");
				tBloqueo esiBloqueado = {1,operacion->clave};
				list_add(colaBloqueos,(void*)&esiBloqueado);
				sem_post(planificador->semaforo);
<<<<<<< Updated upstream
			}
			*/

			puts("Preparamos el resultado");
			tResultado * resultadoCompleto = malloc(sizeof(tResultado));
			resultadoCompleto->resultado = OK;
			strcpy(resultadoCompleto->clave,operacion->clave);

			//free(operacion->valor);

			//Debo avisarle al ESI que me invoco el resultado
			pthread_mutex_lock(&mutex);
			list_add(colaResultados,(void*)resultadoCompleto);
			pthread_mutex_unlock(&mutex);

		}
	close(parametros->new_fd);
			return 1;
	}

        //		*************** PRUEBAS HARDCODEADAS *****************

        //		Envio 3 operaciones, 2 sets (con la misma clave para que se pisen los valores) y un get.
        // 		En la instancia definí que el tamanio de los valores sea 3, si le mando algo de mayor tamannio, guarda lo restante en otra/s entrada/s

//                puts("Declaro el header");
//                OperaciontHeader *header1 = malloc(sizeof(OperaciontHeader));
//                header1->tamanioValor = 5;
//                header1->tipo = OPERACION_SET;
//                puts("Declare el primero");
//
//                OperaciontHeader *header2 = malloc(sizeof(OperaciontHeader));
//                header2->tamanioValor = 6;
//                header2->tipo = OPERACION_SET;
//                puts("Declare el primero");
//
//                OperaciontHeader *header3 = malloc(sizeof(OperaciontHeader));
//                header3->tamanioValor = 0;
//                header3->tipo = OPERACION_GET;
//                puts("Declare el primero");
//
//                char *unaClave = "Probando claves\0";
//                char *valor1 = "bbbb\0";
//                char *valor2 = "bbbbb\0";
//                char *unGet = "Probando claves\0";
//
//                int sendHeader;
//
//                puts("Enviando el header");
//        		if ((sendHeader = send(parametros->new_fd, header1, sizeof(OperaciontHeader), 0)) <= 0){
//        			puts("Error al send");l
//        			perror("send");
//        			exit_gracefully(1);
//        		}
//
//        		puts("Se mando bien el prmer header");
//
//
//
//        		//clave set 1
//        				if (send(parametros->new_fd, unaClave, TAMANIO_CLAVE, 0) <= 0){
//        					perror("send");
//        					exit_gracefully(1);
//        				}
//
//        				//valor set 1
//        				if (send(parametros->new_fd, valor1, header1->tamanioValor, 0) <= 0){
//        					perror("send");
//        					exit_gracefully(1);
//        				}
//        					puts("Se mandaron el seyt y el get");
//
//        		free(header1);
//
//        		puts("Enviando el header");
//        		if ((sendHeader = send(parametros->new_fd, header2, sizeof(OperaciontHeader), 0)) <= 0){
//        		puts("Error al send");
//        		perror("send");
//        		exit_gracefully(1);
//        		}
//        		puts("MANDE EL SEGUNDO HEADER");
//
//        		//clave	set 2
//        				if (send(parametros->new_fd, unaClave, TAMANIO_CLAVE, 0) <= 0){
//        					perror("send");
//        					exit_gracefully(1);
//        				}
//
//        				//valor set 2
//        				if (send(parametros->new_fd, valor2, header2->tamanioValor, 0) <= 0){
//        					perror("send");
//        					exit_gracefully(1);
//        				}
//
//        		free(header2);
//
//        		puts("Enviando el header");
//        		if ((sendHeader = send(parametros->new_fd, header3, sizeof(OperaciontHeader), 0)) <= 0){
//        		puts("Error al send");
//        		perror("send");
//        		exit_gracefully(1);
//        		}
//
//        		//clave get
//        		if (send(parametros->new_fd, unaClave, TAMANIO_CLAVE, 0) <= 0){
//        		perror("send");
//        		exit_gracefully(1);
//        		}
//
//
//        		free(header3);


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
			return 3;
			//exit_gracefully(1);
		}

		clave[strlen(clave)+1] = '\0';
		printf("ESI: Recibi la clave: %s \n", clave);
		strcpy(CLAVE,clave);

		if ((!list_is_empty(colaBloqueos)) && EncontrarEnLista(colaBloqueos, &clave) && EncontrarEnLista(clavesTomadas,&clave)){
			puts("ESI: La clave esta bloqueada");

			/* No es necesario avisarle el bloqueo porque se lo esta avisando el ESI
			notificacion->tipoNotificacion=BLOQUEO;
			strcpy(notificacion->clave,clave);
			notificacion->pid = parametros->pid;
			printf("ESI: le voy a avisar al planificador que se bloqueo la clave: %s \n",clave);
			sem_post(planificador->semaforo);
			puts("ESI: Ya le avise al planificador que se bloqueo la clave");
			*/
			return 2;
		} // ACA HAY QUE AVISARLE AL PLANIFICDOR DEL BLOQUEO PARA QUE FRENE AL ESI
		list_add(clavesTomadas,&clave);
		char * claveCopia = malloc(strlen(clave)+1);
		strcpy(claveCopia,clave);
		list_add(clavesTomadas,claveCopia);
		list_add(parametros->claves,claveCopia);

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
		bloqueo->pid = parametros->pid;
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

		OPERACION_ACTUAL = OPERACION_SET;

		char clave[TAMANIO_CLAVE];
		int recvClave, recvValor;
		if ((recvClave = recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			return 3;
			//exit_gracefully(1);
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

			return 3;
		}


		strcpy(CLAVE,clave);

		tBloqueo *bloqueo = malloc(sizeof(tBloqueo));
		strcpy(bloqueo->clave,clave);
		bloqueo->pid = parametros->pid;

		if (!(!list_is_empty(colaBloqueos) && LePerteneceLaClave(colaBloqueos, bloqueo))){
			printf("ESI: No se puede realizar un SET sobre la clave: %s debido a que nunca se la solicito \n",clave);
		}
		free(bloqueo);

		char *valor = malloc(tamanioValor);
		//char valor[tamanioValor];
		if (( recvValor = recv(parametros->new_fd, valor, tamanioValor + 1, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			return 3;
			//exit_gracefully(1);
		}
		valor[tamanioValor] = '\0';
		printf("ESI: Recibi el valor: %s \n", valor);
		operacion->tipo = OPERACION_SET;
		strcpy(operacion->clave,clave);
		operacion->valor= valor;
		printf("ESI: El valor dentro de operacion es %s \n",operacion->valor);
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

		OPERACION_ACTUAL = OPERACION_STORE;

		char clave[TAMANIO_CLAVE];
		int resultadoRecv;

		if ((resultadoRecv =recv(parametros->new_fd, clave, TAMANIO_CLAVE, 0)) <= 0) {
			perror("recv");
			log_info(logger, "TID %d  Mensaje: ERROR en ESI",
					process_get_thread_id());
			//exit_gracefully(1);
			return 3;
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

			 RemoverClaveDeBloqueados(clave);

			return 3;
		}
		strcpy(CLAVE,clave);

		tBloqueo *bloqueo = malloc(sizeof(tBloqueo));
		strcpy(bloqueo->clave,clave);
		bloqueo->pid = parametros->pid;

		if (!list_is_empty(colaBloqueos) && LePerteneceLaClave(colaBloqueos, bloqueo)){
			printf("ESI: Se desbloqueo la clave: %s \n",clave);

			notificacion->tipoNotificacion=DESBLOQUEO;
			strcpy(notificacion->clave,clave);
			notificacion->pid = parametros->pid;

			RemoverClaveDeLaLista(colaBloqueos, &clave);

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

	int ConexionESISinBloqueo(OperacionAEnviar* operacion,
			parametrosConexion* parametros) {
		//Debo avisar a una Instancia cuando recibo una operacion (QUE NO SEA UN GET)
		//Agregamos el mensaje a una cola en memoria
		printf("ESI: clave de la operacion: %s \n", operacion->clave);
		printf("ESI: La operacion es de tipo: %d\n",operacion->tipo);


		//free(operacion);

		puts("ESI: Vamos a chequear el tipo");


		puts("ESI: Voy a seleccionar la Instancia");
		int seleccionInstancia = SeleccionarInstancia(&CLAVE);
		puts("ESI: Se selecciono la Instancia");
		if(operacion->tipo != OPERACION_GET){
			printf("ESI: valor de la operacion: %s \n", operacion->valor);
			pthread_mutex_lock(&mutex); // Para que nadie mas me pise lo que estoy trabajando en la cola
			list_add(colaMensajes, (void*) operacion);
			pthread_mutex_unlock(&mutex);
			//esperamos el resultado para devolver
			puts("ESI: Vamos a ver si hay algun resultado en la cola");
			while (list_is_empty(colaResultados));
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

		clavesTomadas = list_create();

		//colaMensajesParaPlanificador = list_create();
		//colaBloqueos->head = malloc(TBLOQUEO * ENTRADAS);

		return 1;
	}

    int exit_gracefully(int return_nr) {

       log_destroy(logger);
       exit(return_nr);
       free(colaInstancias);
       free(colaMensajes);
       free(colaResultados);
       free(colaESIS);
       free(colaBloqueos);
       free(notificacion);
       free(clavesTomadas);

		return return_nr;
     }

    int configure_logger() {
      logger = log_create("Coordinador.log", "CORD", true, LOG_LEVEL_INFO);

      return 1;
     }

    bool EncontrarEnLista(t_list * lista, char * claveABuscar){
		bool yaExisteLaClave(tBloqueo *bloqueo) {
			printf("ESI: Comparando la clave %s con %s \n",claveABuscar, bloqueo->clave);
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
    		return string_equals_ignore_case(clave,claveAComparar) == true;
    	}
    	bool lePerteneceLaClave(parametrosConexion * parametros){
    		return list_any_satisfy(parametros->claves,tieneLaClave);
    	}
    	return list_find(colaInstancias,(void*) lePerteneceLaClave);
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
    		if(instanciaAComparar->pid == instancia->pid){
    			//puts("Voy a hacer el sem_post a la Instancia seleccionada \n");
    			//printf("Semaforo en direccion: %p\n", (void*)&(instanciaAComparar->semaforo));
    			//sem_post(&(instanciaAComparar->semaforo));
    		}
    	}
    	bool Comparar(parametrosConexion * instanciaAComparar){
    		return instanciaAComparar->new_fd == instancia->new_fd;
    	}
    	t_list * listaNueva = malloc(sizeof(lista));
    	puts("ESI: Voy a iterar la lista");
    	list_iterate(lista,Notificar);
    	puts("ESI: Itere la lista");
    	listaNueva = list_take_and_remove(lista, 1);
    	list_add_all(lista,listaNueva);
    	free(listaNueva);
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

		puts("Instancia: Envio header a la instancia");
		// Envio el header a la instancia

		int sendHeader;
		if ((sendHeader = send(parametros->new_fd, header, sizeof(OperaciontHeader), 0))
				<= 0) {
			puts("Fallo al enviar el header");
			perror("send");
			//exit_gracefully(1);
			RemoverInstanciaDeLaLista(parametros);
			close(parametros->new_fd);
			return 2;
		}
		printf("Instancia: Voy a enviarle la operacion de tipo: %d\n",header->tipo);
		free(header);
		printf("Instancia: Envio clave %s a la instancia\n", operacion->clave);
		// Envio la clave
		int sendClave;
		if ((sendClave = send(parametros->new_fd, operacion->clave, TAMANIO_CLAVE,0)) <= 0) {
			puts("Fallo al enviar la clave");
			perror("send");
			//exit_gracefully(1);
			RemoverInstanciaDeLaLista(parametros);
			close(parametros->new_fd);
			return 2;
		}

		if (tipo == OPERACION_SET) {
			int sendSet;
			printf("Instancia: Envio valor %s a la instancia\n", operacion->valor);
			if ((sendSet = send(parametros->new_fd, operacion->valor, tamanioValor, 0)) <= 0){
				perror("send");

			//exit_gracefully(1);
				RemoverInstanciaDeLaLista(parametros);
				close(parametros->new_fd);
				return 2;
			}
		}

		return EXIT_SUCCESS;
	}

	int SeleccionarInstancia(char* clave) {
		while (list_is_empty(colaInstancias));
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

		if(OPERACION_ACTUAL == OPERACION_GET){
			instancia = list_get(colaInstancias,0);
			printf("ESI: Agrego la clave %s a una instancia \n",clave);
			list_add(instancia->claves,clave);
			MandarAlFinalDeLaLista(colaInstancias,instancia);
		}
		else{
			instancia = BuscarInstanciaQuePoseeLaClave(clave);

			if (instancia == NULL){ //Hay que ver si devuelve NULL, esto es en caso de que se desconecte la instancia
				puts("Se desconecto la instancia");
				return 2;
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
			instanciaMenosUsada = BuscarInstanciaMenosUsada(colaInstancias); // Va a buscar la instancia que menos entradas tenga, desempata con fifo
		}
		else{
			instanciaMenosUsada = BuscarInstanciaQuePoseeLaClave(clave);

			if (instanciaMenosUsada == NULL){ //Hay que ver si devuelve NULL, esto es en caso de que se desconecte la instancia
				puts("Se desconecto la instancia");
				return 2;
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

			for (int i = 0; i < cantidadInstancias; i++){
				if (i!= cantidadInstancias - 1 && restoRango == 0){    // Si no es la ultima instancia debo redondear hacia arriba
						if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + rango)){
							instancia = list_get(colaInstancias, i);
							sem_post(instancia->semaforo);
						}
					}
					else if (i!= cantidadInstancias - 1 && restoRango != 0){ // si es la ultima instancia debo recalcular y ver cuantas entradas puedo aceptar
						if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + rango + 1)){
							instancia = list_get(colaInstancias, i);
							sem_post(instancia->semaforo);
						}else{
							if(posicionLetraEnASCII >= (i * rango) && posicionLetraEnASCII <= ((i * rango) + entradasUltimaInstancia)){
								instancia = list_get(colaInstancias, i);
								sem_post(instancia->semaforo);
						}
					}
				}
			}
		}
		else {
			instancia = BuscarInstanciaQuePoseeLaClave(clave);

			if (instancia == NULL){ //Hay que ver si devuelve NULL, esto es en caso de que se desconecte la instancia
				puts("Se desconecto la instancia");
				return 2;
			}

			printf("ESI: Semaforo de list_get en direccion: %p\n", (void*)&(instancia->semaforo));
			//list_remove_and_destroy_element(colaInstancias, 0,(void*)destruirInstancia);
			puts("ESI: Voy a hacer el sem_post a la Instancia seleccionada \n");
			sem_post(instancia->semaforo);
		}
		return 1;
	}

	int RemoverClaveDeLaLista(t_list * lista, char * claveABuscar){
		bool yaExisteLaClave(tBloqueo *bloqueo){
			printf("Comparando la clave %s con %s \n", claveABuscar, bloqueo->clave);
			return string_equals_ignore_case(bloqueo->clave,claveABuscar) == true;
		}
		list_remove_and_destroy_by_condition(colaBloqueos,yaExisteLaClave,(void*)destruirBloqueo);
		return EXIT_SUCCESS;
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





