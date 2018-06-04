#include "Instancia.h"

    struct hostent *he;
    struct sockaddr_in their_addr; // información de la dirección de destino

    int cantidadEntradas = 0;
    int tamanioValor = 0;
    int posicionPunteroCirc = 0;
    t_list *tablaEntradas;
    int algoritmoReemplazo = 1;
    int cantidadClavesEnTabla = 0;

    int main(int argc, char *argv[])
    {

    	cantidadEntradas = 8;
    	tamanioValor = 3;
    	tablaEntradas = list_create();


    	char **arrayEntradas = malloc(cantidadEntradas * sizeof(char*));

    	for (int i= 0; i < cantidadEntradas; i++){
    		arrayEntradas[i] = calloc(tamanioValor, sizeof(char));
    	}
//    	char *punteroCIRC = malloc(sizeof(char*));
//     	punteroCIRC = arrayEntradas[posicionPunteroCirc];

    	int socketCoordinador;
        int longitudMaximaValorBuscado;
        int tamanioValorRecibido;


        verificarParametrosAlEjecutar(argc, argv);
        leerConfiguracion();

        socketCoordinador = conectarmeYPresentarme(PORT);


        while(1){
        	OperaciontHeader *headerRecibido = malloc(sizeof(OperaciontHeader)); // El malloc esta en recibir header
        	puts("Recibiendo header");
        	headerRecibido = recibirHeader(socketCoordinador);
        	puts("pase el ultimo recibir headr");
        	tamanioValorRecibido = headerRecibido->tamanioValor;
        	char *bufferClave;
        	char *bufferValor;
        	char *valorGet;
        	operacionRecibida *operacion = malloc(sizeof(operacionRecibida));


        	bufferClave = recibirMensaje(socketCoordinador, TAMANIO_CLAVE);



        	if (headerRecibido->tipo == OPERACION_SET){

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
        		free(headerRecibido);
        		//free(operacion);
        		free(bufferClave);
        		free(bufferValor);
            	mostrarArray(arrayEntradas, cantidadEntradas);
             	printf("Cantidad de elementos en mi tabla de entradas: %d\n", list_size(tablaEntradas));
             	printf("LO QUE ME MUSTRA EL PUNTERO CIRCULAR %s\n", arrayEntradas[posicionPunteroCirc]);
             	printf("POSICION PUNTERO CIRCULAR: %d\n", posicionPunteroCirc);
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
        		puts("Entrooo en STORE");
        		longitudMaximaValorBuscado = calcularLongitudMaxValorBuscado(bufferClave, tablaEntradas);
        		valorGet = obtenerValor(longitudMaximaValorBuscado, tablaEntradas, bufferClave, arrayEntradas, tamanioValor);
        		guardarUnArchivo(bufferClave, valorGet);

        		free(valorGet);
        		free(headerRecibido);
        		free(bufferClave);
        	}
        	guardarTodasMisClaves(tablaEntradas, arrayEntradas);

        	}





        close(socketCoordinador);
        return 0;
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

    int leerConfiguracion(){
    	char *token;
    	char *search = "=";
    	static const char filename[] = "/home/utnso/workspace2/tp-2018-1c-Sistemas-Operactivos/Instancia/configuracion.config";
    	FILE *file = fopen ( filename, "r" );
    	if ( file != NULL )
    	{
    		puts("Leyendo archivo de configuracion");
    	    char line [ 128 ]; /* or other suitable maximum line size */
    	    while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */
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

    	return socketCoordinador;

    	}

    int enviarHeader(int socketCoordinador){
    	int pid = getpid(); //Los procesos podrian pasarle sus PID al coordinador para que los tenga identificados
    	printf("Mi ID es %d \n",pid);
        tHeader *header = malloc(sizeof(tHeader));
               header->tipoProceso = INSTANCIA;
               header->tipoMensaje = CONECTARSE;
               header->idProceso = pid;
               strcpy(header->nombreProceso, "INSTANCIA DE PRUEBA" );  // El nombre se da en el archivo de configuracion
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
    	OperaciontHeader *header = malloc(tamanioValorGet);

    	header->tipo = GET;
    	header->tamanioValor = tamanioValorGet;

    	if(send(socketCoordinador, header, sizeof(OperaciontHeader), 0) <= 0){
    		puts("Error al enviar header de operacion");
    		perror("Send");
    		free(header);
    	}

    	if(send(socketCoordinador, valorGet, tamanioValorGet, 0) <= 0){
    		puts("Error al enviar el valor buscado");
    		perror("Send");
    	}

    	puts("MANDE TODO BIEN");
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

    OperaciontHeader *recibirHeader(int socketCoordinador){
    	int bytesRecibidos;
    	OperaciontHeader *unHeader = malloc(sizeof(OperaciontHeader));

    	if ((bytesRecibidos = recv(socketCoordinador, unHeader, sizeof(OperaciontHeader), 0)) <= 0){
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

    	char *valorRecibido = malloc(tamanioValorRecibido);
    	memcpy(valorRecibido, unaOperacion->valor, tamanioValorRecibido);


    	char *claveRecibida = calloc(41, 1);
    	strcpy(claveRecibida, unaOperacion->clave); // Copio toda la clave
//
//    	if(validarClaveExistente(claveRecibida, tablaEntradas) == true){
//    		eliminarNodosyValores(claveRecibida, tablaEntradas, arrayEntradas);
//    	    	}


    	if(tamanioValorRecibido > tamanioValor){ //Si el valor que recibi en el mensaje es mayor al valor establecido para cada entada
    		int entradasNecesarias = calcularEntradasNecesarias(tamanioValorRecibido, tamanioValor); //Calculo la cantidad de entradas que necesita el valor para guardarse
    		printf("Entradas necesarias para guardar valor: %d\n", entradasNecesarias);
			int offset;

			for(int i = 0; i < entradasNecesarias; i++){ //Itero tantas veces como entradas se necesitan para guarar el valor
				offset = i * tamanioValor; //Para guardar

				for(int j = 0; j < cantidadEntradas; j++){ //Recorro desde mi primer entrada
					int contadorEntradasGuardadas = 0;
					if(*(arrayEntradas[j]) == NULL){ //Si una entrada no tiene valores, guardo allí

						int bytesRestantes = tamanioValorRecibido - tamanioValor * i;

						if (i == entradasNecesarias - 1){ //Si falta guardar el último pedazo del valor
							memcpy(arrayEntradas[j], valorRecibido + offset, bytesRestantes); //Copio los bytes que quedan guardar
							agregarNodoAtabla(tablaEntradas, j, bytesRestantes, claveRecibida); //Agrego un nodo por cada pedazo de valor guardado
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
							switch (algoritmoReemplazo){
								case 1:
									eliminarEntradasStorageCircular(arrayEntradas, cantidadEntradasPendientes); //Borro las entradas necesarias para guardar el resto del valor
									i = contadorEntradasGuardadas - 1; //Vuelvo una itearcación atrás para guardar los pedazos de valor que faltan en las entradas borradas
									break;
								case 2:
									//AlgoritmoLFU
									break;
								case 3:
									//El otro algoritmo que no me acuerdo el nombre
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
						break;
	    				}
	    				if(i == cantidadEntradas - 1){ // Si llego a la ultima entrada y no hay espacio utilizo un algoritmo de reemplazo
	    					switch (algoritmoReemplazo){
	    						case 1:
	    							puts("Aca me esta haciendo los memset");
	    							eliminarEntradasStorageCircular(arrayEntradas, 1);
	    							i = -1; //Vuelvo a recorrer todas las entradas para guardar el valor
	    							break;
	    						case 2:
	    							//AlgoritmoLFU
	    							break;
	    						case 3:
	    							//El otro algoritmo que no me acuerdo el nombre
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

     	printf("CLAVE GUARDADA EN AGREGAR NODO A TABLA: %s\n", buffer->clave);
     	printf("TAMANIO ALMACENADO GUARDADO EN AGREGAR NODO A TABLA: %d\n", buffer->tamanioAlmacenado);
     	printf("NUMERO ENTRADA GUARDADA EN AGREGAR NODO A TABLA: %d\n", buffer->numeroEntrada);

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

    	puts("Despues de declarar todo");

    	tablaDuplicada = list_duplicate(tablaEntradas);

    	puts("Despues de crear tabla dupliacda");
    	int coincidir(tEntrada *unaEntrada){
    		return string_equals_ignore_case(unaEntrada->clave, claveBuscada);
    	}

    	tablaDuplicada = list_filter(tablaDuplicada, (void*) coincidir);

    	puts("despues de filtrar la tabla");

    	for(int i = 0; i < tablaDuplicada->elements_count; i++){
    		puts("Antes de hacer el GET");
    		bufferEntrada = list_get(tablaDuplicada, i);
    		puts("Antes del index");
    		int index = bufferEntrada->numeroEntrada;
    		//memcpy((valor + i * tamanioValor), arrayEntradas[index], tamanioValor);
    		puts("Antes del memcpy");
    		memcpy((valor + tamanioTotalValor), arrayEntradas[index], bufferEntrada->tamanioAlmacenado);
    		puts("Antes del total valor");
    		tamanioTotalValor += bufferEntrada->tamanioAlmacenado;
    	}
    	puts("Antes de agregar el terminador null");
    	valor[tamanioTotalValor] = '\0';
    	puts("Antes de destruir todo");
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

    	   int desplazamientoPunteroCirc = 0;
    	   for (int i = 0; i < cantidadEntradasABorrar; i++){
    		   puts("Antes memset");
    		   memset(arrayEntradas[posicionPunteroCirc], '\0', tamanioValor);
    		   //memset(punteroCirc + (tamanioValor * i), '\0', tamanioValor);
    		   puts("Despues memset");
    		   eliminarNodoPorIndex(tablaEntradas, posicionPunteroCirc);
    		   posicionPunteroCirc++;
        	   if (posicionPunteroCirc >= cantidadEntradas){
        		   posicionPunteroCirc = 0;
        	   }
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
          	int coincidir(tEntrada *unaEntrada){
          	    	    		return unaEntrada->numeroEntrada == index;
          	    	    	}
          		list_remove_and_destroy_by_condition(tablaEntradas, (void*) coincidir,(void*) destruirNodoDeTabla);

          	return;
          }

       void guardarUnArchivo(char *unaClave, char *valorArchivo){
    	   char rutaAGuardar[150] = "/home/utnso/tp-2018-1c-Sistemas-Operactivos/Instancia/\0";
    	   puts("Antes de hacer strcat");
    	   strcat(rutaAGuardar, unaClave);
    	   strcat(rutaAGuardar, ".txt");



    	   printf("Nombre de la clave a guardar: %s\n", unaClave);

    	   puts("Despues del concat");
    	   FILE *archivo = fopen(rutaAGuardar, "w");
    	   if (archivo == NULL){
    		   puts("Error al abrir el archivo");
    	   }
    	   fprintf(archivo, "%s", valorArchivo);
    	   fclose(archivo);
    	   return;
       }

       void guardarTodasMisClaves(t_list *tablaEntradas, char **arrayEntradas){
    	   t_list *duplicada = malloc(sizeof(t_list));
    	   duplicada = list_duplicate(tablaEntradas);
    	   char *bufferClave;
    	   tEntrada *bufferEntrada;
    	   int index = 0;






    	   for (int i = 0; i < cantidadClavesEnTabla;i++){
        	   bufferEntrada = list_get(duplicada, index);
        	   bufferClave = bufferEntrada->clave;
        	   int longitud = calcularLongitudMaxValorBuscado(bufferClave, duplicada);
    		   puts("Antes del seg fault");
        	   char *valorGet = obtenerValor(longitud, duplicada, bufferClave, arrayEntradas, tamanioValor);
    		   puts("Despues");
        	   guardarUnArchivo(bufferClave, valorGet);
    		   puts("Guarde un archivo");
    		   index += longitud;
    	   }
    	   return;

       }

