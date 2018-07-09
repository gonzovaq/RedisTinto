#include "Instancia.h"

	pthread_mutex_t mutex;
    int main(int argc, char *argv[])
    {
    	puts("Iniciando");
    	//cantidadEntradas = 8;
    	//tamanioValor = 3;
    	tablaEntradas = list_create();
    	colaLRU = list_create();
    	int socketCoordinador;
        int longitudMaximaValorBuscado;
        int tamanioValorRecibido;


        verificarParametrosAlEjecutar(argc, argv);
        LeerArchivoDeConfiguracion(argv);


        he = gethostbyname(IP);
        socketCoordinador = conectarmeYPresentarme(PORTCO);


    	//DUMP
    	pthread_t tid;
    	int stat = pthread_create(&tid, NULL, (void*)Dump, (void*)arrayEntradas);
		if (stat != 0){
			puts("error al generar el hilo");
			perror("thread");
			//continue;
		}
		pthread_detach(tid); //Con esto decis que cuando el hilo termine libere sus recursos



        while(1){
        	//mostrarArray(arrayEntradas, cantidadEntradas);
        	puts("Informo que estoy viva 1");
        	EnviarAvisoDeQueEstoyViva(socketCoordinador);
        	puts("Recibo operacion");
        	tOperacionInstanciaStruct * operacion = malloc(sizeof(tOperacionInstanciaStruct));
        	if ((recv(socketCoordinador, operacion, sizeof(tOperacionInstanciaStruct), 0)) <= 0){
        	    		puts("Fallo al recibir el tipo de operacion");
        	    		perror("recv");
        	    		exit(1);
        	    	}
        	puts("Informo que estoy viva 2");
        	EnviarAvisoDeQueEstoyViva(socketCoordinador);

        	puts("Verifico que tipo de operacion me pidieron");

        	if(operacion->operacion == CUALQUIER_COSA){
        		puts("Me pidieron cualquier cosa");
        	}
        	else{
				if(operacion->operacion == SOLICITAR_VALOR){
					puts("Me pidieron un valor para STATUS");
					free(operacion);
					PedidoDeValores(socketCoordinador, arrayEntradas);
					puts("Terminamos el STATUS");
				}
				else{
					if(operacion->operacion == COMPACTAR){
						puts("Me pidieron que COMPACTE");
						compactar();
						tEntradasUsadas *tuVieja = malloc(sizeof(tEntradasUsadas));
						tuVieja->entradasUsadas = 999;

						if (send(socketCoordinador, tuVieja, sizeof(tEntradasUsadas), 0) <= 0){
							puts("Error al enviar solicitud de compactqacion");
							perror("Send");
							free(tuVieja);
							exit(1);
						}
						free(tuVieja);
					}
					else{
						free(operacion);
						OperaciontHeader *headerRecibido = malloc(sizeof(OperaciontHeader)); // El malloc esta en recibir header
						puts("Intento recibir header del Coordinador");
						headerRecibido = recibirHeader(socketCoordinador);
						puts("AVISO - Recibi el header!");
						if (headerRecibido->tipo != OPERACION_GET){
							tamanioValorRecibido = headerRecibido->tamanioValor;
							char *bufferClave;
							char *bufferValor;
							char *valorGet;
							operacionRecibida *operacion = malloc(sizeof(operacionRecibida));


							bufferClave = recibirMensaje(socketCoordinador, TAMANIO_CLAVE);


							if (headerRecibido->tipo == OPERACION_SET){
								//pthread_mutex_lock(&mutex);
								puts("*****************************************************");
								puts("ENTRO A UN SET");
								printf("INDEX PUNTERO: %d\n", indexInicialPunteroCircular);
								cantidadClavesEnTabla++;
								int indicePuntero = indexInicialPunteroCircular;
								if(validarClaveExistente(bufferClave, tablaEntradas) == true){
									if(esClaveAtomica(bufferClave)){
										eliminarNodos(bufferClave, colaLRU);
									}
									eliminarNodosyValores(bufferClave, tablaEntradas, arrayEntradas);
									cantidadClavesEnTabla--;
										}
								bufferValor = recibirMensaje(socketCoordinador, tamanioValorRecibido);
								tamanioValorRecibido = headerRecibido->tamanioValor;
								memcpy(operacion->clave, bufferClave, strlen(bufferClave) + 1);
								operacion->valor = bufferValor;
								indexInicialPunteroCircular = indicePuntero;
								if(validarEspacioDisponible(tamanioValorRecibido) == true){
									puts("AVISO - Hay espacio contiguo disponible, guardo valor");
									agregarEntrada(operacion, tamanioValorRecibido);
								}
								else{
									if(validarEspacioReal(tamanioValorRecibido) == true){
										puts("AVISO - Hay fragmentacion externa, por lo tanto compacto");
										tEntradasUsadas *tuVieja = malloc(sizeof(tEntradasUsadas));
										tuVieja->entradasUsadas = 500;

										if (send(socketCoordinador, tuVieja, sizeof(tEntradasUsadas), 0) <= 0){
												 puts("Error al enviar solicitud de compactqacion");
												 perror("Send");
												 free(tuVieja);
												 exit(1);
										 }
										free(tuVieja);
										cantidadClavesEnTabla--;
									}
									else{
										puts("AVISO - No hay fragmentacion, Hay que aplicar algoritmo de reemplazo");
										int entradasNecesarias = calcularEntradasNecesarias(tamanioValorRecibido, tamanioValor);
										int entradasABorrar = entradasNecesarias - calcularEntradasVacias();//Calcula cuantas entradas hacen falta borrar

										if(entradasABorrar > cantidadEntradasAtomicas()){
											//TODO - HAY QUE ABORTAR PORQUE NO HAY NI ENTRADAS ATOMICAS PARA BORRAR
											puts("NO HAY ENTRADAS ATOMICAS PARA REEMPLAZAR");
										}
										else{
											puts("por entrar a aplicar algoritmo");
										  	printf("Canttidad de claves atomicas: %d\n", list_size(colaLRU));
											aplicarAlgoritmoReemplazo(entradasABorrar);
											cantidadClavesEnTabla = cantidadClavesEnTabla - entradasABorrar;
										}

										if(validarEspacioDisponible(tamanioValorRecibido) == true){
											agregarEntrada(operacion, tamanioValorRecibido);
										}
										else{
											puts("AVISO - Hay fragmentacion externa luego de reemplazar entradas, solicito compactacion");
											tEntradasUsadas *tuVieja = malloc(sizeof(tEntradasUsadas));
											tuVieja->entradasUsadas = 500;

											if (send(socketCoordinador, tuVieja, sizeof(tEntradasUsadas), 0) <= 0){
												puts("Error al enviar solicitud de compactacion");
												perror("Send");
												free(tuVieja);
												exit(1);
											 }
											free(tuVieja);
											cantidadClavesEnTabla--;
										}
									}
								}


								enviarEntradasUsadas(socketCoordinador, tablaEntradas, bufferClave);
								free(headerRecibido);
								//free(operacion);
								free(bufferClave);
								free(bufferValor);
								puts("HICE UN SET");
								mostrarArray(arrayEntradas, cantidadEntradas);
								printf("Cantidad de elementos en mi tabla de entradas: %d\n", list_size(tablaEntradas));
								puts("*****************************************************");

								//sem_post(semaforo);
								//pthread_mutex_unlock(&mutex);
								}


							if (headerRecibido->tipo == OPERACION_STORE){
								//pthread_mutex_lock(&mutex);
								puts("*****************************************************");
								puts("ENTRO A UN STORE");
								tEntrada *bufferStore = malloc(sizeof(bufferStore));
								longitudMaximaValorBuscado = entradasUsadasPorClave(bufferClave, tablaEntradas);
								valorGet = obtenerValor(longitudMaximaValorBuscado, tablaEntradas, bufferClave, arrayEntradas, tamanioValor);
								guardarUnArchivo(bufferClave, valorGet);
								puts("Guarde un archivo");
								enviarEntradasUsadas(socketCoordinador, tablaEntradas, bufferClave);
								if (esClaveAtomica(bufferClave)){
									bufferStore = obtenerNodoPorClave(bufferClave);
									agregarNodoAtabla(colaLRU, bufferStore->numeroEntrada, bufferStore->tamanioAlmacenado, bufferClave);
									eliminarPrimerNodoEncontradoConClave(bufferClave, colaLRU);
								}


								free(valorGet);
								free(headerRecibido);
								free(bufferClave);
								puts("TERMINE UN STORE");
								puts("*****************************************************");
								//sem_post(semaforo);
								//pthread_mutex_unlock(&mutex);
							}
						}
						puts("Terminamos esta operacion");
					}
				}
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
							if (strcmp(Algoritmo, "BSU") == 0)
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

    int PedidoDeValores(int socketCoordinador, char **arrayEntradas)
    {
		char clave[TAMANIO_CLAVE];
    	if((recv(socketCoordinador, clave, TAMANIO_CLAVE, 0)) <= 0){
    		perror("Fallo al recibir la clave");
    	}

    	printf("STATUS: Recibi la clave %s, voy a buscar su valor \n", clave);

    	char* valor;
    	int tamanioValorBuscado;
    	tEntradasUsadas * tamanio = malloc(sizeof(tEntradasUsadas));

    	tamanioValorBuscado = entradasUsadasPorClave(clave, tablaEntradas);

    	valor = obtenerValor(tamanioValorBuscado, tablaEntradas, clave, arrayEntradas, tamanioValor);
    	printf("STATUS: El valor es: %s\n", valor);
    	tamanio->entradasUsadas = strlen(valor);

    	printf("STATUS: El tamanio de valor es: %d\n", tamanio->entradasUsadas);
 	    if (send(socketCoordinador, tamanio, sizeof(tEntradasUsadas), 0) <= 0){

 	    	puts("Error al enviar el el tamanio valor");
 		   perror("Send");
 	    }

	    if (send(socketCoordinador, valor, tamanio->entradasUsadas, 0) <= 0){
	     		   puts("Error al enviar el valor");
	     		   perror("Send");
	     	    }
	    free(tamanio);
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

         	 arrayEntradas = malloc(cantidadEntradas * sizeof(char*)); //Declaro e inicializo el storage para los valores (n entradas con m tamaño)

         	    	for (int i= 0; i < cantidadEntradas; i++){
         	    		arrayEntradas[i] = calloc(tamanioValor, sizeof(char));
         	    	}

             char * line = NULL;
             size_t len = 0;
             FILE * archivo;

             operacionRecibida *operacion = malloc(sizeof(operacionRecibida));

         	for(int i = 0; i < clavesPrevias; i++){
         		char bufferClave[TAMANIO_CLAVE];

             	if((recv(sockeCoordinador, bufferClave, TAMANIO_CLAVE, 0)) <= 0){
             		perror("Fallo al recibir la configuracion");
             	}
             	printf("Lo que recibí del coordinador: %s\n", bufferClave);
             	strcpy(operacion->clave, bufferClave);
             	printf("Recibi la clave %s \n",operacion->clave);

         		char nombreArchivo[strlen(PuntoMontaje)+strlen(operacion->clave)+5];
         		strcpy(nombreArchivo,PuntoMontaje);
         		strcat(nombreArchivo,operacion->clave);
         		strcat(nombreArchivo,".txt\0");
         		printf("El archivo a abrir esta en %s \n",nombreArchivo);
         		archivo = leerArchivo(&nombreArchivo);
         		puts("Voy a leer la linea del archivo");
             	if(getline(&line, &len, archivo) == -1){
             		puts("No habia nada dentro del archivo");
             	}else{
             		puts("Pruebo el operacion->valor");

             		operacion->valor = line;
             		int tamanio = strlen(operacion->valor);
             		printf("El valor de la clave %s es %s \n",operacion->clave,operacion->valor);
             		printf("Tamanio del valor: %d\n", tamanio);
             		if(tamanio > 0){
                 		if(tamanio > calcularEntradasVacias()){
                 			int entradasNecesarias = calcularEntradasNecesarias(tamanio, tamanioValor);
                 			int entradasABorrar = entradasNecesarias - calcularEntradasVacias();
                 			aplicarAlgoritmoReemplazo(entradasABorrar);
                 			cantidadClavesEnTabla = cantidadClavesEnTabla - entradasABorrar;
                     		agregarEntrada(operacion, tamanio);
                     		cantidadClavesEnTabla++;
    //						if(validarEspacioDisponible(tamanio) == true){
    //							agregarEntrada(operacion, tamanio);
    //						}
    //						else{
    //							puts("AVISO - Hay fragmentacion externa luego de reemplazar entradas, solicito compactacion");
    //							tEntradasUsadas *tuVieja = malloc(sizeof(tEntradasUsadas));
    //							tuVieja->entradasUsadas = 500;
    //
    //							if (send(sockeCoordinador, tuVieja, sizeof(tEntradasUsadas), 0) <= 0){
    //								puts("Error al enviar solicitud de compactacion");
    //								perror("Send");
    //								free(tuVieja);
    //								exit(1);
    //							 }
    //							free(tuVieja);
                 		}
                 		else{
                     		agregarEntrada(operacion, tamanio);
                     		cantidadClavesEnTabla++;
                 		}
             		}


             		//list_add(claves,operacion->clave);
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
            perror("No existe el archivo");
            file = fopen(archivo, "w");
            //exit(EXIT_FAILURE);
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
    	buffer->entradasUsadas = entradasUsadasPorClave(bufferClave, tablaEntradas);

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


    void agregarEntrada(operacionRecibida *unaOperacion, int tamanioValorRecibido){

    	int contadorEntradasGuardadas = 0;
    	int index = indexParaGuardarValores(tamanioValorRecibido);
    	int numeroEntradaAGuardar;

    	char *valorRecibido = malloc(tamanioValorRecibido);
    	memcpy(valorRecibido, unaOperacion->valor, tamanioValorRecibido);

    	char *claveRecibida = calloc(41, 1);
    	strcpy(claveRecibida, unaOperacion->clave); // Copio toda la clave

    	int entradasNecesarias = calcularEntradasNecesarias(tamanioValorRecibido, tamanioValor); //Calculo la cantidad de entradas que necesita el valor para guardarse

    	for(int i = 0; i < entradasNecesarias; i++){
    		numeroEntradaAGuardar = index + i;
    		int offset = i * tamanioValor;
    		int bytesRestantes = tamanioValorRecibido - tamanioValor * i;

//    		if(*(arrayEntradas[numeroEntradaAGuardar]) == NULL){
//        		memcpy(arrayEntradas[numeroEntradaAGuardar], valorRecibido + offset, bytesRestantes); //Copio los bytes que quedan guardar
//        		agregarNodoAtabla(tablaEntradas, numeroEntradaAGuardar, bytesRestantes, claveRecibida);
//    		}

    		if (i == entradasNecesarias - 1){ //Si falta guardar el último pedazo del valor
    			memcpy(arrayEntradas[numeroEntradaAGuardar], valorRecibido + offset, bytesRestantes); //Copio los bytes que quedan guardar
    			agregarNodoAtabla(tablaEntradas, numeroEntradaAGuardar, bytesRestantes, claveRecibida);//Agrego un nodo por cada pedazo de valor guardado

    		}
    		else{ //Si no es el ultimo pedazo del valor, guardo y luego busco otra entrada para el proximo pedazo
    			memcpy(arrayEntradas[numeroEntradaAGuardar], valorRecibido + offset, tamanioValor);
    			agregarNodoAtabla(tablaEntradas, numeroEntradaAGuardar, tamanioValor, claveRecibida);
    			contadorEntradasGuardadas++;

    		}
    		if(entradasNecesarias == 1){
    			agregarNodoAtabla(colaLRU, numeroEntradaAGuardar, bytesRestantes, claveRecibida);
    		}

    	}


//    	if(tamanioValorRecibido > tamanioValor){ //Si el valor que recibi en el mensaje es mayor al valor establecido para cada entada
//    		int entradasNecesarias = calcularEntradasNecesarias(tamanioValorRecibido, tamanioValor); //Calculo la cantidad de entradas que necesita el valor para guardarse
//    		printf("Entradas necesarias para guardar valor: %d\n", entradasNecesarias);
//    		int offset;
//
//
//    		for(int i = 0; i < entradasNecesarias; i++){ //Itero tantas veces como entradas se necesitan para guarar el valor
//    			offset = i * tamanioValor; //Para guardar
//    			for(int j = 0; j < cantidadEntradas; j++){ //Recorro desde mi primer entrada
//    				if(*(arrayEntradas[j]) == NULL){ //Si una entrada no tiene valores, guardo allí
//    					int bytesRestantes = tamanioValorRecibido - tamanioValor * i;
//    					if (i == entradasNecesarias - 1){ //Si falta guardar el último pedazo del valor
//    						memcpy(arrayEntradas[j], valorRecibido + offset, bytesRestantes); //Copio los bytes que quedan guardar
//							agregarNodoAtabla(tablaEntradas, j, bytesRestantes, claveRecibida);//Agrego un nodo por cada pedazo de valor guardado
//							break; //Al hacer el break en el último pedazo de valor, deja de buscar entradas para guardar
//						}
//						else{ //Si no es el ultimo pedazo del valor, guardo y luego busco otra entrada para el proximo pedazo
//							memcpy(arrayEntradas[j], valorRecibido + offset, tamanioValor);
//							agregarNodoAtabla(tablaEntradas, j, tamanioValor, claveRecibida);
//							contadorEntradasGuardadas++;
//							break; //Sale a buscar la proxima entrada vacia para el proximo pedazo de valor
//						}
//
//					}
//					if(j == cantidadEntradas - 1){// Si llego a la ultima entrada y no hay espacio utilizo un algoritmo de reemplazo
//							int cantidadEntradasPendientes = entradasNecesarias - contadorEntradasGuardadas; //Las entradas que me faltan guardar (Las uso para borrar la misma cantidad de entradas)
//							t_list * tablaAuxiliar;
//							switch (algoritmoReemplazo){
//								case 1:
//									eliminarEntradasStorageCircular(arrayEntradas, cantidadEntradasPendientes); //Borro las entradas necesarias para guardar el resto del valor
//									i = contadorEntradasGuardadas - 1; //Guardo a partir del ultimo pedazo gguardado en alguna entrada vacia
//									break;
//								case 2:
//									eliminarEntradasStorageLRU(arrayEntradas, cantidadEntradasPendientes);
//									i--;
//									break;
//								case 3:
//									tablaAuxiliar = obtenerTablaParaBSU(tablaEntradas, cantidadEntradasPendientes);//Obtengo una tabla con la cantidad de nodos igual a las entradas necesarias que faltan borrar para el nuevo valor
//																												   // y ordenada por el tamanio almacenado en las entradas (De mayor a menor) HAY REEMPLAZAR LAS QUE MAS ESPACIO OCUPAN.
//									eliminarEntradasStorageBSU(arrayEntradas, tablaAuxiliar); //Borro las entradas referenciadas a la tabla que obtuve antes.
//									break;
//							}
//					}
//
//				}
//
//			}
//			free(valorRecibido);
//			free(unaOperacion);
//
//    	}
//    	else{  //Si el tamanio del valor recibido entra en una sola entrada
//    		for(int i = 0; i < cantidadEntradas; i++){ //Recorro mis entradas desde 0 hasta entontrar una vacía para guardar el valor
//    			if(*(arrayEntradas[i]) == NULL){
//    				memcpy(arrayEntradas[i], valorRecibido, tamanioValorRecibido);
//    				agregarNodoAtabla(tablaEntradas, i, tamanioValorRecibido, claveRecibida);
//    				puts("Agregué el nodo a la tabla");
//    				break;
//    			}
//    			if(i == cantidadEntradas - 1){ // Si llego a la ultima entrada y no hay espacio utilizo un algoritmo de reemplazo
//    				t_list * tablaAuxiliar;
//    				switch (algoritmoReemplazo){
//    				case 1:
//    					eliminarEntradasStorageCircular(arrayEntradas, 1);
//    					i = -1; //Vuelvo a recorrer todas las entradas para guardar el valor
//    					break;
//    				case 2:
//    					eliminarEntradasStorageLRU(arrayEntradas, 1);
//    					break;
//    				case 3:
//    					tablaAuxiliar = obtenerTablaParaBSU(tablaEntradas, 1);//Obtengo una tabla con la cantidad de nodos igual a las entradas necesarias que faltan borrar para el nuevo valor
//    																								   // y ordenada por el tamanio almacenado en las entradas (De mayor a menor) HAY REEMPLAZAR LAS QUE MAS ESPACIO OCUPAN.
//    					eliminarEntradasStorageBSU(arrayEntradas, tablaAuxiliar); //Borro las entradas referenciadas a la tabla que obtuve antes.
//    					break;
//    				}
//
//    			}
//    		}
//    		free(unaOperacion);
//    		free(valorRecibido);
//    	}


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

    int entradasUsadasPorClave(char *unaClave,t_list *unaTabla){
     	int coincidir(tEntrada *unaEntrada){
     	    		return string_equals_ignore_case(unaEntrada->clave, unaClave);
     	    	}
     	return list_count_satisfying(unaTabla, (void*) coincidir);
     }

    void agregarNodoAtabla(t_list *unaTabla, int nroEntradaStorage, int tamanioAlmacenado, char *nombreClave){
     	tEntrada *buffer = malloc(sizeof(tEntrada));
     	buffer->numeroEntrada = nroEntradaStorage;
     	buffer->tamanioAlmacenado = tamanioAlmacenado;
     	strcpy(buffer->clave, nombreClave);


     	list_add(unaTabla, (tEntrada *) buffer);

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
    		int index = bufferEntrada->numeroEntrada;
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

    bool validarClaveExistente(char *unaClave, t_list *unaTabla){

       	bool coincidir(tEntrada *unaEntrada){
       	    		return string_equals_ignore_case(unaEntrada->clave, unaClave);
       	    	}

       	if(unaTabla->head){

       		return list_any_satisfy(unaTabla, (void*) coincidir);;
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

       void eliminarNodosyValores(char *nombreClave, t_list *unaTabla, char **arrayEntradas, int tamanioValor){
       	t_list *tablaDuplicada = malloc(sizeof(t_list));
       	t_list *tablaFiltrada = malloc(sizeof(t_list));
       	tEntrada *bufferEntrada;

       	tablaDuplicada = list_duplicate(unaTabla);
       	tablaFiltrada = filtrarLista(nombreClave, tablaDuplicada);


       	for(int i = 0; i < tablaFiltrada->elements_count; i++){
       	    		bufferEntrada = list_get(tablaFiltrada, i);
       	    		int index = bufferEntrada->numeroEntrada;
       	    		int bytes = bufferEntrada->tamanioAlmacenado;
       	    		memset(arrayEntradas[index], '\0', bytes);
       	        	eliminarNodos(nombreClave, unaTabla);
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


       void eliminarNodos(char *nombreClave, t_list *unaTabla){

       	int coincidir(tEntrada *unaEntrada){
       	    	    		return string_equals_ignore_case(unaEntrada->clave, nombreClave);
       	    	    	}
       		list_remove_and_destroy_by_condition(unaTabla, (void*) coincidir,(void*) destruirNodoDeTabla);

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
    		   printf("CONTADOR DE ENTRADAS A BORRAR: %d\n", i);
    		   for (int j = indexInicialPunteroCircular; j < cantidadEntradas; j++){
    			   printf("Numero de entrada a evaluar: %d", j);
    			   printf("Indicide de un puntero: %d\n", indexInicialPunteroCircular);
    			   if (validarEntradaAtomica(j) == true){
    				   indexInicialPunteroCircular = j;
    				   //punteroCircular = arrayEntradas[indexInicialPunteroCircular];
    				   break;
    			   }
    			   else{
    				   if (j == cantidadEntradas - 1){
    					   j = -1;//CUIDADO CON LOOP INFINITO SI NO EXISTE NINGUNA ENTRADA ATOMICA
    				   }
    			   }
    		   }
    		   memset(arrayEntradas[indexInicialPunteroCircular], '\0', tamanioValor);
    		   //memset(punteroCirc + (tamanioValor * i), '\0', tamanioValor);
    		   eliminarNodoPorIndex(tablaEntradas, indexInicialPunteroCircular);
    		   indexInicialPunteroCircular++;
    		   if(indexInicialPunteroCircular == cantidadEntradas){
    			   indexInicialPunteroCircular = 0;
    		   }
    	   }

    	   return;
       }

       void eliminarEntradasStorageLRU(char **arrayEntradas, int cantidadEntradasABorrar){
    	   tEntrada *buffer;
    	   char claveGuardada[TAMANIO_CLAVE];
    	   int nroParaBorrarElNodoDesactualizadoDecolaLRU;

    	   printf("Canttidad de claves atomicas: %d\n", list_size(colaLRU));

    	   for (int i = 0; i < cantidadEntradasABorrar; i++){
    		   buffer = list_get(colaLRU, 0);
    		   puts("despues de hacer el get");
    		   //nroParaBorrarElNodoDesactualizadoDecolaLRU = buffer->numeroEntrada;
    		   puts("despues de la iguakldad rara");
    		   buffer = obtenerNodoPorClave(buffer->clave);
    		   puts("Antes de hacer el memset");
    		   memset(arrayEntradas[buffer->numeroEntrada], '\0', buffer->tamanioAlmacenado);
    		   puts("Despues de hacer el memset");
    		   eliminarNodoPorIndex(tablaEntradas, buffer->numeroEntrada);
    		   list_remove_and_destroy_element(colaLRU, 0, destruirNodoDeTabla);
    		   //eliminarNodoPorIndex(colaLRU, nroParaBorrarElNodoDesactualizadoDecolaLRU);
    	   }

    	   return;

       }

       void eliminarEntradasStorageBSU(char **arrayEntradas, int entradasABorrar){ //Paso una tabla filtrada solo con las entradas que se tienen que borrar
    	   tEntrada *buffer;
    	   t_list *duplicada;
    	   t_list *filtrada;
    	   int cantidadEntradasEmpatadas;
    	   t_list *tablaDeEmpate;
    	   int contadorEntradasBorradas = 0;

    	   //Si las entradas que tienen el mismo tamanio > entradasABorrar

    	   duplicada= list_duplicate(tablaEntradas);
    	   filtrada = list_filter(duplicada, (void*) esClaveAtomica);
    	   ordenarTablaPorTamanioAlmacenado(filtrada);

    	   puts("DESPUES DE DECLARAR TODO");

//    	   for (int i = 0; i < entradasABorrar;i++){
//    		   buffer = list_get(entradasABorrar, i); //Tomo de a uno los nodos con el numero de entrada referenciado a borrar
//    		   if(validarSiHayEmpate(filtrada, buffer->tamanioAlmacenado) == true){
//    			   cantidadEntradasEmpatadas = calcularEntradasEmpatadas(filtrada, buffer->tamanioAlmacenado);
//    			   tablaDeEmpate = list_take(filtrada, cantidadEntradasEmpatadas);
//    			   ordenarTablaPorNroEntrada(tablaDeEmpate);
//    			   for(int j = 0; j < cantidadEntradasEmpatadas; j++){
//    				   buffer = list_get(tablaDeEmpate, j);
//    				   for(int h = indexInicialPunteroCircular; h < cantidadEntradas; h++){
//    					   if(buffer->numeroEntrada == indexInicialPunteroCircular){
//
//    					   }
//    				   }
//
//    			   }
//    		   }
//    		   //Hacer el desempate
//    		   memset(arrayEntradas[buffer->numeroEntrada], '\0', buffer->tamanioAlmacenado); //Borro el valor de la entrada con el numero referenciado y el tamanio que tenía
//    		   eliminarNodoPorIndex(tablaEntradas, buffer->numeroEntrada); //Elimino los nodos de la tabla original ya que no tendrán mas una entrada referenciada para el valor borrado
//    	   }

    	   for (int i = 0; i < entradasABorrar;i++){
    		   buffer = list_get(filtrada, i); //Tomo de a uno los nodos con el numero de entrada referenciado a borrar
    		   if(validarSiHayEmpate(filtrada, buffer->tamanioAlmacenado) == true){
    			   cantidadEntradasEmpatadas = calcularEntradasEmpatadas(filtrada, buffer->tamanioAlmacenado);
    			   tablaDeEmpate = list_take(filtrada, cantidadEntradasEmpatadas);
    			   ordenarTablaPorNroEntrada(tablaDeEmpate);
    			   for(int j = 0; j < cantidadEntradas; j++){
    				   tEntrada *entrada = list_find(tablaDeEmpate, (void*) esEntradaAReemplazar);
    				   if(entrada != NULL){
    					   indexInicialPunteroCircular++;
    					   if(indexInicialPunteroCircular == cantidadEntradas){
    						   indexInicialPunteroCircular = 0;
    					   }
    					   break;
    				   }
    				   else{
    					   indexInicialPunteroCircular++;
    					   if(indexInicialPunteroCircular == cantidadEntradas){
    						   indexInicialPunteroCircular = 0;
    					   }
    				   }
    			   }
    		   }
    		   //Hacer el desempate
    		   memset(arrayEntradas[indexInicialPunteroCircular - 1], '\0', tamanioValor); //Borro el valor de la entrada con el numero referenciado y el tamanio que tenía
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

       void eliminarNodoPorIndex(t_list *unaTabla, int index){
          	int coincidir(tEntrada *unaEntrada)
          	{
          	    	    		return unaEntrada->numeroEntrada == index;
			}
			list_remove_and_destroy_by_condition(unaTabla, (void*) coincidir,(void*) destruirNodoDeTabla);
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

       void guardarTodasMisClaves(t_list *unaTabla, char **arrayEntradas){
    	   t_list *duplicada = malloc(sizeof(t_list));
    	   duplicada = list_duplicate(unaTabla);
    	   char *bufferClave;
    	   tEntrada *bufferEntrada;
    	   int index = 0;
    	   printf("Claves en mi tabla: %d\n",cantidadClavesEnTabla);

    	   for (int i = 0; i < cantidadClavesEnTabla;i++){
        	   bufferEntrada = list_get(duplicada, index);
        	   bufferClave = bufferEntrada->clave;
        	   int longitud = entradasUsadasPorClave(bufferClave, duplicada);
        	   char *valorGet = obtenerValor(longitud, duplicada, bufferClave, arrayEntradas, tamanioValor);
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

       bool identificarFragmentacion(){


		   for(int i = 0; i < cantidadEntradas; i++){
			   if(arrayEntradas[i] == NULL){ //Entra cuando encuentra la primer entrada libre
				   for(int j = i+1; j < cantidadEntradas; j++){ //Recorro desde la entrada siguiente
					   if(arrayEntradas[j] != NULL){ //Entra si encuentra un valor
						   for(int h = j+1; h < cantidadEntradas; h++){
							   if(arrayEntradas[h] == NULL){ //Entra si encuentra otra entrada libre luego de encontrar el valor (Habría fragmentación externa)
								   return true;
							   }
						   }
					   }
				   }
			   }
		   }



    	   return false;
       }

       void compactar(){

    	   t_list *nuevaTabla;
    	   nuevaTabla = list_create();
    	   char *burbuja;
    	   tEntrada *nuevo;




    	   for(int i = 0; i < cantidadEntradas - 1; i++){
    		   if(*(arrayEntradas[i]) != NULL){
				   nuevo = obtenerNodoPorIndexstorage(tablaEntradas, i);
				   printf("El nodo que tiene el numero de entrada: %d QUEDA IGUAL\n", nuevo->numeroEntrada);
				   agregarNodoAtabla(nuevaTabla, i, nuevo->tamanioAlmacenado, nuevo->clave);
			   }
    		   for(int j = i + 1; j < cantidadEntradas; j++){
    			   if(*(arrayEntradas[i]) == NULL && *(arrayEntradas[j]) != NULL){
    				   nuevo = obtenerNodoPorIndexstorage(tablaEntradas, j);
    				   printf("El nodo tiene la clave: %s y su numero de entrada viejo es: %d\n", nuevo->clave, nuevo->numeroEntrada);
    				   nuevo->numeroEntrada = i;
    				   printf("Su nuevo numero de entrada es: %d\n", nuevo->numeroEntrada);
    				   agregarNodoAtabla(nuevaTabla, i, nuevo->tamanioAlmacenado, nuevo->clave);
    				   burbuja = arrayEntradas[j];
    				   arrayEntradas[j] = arrayEntradas[i];
    				   arrayEntradas[i] = burbuja;
    				   break;
    			   }
    		   }

    	   }

    	   list_clean_and_destroy_elements(tablaEntradas, (void *)destruirNodoDeTabla);
    	   list_add_all(tablaEntradas, nuevaTabla);
    	   //list_destroy(nuevaTabla);
    	   return;

       }
       tEntrada *obtenerNodoPorIndexstorage(t_list *unaTabla, int index){
          	tEntrada *nodo;
    	   int coincidir(tEntrada *unaEntrada)
          	{
          	    	    		return unaEntrada->numeroEntrada == index;
			}
			nodo = list_find(unaTabla, (void*) coincidir);
          	return nodo;
       }

       tEntrada *obtenerNodoPorClave(char *unaClave){
    	   tEntrada *nodo;
    	   int coincidir(tEntrada *unaEntrada){
    		   return string_equals_ignore_case(unaEntrada->clave, unaClave);
    	   }

    	   nodo = list_find(tablaEntradas, (void*) coincidir);
    	   return nodo;
       }

       int calcularEntradasContiguas(int entradasNecesarias){
    	   int contador = 0;

    	   for(int i = entradaProxima; i < cantidadEntradas; i++){
    		   if(*(arrayEntradas[i]) == NULL){
    			   contador++;
    		   }
    		   else{
    			   contador = 0;
    		   }
    		   if(contador >= entradasNecesarias){
    			   return contador;
    		   }
    	   }

    	   return 0;
       }

       bool validarEspacioDisponible(int tamanioValorRecibido){
    	   int entradasNecesarias = calcularEntradasNecesarias(tamanioValorRecibido, tamanioValor);
    	   int contador = 0;
    	   puts("Ya calcule entradas necesarias");
    	   for(int i = 0; i < cantidadEntradas; i++){
    		   if(*(arrayEntradas[i]) == NULL){
    			   puts("Valide entrada vacia");
    			   contador++;
    		   }
    		   else{
    			   contador = 0;
    		   }
    		   if(contador >= entradasNecesarias){
    			   puts("SI HAY ESPACIO DISPONIBLE DEVUELVO TRUE");
    			   return true;
    		   }
    	   }


    	   return false;
       }

       int indexParaGuardarValores(int tamanioValorRecibido){
    	   int entradasNecesarias = calcularEntradasNecesarias(tamanioValorRecibido, tamanioValor);
    	   int contador = 0;

    	   for(int i = 0; i < cantidadEntradas; i++){
    		   if(*(arrayEntradas[i]) == NULL){
    			   contador++;
    		   }
    		   else{
    			   contador = 0;
    		   }
    		   if(contador >= entradasNecesarias){
    			   return i - entradasNecesarias + 1;
    		   }
    	   }
    	   return 9999999;
       }

       bool validarEspacioReal(int tamanioValorRecibido){
    	   int entradasNecesarias = calcularEntradasNecesarias(tamanioValorRecibido, tamanioValor);
    	   int contador = 0;

    	   for(int i = 0; i < cantidadEntradas; i++){
    		   if(*(arrayEntradas[i]) == NULL){
    			   contador++;
    		   }
    	   }

    	   if (contador >= entradasNecesarias){
    		   return true;
    	   }
    	   else{
    		   return false;
    	   }

       }

       int calcularEntradasUsadasEnStorage(){
    	   int contador = 0;

    	   for(int i = 0; i < cantidadEntradas; i++){
    		   if(*(arrayEntradas[i]) != NULL){
    			   contador++;
    		   }
    	   }
    	   return contador;
       }

       int calcularEntradasVacias(){
    	   return cantidadEntradas - calcularEntradasUsadasEnStorage();
       }

       void aplicarAlgoritmoReemplazo(int entradasABorrar){


    	   switch (algoritmoReemplazo){
    	   case 1:
    		   eliminarEntradasStorageCircular(arrayEntradas, entradasABorrar);
    		   break;
    	   case 2:
    		   eliminarEntradasStorageLRU(arrayEntradas, entradasABorrar);
    		   break;
    	   case 3:
    		   eliminarEntradasStorageBSU(arrayEntradas, entradasABorrar);
    		   break;
    	   }
       }

       void ordenarTablaPorNroEntrada(t_list *unaTabla){

       			bool compare(tEntrada *nodo1,tEntrada *nodo2)
       			{
       				return nodo1->numeroEntrada <= nodo2->numeroEntrada;
       			}
       			list_sort(unaTabla,(void *)compare);

       		}


       bool esClaveAtomica(char *unaClave){
    	   int coincidir(tEntrada *unaEntrada){
    		   return string_equals_ignore_case(unaEntrada->clave, unaClave);
    	   }

    	   return list_count_satisfying(tablaEntradas, (void*) coincidir) == 1;
       }



       validarEntradaAtomica(int nroEntrada){
    	   tEntrada *buffer;

    	   buffer = obtenerNodoPorIndexstorage(tablaEntradas, nroEntrada);

    	   return esClaveAtomica(buffer->clave);
       }

       int cantidadEntradasAtomicas(){
    	   t_list *duplicada;
    	   t_list *filtrada;

    	   duplicada= list_duplicate(tablaEntradas);
    	   filtrada = list_filter(duplicada, (void*) esClaveAtomica);

    	   return list_size(filtrada);

       }

       void eliminarPrimerNodoEncontradoConClave(char *unaClave, t_list *unaTabla){
    	   tEntrada *buffer;

    	   int coincidir(tEntrada *unaEntrada){
    		   return string_equals_ignore_case(unaEntrada->clave, unaClave);
    	   }

    	   buffer = list_find(unaTabla, (void*) coincidir);

    	   list_remove_and_destroy_by_condition(unaTabla, (void*) coincidir, (void*) destruirNodoDeTabla);


       }

       bool validarSiHayEmpate(t_list *listaOrdenada, int tamanio){


    	   int coincidir(tEntrada *unaEntrada){
    		   return unaEntrada->tamanioAlmacenado == tamanio;
    	   }

    	   return list_any_satisfy(listaOrdenada, (void *) coincidir);
       }

       int calcularEntradasEmpatadas(t_list *lista, int unTamanio){

    	   int coincidir(tEntrada *unaEntrada){
    		   return unaEntrada->tamanioAlmacenado == unTamanio;
    	   }

    	   return list_count_satisfying(lista, (void *) coincidir);

       }

       bool validarExistenciaNodoPorIndex(t_list *tabla, int index){
   	   int coincidir(tEntrada *unaEntrada)
         	{
         	    	    		return unaEntrada->numeroEntrada == index;
			}
         	return list_find(tabla, (void*) coincidir);;
       }

       bool esEntradaAReemplazar(tEntrada *unaEntrada){
    	   return unaEntrada->numeroEntrada == indexInicialPunteroCircular;
       }
