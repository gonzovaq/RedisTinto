	#include "Planificador.h"

void intHandler(int dummy) { // para atajar ctrl c
		keepRunning = 0;
		sleep(1);
		puts("CHAU ME VOY!");
		exit(1);
	}

    int main(int argc, char *argv[])
    {
		signal(SIGINT, intHandler);

    			while (keepRunning) {

    	verificarParametrosAlEjecutar(argc, argv);
    	LeerArchivoDeConfiguracion(argv);
				int desalojar=0;
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
				
				struct timeval timeout;
			//	timeout.tv_sec=SOCKET_READ_TIMEOUT_SEC;
			//	timeout.tv_usec=0;
//
    	        // seguir la pista del descriptor de fichero mayor
    	        fdmax = listener; // por ahora es éste
    	        // bucle principal

						int enviarConfirmacion=1;
						t_esi *esi=malloc(sizeof(t_esi));
						char clave[TAMANIO_CLAVE];
    	        		//timeout.tv_sec=2;
						//timeout.tv_usec=50000;
				for(;;) {	
					
					timeout.tv_sec=2;
					timeout.tv_usec=50000;
					//puts("DEBUGGG");		
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
						//printf("SOCKET: %d\n",i);
    	                if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
    	                    if (i == listener) {
    	                        // gestionar nuevas conexiones
    	                        addrlen = sizeof(remoteaddr);
    	                        if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr,
    	                                                                 &addrlen)) == -1) {
    	                            perror("accept");
    	                        } else {
    	                            FD_SET(newfd, &master); // añadir al conjunto maestro
									printf("SOCKET: agregado a conjunto%d\n",fdmax);
    	                            if (newfd > fdmax) {    // actualizar el máximo
										printf("SOCKET: actualizar max %d\n",i);
    	                                fdmax = newfd;
    	                            }
    	                            printf("selectserver: new connection from %s on "
    	                                "socket %d \n", inet_ntoa(remoteaddr.sin_addr), newfd);
								
									int idEsi=obtenerEsi(newfd);
									if(algoritmo==SJF || algoritmo==SJFD)
									{

										queue_push(ready,new_ESI_nuevo(idEsi,newfd,estimacionIni,0,0,"\0"));
										ordenarEsis(ready);
										if(algoritmo==SJFD)
												{
													desalojar=1;
												}		
									}
									if(algoritmo==HRRN)
									{
										
										t_esi * esi = malloc(sizeof(t_esi));
										esi->id=idEsi;esi->fd=newfd;esi->estimacion=estimacionIni;
										esi->espera=0;
										estimacionHRRN(esi);
										queue_push(ready,new_ESI_nuevo(esi->id,esi->fd,esi->estimacion,esi->responseRatio,esi->espera,esi->clave));
										//desalojar=1;
										free(esi);

										ordenarEsis(ready);

									}

									printf("Esi id %d conectado y puesto en cola \n",idEsi);
									}
								}else{//Gestionar datos de un cliente
								 if(i==sockCord){
									tNotificacionPlanificador *notificacion = malloc(sizeof(tNotificacionPlanificador));
									//notificacion=recibirNotificacionCoordinador(sockCord);
									 int numbytes;
									if ((numbytes=recv(i, notificacion, sizeof(tNotificacionPlanificador), 0)) <= 0) {
										//perror("Fallo el recibir Notificacion. Probablemente el Coordiandor la quedo");

										puts("CORDINADOR NO ESTA VIVO, SIN EL... NO SOY NADA... ADIOS");
										killEsi(getpid());
									}else{
										if(notificacion->tipoNotificacion==ERROR)
										{
											printf("Vamos a matar al esi de id: %d \n",esi->id);//abortar al esi
											kill(esi->id,SIGTERM);
										}
										if(notificacion->tipoNotificacion==STATUS_RECV)
										{
											puts("*****************LABURAREMOS UN STATUS*****************");
											//char instancia[TAMANIO_NOMBREPROCESO];
											//char * valor = malloc(tamanioValor+1);
											tStatusParaPlanificador * status = malloc(sizeof(tStatusParaPlanificador));
											if ( (recv(i, status,sizeof(tStatusParaPlanificador), 0)) <= 0) {
												perror("recv");
												puts("Error recibir el status del planificador");
												//return ERROR;
											}
											else
											{
												char * valor = malloc(status->tamanioValor+1);
												printf("El nombre de la instancia es: %s \n",status->proceso);
												//printf("La instancia que utiliza la clave es: %s \n",);
												if(status->tamanioValor==0)
												{
													puts("La clave no posee valor");
												}
												else{
													puts("La clave posee valor");
													printf("El tamanioValor de la clave es: %d\n", status->tamanioValor);
													if ( (recv(i, valor,status->tamanioValor + 1, 0)) <= 0)
													{
														perror("recv");
														puts("Error recibir el valor de la clave");
														//return ERROR;
													}
													else
													{
														printf("El valor de la clave es: %s \n",valor);
													}
												}
												free(valor);
											}
											free(status);

											obtenerBloqueados(notificacion->clave);
										}
										if(notificacion->tipoNotificacion==DESBLOQUEO)
										{
											printf("coordi nos dijo se desbloquea la clave: %s \n ",notificacion->clave);
											t_esi * desbloqueado = malloc(sizeof(t_esi));
											//notificacion->clave
											
											desbloqueado=buscarEsi(bloqueados,notificacion->clave,desbloqueado);
											if(desbloqueado!=NULL)
											{
												puts("desbloqueado no es nulo");
												queue_push(ready,new_ESI_hrrn(desbloqueado->id,desbloqueado->fd,desbloqueado->estimacion,desbloqueado->responseRatio,desbloqueado->espera,desbloqueado->clave,1,desbloqueado->clavesTomadas,desbloqueado->cont));
												eliminarEsiPorId(bloqueados,desbloqueado->id);

												if(algoritmo==SJF || algoritmo==SJFD)
												{
													ordenarEsis(ready);
													if(algoritmo==SJFD)
													{
														desalojar=1;
													}
													
												}
												if(algoritmo==HRRN)
												{
													ordenarEsis(ready);
													//desalojar=1;
												}
											}
											else
											{
												puts("desbloqueado es nulo");
											}
											free(desbloqueado);	
										}
									
									free(notificacion);
									
									}
								}
								
							tResultado * resultado = malloc(sizeof(tResultado));
							//int re=0;
							//printf("fd del esi es: %d",esi->fd);
							if(i==esi->fd)
							{
								puts("Recibiendo el resultado...");
								int numbytes;
									if ((numbytes=recv(i, resultado, sizeof(tResultado), 0)) <= 0) {
										puts("esi desconectado");
										//TODO:Cuando elimino al esi tengo que nullear el esi global
										//para no recibir mas nada, es decir que me quede esperando
										//un esi nuevo. CREO QUE ESTO NO ES NECESARIO YA.
										
										eliminarEsiPorId(ready,esi->id);
										eliminarEsiPorId(ejecucion,esi->id);
										eliminarEsiPorId(bloqueados,esi->id);
									 	if (FD_ISSET(i, &master)) {
										 //flagEjecutar=0;
										
										puts("Cerramos el socket del ESI y lo borramos del conjunto maestro");

										 close(i);
										 FD_CLR(i, &master);
										
									 	 }
										break;
										f_ejecutar=1;
									}
									puts("Recibi el resultado");
									printf("Resultado: %d \n",resultado->tipoResultado);
									printf("Resultado: %s \n",resultado->clave);
									printf("La operacion fue un %d \n",resultado->tipoOperacion);
									strcpy(esi->clave,resultado->clave);
									re=recibirResultado2(resultado);
									if(re==-4)
									{
										killEsi(esi->id);
									}
									if(re==1)
									{
										if(resultado->tipoOperacion==OPERACION_GET)
										{
											puts("El esi hizo un GET");
											int size=list_size(esi->clavesTomadas);
											/*
												void verClavesTomadas(char * claveArevisar){
													printf("Una de mis claves antes de agregar es:%s\n",claveArevisar);
												}
												list_iterate(esi->clavesTomadas,(void *)verClavesTomadas);
												*/
											if(!tieneLaClaveTomada(resultado->clave,esi))
											{
												printf("Agregue la clave tomada %s al esi %d\n",resultado->clave,esi->id);

												char * claveCopia = malloc(TAMANIO_CLAVE+1);
												strcpy(claveCopia,resultado->clave);
												//list_add(esi->clavesTomadas,newClave(resultado->clave));
												list_add(esi->clavesTomadas,claveCopia);
												/*
												void verClavesTomadas(char * claveArevisar){
													printf("Una de mis claves es:%s\n",claveArevisar);
												}
												list_iterate(esi->clavesTomadas,(void *)verClavesTomadas);
												*/	
											}
											else{
												puts("Ya tenia la clave");
											}
										
											size=list_size(esi->clavesTomadas);
											//printf("DEBUG: el size es %d \n",size);
										}
										if(resultado->tipoOperacion==OPERACION_STORE)
										{
											printf("El esi %d hizo un STORE de la clave %s\n",esi->id,resultado->clave);
											int coincidir(char clave[TAMANIO_CLAVE]){
																return (string_equals_ignore_case(clave,resultado->clave));
															}
																						
										list_remove_by_condition(esi->clavesTomadas, (void*) coincidir);
										int size=list_size(esi->clavesTomadas);
											//printf("DEBUG: el size es %d \n",size);
										}
										if(resultado->tipoOperacion==OPERACION_SET)
										{
											puts("El esi hizo un SET");
										}
									}
									enviarConfirmacion=1;
							
							}
							free(resultado);
						}//cierre el else recibir
					}//cierro el fdset

					if(flagOperar==1){
						//puts("entre ");
						if(queue_is_empty(ejecucion)==0)
						{
						
							f_ejecutar=0;
							
							if(re==2)
							{
								//Si se bloquea
								esi->cont++;
								if(algoritmo==HRRN)
								{
									sumarEspera(ready);
								}
								queue_pop(ejecucion);
								if(algoritmo==SJF ||algoritmo==SJFD)
									estimacionEsi(esi);
								if(algoritmo==HRRN)
								{
									puts("ESTIMACION HRRN");
									estimacionEsi(esi);
									estimacionHRRN(esi);
									//estimacionHRRN(esi);
									//haria algo mas	
								}
								printf("Esi de id:%d entro a bloqueados pidiendo la clave: %s \n",esi->id,esi->clave);
								queue_push(bloqueados,new_ESI(esi->id,esi->fd,esi->estimacion,esi->responseRatio,esi->espera,esi->clave,esi->clavesTomadas));
								puts("Se agrego a la cola de bloqueados");

								//DEBUG
									int i=0;
									int j=0;
									char * claveAux;
									printf("La clave a buscar es: %s \n",clave);
									while(i<list_size(bloqueados->elements))
									{
										esi=list_get(bloqueados->elements,i);
										//recorrer claves tomadas
										while(j<list_size(esi->clavesTomadas))
										{
											claveAux=list_get(esi->clavesTomadas,j);
											printf("El esi %d tiene la claveTomada %s \n",esi->id,claveAux);
											j++;
										}
										j=0;
										i++;
									}
									printf("DEBUG: Se bloqueo pidiendo la clave: %s \n",esi->clave);
									
								//FIN DEBUG

								f_ejecutar=1;
								enviarConfirmacion=0;
								if(queue_is_empty(ready))
									printf("No hay nada en Ready, asi que seguro me quedo esperando que desbloqueen la clave %s \n",esi->clave);
								else
									puts("Hay algo en ready");
								re=0;
							}
							if (re==1){
								esi->cont++;
								if(algoritmo==HRRN)
								{
									sumarEspera(ready);
								}
								printf("Contador De ESI %d  estimacion %f espera %d\n",esi->cont, esi->estimacion,esi->espera);
								if (desalojar != 1)
									re=0;
							}
							if(re==-5)
							{
								queue_pop(ejecucion);
								queue_push(finalizados,new_ESI(esi->id,esi->fd,esi->estimacion,esi->responseRatio,esi->espera,esi->clave,esi->clavesTomadas));
								puts("____________________________FIN ESI_________________________");
								printf("Estimacion final del ESI %d es %f\n", esi->id, esi->estimacion);
								puts("____________________________FIN ESI_________________________");

								//puts("DEBUG: Vamos a ver las claves tomadas que tiene antes de irse");
								
								if (FD_ISSET(i, &master)) {
										 //flagEjecutar=0;
										
										 puts("Cerramos el socket del ESI y lo borramos del conjunto maestro");

										 close(i);
										 FD_CLR(i, &master);
									 	 }
								puts("Dame otro esi");
								f_ejecutar=1;
								enviarConfirmacion=0;
							    re=0;
							}
							if(desalojar==1 && re !=0)
							{
								puts("ENTRE A DESALOJO -- ENTRE A DESALOJO -- ENTRE A DESALOJO -- ENTRE A DESALOJO-- ENTRE A DESALOJO");
									t_esi* esi1 = malloc(sizeof(t_esi));
									if(queue_is_empty(ejecucion)==0){
										esi1=queue_pop(ejecucion);
										//printf("EL CONTADOR DEL ESI ESSSSS %d \n",esi1->cont);
										//No se reestima el esi al desalojarse, solo se guarda ese contador para reestimar luego.
										queue_push(ready,new_ESI_desalojo(esi1->id,esi1->fd,esi1->estimacion,esi1->responseRatio,esi1->espera,esi1->clave,esi->clavesTomadas,esi1->cont));
										ordenarEsis(ready);
									}
									free(esi1);
									enviarConfirmacion=0;
									puts("Entre a desalojo");
									re=0;
									desalojar=0;
									f_ejecutar=1;				
							}
							if(enviarConfirmacion==1)
							{
								//puts("Le voy a decir que ejecute al ESI");
								esi = queue_peek(ejecucion);
								int re=EnviarConfirmacion(esi);
								if(re==-1)
								{
									puts("Se desconecto el esi pero entramos a un caso especial");
									//fallo el send por lo tanto desconectamos el esi.
									//queue_pop(ejecucion);
								}
								printf("ejecuta el esi:%d de contador %d\n",esi->id,esi->cont);
								
								enviarConfirmacion=0;
								recibi=1;
								re=0;
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
								//if(algoritmo==SJF || algoritmo==SJFD)
								list_map(ready->elements,(void *)estimacionHRRN);

								void mostrarRR(t_esi * esito){
													printf("RR: el rr del esi %d es:%f\n",esito->id,esito->responseRatio);
												}
								list_iterate(ready->elements,(void *)mostrarRR);

								//estimacionHRRN(esi);
								ordenarEsis(ready);
								esi = queue_pop(ready);
								re=0;
								enviarConfirmacion=1;	
								printf("Id del esi a buscar:%d \n",esi->id);
								printf("esi de id %d cambiado de cola con espera de: %d \n",esi->id,esi->espera);
								queue_push(ejecucion,new_ESI_desalojo(esi->id,esi->fd,esi->estimacion,esi->responseRatio,0,esi->clave,esi->clavesTomadas,esi->cont));
							}
							//puts("Ready no vacia");
						}
						
					}
				}//ciero el for del select

				}//Cierra el for del main
    	        
    	        return 0;
				}//cierro el while keeprunning
    }//Cierra el main
bool tieneLaClaveTomada (char clave[TAMANIO_CLAVE],t_esi * esi)
{
	bool b=false;
	int i=0;
	char * claveAux;
	while((i<list_size(esi->clavesTomadas)) && (b==false))
	{
		claveAux=list_get(esi->clavesTomadas,i);
		if(string_equals_ignore_case(clave,claveAux)==true)
		{
			b=true;
		}
		i++;
	}
	//puts("Debug Error malloc Pruebo free en claveaux");
	return b;

}

void ordenarEsis(t_queue *cola)
	{
		if(algoritmo==SJF || algoritmo==SJFD)
		{
			bool compare(t_esi *esi1,t_esi *esi2)
			{
				return esi1->estimacion <= esi2->estimacion;
			}
			list_sort(cola->elements,(void *)compare);
		}
		if(algoritmo==HRRN)
		{
			bool HrrnComp(t_esi *esi1,t_esi *esi2)
			{
				printf("Comparando el rr %f del esi %d con el rr %f del esi %d \n",esi1->responseRatio,esi1->id,esi2->responseRatio,esi2->id);
				if(esi1->responseRatio==esi2->responseRatio)
				{
					puts("DEBUG: RR IGUAL");
					if(esi2->exe==1)
					{
						printf("Esi de id %d estaba en ejec antes \n",esi1->id);
						return false;
					}
					else

						return true;
				}
				else
				{
					return esi1->responseRatio>esi2->responseRatio;
				}
				//return esi1->responseRatio>=esi2->responseRatio;
			}
			list_sort(cola->elements,(void *)HrrnComp);
		}
		return cola;
	}
	
	t_esi * buscarEsi(t_queue *lista,char clave[TAMANIO_CLAVE],t_esi* esi)
	{
		int coincidir(t_esi *unEsi){
          	    	    		return (string_equals_ignore_case(unEsi->clave,clave)==true);//unEsi->clave == clave;
          	    	    	}
		//t_esi *esi=malloc(sizeof(t_esi));
		esi=list_find(lista->elements,(void*)coincidir);
		return esi;
		//free(esi);
	}

	t_esi * buscarEsiPorId(t_queue *lista,int id,t_esi * esi)
	{
		int coincidir(t_esi *unEsi){
          	    	    		return (unEsi->id==id);//unEsi->clave == clave;
          	    	    	}
		//t_esi *esi=malloc(sizeof(t_esi));
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
		   return -1;
		   //exit(1);
	    }
	   puts("Se envió confirmacion");
	   return EXIT_SUCCESS;
    }

	void destruirEsi(t_esi *unEsi){
		puts("Destruyendo el esi ");
    	   //free(unaEntrada->clave);
    	 // free(unEsi);
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
	  while (keepRunning) {
		
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
				
				killEsi(id);
			}
			if(strncmp(linea,"status",6)==0)
			{
				//int id=obtenerId(6,linea);
				//printf("id: %d \n",id);
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
				status(clave);
			}
			if(strncmp(linea,"deadlocks",9)==0)
			{
				puts("Mostrar bloqueos mutuos");
				deadlock();
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
	void status (char clave[TAMANIO_CLAVE])
	{
		tSolicitudPlanificador * solicitud = malloc(sizeof(tSolicitudPlanificador));
		solicitud->solicitud= STATUS;
		   if (send(sockCord, solicitud, sizeof(tSolicitudPlanificador), 0) <= 0){
			   puts("Error al enviar header de STATUS de una clave");
			   perror("Send");
		   }
		   printf("Se envió header de STATUS  %d \n",solicitud->solicitud);
		   

		   if (send(sockCord, clave, TAMANIO_CLAVE, 0) == -1){
			   puts("Error al enviar una clave");
			   perror("Send");

		   }
		   printf("Se envió la clave de status: %s \n",clave);
	}
	void killEsi (int id)
	{
		kill(id,SIGTERM);
		tSolicitudPlanificador * solicitud = malloc(sizeof(tSolicitudPlanificador));
		solicitud->solicitud= KILL;
		   if (send(sockCord, solicitud, sizeof(tSolicitudPlanificador), 0) <= 0){
			   puts("Error al enviar header de KILL un esi");
			   perror("Send");
		   }
		   printf("Se envió header de KILL  %d \n",solicitud->solicitud);
		char idS[5];//=malloc(sizeof(int));
		sprintf(idS,"%d",id);
		 if (send(sockCord,idS,sizeof(idS), 0) == -1){
			   printf("Error al enviar el id: %d \n",id);
			   perror("Send");
		   }
		   printf("Se envió el id del esi: %d \n",id);


	}
	
	void desbloquearEsi(char clave[TAMANIO_CLAVE])
	{
		t_esi * desbloqueado = malloc(sizeof(t_esi));
											//notificacion->clave
		ordenarEsis(bloqueados);
		
		desbloqueado=buscarEsi(bloqueados,clave,desbloqueado);
		if(desbloqueado!=NULL)
		{
			puts("desbloqueado no es nulo");
			queue_push(ready,new_ESI(desbloqueado->id,desbloqueado->fd,desbloqueado->estimacion,desbloqueado->responseRatio,desbloqueado->espera,desbloqueado->clave,desbloqueado->clavesTomadas));
			eliminarEsiPorId(bloqueados,desbloqueado->id);
		}
		else
		{
			puts("No hay ningun esi bloqueado con esa clave pero la liberamos igualmente");
		}
		free(desbloqueado);
		
		puts("Envio header de clave a desbloquear al Coordinador");
		tSolicitudPlanificador * solicitud = malloc(sizeof(tSolicitudPlanificador));
		solicitud->solicitud= DESBLOQUEAR;
		   if (send(sockCord, solicitud, sizeof(tSolicitudPlanificador), 0) <= 0){
			   puts("Error al enviar header de una clave bloqueada");
			   perror("Send");

		   }
		   printf("Se envió header de clave desbloqueada %d \n",solicitud->solicitud);
		  //free(solicitud);

		puts("Envio la clave desbloqueada");

		   if (send(sockCord, clave, TAMANIO_CLAVE, 0) == -1){
			   puts("Error al enviar una clave bloqueada");
			   perror("Send");
		   }
		   printf("Se envió una clave desbloqueada: %s \n",clave);

		if(algoritmo==SJF || algoritmo==SJFD)
		{
			ordenarEsis(ready);
			if(algoritmo==SJFD)
			{
				if(queue_is_empty(ejecucion)==0){
					t_esi* esi1 = malloc(sizeof(t_esi));
					esi1=queue_pop(ejecucion);
					queue_push(ready,new_ESI(esi1->id,esi1->fd,esi1->estimacion,esi1->responseRatio,esi1->espera,esi1->clave,esi1->clavesTomadas));
					ordenarEsis(ready);
					free(esi1);
				}
			}
			
		}
		if(algoritmo==HRRN)
		{
			//EstimarHRRN(ready);
			ordenarEsis(ready);
			/*if(queue_is_empty(ejecucion)==0)
			{
				t_esi* esi1 = malloc(sizeof(t_esi));
				esi1=queue_pop(ejecucion);//(int id,int fd,int esti,char clave[TAMANIO_CLAVE])
				queue_push(ready,new_ESI_hrrn(esi1->id,esi1->fd,esi1->estimacion,esi1->responseRatio,esi1->espera,esi1->clave,1,esi1->clavesTomadas,esi1->cont));
				ordenarEsis(ready);
				free(esi1);
			}*/
		}
		free(solicitud);
	}
	
	void bloquearEsi(int id,char clave[TAMANIO_CLAVE])
	{
		t_esi* esi = malloc(sizeof(t_esi));
		esi=buscarEsiPorId(ready,id,esi);
		if(esi!=NULL)
		{
			//Esi estaba en ready
			puts("esi estaba en ready");
			eliminarEsiPorId(ready,esi->id);
			queue_push(bloqueados,new_ESI(esi->id,esi->fd,esi->estimacion,esi->responseRatio,esi->espera,clave,esi->clavesTomadas));
			enviarClaveCoordinador(clave,BLOQUEAR);
		}else
		{
			puts("esi no estaba en ready");
		}
		puts("Voy a buscar en ejecucion");
		esi=buscarEsiPorId(ejecucion,id,esi);
		puts("Ya busque");
		if(esi!=NULL)
		{
			//Esi estaba en ready
			puts("esi estaba en ejecucion");
			if(algoritmo==SJF||algoritmo==SJFD)
				estimacionEsi(esi);
			eliminarEsiPorId(ejecucion,esi->id);
			queue_push(bloqueados,new_ESI(esi->id,esi->fd,esi->estimacion,esi->responseRatio,esi->espera,clave,esi->clavesTomadas));
			enviarClaveCoordinador(clave,BLOQUEAR);
		}
		else{
			puts("esi no estaba en ejecucion");
			puts("No habia esi pa bloquear ");
		}
		
		
	}
	void enviarClaveCoordinador(char clave[TAMANIO_CLAVE],tSolicitudesDeConsola soli)
	{

	tSolicitudPlanificador * solicitud = malloc(sizeof(tSolicitudPlanificador));
	solicitud->solicitud=soli;
		 if (send(sockCord, solicitud, sizeof(tSolicitudesDeConsola), 0) <= 0){
					   puts("Error al enviar solicitud");
					   perror("Send");
				   }
				   else
				   {
				   		puts("Se envió el header BLOQUEAR");
				   }
		if (send(sockCord, clave,TAMANIO_CLAVE, 0) <= 0){
					   puts("Error al enviar la clave");
					   perror("Send");
				   }
				   else
				   {
				   		puts("Ya le avise al coordinador la clave bloqueada");
				   }
	free(solicitud);
	}

	void obtenerBloqueados(char clave[TAMANIO_CLAVE])
	{
		t_queue * aux=queue_create();
	    //printf("La clave es: %s \n",clave);
		int coincidir(t_esi *unEsi){
								//printf("Comparando clave %s con clave %s \n",unEsi->clave,clave);
          	    	    		return (string_equals_ignore_case(unEsi->clave,clave));//unEsi->clave == clave;
          	    	    	}
		aux->elements=list_filter(bloqueados->elements,coincidir);
		t_esi *esi=malloc(sizeof(t_esi));
		//puts("voy a ver si hay algo adentro");
		while(queue_is_empty(aux)==0)
		{
			esi=queue_pop(aux);
			printf("Esi de id: %d esperando recurso %s \n",esi->id,esi->clave);
		}
		free(aux);
		//free(esi);
		

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

	 	 puts("recalculamos la estimacion");
		esi->estimacion= ((0.01*Alfa*esi->cont)+((1-(Alfa*0.01))*(esi->estimacion)));
		printf("La estimacion ahora del esi %d es %f : \n",esi->id,esi->estimacion);
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
					     int id = headerRecibido->idProceso;
						 free(headerRecibido);
						return(id);
					   }
	}
int recibirResultado2(tResultado * resultado){
	switch(resultado->tipoResultado){
    		case OK:
    			puts("La operación salio OK");
				return 1;
    			break;
    		case BLOQUEO:
				puts("La operación se BLOQUEO");
				return 2;
				break;
    		case ERROR:
				puts("La operación tiro ERROR");
				return -4;
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
		if(resultado->tipoResultado==CHAU)
		{
			puts("El esi se finalizo correctamente");
		}else{
		printf("Resultado: %d \n",resultado->tipoResultado);
        printf("Resultado: %s \n",resultado->clave);
		printf("La operacion fue un %d \n",resultado->tipoOperacion);
		}
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
				   exit(1);
			   }
			   puts("Se envió una clave bloqueada");
		}

		free(cantidadClavesBloqueadas);
	}

	void sumarEspera(t_queue *cola)
	{
		void * SumarEspera(t_esi * esi)
		{
			
			esi->espera=(esi->espera)+1;
			printf("Esi de id: %d con espera actual de: %d \n",esi->id,esi->espera);
			return esi;
		}
		//puts("Voy a mapear ready");
		if(queue_is_empty(cola)==0){
			//puts("ready no vacia");
		cola->elements=list_map(cola->elements,SumarEspera);
		//puts("mapie ready");
		}	
	}

	void estimacionHRRN(t_esi* esi){    //Calcula la tasa de Transferencia de los esis
			//puts("estimamos TASA de trans");
			//estimacionEsi(esi);
			esi->responseRatio = ((esi->espera + esi->estimacion)/(esi->estimacion));
			printf("El RR del esi: %d es : %f \n",esi->id,esi->responseRatio);
			//return esi;
		}

	
void deadlock ()
{
	//puts("Entre a deadlocks");
	t_queue * colaDeadlock = queue_create();
	t_list * aux=list_create();
	//aux = bloqueados->elements;
	list_add_all(aux,bloqueados->elements);

	t_esi* esi;//=malloc(sizeof(t_esi));

	int f=0;
	
	while(f<list_size(bloqueados->elements))
	{
		esi=list_get(bloqueados->elements,f);
		//printf("DEBUG: En bloqueados hay un esi de id: %d \n",esi->id);
		f++;
	}
	

	t_esi* esiClave;//=malloc(sizeof(t_esi));
	t_esi* esiP;//=malloc(sizeof(t_esi));

	int encontrado=0;
	int size=list_size(aux);
	//printf("El tamanio de la lista es de: %d \n",size);
	int i=0;
	while(i<size)//for(int i=0;i<size;i++)
	{
		//puts("voy a agarrar un esi");
		if(list_size(aux)==0)
		{
			puts("voy a breakear");
			break;
		}	
		esiP=list_get(aux,i);
		
		if(esiP==NULL)
		{
			puts("breakeo porque es nulo");
			break;
		}
		
		//printf("El esi que agarre es %d \n",esiP->id);
		queue_push(colaDeadlock,new_ESI(esiP->id,esiP->fd,esiP->estimacion,esiP->responseRatio,esiP->espera,esiP->clave,esiP->clavesTomadas));
		//printf("DEBUG: acabo de meter en cola el esi con id %d \n",esiP->id);
				
		//puts("voy a revisar bloqueados");
		esiClave = revisarBloqueado(esiP->clave,aux,esiClave);
		//puts("Voy a printear x2");
		//printf("Primer revisarBloqueado nos da el esi %d \n",esiClave->id);
		int x=0;
		//esi=queue_peek(colaDeadlock);
		while(esiClave!=NULL && x<size && encontrado!=1)
		{	
			
			if(esiP->id==esiClave->id)
			{
				//hay deadlock, porque volvimos al esi primero
				puts("Hay deadlock");
				//printf("DEBUG: acabo de meter en cola el esi con id %d \n",esiClave->id);
				encontrado=1;
			}
			else
			{
				puts("Entre al else (no deadlock)");
				queue_push(colaDeadlock,new_ESI(esiClave->id,esiClave->fd,esiClave->estimacion,esiClave->responseRatio,esiClave->espera,esiClave->clave,esiClave->clavesTomadas));
				esiClave = revisarBloqueado(esiClave->clave,aux,esiClave);//Buscamos quien tiene la clave del que concatenamos
			}

			x++;
		}
		x=0;
		if(encontrado==1)
		{
			
			//Mostramos el deadlock y removemos de la cola quienes esten en deadlock
			while(queue_is_empty(colaDeadlock)==0)
			{
				esi=queue_pop(colaDeadlock);
				printf("DEADLOCK: con esi de id %d \n",esi->id);

				int coincidir(t_esi *unEsi){
          	    	    		return unEsi->id == esi->id;
          	    	    	}
							  
          		list_remove_and_destroy_by_condition(aux,(void*) coincidir,(void *)destruirEsi);
				 // puts("Ya destrui");
			}	
			i=-1;
			size = list_size(aux);
			/*printf("Sizie de vuelta %d, el pointer esta en %d \n",size,i);
				esiP=list_get(aux,i+1);
				
				if(esiP==NULL)
				{
					puts("nulo el get");
					
				}else{
					puts("no nulo el esip");
				}*/
			queue_clean(colaDeadlock);
		}else
		{
			//puts("he entrado al limpiar colas");
			//debemos limpiar la cola deadlocks
			queue_clean(colaDeadlock);
		}	
		i++;
	}//fin de while principal
	//free(esi);
	//free(esiP);	
}

t_esi * revisarBloqueado(char clave[TAMANIO_CLAVE],t_list * lista,t_esi* esi)
{
	int i=0;
	int j=0;
	char * claveAux;
	printf("La clave a buscar es: %s \n",clave);
	/*
	while(i<list_size(lista))
	{
		esi=list_get(lista,i);
		//recorrer claves tomadas
		while(j<list_size(esi->clavesTomadas))
		{
			claveAux=list_get(esi->clavesTomadas,j);
			printf("El esi %d tiene la claveTomada %s \n",esi->id,claveAux);
			j++;
		}
		j=0;
		i++;
	}
	j=0;i=0;
	*/
	int sizel = list_size(lista);
	while(i<sizel)
	{
		//puts("Voy a obtener esi");
		esi=list_get(lista,i);
		//recorrer claves tomadas
		//printf("DEBUG: esi que agarre es %d \n",esi->id);

		//puts("DEBUG: busco size de clave");
		int sizec=list_size(esi->clavesTomadas);
	//	puts("DEBUG: salgo size de clave");
		while(j<sizec)
		{
			//puts("DEBUG: Voy a obtener clave");
			claveAux=list_get(esi->clavesTomadas,j);
			//printf("El esi %d tiene la claveTomada %s \n",esi->id,claveAux);
			if(string_equals_ignore_case(clave,claveAux)==true && esi->clave!=clave)
			{
			//	puts("Encontre al esi que tiene la clave");
				return esi;
			}
			else{
			//	puts("No lo encontre (todavia)");
			}
			//printf("DEBUG:  J esta en %d \n",j);
			j++;
		}
		j=0;
		i++;
	}
	//puts("Voy a printear");
	//printf("DEBUG: El esi que tiene la clave %s es el esi de id %d",clave,esi->id);
	return NULL;
}
