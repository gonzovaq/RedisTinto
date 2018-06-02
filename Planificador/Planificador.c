	#include "Planificador.h"

    int main(int argc, char *argv[])
    {
    	LeerArchivoDeConfiguracion();
    	verificarParametrosAlEjecutar(argc, argv);
    			//FIFO
    		  	//Fifo *primero = malloc(sizeof(Fifo));
    		  	//Fifo *ultimo = malloc(sizeof(Fifo));
    	        //primero = NULL;
    	       // ultimo = NULL;
    			t_queue *ready;
				t_queue *ejecucion;
    	   		t_queue *colaFinalizados;
    	   		t_queue *colaEspera;
    	   		t_queue *colaX;
    	        fd_set master;   // conjunto maestro de descriptores de fichero
    	        fd_set read_fds; // conjunto temporal de descriptores de fichero para select()
    	        struct sockaddr_in myaddr;     // dirección del servidor
    	        struct sockaddr_in remoteaddr; // dirección del cliente
    	        int fdmax;        // número máximo de descriptores de fichero
    	        int listener;     // descriptor de socket a la escucha
    	        int newfd;        // descriptor de socket de nueva conexión aceptada
    	        int sockCord; 	  // descriptor para conectarnos al Coordinador
    	        char buf[256];    // buffer para datos del cliente
    	        int nbytes;
    	        int yes=1;        // para setsockopt() SO_REUSEADDR, más abajo
    	        int addrlen;
    	        int i, j;
    	        FD_ZERO(&master);    // borra los conjuntos maestro y temporal
    	        FD_ZERO(&read_fds);
				//Creamos las colas que vamos a manejar con las funciones de abajo
								ready = queue_create();
								ejecucion=queue_create();
								colaEspera = queue_create();
								colaFinalizados = queue_create();
								colaX = queue_create();
				//finalizamos el creado de colas
    	        // obtener socket para coordinador
    	        ConectarAlCoordinador(sockCord, &cord_addr, he);


    	        /*Inicializar cola
    	        Fifo *nodo;
    	        nodo->pid = pid;
    	        nodo->sgt = NULL;
    	        primero=nodo;
     	 	 	ultimo=nodo;
    	        */
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
    	        // seguir la pista del descriptor de fichero mayor
    	        fdmax = listener; // por ahora es éste
    	        // bucle principal
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
    	            if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
    	                perror("select");
    	                exit(1);
    	            }
    	            // explorar conexiones existentes en busca de datos que leer
					
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
									queue_push(ready,new_ESI(idEsi,newfd));
																		
									printf("Esi id %d conectado y puesto en cola \n",idEsi);
									
									
    	                        }
							 } /*else {
    	                        // gestionar datos de un cliente
								puts("Entre a gestion de clientes");
    	                        if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
    	                            // error o conexión cerrada por el cliente
    	                            if (nbytes == 0) {
    	                                // conexión cerrada
    	                                printf("selectserver: socket %d hung up\n", i);
    	                            } else {
    	                                perror("recv de gestion de cliente");
    	                            }
    	                            close(i); // bye!
									
    	                            FD_CLR(i, &master); // eliminar del conjunto maestro
    	                        } else {/*
    	                        // gestionar datos de un cliente
								puts("Entre a gestion de clientes");
								tResultado resultado = malloc(sizeof(resultado));
								if((numbytes=recv(sockfd, resultado, sizeof(tResultado), 0)) <= 0){
    	                       // if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
    	                            // error o conexión cerrada por el cliente
    	                            if (nbytes == 0) {
    	                                // conexión cerrada
    	                                printf("selectserver: socket %d hung up\n", i);
    	                            } else {
    	                                perror("recv de gestion de cliente");
    	                            }
    	                            close(i); // bye!
									
    	                            FD_CLR(i, &master); // eliminar del conjunto maestro
    	                        } else {
    	                            // tenemos datos de algún cliente
    	                            for(j = 0; j <= fdmax; j++) {
    	                                // ¡enviar a todo el mundo!
    	                                if (FD_ISSET(j, &master)) {
    	                                    // excepto al listener y a nosotros mismos
    	                                    if (j != listener && j != i) {
    	                                        if (send(j, buf, nbytes, 0) == -1) {
    	                                            perror("send");
    	                                        }
    	                                    }
    	                                }
    	                            }
    	                        }*/
    	                     // Esto es ¡TAN FEO!
						}
					}
						//Planificar
					int f_ejecutar=0;//Flag para mandar de ready a ejecucion.
					//puts("voy a verificar las colas");
					if(queue_is_empty(ejecucion)==0)
					{
						puts("Entre a ejecucion");
						printf("Tamanio cola %d \n",queue_size(ejecucion));
						f_ejecutar=0;
						t_esi *esi=malloc(sizeof(t_esi));
						esi = queue_peek(ejecucion);
						printf("Id del esi a ejecutar: %d \n",esi->id);
						EnviarConfirmacion(esi);
						printf("ejecuta el esi:%d \n",esi->id);
						
						tResultado * resultado = malloc(sizeof(tResultado));
                   		int re=recibirResultado(esi->fd, resultado);
						if(re==-5)
						{
							queue_pop(ejecucion);
							free(esi);
							f_ejecutar=1;
							puts("Dame otro esi");
						}
						//queue_push(ejecucion,new_ESI(esi->id,esi->fd));
						//free(esi);
                    	free(resultado);
					}
					else
					{
						f_ejecutar=1;//Como esta vacia, pedimos que nos manden un esi de ready
					}
					if(queue_is_empty(ready)==0)
					{
						if(f_ejecutar==1)
						{
							t_esi *esi=malloc(sizeof(t_esi));
						    esi = queue_pop(ready);
							printf("esi de id %d cambiado de cola \n",esi->id);
							queue_push(ejecucion,new_ESI(esi->id,esi->fd));
							free(esi);
						}
						puts("Ready no vacia");
					}
    	            
				
				}
    	        
				
    	        return 0;
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

    void ConectarAlCoordinador(int sockCord, struct sockaddr_in* cord_addr,
    		struct hostent* he) {
    	// obtener socket para coordinador
    	if ((sockCord = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    		puts("Error al crear el socket");
    		perror("socket");
    		exit(1);
    	}
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
    	puts("Conectado con el coordinador!\n");
		enviarHeader(sockCord);
    }

    int verificarParametrosAlEjecutar(int argc, char *argv[]){

        if (argc != 2) {
        	puts("Error al ejecutar, te faltan parametros.");
            exit(1);
        }


        if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina
        	puts("Error al obtener el hostname, te faltan parametros.");
        	perror("gethostbyname");
            exit(1);
        }
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
				puts("va a pausar");
			}
			if(strncmp(linea,"cola",4)==0)
			{
		
				mostrarCola();
			}
			if(strncmp(linea,"continuar",9)==0)
			{
				puts("va a continuar");
				puts(linea);
			}
			if(strncmp(linea,"bloquear",8)==0)
			{

				char* clave1;
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
				clave1=malloc(sizeof(char[j]));
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
				free(clave1);
			}
			if(strncmp(linea,"desbloquear",11)==0)
			{
				//obtener id
				int id=obtenerId(11,linea);
				printf("id: %d \n",id);
			}
			if(strncmp(linea,"kill",4)==0)
			{
				//obtener id
				int id=obtenerId(4,linea);
				printf("id: %d \n",id);
				kill(id);
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

		}

	return 0;
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
		return id;
	}

	
/*	void LeerArchivoDeConfiguracion() {
	// Ejemplo para leer archivo de configuracion en formato clave=valor por linea
	char* token;
	char* search = "=";
	static const char filename[] =
			"/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/Coordinador/src/configuracion.config";
	FILE* file = fopen(filename, "r");
	if (file != NULL) {
		puts("Leyendo archivo de configuracion");
		char line[128];
		 or other suitable maximum line size while (fgets(line, sizeof line,
				file) != NULL)//
		{
			// Token will point to the part before the =.
			token = strtok(line, search);
			puts(token);
			// Token will point to the part after the =.
			token = strtok(NULL, search);
			puts(token);
		}
		fclose(file);
	} else
		puts("Archivo de configuracion vacio");
	}*/

	int  LeerArchivoDeConfiguracion() {

			// Leer archivo de configuracion con las commons
			t_config* configuracion;

			configuracion = config_create(ARCHIVO_CONFIGURACION);

			PORT = config_get_int_value(configuracion, "PORT");

			IP = config_get_string_value(configuracion, "IP");

			IPCO =config_get_string_value(configuracion, "IPCO");
			//puts("entram02");
			PORT_COORDINADOR = config_get_int_value(configuracion, "PORT_CORDINADOR");
			//puts("entram03");
			MAXDATASIZE = config_get_int_value(configuracion, "MAX_DATASIZE");
			//puts("entram04");

			return 1;

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
						free(headerRecibido);
						return(headerRecibido->idProceso);
					   }
	}

	int recibirResultado(int socket, tResultado * resultado){
    	puts("Vor a recibir el resultado");
    	recibirResultadoDelEsi(socket,resultado);

    	switch(resultado->tipoResultado){
    		case OK:
    			puts("La operación salio OK"); //EN ESTOS CASE DEBERIA LOGGEAR O ALGO ASI, PREGUNTAR MAS ADELANTE
    			break;
    		case BLOQUEO:
				puts("La operación se BLOQUEO");
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
            //exit(1);
        }
        puts("Recibi el resultado");
		printf("Resultado: %d \n",resultado->tipoResultado);
        printf("Resultado: %s \n",resultado->clave);
		return EXIT_SUCCESS;
    }

	///////////////
	/*
		void *gestionarConexion(int socket){ //(int* sockfd, int* new_fd)
		        int bytesRecibidos;
		               tHeader *headerRecibido = malloc(sizeof(tHeader));
				   if ((bytesRecibidos = recv(socket, headerRecibido, sizeof(tHeader), 0)) == -1){
					perror("recv");
					exit(1);
				   }
				   //fprintf("Mensaje : %s",headerRecibido->tipoMensaje);
				   if (headerRecibido->tipoMensaje == CONECTARSE){
					IdentificarProceso(headerRecibido, socket);
					free(headerRecibido);
				   }
		    }
		 void IdentificarProceso(tHeader* headerRecibido,
		    		int socket) {
		    	switch (headerRecibido->tipoProceso) {
		    	case ESI:
		    		printf("Se conecto el proceso %d \n", headerRecibido->idProceso);
					
		    		readyEsi(socket,headerRecibido->idProceso);
		    		// si hago un AgregarACola(colaESIS,new_esi bla bla aca me tira segmentation fault, osea que no tiene acceso a la memoria de la cola
		    		break;
		    	default:
		    		puts("Error al intentar conectar un proceso");
		    		close(socket);
		    	}
		    }
			
		    void readyEsi(int socket,int id){
		        puts("ESI conectando");
				t_esi *esi=malloc(2*sizeof(int));
				esi->id=id;
				esi->fd=socket;
				//AgregarACola(*ready,esi);
				queue_push(*ready,esi);
				free(esi);
				//Aca se va a gestionar lo que se haga con este esi.
		       
		    }
*/
		    void AgregarACola(t_queue *Cola,t_esi *esi){

		    				queue_push(Cola,esi);

		    				printf("se Agrego a la Cola el proceso: %d \n",esi->id);
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


		    // para DESENCOLAR usarmos queue_pop(t_queue * cola);
		  /*  void BuscarElementoEnCola(t_queue *Cola,t_Esi * elemento)
		    			{
		    		    	Cola-> elements;
		    		  	}*/

		    /*
		    void Encolar(Fifo*ultimo,int EsiId){  //agrega al final de la cola
		    	while(ultimo->sgt != NULL)
		    	         ultimo=ultimo->sgt;
		        Fifo *nodo;
		        nodo->pid=EsiId;
		        nodo->sgt=NULL;
		        ultimo->sgt = nodo;
		        ultimo = nodo;
		    }
		    void DesEncolar(){
		    }
			void mostrarCola()
			{	
				int tam = queue_size(colaESIS);
				printf("Tamanio de cola: %d \n",tam);
			}
			bool compare(int a)
			{
				if(a==21)
					return true;
				else
					return false;
			}
			void kill(int id) // esto no se puede hacer porqe no hay un index en las colas
			{
				int pos;
				int buscado;
				//list_find(colaESIS->elements,compare);
				//TODO: Hay que buscar al proceso dentro de la cola y removerlo para que no replanifique.
				printf("Se mata al proceso: %d en la pos:%d \n",id,pos);
			}
			
			*/
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
				   free(header);
				   exit(1);
			   }
			   puts("Se envió mi identificador");
			   free(header);
			   return EXIT_SUCCESS;
    }