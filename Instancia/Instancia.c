#include "Instancia.h"

	pthread_mutex_t mutex;
    int main(int argc, char *argv[])
    {
    	puts("Iniciando");
    	//cantidadEntradas = 8;
    	//tamanioValor = 3;
    	tablaEntradas = list_create();
    	int socketCoordinador;
        int longitudMaximaValorBuscado;
        int tamanioValorRecibido;


        verificarParametrosAlEjecutar(argc, argv);
        LeerArchivoDeConfiguracion(argv);

        he = gethostbyname(IP);
        socketCoordinador = conectarmeYPresentarme(PORTCO);


        char **arrayEntradas = malloc(cantidadEntradas * sizeof(char*));

    	for (int i= 0; i < cantidadEntradas; i++){
    		arrayEntradas[i] = calloc(tamanioValor, sizeof(char));
    	}

    	colaParaLFU = queue_create();

//    	char *punteroCIRC = malloc(sizeof(char*));
//     	punteroCIRC = arrayEntradas[posicionPunteroCirc];

    	//DUMP
    	pthread_t tid;
    	int stat = pthread_create(&tid, NULL, (void*)Dump, (void*)arrayEntradas);
		if (stat != 0){
			puts("error al generar el hilo");
			perror("thread");
			//continue;
		}
		pthread_detach(tid); //Con esto decis que cuando el hilo termine libere sus recursos

	//	semaforo = malloc(sizeof(sem_t));

		//sem_wait(semaforo);

        while(1){
        	puts("Informo que estoy viva");
        	//EnviarAvisoDeQueEstoyViva(socketCoordinador);
        	puts("Recibo operacion");
        	tOperacionInstanciaStruct * operacion = malloc(sizeof(tOperacionInstanciaStruct));
        	if ((recv(socketCoordinador, operacion, sizeof(tOperacionInstanciaStruct), 0)) <= 0){
        	    		puts("Fallo al recibir el tipo de operacion");
        	    		perror("recv");
        	    		exit(1);
        	    	}
        	puts("Informo que estoy viva");
        	EnviarAvisoDeQueEstoyViva(socketCoordinador);

        	if(operacion->operacion == SOLICITAR_VALOR){
        		free(operacion);
        		PedidoDeValores(socketCoordinador);
        		puts("Terminamos el STATUS");
        	}
        	else{
        		free(operacion);
				OperaciontHeader *headerRecibido = malloc(sizeof(OperaciontHeader)); // El malloc esta en recibir header
				puts("Intento recibir header del Coordinador");
				headerRecibido = recibirHeader(socketCoordinador);
				puts("Recibi el header!");
				if (headerRecibido->tipo != OPERACION_GET){
					tamanioValorRecibido = headerRecibido->tamanioValor;
					char *bufferClave;
					char *bufferValor;
					char *valorGet;
					operacionRecibida *operacion = malloc(sizeof(operacionRecibida));


					bufferClave = recibirMensaje(socketCoordinador, TAMANIO_CLAVE);


					if (headerRecibido->tipo == OPERACION_SET){
						//pthread_mutex_lock(&mutex);
						puts("ENTRO A UN SET");
						cantidadClavesEnTabla++;
						if(validarClaveExistente(bufferClave, tablaEntradas) == true){
							eliminarNodosyValores(bufferClave, tablaEntradas, arrayEntradas);
							cantidadClavesEnTabla--;
								}
						bufferValor = recibirMensaje(socketCoordinador, tamanioValorRecibido);
						tamanioValorRecibido = headerRecibido->tamanioValor;
						memcpy(operacion->clave, bufferClave, strlen(bufferClave) + 1);
						operacion->valor = bufferValor;
						agregarEntrada(operacion, arrayEntradas, cantidadEntradas, tamanioValor, tablaEntradas, tamanioValorRecibido);

						enviarEntradasUsadas(socketCoordinador, tablaEntradas, bufferClave);
						free(headerRecibido);
						//free(operacion);
						free(bufferClave);
						free(bufferValor);
						mostrarArray(arrayEntradas, cantidadEntradas);
						puts("HICE UN SET");
						printf("Cantidad de elementos en mi tabla de entradas: %d\n", list_size(tablaEntradas));
						printf("LO QUE ME MUSTRA EL PUNTERO CIRCULAR %s\n", arrayEntradas[posicionPunteroCirc]);
						printf("POSICION PUNTERO CIRCULAR: %d\n", posicionPunteroCirc);
						//sem_post(semaforo);
						//pthread_mutex_unlock(&mutex);
						}


		//        	if (headerRecibido->tipo == OPERACION_GET){
		//        		printf("Buffer claveeeee %s\n",bufferClave);
		//        		if(validarClaveExistente(bufferClave, tablaEntradas) == true)
		//        		{
		//        			puts("ENCONTROOOOOOOOOOOOOOOOOOOOOOOOO");
		//        		}
		//        		longitudMaximaValorBuscado = calcularLongitudMaxValorBuscado(bufferClave,tablaEntradas); //Cantidad de entradas para ese valor (despues multiplico por TamanioValor)
		//        		printf("Longitud maxima valor buscado: %d\n", longitudMaximaValorBuscado);
		//        		valorGet = obtenerValor(longitudMaximaValorBuscado, tablaEntradas, bufferClave,arrayEntradas, tamanioValor);
		//        		printf("A ver que pija sdale: %s\n", valorGet);
		//        		puts("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		//        		enviarValorGet(socketCoordinador, valorGet);
		//        		free(valorGet);
		//        		free(headerRecibido);
		//        		//free(operacion);
		//        		free(bufferClave);
		//        	}
					if (headerRecibido->tipo == OPERACION_STORE){
						//pthread_mutex_lock(&mutex);
						puts("ENTRO A UN STORE");
						longitudMaximaValorBuscado = calcularLongitudMaxValorBuscado(bufferClave, tablaEntradas);
						valorGet = obtenerValor(longitudMaximaValorBuscado, tablaEntradas, bufferClave, arrayEntradas, tamanioValor);
						guardarUnArchivo(bufferClave, valorGet);
						puts("Guarde un archivo");
						enviarEntradasUsadas(socketCoordinador, tablaEntradas, bufferClave);


						free(valorGet);
						free(headerRecibido);
						free(bufferClave);
						puts("TERMINE UN STORE");
						//sem_post(semaforo);
						//pthread_mutex_unlock(&mutex);
					}
				}
				puts("Terminamos esta operacion");
        	}
        }
        close(socketCoordinador);
        return 0;
    }

    int verificarParametrosAlEjecutar(int argc, char *argv[]){
    	if (argc != 2) {
    	    puts("Falta el nombre del archivo");
    	    exit(1);
    	 }

    	 return 1;
    }

 /*   int leerConfiguracion(){
    	char *token;
    	char *search = "=";
    	static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/Instancia/configuracion.config";
    	FILE *file = fopen ( filename, "r" );
    	if ( file != NULL )
    	{
    		puts("Leyendo archivo de configuracion");
    	    char line [ 128 ]; /* or other suitable maximum line size
    	    while ( fgets ( line, sizeof line, file ) != NULL )  read a line
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

        	return 1;
        }*/
	int LeerArchivoDeConfiguracion(char *argv[]) {

			// Leer archivo de configuracion con las commons
			t_config* configuracion;

			puts("Leemos el archivo de configuracion");

			configuracion = config_create(argv[1]);

			PORTCO = config_get_int_value(configuracion, "PUERTO_COORDINADOR");
			IP = config_get_string_value(configuracion, "IP");
			Intervalo = config_get_int_value(configuracion, "Intervalo");
			nombreInstancia = config_get_string_value(configuracion, "NombreInstancia");
			Algoritmo = config_get_string_value(configuracion, "Algoritmo");
			PuntoMontaje = config_get_string_value(configuracion, "PuntoMontaje");

			if (strcmp(Algoritmo, "CIRC") == 0)
				algoritmoReemplazo = CIRC;
					else {
						if (strcmp(Algoritmo, "LRU") == 0)
							algoritmoReemplazo = LRU;
						else {
							if (strcmp(Algoritmo, "LSU") == 0)
								algoritmoReemplazo = BSU;
							else {
								puts("Error al cargar el algoritmo de distribucion");
								exit(1);
							}
						}
					}

			puts("Se leyo el archivo de configuracion");

			return 1;

		}

    int conectarmeYPresentarme(int port){
    	int socketCoordinador;
    	if ((socketCoordinador = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
    		puts("Error al crear el socket");
    		perror("socket");
    		exit(1);
    	}

    	puts("El socket se creo correctamente\n");

    	their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina
    	their_addr.sin_port = htons(port);  // short, Ordenación de bytes de la red
    	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    	memset(&(their_addr.sin_zero),'\0', 8);  // poner a cero el resto de la estructura

    	if (connect(socketCoordinador, (struct sockaddr *)&their_addr,
    	                                              sizeof(struct sockaddr)) == -1) {
    		puts("Error al conectarme al servidor.");
    		perror("connect");
    		exit(1);
    	}

    	puts("Instancia conectada!\n");

    	enviarHeader(socketCoordinador);

    	recibirConfiguracion(socketCoordinador);

    	//recibirClavesPrevias(socketCoordinador);

    	return socketCoordinador;

    	}

    int PedidoDeValores(int socketCoordinador)
    {
		char clave[TAMANIO_CLAVE];
    	if((recv(socketCoordinador, clave, TAMANIO_CLAVE, 0)) <= 0){
    		perror("Fallo al recibir la clave");
    	}

    	// TODO: BUSCO VALOR!

    	char* valor;
    	int tamanioValor;
    	tEntradasUsadas * tamanio = malloc(sizeof(tEntradasUsadas));

	    if (send(socketCoordinador, tamanio, sizeof(tEntradasUsadas), 0) <= 0){
		   puts("Error al enviar el el tamanio valor");
		   perror("Send");
	    }
	    free(tamanio);

	    if (send(socketCoordinador, valor, tamanioValor, 0) <= 0){
		   puts("Error al enviar el valor");
		   perror("Send");
	    }
    }

    int recibirConfiguracion(int sockeCoordinador){
    	tInformacionParaLaInstancia * informacion = malloc(sizeof(tInformacionParaLaInstancia));

    	claves = list_create();

    	if((recv(sockeCoordinador, informacion, sizeof(tInformacionParaLaInstancia), 0)) <= 0){
    		perror("Fallo al recibir la configuracion");
    	}

    	cantidadEntradas = informacion->entradas;
    	tamanioValor = informacion->tamanioEntradas;
    	int clavesPrevias = informacion->cantidadClaves;
    	free(informacion);

    	printf("La cantidad de entradas que manejo es %d y tienen un tamanio de %d \n", cantidadEntradas,tamanioValor);

        char * line = NULL;
        size_t len = 0;
        FILE * archivo;

    	for(int i = 0; i < clavesPrevias; i++){
    		char clave[TAMANIO_CLAVE];
        	if((recv(sockeCoordinador, clave, TAMANIO_CLAVE, 0)) <= 0){
        		perror("Fallo al recibir la configuracion");
        	}
        	printf("Recibi la clave %s \n",clave);

    		char nombreArchivo[strlen(PuntoMontaje)+strlen(clave)+5];
    		strcpy(nombreArchivo,PuntoMontaje);
    		strcat(nombreArchivo,clave);
    		strcat(nombreArchivo,".txt\0");
    		printf("El archivo a abrir esta en %s \n",nombreArchivo);
    		archivo = leerArchivo(&nombreArchivo);
    		puts("Voy a leer la linea del archivo");
        	if(getline(&line, &len, archivo) == -1){
        		puts("No habia nada dentro del archivo");
        	}else{
        		printf("El valor de la clave %s es %s \n",clave,line);
        		list_add(claves,clave);
        	} // TODO: Ale te dejo aca para que veas que hacer con esto!
    	}

    	return EXIT_SUCCESS;
    }

    FILE *leerArchivo(char *archivo){

    	//Esto lo copié literal del ejemplo del ParSI. A medida que avancemos, esto en vez de imprimir en pantalla,
    	//va a tener que enviarle los scripts al coordinador (siempre y cuando el planificador me lo indique)

    	printf("El archivo a abrir esta en %s \n",archivo);

    	FILE * file;
        file = fopen(archivo, "r");
        if (file == NULL){
            perror("Error al abrir el archivo: ");
            exit(EXIT_FAILURE);
        }


        return file;
    }

    int enviarHeader(int socketCoordinador){
    	int pid = getpid(); //Los procesos podrian pasarle sus PID al coordinador para que los tenga identificados
    	printf("Mi ID es %d \n",pid);
        tHeader *header = malloc(sizeof(tHeader));
               header->tipoProceso = INSTANCIA;
               header->tipoMensaje = CONECTARSE;
               header->idProceso = pid;
               strcpy(header->nombreProceso, nombreInstancia );  // El nombre se da en el archivo de configuracion
			   if (send(socketCoordinador, header, sizeof(tHeader), 0) <= 0){
				   puts("Error al enviar mi identificador");
				   perror("Send");
				   free(header);
				   exit(1);
			   }
			   puts("Se envió mi identificador");
			   free(header);
			   return 1;
    }

    int enviarValorGet(int socketCoordinador, char *valorGet){
    	int tamanioValorGet = strlen(valorGet);
    	tResultadoInstancia *header = malloc(sizeof(tResultadoInstancia));

    	header->resultado = OK;
    	header->tamanioValor = tamanioValorGet;

    	if(send(socketCoordinador, header, sizeof(OperaciontHeader), 0) <= 0){
    		puts("Error al enviar header de operacion");
    		perror("Send");
    	}

    	if(send(socketCoordinador, valorGet, tamanioValorGet, 0) <= 0){
    		puts("Error al enviar el valor buscado");
    		perror("Send");
    	}
    	free(header);
    	puts("MANDE TODO BIEN");
    	return 1;
    }

    int enviarEntradasUsadas(int socketCoordinador,t_list *tablaEntradas, char *bufferClave){
    	tEntradasUsadas *buffer = malloc(sizeof(tEntradasUsadas));
    	buffer->entradasUsadas = calcularLongitudMaxValorBuscado(bufferClave, tablaEntradas);

    	if(send(socketCoordinador, buffer, sizeof(tEntradasUsadas), 0)<= 0){
    		puts("Error al enviar las entradas usadas");
    		perror("Send");
    	}
    	free(buffer);
    	return 1;
    }

    char *recibirMensaje(int socketCoordinador, int bytesARecibir){
        char* buf = malloc(bytesARecibir) ; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.


        if (recv(socketCoordinador, buf, bytesARecibir, 0) <= 0) {
            perror("recv");
            exit(1);
        }

        return buf;
    }

    int EnviarAvisoDeQueEstoyViva(int socketCoordinador){
    	tEntradasUsadas * entradas = malloc(sizeof(tEntradasUsadas));
    	entradas->entradasUsadas = 1;
        if ((send(socketCoordinador, entradas, sizeof(tEntradasUsadas) , 0)) <= 0) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
        	free(entradas);
            exit(1);
        }

        free(entradas);

        return EXIT_SUCCESS;
    }

    OperaciontHeader *recibirHeader(int socketCoordinador){
    	OperaciontHeader *unHeader = malloc(sizeof(OperaciontHeader));
    	printf("Socket del coordinador: %d\n", socketCoordinador);
    	if ((recv(socketCoordinador, unHeader, sizeof(OperaciontHeader), 0)) <= 0){
    		puts("Fallo al recibir el header");
    		perror("recv");
    		exit(1);
    	}
    	return unHeader;
    }

    int enviarMensaje(int socketCoordinador, char* mensaje){
    	int longitud_mensaje, bytes_enviados;

        longitud_mensaje = strlen(mensaje);
        if ((bytes_enviados=send(socketCoordinador, mensaje, longitud_mensaje , 0)) <= 0) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",mensaje);
        return 1;
    }

	int enviarResultado(int socket,tResultadoInstancia * resul)
	{
		if ((send(socket, resul, sizeof(tResultadoInstancia), 0)) <= 0) {
        	puts("Error al enviar resultado al Coordinador");
        	perror("send");
            exit(1);
        }
		printf("Resultado es: %d \n",resul->resultado);//Envio bien el resultado
		printf("Resultado tamanioValor: %d \n",resul->tamanioValor);
    	puts("Se envió el resultado al Coordinador");
        return EXIT_SUCCESS;
	}

    operacionRecibida *recibirOperacion(int socketCoordinador, int tamanioValor){
    	operacionRecibida *operacion = malloc(sizeof(operacionRecibida));
    	operacion->valor = malloc(tamanioValor);
    	int numBytes;

    	if ((numBytes = recv(socketCoordinador, operacion, sizeof(operacionRecibida) + tamanioValor, 0)) <= 0 ){
    		perror("Recv");
    		exit(1);
    	}
    	return operacion;
    }


    void agregarEntrada(operacionRecibida *unaOperacion, char ** arrayEntradas, int cantidadEntradas, int tamanioValor, t_list *tablaEntradas, int tamanioValorRecibido){

    	int contadorEntradasGuardadas = 0;

    	char *valorRecibido = malloc(tamanioValorRecibido);
    	memcpy(valorRecibido, unaOperacion->valor, tamanioValorRecibido);


    	char *claveRecibida = calloc(41, 1);
    	strcpy(claveRecibida, unaOperacion->clave); // Copio toda la clave

    	if(tamanioValorRecibido > tamanioValor){ //Si el valor que recibi en el mensaje es mayor al valor establecido para cada entada
    		int entradasNecesarias = calcularEntradasNecesarias(tamanioValorRecibido, tamanioValor); //Calculo la cantidad de entradas que necesita el valor para guardarse
    		printf("Entradas necesarias para guardar valor: %d\n", entradasNecesarias);
			int offset;

			for(int i = 0; i < entradasNecesarias; i++){ //Itero tantas veces como entradas se necesitan para guarar el valor
				offset = i * tamanioValor; //Para guardar

				for(int j = 0; j < cantidadEntradas; j++){ //Recorro desde mi primer entrada

					if(*(arrayEntradas[j]) == NULL){ //Si una entrada no tiene valores, guardo allí
						int bytesRestantes = tamanioValorRecibido - tamanioValor * i;

						if (i == entradasNecesarias - 1){ //Si falta guardar el último pedazo del valor
							memcpy(arrayEntradas[j], valorRecibido + offset, bytesRestantes); //Copio los bytes que quedan guardar
							agregarNodoAtabla(tablaEntradas, j, bytesRestantes, claveRecibida);//Agrego un nodo por cada pedazo de valor guardado
							agregarNodoACola(colaParaLFU, j, bytesRestantes, claveRecibida);
							break; //Al hacer el break en el último pedazo de valor, deja de buscar entradas para guardar
						}
						else{ //Si no es el ultimo pedazo del valor, guardo y luego busco otra entrada para el proximo pedazo
							memcpy(arrayEntradas[j], valorRecibido + offset, tamanioValor);
							agregarNodoAtabla(tablaEntradas, j, tamanioValor, claveRecibida);
							contadorEntradasGuardadas++;
							break; //Sale a buscar la proxima entrada vacia para el proximo pedazo de valor
						}

					}
					if(j == cantidadEntradas - 1){// Si llego a la ultima entrada y no hay espacio utilizo un algoritmo de reemplazo
							int cantidadEntradasPendientes = entradasNecesarias - contadorEntradasGuardadas; //Las entradas que me faltan guardar (Las uso para borrar la misma cantidad de entradas)
							t_list * tablaAuxiliar;
							switch (algoritmoReemplazo){
								case 1:
									eliminarEntradasStorageCircular(arrayEntradas, cantidadEntradasPendientes); //Borro las entradas necesarias para guardar el resto del valor
									i = contadorEntradasGuardadas - 1; //Guardo a partir del ultimo pedazo gguardado en alguna entrada vacia
									break;
								case 2:
									eliminarEntradasStorageLFU(arrayEntradas, cantidadEntradasPendientes);
									break;
								case 3:
									tablaAuxiliar = obtenerTablaParaBSU(tablaEntradas, cantidadEntradasPendientes);//Obtengo una tabla con la cantidad de nodos igual a las entradas necesarias que faltan borrar para el nuevo valor
																												   // y ordenada por el tamanio almacenado en las entradas (De mayor a menor) HAY REEMPLAZAR LAS QUE MAS ESPACIO OCUPAN.
									eliminarEntradasStorageBSU(arrayEntradas, tablaAuxiliar); //Borro las entradas referenciadas a la tabla que obtuve antes.
									break;
							}
					}

				}

			}
			free(valorRecibido);
			free(unaOperacion);

    	}
    	else{  //Si el tamanio del valor recibido entra en una sola entrada
    		for(int i = 0; i < cantidadEntradas; i++){ //Recorro mis entradas desde 0 hasta entontrar una vacía para guardar el valor
    			if(*(arrayEntradas[i]) == NULL){
    				memcpy(arrayEntradas[i], valorRecibido, tamanioValorRecibido);
    				agregarNodoAtabla(tablaEntradas, i, tamanioValorRecibido, claveRecibida);
    				puts("Agregué el nodo a la tabla");
    				break;
    			}
    			if(i == cantidadEntradas - 1){ // Si llego a la ultima entrada y no hay espacio utilizo un algoritmo de reemplazo
    				t_list * tablaAuxiliar;
    				switch (algoritmoReemplazo){
    				case 1:
    					eliminarEntradasStorageCircular(arrayEntradas, 1);
    					i = -1; //Vuelvo a recorrer todas las entradas para guardar el valor
    					break;
    				case 2:
    					eliminarEntradasStorageLFU(arrayEntradas, 1);
    					break;
    				case 3:
    					tablaAuxiliar = obtenerTablaParaBSU(tablaEntradas, 1);//Obtengo una tabla con la cantidad de nodos igual a las entradas necesarias que faltan borrar para el nuevo valor
    																								   // y ordenada por el tamanio almacenado en las entradas (De mayor a menor) HAY REEMPLAZAR LAS QUE MAS ESPACIO OCUPAN.
    					eliminarEntradasStorageBSU(arrayEntradas, tablaAuxiliar); //Borro las entradas referenciadas a la tabla que obtuve antes.
    					break;
    				}

    			}
    		}
    		free(unaOperacion);
    		free(valorRecibido);
    	}


    	return;
    }


    int calcularEntradasNecesarias(int tamanioValorRecibido, int tamanioValor){
    	if ((tamanioValorRecibido%tamanioValor) == 0){
        	return tamanioValorRecibido / tamanioValor;
        }
        else{
        	return tamanioValorRecibido / tamanioValor + 1;
        }

        }

    int calcularLongitudMaxValorBuscado(char *unaClave,t_list *tablaEntradas){
     	int coincidir(tEntrada *unaEntrada){
     	    		return string_equals_ignore_case(unaEntrada->clave, unaClave);
     	    	}
     	return list_count_satisfying(tablaEntradas, (void*) coincidir);
     }

    void agregarNodoAtabla(t_list *tablaEntradas, int nroEntradaStorage, int tamanioAlmacenado, char *nombreClave){
     	tEntrada *buffer = malloc(sizeof(tEntrada));
     	buffer->numeroEntrada = nroEntradaStorage;
     	buffer->tamanioAlmacenado = tamanioAlmacenado;
     	strcpy(buffer->clave, nombreClave);


     	list_add(tablaEntradas, (tEntrada *) buffer);

    	return;
     }

    void agregarNodoACola(t_queue *colaParaLFU, int nroEntradaStorage, int tamanioAlmacenado,char *nombreClave){
    	tEntrada *buffer = malloc(sizeof(tEntrada)); //Creo el buffer que se agregará a la cola
    	buffer->numeroEntrada = nroEntradaStorage;
    	buffer->tamanioAlmacenado = tamanioAlmacenado;
    	strcpy(buffer->clave, nombreClave);

    	queue_push(colaParaLFU, (tEntrada *) buffer);

    	return;

    }

    void mostrarArray(char **arrayEntradas, int cantidadEntradas){
    	for (int i = 0; i < cantidadEntradas; i++){
    		if(*(arrayEntradas[i]) != NULL){
    			printf("Valor de la %d entrada: %s\n", i,arrayEntradas[i]);
    		}
    	}
    return;
    }

    char *obtenerValor(int longitudMaximaValorBuscado, t_list *tablaEntradas, char *claveBuscada,char **arrayEntradas,int tamanioValor){
    	char *valor = malloc(longitudMaximaValorBuscado * tamanioValor + 1);
    	int tamanioTotalValor = 0;
    	t_list *tablaDuplicada = malloc(sizeof(t_list));
    	tEntrada *bufferEntrada = malloc(sizeof(tEntrada));


    	tablaDuplicada = list_duplicate(tablaEntradas);

    	int coincidir(tEntrada *unaEntrada){
    		return string_equals_ignore_case(unaEntrada->clave, claveBuscada);
    	}

    	tablaDuplicada = list_filter(tablaDuplicada, (void*) coincidir);


    	for(int i = 0; i < tablaDuplicada->elements_count; i++){
    		bufferEntrada = list_get(tablaDuplicada, i);
    		puts("********DESPUES DEL GET***********");
    		int index = bufferEntrada->numeroEntrada;
    		puts("********DESPUES DEL INDEX***********");
    		//memcpy((valor + i * tamanioValor), arrayEntradas[index], tamanioValor);
    		memcpy((valor + tamanioTotalValor), arrayEntradas[index], bufferEntrada->tamanioAlmacenado);

    		tamanioTotalValor += bufferEntrada->tamanioAlmacenado;
    	}

    	valor[tamanioTotalValor] = '\0';
    	list_destroy(tablaDuplicada);
    	//free(bufferEntrada);
    	return valor;
    }

    t_list* list_duplicate(t_list* self) {
    	t_list* duplicated = list_create();
    	list_add_all(duplicated, self);
    	return duplicated;
    }

    bool validarClaveExistente(char *unaClave, t_list *tablaEntradas){

       	bool coincidir(tEntrada *unaEntrada){
       	    		return string_equals_ignore_case(unaEntrada->clave, unaClave);
       	    	}

       	if(tablaEntradas->head){

       		return list_any_satisfy(tablaEntradas, (void*) coincidir);;
       	}
       	else{
       		return false;
       	}

       }

       t_list *filtrarLista(char *unaClave, t_list *tabla){
       	t_list *tablaFiltrada = malloc(sizeof(t_list));
       	int coincidir(tEntrada *unaEntrada){
           		return string_equals_ignore_case(unaEntrada->clave, unaClave);
           	}
       	tablaFiltrada = list_filter(tabla, (void*) coincidir);
       	return tablaFiltrada;

       }

       void eliminarNodosyValores(char *nombreClave, t_list *tablaEntradas, char **arrayEntradas, int tamanioValor){
       	t_list *tablaDuplicada = malloc(sizeof(t_list));
       	t_list *tablaFiltrada = malloc(sizeof(t_list));
       	tEntrada *bufferEntrada;

       	tablaDuplicada = list_duplicate(tablaEntradas);
       	tablaFiltrada = filtrarLista(nombreClave, tablaDuplicada);


       	for(int i = 0; i < tablaFiltrada->elements_count; i++){
       	    		bufferEntrada = list_get(tablaFiltrada, i);
       	    		int index = bufferEntrada->numeroEntrada;
       	    		int bytes = bufferEntrada->tamanioAlmacenado;
       	    		memset(arrayEntradas[index], '\0', bytes);
       	        	eliminarNodos(nombreClave, tablaEntradas);
       	    	}



       	list_destroy(tablaDuplicada);
       	list_destroy(tablaFiltrada);
       	//free(bufferEntrada);

       	return;
       }
       void destruirNodoDeTabla(tEntrada *unaEntrada){
    	   //free(unaEntrada->clave);
    	   free(unaEntrada);
    	  return;
       }


       void eliminarNodos(char *nombreClave, t_list *tablaEntradas){

       	int coincidir(tEntrada *unaEntrada){
       	    	    		return string_equals_ignore_case(unaEntrada->clave, nombreClave);
       	    	    	}
       		list_remove_and_destroy_by_condition(tablaEntradas, (void*) coincidir,(void*) destruirNodoDeTabla);

       	return;
       }

       bool validarEntradaLibre(char **arrayEntradas, int cantidadEntradas){
    	   int buffer = 0;
    	   for (int i = 1; i <= cantidadEntradas; i++){
    		   if(*arrayEntradas[i] == NULL){
    			   break;
    		   }
			   buffer = i;
    	   }
    	   if(buffer < cantidadEntradas){
    		   return true;
    	   }
    	   else{
    		   return false;
    	   }
       }

       void eliminarEntradasStorageCircular(char **arrayEntradas, int cantidadEntradasABorrar){

    	   for (int i = 0; i < cantidadEntradasABorrar; i++){
    		   memset(arrayEntradas[posicionPunteroCirc], '\0', tamanioValor);
    		   //memset(punteroCirc + (tamanioValor * i), '\0', tamanioValor);
    		   eliminarNodoPorIndex(tablaEntradas, posicionPunteroCirc);
    		   posicionPunteroCirc++;
        	   if (posicionPunteroCirc >= cantidadEntradas){
        		   posicionPunteroCirc = 0;
        	   }
    	   }



    	   return;
       }

       void eliminarEntradasStorageLFU(char **arrayEntradas, int cantidadEntradasABorrar){
    	   tEntrada *buffer;

    	   for (int i = 0; i < cantidadEntradasABorrar; i++){
    		   buffer = queue_peek(colaParaLFU);
    		   memset(arrayEntradas[buffer->numeroEntrada], '\0', buffer->tamanioAlmacenado);
    		   queue_pop(colaParaLFU);
    	   }

    	   return;

       }

       void eliminarEntradasStorageBSU(char **arrayEntradas, t_list *entradasABorrar){ //Paso una tabla filtrada solo con las entradas que se tienen que borrar
    	   tEntrada *buffer;

    	   for (int i = 0; i < list_size(entradasABorrar);i++){
    		   buffer = list_get(entradasABorrar, i); //Tomo de a uno los nodos con el numero de entrada referenciado a borrar
    		   memset(arrayEntradas[buffer->numeroEntrada], '\0', buffer->tamanioAlmacenado); //Borro el valor de la entrada con el numero referenciado y el tamanio que tenía
    		   eliminarNodoPorIndex(tablaEntradas, buffer->numeroEntrada); //Elimino los nodos de la tabla original ya que no tendrán mas una entrada referenciada para el valor borrado
    	   }
    	   return;
       }

       int calcularEntradasABorrar(char **arrayEntradas, int entradasNecesarias){
    	   int contadorEntradasLibres = 0;

    	   for (int i = 0; i < cantidadEntradas; i++){
    		   if (*arrayEntradas[i] == NULL){
    			   contadorEntradasLibres++;
    		   }
    	   }

    	   if (contadorEntradasLibres >= entradasNecesarias){
    		   return 0;
    	   }
    	   else{
    		   return entradasNecesarias - contadorEntradasLibres;
    	   }
       }

       void eliminarNodoPorIndex(t_list *tablaEntradas, int index){
          	int coincidir(tEntrada *unaEntrada)
          	{
          	    	    		return unaEntrada->numeroEntrada == index;
			}
			list_remove_and_destroy_by_condition(tablaEntradas, (void*) coincidir,(void*) destruirNodoDeTabla);
          	return;
       }

       int Dump(char **arrayEntradas)
       {
    	   while(1)
    	   {
    		   sleep(Intervalo);
    		   puts("REALIZO DUMP");
    		   //pthread_mutex_lock(&mutex);
    		   guardarTodasMisClaves(tablaEntradas, arrayEntradas);
    		   //pthread_mutex_unlock(&mutex);
    		   //sem_post(semaforo);
    	   }
    	   return EXIT_SUCCESS;
       }


       void guardarUnArchivo(char *unaClave, char *valorArchivo){
    	   char *rutaAGuardar = malloc(159);
    	   strcpy(rutaAGuardar, PuntoMontaje);
    	   strcat(rutaAGuardar, unaClave);
    	   strcat(rutaAGuardar, ".txt");
    	  *(rutaAGuardar+158) = '\0';



    	   printf("Nombre de la clave a guardar: %s\n", unaClave);

    	   FILE *archivo = fopen(rutaAGuardar, "w");

    	   if (archivo == NULL){
    		   puts("Error al abrir el archivo");
    	   }
    	   fprintf(archivo, "%s", valorArchivo);
    	   fclose(archivo);
    	   free(rutaAGuardar);
    	   return;
       }

       void guardarTodasMisClaves(t_list *tablaEntradas, char **arrayEntradas){
    	   t_list *duplicada = malloc(sizeof(t_list));
    	   puts("declare tabla duplicada");
    	   duplicada = list_duplicate(tablaEntradas);
    	   puts("Cree tabla duplicada");
    	   char *bufferClave;
    	   puts("Declare buffer clave");
    	   tEntrada *bufferEntrada;
    	   puts("Declare buffer entrada");
    	   int index = 0;

    	   for (int i = 0; i < cantidadClavesEnTabla;i++){
        	   bufferEntrada = list_get(duplicada, index);
        	   puts("Hice el get de la lista");
        	   bufferClave = bufferEntrada->clave;
        	   puts("Apunte con buffer clave a buffer entrada");
        	   int longitud = calcularLongitudMaxValorBuscado(bufferClave, duplicada);
        	   puts("Calcule longitud");
        	   char *valorGet = obtenerValor(longitud, duplicada, bufferClave, arrayEntradas, tamanioValor);
        	   puts("Por entrar a guardar un archivo");
        	   guardarUnArchivo(bufferClave, valorGet);
    		   index += longitud;
    	   }

    	   return;

       }


       t_list *obtenerTablaParaBSU(t_list *tablaEntradas, int cantidadEntradasPendientes){
    	   t_list *duplicada = list_duplicate(tablaEntradas);
    	   ordenarTablaPorTamanioAlmacenado(duplicada);

    	   t_list *tablaABorrar = list_take(duplicada, cantidadEntradasPendientes);
    	   list_destroy(duplicada); //Ver si va destroy elements también
    	   return tablaABorrar;
       }

       void ordenarTablaPorTamanioAlmacenado(t_list *unaTabla){
    	   bool compare(tEntrada *unaEntrada, tEntrada * otraEntrada){
    		   return unaEntrada->tamanioAlmacenado >= otraEntrada->tamanioAlmacenado;
    	   }
    	   list_sort(unaTabla, compare);
    	   return;
       }

