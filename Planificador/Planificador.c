	#include "Planificador.h"

    int main(int argc, char *argv[])
    {
    	verificarParametrosAlEjecutar(argc, argv);
    	LeerArchivoDeConfiguracion(argv);

    			//FIFO
    		  	//Fifo *primero = malloc(sizeof(Fifo));
    		  	//Fifo *ultimo = malloc(sizeof(Fifo));
    	        //primero = NULL;
    	       // ultimo = NULL;
    	   		t_queue *colaX;
    	        fd_set master;   // conjunto maestro de descriptores de fichero
    	        fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
    	        struct sockaddr_in myaddr;     // dirección del servidor
    	        struct sockaddr_in remoteaddr; // dirección del cliente
			    int fdmax;        // número máximo de descriptores de fichero
    	        int listener;     // descriptor de socket a la escucha
    	        int newfd;        // descriptor de socket de nueva conexión aceptada
    	       // int sockCord; 	  // descriptor para conectarnos al Coordinador
    	        char buf[256];    // buffer para datos del cliente
    	        int nbytes;
    	        int yes=1;        // para setsockopt() SO_REUSEADDR, más abajo
    	        int addrlen;
    	        int i, j;
    	        int flagEjecutar=0;
    	        FD_ZERO(&master);    // borra los conjuntos maestro y temporal
    	        FD_ZERO(&read_fds);
				//Creamos las colas que vamos a manejar con las funciones de abajo
								ready = queue_create();
								ejecucion = queue_create();
								finalizados = queue_create();
								bloqueados = queue_create();
				//finalizamos el creado de colas
    	        // obtener socket para coordinador
								he=gethostbyname(IPCO);
    	        sockCord=ConectarAlCoordinador(sockCord, &cord_addr, he);
				printf("El socket cordinador es: %d \n",sockCord);
    	        puts("Obtenemos listener");
    	        // obtener socket a la escucha
    	        if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    	            perror("socket");
    	            exit(1);
    	        }

    	        // obviar el mensaje "address already in use" (la dirección ya se está usando)
    	        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
    	                                                            sizeof(int)) == -1) {
    	            perror("setsockopt");
    	            exit(1);
    	        }
    	        // enlazar
    	        myaddr.sin_family = AF_INET;
    	        myaddr.sin_addr.s_addr = INADDR_ANY;
    	        myaddr.sin_port = htons(PORT); // PUERTO en el que escuchamos
    	        memset(&(myaddr.sin_zero), '\0', 8);
    	        if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
    	            perror("bind");
    	            exit(1);
    	        }
    	        // escuchar
    	        if (listen(listener, 10) == -1) {
    	            perror("listen");
    	            exit(1);
    	        }
    	        puts("Añadimos el listener al conjunto maestro");
    	        // añadir listener al conjunto maestro
    	        FD_SET(listener, &master);

    	        puts("Añadimos la conexion al coordinador al conjunto maestro");
    	        // añadir conexion con coordinador al conjunto maestro
    	        FD_SET(sockCord, &master);

    	        puts("Descripores bases añanidos");

    	        // Iniciemos la consola
		
				pthread_t tid;
	            int stat = pthread_create(&tid, NULL, (void*)ejecutarConsola, NULL);
				if (stat != 0){
					puts("error al generar el hilo");
					perror("thread");
					//continue;
				}
				pthread_detach(tid); //Con esto decis que cuando el hilo termine libere sus recursos

/*
				pthread_t tid2;
	            int stat = pthread_create(&tid2, NULL, (void*)recibirNotificacionCoordinador, &sockCord);
				if (stat != 0){
					puts("error al generar el hilo");
					perror("thread");
					//continue;
				}
				pthread_detach(tid2); //Con esto decis que cuando el hilo termine libere sus recursos
*/
				struct timeval timeout;
				timeout.tv_sec=SOCKET_READ_TIMEOUT_SEC;
				timeout.tv_usec=0;

    	        // seguir la pista del descriptor de fichero mayor
    	        fdmax = listener; // por ahora es éste
    	        // bucle principal

						int enviarConfirmacion=1;
						t_esi *esi=malloc(sizeof(t_esi));
						char clave[TAMANIO_CLAVE];
    	        for(;;) {
						
    	        	/*
    	        	dos conjuntos de descriptores de fichero: master y read_fds.
    	        	El primero, master, contiene todos los descriptores de fichero que están actualmente conectados,
    	        	incluyendo el descriptor de socket que está escuchando para nuevas conexiones.
    	        	La razón por la cual uso el conjunto master es que select()
    	        	va a cambiar el conjunto que le paso para reflejar que sockets están listos para lectura.
    	        	Como tengo que recordar las conexiones activas entre cada llamada de select(),
    	        	necesito almacenar ese conjunto en algún lugar seguro.
    	        	En el último momento copio master sobre read_fs y entonces llamo a select().
    	        	*/
				
    	            read_fds = master; // cópialo
    	            if (select(fdmax+1, &read_fds, NULL, NULL, &timeout) == -1) {
    	                perror("select");
    	                exit(1);
    	            }
    	            // explorar conexiones existentes en busca de datos que leer
					int re=0;
						int f_ejecutar=0;//Flag para mandar de ready a ejecucion.
						int recibi=0;
						
    	            for(i = 0; i <= fdmax; i++) {
						
    	                if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
    	                    if (i == listener) {
    	                        // gestionar nuevas conexiones
    	                        addrlen = sizeof(remoteaddr);
    	                        if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr,
    	                                                                 &addrlen)) == -1) {
    	                            perror("accept");
    	                        } else {
    	                            FD_SET(newfd, &master); // añadir al conjunto maestro

    	                            if (newfd > fdmax) {    // actualizar el máximo
    	                                fdmax = newfd;
    	                            }
    	                            printf("selectserver: new connection from %s on "
    	                                "socket %d \n", inet_ntoa(remoteaddr.sin_addr), newfd);
									
									int idEsi=obtenerEsi(newfd);
									
									queue_push(ready,new_ESI(idEsi,newfd,estimacionIni,"\0"));
									
									if(algoritmo==SJF || algoritmo==SJFD)
									{
										ordenarEsis(ready);
										if(algoritmo==SJFD)
												{
													t_esi* esi1 = malloc(sizeof(t_esi));
													esi1=queue_pop(ejecucion);
													queue_push(ready,esi1);
													ordenarEsis(ready);
												}		
									}					
									printf("Esi id %d conectado y puesto en cola \n",idEsi);
									
    	                        }
							 }else{
								 if(i==sockCord){
									tNotificacionPlanificador *notificacion = malloc(sizeof(tNotificacionPlanificador));
									//notificacion=recibirNotificacionCoordinador(sockCord);
									 int numbytes;
									if ((numbytes=recv(i, notificacion, sizeof(tNotificacionPlanificador), 0)) <= 0) {
										perror("Fallo el recibir Notificacion. Probablemente el Coordiandor la quedo");
										//exit(1);
									}else{
										if(notificacion->tipoNotificacion==ERROR)
										{
											printf("Vamos a matar al esi de id: %d \n",esi->id);//abortar al esi
											kill(esi->id,SIGTERM);
										}
										if(notificacion->tipoNotificacion==DESBLOQUEO)
										{
											printf("coordi nos dijo se desbloquea la clave: %s \n ",notificacion->clave);
											t_esi * desbloqueado = malloc(sizeof(t_esi));
											//notificacion->clave
											
											desbloqueado=buscarEsi(bloqueados,notificacion->clave);
											if(desbloqueado!=NULL)
											{
												puts("desbloqueado no es nulo");
												queue_push(ready,new_ESI(desbloqueado->id,desbloqueado->fd,desbloqueado->estimacion,desbloqueado->clave));
												eliminarEsiPorId(bloqueados,desbloqueado->id);
											}
											else
											{
												puts("desbloqueado es nulo");
											}
											free(desbloqueado);
											
											if(algoritmo==SJF || algoritmo==SJFD)
											{
												ordenarEsis(ready);
												if(algoritmo==SJFD)
												{
													t_esi* esi1 = malloc(sizeof(t_esi));
													esi1=queue_pop(ejecucion);
													queue_push(ready,esi1);
													ordenarEsis(ready);
												}
												
											}
											
										}
									
									free(notificacion);
									
									}
								}
								/*else{
								 if (flagEjecutar==1 && i!=sockCord){
									 puts("Cerramos el socket del ESI y lo borramos del conjunto maestro");
									 if (FD_ISSET(i, &master)) {
										 flagEjecutar=0;
										 close(i);
										 FD_CLR(i, &master);
									 	 }
	                                   }

							  	   }*/
							//}cierre el else recibir
							 //}//ciere del fdset 
						//}
						//Planificar
					//int f_ejecutar=0;//Flag para mandar de ready a ejecucion.
					//puts("voy a verificar las colas");	
					//if(flagOperar==1){
						//puts("entre ");
						//if(recibi==1)
						//{
							tResultado * resultado = malloc(sizeof(tResultado));
							//int re=0;
							//printf("fd del esi es: %d",esi->fd);
							if(i==esi->fd)
							{
								puts("voy a recibir el resultado o bloquearme, quien sabe?");
								int numbytes;
									if ((numbytes=recv(i, resultado, sizeof(tResultado), 0)) <= 0) {
										puts("esi desconectado");
										//TODO:Cuando elimino al esi tengo que nullear el esi global
										//para no recibir mas nada, es decir que me quede esperando
										//un esi nuevo.
										
										eliminarEsiPorId(ready,esi->id);
										eliminarEsiPorId(ejecucion,esi->id);
										eliminarEsiPorId(bloqueados,esi->id);
										puts("Cerramos el socket del ESI y lo borramos del conjunto maestro");
									 	if (FD_ISSET(i, &master)) {
										 //flagEjecutar=0;
										 close(i);
										 FD_CLR(i, &master);
									 	 }
										//perror("esi desconectado");
										//resultado->tipoResultado=CHAU;
										//exit(1);
										break;
										f_ejecutar=1;
									}
									puts("Recibi el resultado");
									printf("Resultado: %d \n",resultado->tipoResultado);
									printf("Resultado: %s \n",resultado->clave);
									strcpy(esi->clave,resultado->clave);
									re=recibirResultado2(resultado);
									enviarConfirmacion=1;
							
							}
							}//cierre el else recibir
						//}	
				//	}//cierre flag operar
					}//cierro el fdset

					if(flagOperar==1){
						//puts("entre ");
						if(queue_is_empty(ejecucion)==0)
						{
							//recibi=0;
							
							//puts("Entre a ejecucion");
							//printf("Tamanio cola %d \n",queue_size(ejecucion));
							f_ejecutar=0;
							
							if(re==2)
							{
								//Si se bloquea
								queue_pop(ejecucion);
								if(algoritmo==SJF ||algoritmo==SJFD)
									estimacionEsi(esi);
									//haria algo mas	
								printf("Esi de id:%d entro a bloqueados \n",esi->id);
								queue_push(bloqueados,esi);
								puts("Se agrego a la cola de bloqueados");
								f_ejecutar=1;
								enviarConfirmacion=0;
								if(queue_is_empty(ready))
									printf("No hay nada en Ready, asi que seguro me quedo esperando que desbloqueen la clave %s \n",esi->clave);
								else
									puts("Hay algo en ready");
							}
							if (re==1){
								esi->cont++;
								printf("Contador De ESI %d  estimacion %f \n",esi->cont, esi->estimacion);
							}
							if(re==-5)
							{
								queue_pop(ejecucion);
								queue_push(finalizados,new_ESI(esi->id,esi->fd,esi->estimacion,esi->clave));
								f_ejecutar=1;
								puts("Dame otro esi");
								f_ejecutar=1;
								enviarConfirmacion=0;
							    //free(esi);
							}
							if(enviarConfirmacion==1)
							{
								puts("Le voy a decir que ejecute al ESI");
								esi = queue_peek(ejecucion);
								printf("Id del esi a ejecutar: %d \n",esi->id);
								EnviarConfirmacion(esi);
								printf("ejecuta el esi:%d \n",esi->id);
								enviarConfirmacion=0;
								recibi=1;
							}
						}
						else
						{
							recibi=0;	
							//puts("Dame un esi");
							f_ejecutar=1;//Como esta vacia, pedimos que nos manden un esi de ready
						}
						if(queue_is_empty(ready)==0)
						{

							if(f_ejecutar==1)
							{
								puts("Voy a seleccionar alguno para ejecutar de la cola de ready");
								//t_esi *esi2=malloc(sizeof(t_esi));
								if(algoritmo==SJF || algoritmo==SJFD)
									ordenarEsis(ready);
								esi = queue_pop(ready);
								re=0;
								enviarConfirmacion=1;
								printf("Id del esi a buscar:%d \n",esi->id);
								printf("esi de id %d cambiado de cola \n",esi->id);
								queue_push(ejecucion,new_ESI(esi->id,esi->fd,esi->estimacion,esi->clave));
								//free(esi2);
							}
							//puts("Ready no vacia");
						}
						
					}

					}//ciero el for del select

				}//Cierra el for del main
    	        
				
    	        return 0;
    }//Cierra el main

void ordenarEsis(t_queue *cola)
	{
		bool compare(t_esi *esi1,t_esi *esi2)
		{
			/*
			float resul;
			resul= esi2->estimacion-esi1->estimacion;
			return resul;
			*/
			return esi1->estimacion <= esi2->estimacion;
		}

		list_sort(cola->elements,(void *)compare);
		return cola;
		
	}
	
	t_esi * buscarEsi(t_queue *lista,char clave[TAMANIO_CLAVE])
	{
		int coincidir(t_esi *unEsi){
          	    	    		return (string_equals_ignore_case(unEsi->clave,clave));//unEsi->clave == clave;
          	    	    	}
		t_esi *esi=malloc(sizeof(t_esi));
		esi=list_find(lista->elements,(void*)coincidir);
		return esi;
		//free(esi);
	}

	t_esi * buscarEsiPorId(t_queue *lista,int id)
	{
		int coincidir(t_esi *unEsi){
          	    	    		return (string_equals_ignore_case(unEsi->id,id));//unEsi->clave == clave;
          	    	    	}
		t_esi *esi=malloc(sizeof(t_esi));
		esi=list_find(lista->elements,(void*)coincidir);
		return esi;
		//free(esi);
	}

    int EnviarConfirmacion (t_esi * esi){
    	puts("enviando informacion al esi \n");
    	char *confirmacion = "EJECUTATE";
	    if (send(esi->fd, confirmacion , sizeof(confirmacion), 0) <= 0){
		   printf("Error al enviar ejecucion al ESI (%d) a direccion (%d) \n ",esi->id, esi->fd);
		   perror("Send");

		   exit(1);
	    }
	   puts("Se envió confirmacion");
	   return EXIT_SUCCESS;
    }

	void destruirEsi(t_esi *unEsi){
    	   //free(unaEntrada->clave);
    	   //free(unEsi);
    	  return;
       }

	void eliminarEsiPorId(t_queue *lista, int pid){
          	int coincidir(t_esi *unEsi){
          	    	    		return unEsi->id == pid;
          	    	    	}
							  
          		list_remove_and_destroy_by_condition(lista->elements, (void*) coincidir,(void*) destruirEsi);

				return;
          }

	
    int ConectarAlCoordinador(int sockCord, struct sockaddr_in* cord_addr,
    		struct hostent* he) {
    	// obtener socket para coordinador
    	if ((sockCord = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
    		puts("Error al crear el socket");
    		perror("socket");
    		exit(1);
    	}
		printf("El socket cordinador es: %d \n",sockCord);
    	cord_addr->sin_family = AF_INET; // Ordenación de bytes de la máquina
    	cord_addr->sin_port = htons(PORT_COORDINADOR); // short, Ordenación de bytes de la red
    	cord_addr->sin_addr = *((struct in_addr*) he->h_addr);
    	memset(&(cord_addr->sin_zero), '\0', 8); // poner a cero el resto de la estructura
    	// conectar al coordinador
    	puts("Por conectarnos al coordinador");
    	if (connect(sockCord, (struct sockaddr*) &*cord_addr,
    			sizeof(struct sockaddr)) == -1) {
    		puts("Error al conectarme al servidor.");
    		perror("connect");
    		exit(1);
    	}
		printf("El socket cordinador es: %d \n",sockCord);
    	//puts("Conectado con el coordinador!\n");
		enviarHeader(sockCord);

		enviarClavesBloqueadas(sockCord);

	return sockCord;
		//printf("El socket cordinador es: %d \n",sockCord);
    }

    int verificarParametrosAlEjecutar(int argc, char *argv[]){

        if (argc != 2) {
        	puts("Error al ejecutar, te faltan parametros. Intenta con: ./Planificador unaConfiguracion.config");
            exit(1);
        }


        /*if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina
        	puts("Error al obtener el hostname, te faltan parametros.");
        	perror("gethostbyname");
            exit(1);
        }*/
        return 1;
    }

    void *ejecutarConsola()
	{
	  while(1)
		{
			//char * readline(const char * linea);
			char * linea = readline(": ");
/*
   			 if(linea)
      			add_history(linea);*/

			if(strncmp(linea,"pausar",7)==0)
			{
				puts("Planificador pausado");
				flagOperar=0;
			}
			if(strncmp(linea,"cola",4)==0)
			{
				mostrarCola();
			}
			if(strncmp(linea,"continuar",9)==0)
			{
				puts("Continuamos ejecutando");
				flagOperar=1;
				puts(linea);
			}
			if(strncmp(linea,"bloquear",8)==0)
			{

				char clave1[TAMANIO_CLAVE];
				int id;
				int flag=0;
				int j=0;
				int i=9;
				while(linea[i]!=' ' && linea[i]!='/0')
				{
					j++;
					i++;
				}
				i=9;
				j=0;
				while(linea[i]!=' ' && linea[i]!='/0')
							{	//printf("%d \n",i);
								clave1[j]=linea[i];
								j++;
								i++;
								if(linea[i]=='\0')
									{
										puts("Falta que ingrese el id");
										flag=1;
										break;
									}
							}

				clave1[j]='\0'; //Agrega un /0 al final de la clave y descarta toda la basura
				printf("Clave: %s \n",clave1);
				j=0;
				i++;
				if(flag==0)
				{
					id=obtenerId(i,linea);
				}
				printf("id: %d \n",id);

				bloquearEsi(id,clave1);

			}
			if(strncmp(linea,"desbloquear",11)==0)
			{
				//obtener id
				//int id=obtenerId(11,linea);
				//printf("id: %d \n",id);
				char clave[TAMANIO_CLAVE];
				int i=12;
				int j=0;
				while(linea[i]!='\0')
				{
					clave[j]=linea[i];
					j++;
					i++;
				}
				clave[j]='\0';
				printf("La clave es: %s \n",clave);

				desbloquearEsi(clave);
			}
			if(strncmp(linea,"kill",4)==0)
			{
				//obtener id
				int id=obtenerId(4,linea);
				printf("id: %d \n",id);
				kill(id,SIGTERM);
			}
			if(strncmp(linea,"status",6)==0)
			{
				int id=obtenerId(6,linea);
				printf("id: %d \n",id);
			}
			if(strncmp(linea,"deadlocks",9)==0)
			{
				puts("Mostrar bloqueos mutuos");
			}
			if(strncmp(linea,"listar",6)==0)
			{
				puts("Vamos a listar");
				char clave[TAMANIO_CLAVE];
				int i=7;
				int j=0;
				while(linea[i]!='\0')
				{
					clave[j]=linea[i];
					j++;
					i++;
				}
				clave[j]='\0';
				printf("La clave es: %s \n",clave);
				//t_list * aux=malloc(sizeof(t_list));//list_create();
				//aux = filtrarLista(clave,bloqueados->elements);
				obtenerBloqueados(clave);
			}

		}

	return 0;
	}
	
	void desbloquearEsi(char clave[TAMANIO_CLAVE])
	{
		t_esi * desbloqueado = malloc(sizeof(t_esi));
											//notificacion->clave
											
		desbloqueado=buscarEsi(bloqueados,clave);
		if(desbloqueado!=NULL)
		{
			puts("desbloqueado no es nulo");
			queue_push(ready,new_ESI(desbloqueado->id,desbloqueado->fd,desbloqueado->estimacion,desbloqueado->clave));
			eliminarEsiPorId(bloqueados,desbloqueado->id);
		}
		else
		{
			puts("desbloqueado es nulo");
		}
		free(desbloqueado);
		
		puts("Envio header de clave a desbloquear al Coordinador");
		tSolicitudPlanificador * solicitud = malloc(sizeof(tSolicitudPlanificador));
		solicitud->solicitud= DESBLOQUEAR;
		   if (send(sockCord, solicitud, sizeof(tSolicitudPlanificador), 0) <= 0){
			   puts("Error al enviar header de una clave bloqueada");
			   perror("Send");
			   //free(&header->tipoMensaje);
			   //free(&header->tipoProceso);
			 //  free(header->idProceso);

		   }
		   printf("Se envió header de clave desbloqueada %d \n",solicitud);
		  //free(solicitud);

		puts("Envio la clave desbloqueada");

		   if (send(sockCord, clave, TAMANIO_CLAVE, 0) == -1){
			   puts("Error al enviar una clave bloqueada");
			   perror("Send");
			   //free(&header->tipoMensaje);
			   //free(&header->tipoProceso);
			 //  free(header->idProceso);

		   }
		   printf("Se envió una clave desbloqueada: %s \n",clave);

		if(algoritmo==SJF || algoritmo==SJFD)
		{
			ordenarEsis(ready);
			if(algoritmo==SJFD)
			{
				t_esi* esi1 = malloc(sizeof(t_esi));
				esi1=queue_pop(ejecucion);
				queue_push(ready,esi1);
				ordenarEsis(ready);
			}
			
		}
	}
	
	void bloquearEsi(int id,char clave[TAMANIO_CLAVE])
	{
		// TODO: Comprobar que este en ready o en ejecucion nomas
		t_esi* esi = malloc(sizeof(t_esi));
		esi=buscarEsiPorId(ready,id);
		if(esi!=NULL)
		{
			//Esi estaba en ready
			puts("esi estaba en ready");
			eliminarEsiPorId(ready,id);
			queue_push(bloqueados,new_ESI(esi->id,esi->fd,esi->estimacion,clave));
			enviarClaveCoordinador(clave,BLOQUEAR);
		}{
			puts("esi no estaba en ready");
		}
		esi=buscarEsiPorId(ejecucion,id);
		if(esi!=NULL)
		{
			//Esi estaba en ready
			puts("esi estaba en ejecucion");
			if(algoritmo==SJF||algoritmo==SJFD)
				estimacionEsi(esi);
			eliminarEsiPorId(ready,id);
			queue_push(bloqueados,new_ESI(esi->id,esi->fd,esi->estimacion,clave));
			enviarClaveCoordinador(clave,BLOQUEAR);
		}
		else{
			puts("esi no estaba en ejecucion");
			puts("No habia esi pa bloquear ");
		}
		
		
	}
	void enviarClaveCoordinador(char clave[TAMANIO_CLAVE],tSolicitudesDeConsola *solicitud)
	{
	
		 if (send(sockCord, solicitud, sizeof(tSolicitudesDeConsola), 0) <= 0){
					   puts("Error al enviar solicitud");
					   perror("Send");
				   }
				   puts("Se envió el header BLOQUEAR");

		if (send(sockCord, clave, sizeof(clave), 0) <= 0){
					   puts("Error al enviar la clave");
					   perror("Send");
				   }
				   puts("Ya le avise al coordinador la clave bloqueada");
		
	}

	void obtenerBloqueados(char clave[TAMANIO_CLAVE])
	{
		t_queue * aux=queue_create();
	    //printf("La clave es: %s \n",clave);
		int coincidir(t_esi *unEsi){
								printf("Comparando clave %s con clave %s \n",unEsi->clave,clave);
          	    	    		return (string_equals_ignore_case(unEsi->clave,clave));//unEsi->clave == clave;
          	    	    	}
		queue_push(bloqueados,new_ESI(1,2,5,clave));
		puts("meti algo en la cola");
		aux->elements=list_filter(bloqueados->elements,coincidir);
		puts("filtre");
		
		t_esi *esi=malloc(sizeof(t_esi));
		puts("voy a ver si hay algo adentro");
		while(queue_is_empty(aux)==0)
		{
			esi=queue_pop(aux);
			printf("Esi de id: %d esperando recurso %s",esi->id,esi->clave);
		}
		free(esi);
		free(aux);
		

	}

	int obtenerId(int size, char * linea)
	{
		int id;
		char * cid = malloc(sizeof(int));
		int j=0;
		int i=size;
		while(linea[i]!='\0')
		{
			cid[j]=linea[i];
			j++;i++;
		}
		id=atoi(cid);
		free(cid);//Probar esto
		return id;
	}


	int  LeerArchivoDeConfiguracion(char *argv[]) {

			// Leer archivo de configuracion con las commons
			t_config* configuracion;
			configuracion = config_create(argv[1]);

			PORT = config_get_int_value(configuracion, "PORT");

			IP = config_get_string_value(configuracion, "IP");

			IPCO =config_get_string_value(configuracion, "IPCO");
			//puts("entram02");
			PORT_COORDINADOR = config_get_int_value(configuracion, "PORT_CORDINADOR");
			//puts("entram03");
			MAXDATASIZE = config_get_int_value(configuracion, "MAX_DATASIZE");
			//puts("entram04");
			puts("estimando");
			estimacionIni = config_get_int_value(configuracion, "estimacion");
			puts("estimado");
			Alfa = config_get_int_value(configuracion, "Alfa");
			algoritmo = config_get_int_value(configuracion, "algoritmo");

			clavesBloqueadas = list_create();
			char** claves = config_get_array_value(configuracion,"Claves_Bloqueadas"); // esto devuelve un char**
			int clavesInicialmenteBloqueadas = config_get_int_value(configuracion, "cantidadClavesBloqueadas");

			puts("Voy a ver cuantas claves tengo bloqueadas inicialmente");
			for(int i = 0; i< clavesInicialmenteBloqueadas; i++ ){
				printf("Una clave inicialmente bloqueada es: %s \n",claves[i]);
				list_add(clavesBloqueadas,claves[i]);
			}

			return 1;

		}
 void estimacionEsi (t_esi* esi){ //float EstimacionEsi (t_esi* esi)


		esi->estimacion= ((0.01*Alfa*esi->cont)+((1-(Alfa*0.01))*(esi->estimacion)));

		//return (esi);
	}

	int obtenerEsi(int socket)
	{
		int bytesRecibidos;
						tHeader *headerRecibido = malloc(sizeof(tHeader));
					    if ((bytesRecibidos = recv(socket, headerRecibido, sizeof(tHeader), 0)) == -1){
						perror("recv");
						exit(1);
					   }
					   //fprintf("Mensaje : %s",headerRecibido->tipoMensaje);
					   if (headerRecibido->tipoMensaje == CONECTARSE){
					//TODO: falta un free
						return(headerRecibido->idProceso);
					   }
	}
int recibirResultado2(tResultado * resultado){
	switch(resultado->tipoResultado){
    		case OK:
    			puts("La operación salio OK");
    			//EN ESTOS CASE DEBERIA LOGGEAR O ALGO ASI, PREGUNTAR MAS ADELANTE
    			return 1;
    			break;
    		case BLOQUEO:
				puts("La operación se BLOQUEO");
				return 2;
				break;
    		case ERROR:
				puts("La operación tiro ERROR");
				break;
			case CHAU:
				puts("Cerro el esi");
				return -5;
				break;
    		default:
    			puts("ERROR AL RECIBIR EL RESULTADO");
    			EXIT_FAILURE;
    			break;
    	}
    	return EXIT_SUCCESS;
    }


	int recibirResultado(int socket, tResultado * resultado){
    	puts("Vor a recibir el resultado");
    	recibirResultadoDelEsi(socket,resultado);

    	switch(resultado->tipoResultado){
    		case OK:
    			puts("La operación salio OK");
    			//EN ESTOS CASE DEBERIA LOGGEAR O ALGO ASI, PREGUNTAR MAS ADELANTE
    			return 1;
    			break;
    		case BLOQUEO:
				puts("La operación se BLOQUEO");
				return 2;
				break;
    		case ERROR:
				puts("La operación tiro ERROR");
				break;
			case CHAU:
				puts("Cerro el esi");
				return -5;
				break;
    		default:
    			puts("ERROR AL RECIBIR EL RESULTADO");
    			EXIT_FAILURE;
    			break;
    	}
    	return EXIT_SUCCESS;
    }

int recibirResultadoDelEsi(int sockfd, tResultado * resultado){
    	int numbytes;


        if ((numbytes=recv(sockfd, resultado, sizeof(tResultado), 0)) <= 0) {
            perror("esi desconectado");
			resultado->tipoResultado=CHAU;
            //exit(1);
        }
        puts("Recibi el resultado");
		printf("Resultado: %d \n",resultado->tipoResultado);
        printf("Resultado: %s \n",resultado->clave);
		return EXIT_SUCCESS;
    }

		    void AgregarACola(t_queue *Cola,t_esi *esi){

		    				queue_push(Cola,esi);

		    				printf("se Agrego a la Cola el proceso: %d \n",esi->id);puts("Aca escuchamos al cordi");
		    			}
		    void mostrarCola(t_queue *Cola)
		    		  	{
		    		    	int tam = queue_size(Cola);
		    		    	printf("Tamanio de cola: %d \n",tam);
		    		   	}
		    void moverCola(t_queue *ColaInicio, t_queue *ColaFin)
		    {
		    		    		    	queue_push(ColaFin,queue_pop(ColaInicio));
		    		    		   	}

	int enviarHeader(int sockfd){
			int pid = getpid(); //Los procesos podrian pasarle sus PID al coordinador para que los tenga identificados
			printf("Mi ID es %d \n",pid);
			tHeader *header = malloc(sizeof(tHeader));
				   header->tipoProceso = PLANIFICADOR;
				   header->tipoMensaje = CONECTARSE;
				   header->idProceso = pid;
				   strcpy(header->nombreProceso, "Planificador" ); // El nombre se da en el archivo de configuracion --> MINOMBRE
				   if (send(sockfd, header, sizeof(tHeader), 0) == -1){
					   puts("Error al enviar mi identificador");
					   perror("Send");
					   //free(&header->tipoMensaje);
					   //free(&header->tipoProceso);
					 //  free(header->idProceso);
					   free(header);
					   exit(1);
				   }
				   puts("Se envió mi identificador");
				   //free(&header->tipoMensaje);
				   //free(&header->tipoProceso);
				   //free(header->idProceso);
				   free(header);
				   return EXIT_SUCCESS;
		}

	int enviarClavesBloqueadas(int sockfd){
		tClavesBloqueadas * cantidadClavesBloqueadas = malloc(sizeof(tClavesBloqueadas));
		cantidadClavesBloqueadas->cantidadClavesBloqueadas = list_size(clavesBloqueadas);

		puts("Estoy por enviar la cantidad de claves bloqueadas que me dieron en el config");
		if(send(sockfd,cantidadClavesBloqueadas,sizeof(tClavesBloqueadas),0) <= 0){
			perror("Hubo un problema al enviar la cantidad de claves bloqueadas");
			exit(1);
		}

		for(int i = 0; i < cantidadClavesBloqueadas->cantidadClavesBloqueadas; i++){
			char * clave = list_get(clavesBloqueadas,i);
			printf("Envio la clave: %s \n",clave);
			   if (send(sockfd, clave, TAMANIO_CLAVE, 0) == -1){
				   puts("Error al enviar una clave bloqueada");
				   perror("Send");
				   //free(&header->tipoMensaje);
				   //free(&header->tipoProceso);
				 //  free(header->idProceso);
				   exit(1);
			   }
			   puts("Se envió una clave bloqueada");
		}

		free(cantidadClavesBloqueadas);
	}
