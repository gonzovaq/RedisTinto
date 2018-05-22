	#include "Planificador.h"

    int main(int argc, char *argv[])
    {
    	LeerArchivoDeConfiguracion();
    	verificarParametrosAlEjecutar(argc, argv);
    			//FIFO
    		  	Fifo *primero = malloc(sizeof(Fifo));
    		  	Fifo *ultimo = malloc(sizeof(Fifo));
    	        primero = NULL;
    	        ultimo = NULL;
    	        //
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

    	        // obtener socket para coordinador
    	        ConectarAlCoordinador(&sockCord, &cord_addr, he);


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

				//Creemos la cola ready de los esis
				colaESIS = queue_create();

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
    	                            queue_push(colaESIS,newfd); //todo probablemente no debamos agregar al ESI de esta forma, sino con un mensaje que nos mande con cierto struct
    	                            if (newfd > fdmax) {    // actualizar el máximo
    	                                fdmax = newfd;
    	                            }
    	                            printf("selectserver: new connection from %s on "
    	                                "socket %d\n", inet_ntoa(remoteaddr.sin_addr), newfd);
    	                        	gestionarConexion(newfd);
									
    	                        	//printf("tst2");
    	                        }
    	                    } else {
    	                        // gestionar datos de un cliente
    	                        if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
    	                            // error o conexión cerrada por el cliente
    	                            if (nbytes == 0) {
    	                                // conexión cerrada
    	                                printf("selectserver: socket %d hung up\n", i);
    	                            } else {
    	                                perror("recv");
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
    	                        }
    	                    } // Esto es ¡TAN FEO!
    	                }
    	            }
    	        }
				
    	        return 0;
    }
    void ConectarAlCoordinador(int  * sockCord, struct sockaddr_in* cord_addr,
    		struct hostent* he) {
    	// obtener socket para coordinador
    	if ((*sockCord = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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
    	if (connect(*sockCord, (struct sockaddr*) &*cord_addr,
    			sizeof(struct sockaddr)) == -1) {
    		puts("Error al conectarme al servidor.");
    		perror("connect");
    		exit(1);
    	}
    	puts("Conectado con el coordinador!\n");
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

   			 if(linea)
      			add_history(linea);

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

	void LeerArchivoDeConfiguracion() {
	// Ejemplo para leer archivo de configuracion en formato clave=valor por linea
	char* token;
	char* search = "=";
	static const char filename[] =
			"/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/Coordinador/src/configuracion.config";
	FILE* file = fopen(filename, "r");
	if (file != NULL) {
		puts("Leyendo archivo de configuracion");
		char line[128];
		/* or other suitable maximum line size */while (fgets(line, sizeof line,
				file) != NULL)/* read a line */
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
	}


	///////////////
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
		    		conexionESI(socket);
		    		break;
		    	default:
		    		puts("Error al intentar conectar un proceso");
		    		close(socket);
		    	}
		    }


		    void *conexionESI(int *socket){
		        puts("ESI conectando");
				//Aca se va a gestionar lo que se haga con este esi.
		       
		    }

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
			void kill(int id)
			{
				int pos;
				int buscado;
				//list_find(colaESIS->elements,compare);
				//TODO: Hay que buscar al proceso dentro de la cola y removerlo para que no replanifique.
				printf("Se mata al proceso: %d en la pos:%d \n",id,pos);
			}
			
			