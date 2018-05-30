#include "Instancia.h"

    struct hostent *he;
    struct sockaddr_in their_addr; // información de la dirección de destino

    int main(int argc, char *argv[])
    {

    	int cantidadEntradas = 8;
    	int tamanioValor = 3;
    	t_list *tablaEntradas = list_create();

    	char **arrayEntradas = malloc(cantidadEntradas * sizeof(char*));

    	for (int i= 0; i < cantidadEntradas; i++){
    		arrayEntradas[i] = calloc(tamanioValor, sizeof(char));
    	}

    	int socketCoordinador;
        int longitudMaximaValorBuscado;
        int tamanioValorRecibido;


        verificarParametrosAlEjecutar(argc, argv);
        leerConfiguracion();

        socketCoordinador = conectarmeYPresentarme(PORT);


        while(1){
        	OperaciontHeader *headerRecibido;
        	headerRecibido = recibirHeader(socketCoordinador);
        	tamanioValorRecibido = headerRecibido->tamanioValor;
        	char *bufferClave;
        	char *bufferValor;
        	char *valorGet;
        	operacionRecibida *operacion = malloc(sizeof(operacionRecibida));


        	bufferClave = recibirMensaje(socketCoordinador, TAMANIO_CLAVE);

        	tamanioValorRecibido = headerRecibido->tamanioValor;

        	if (headerRecibido->tipo == OPERACION_SET){
        		bufferValor = recibirMensaje(socketCoordinador, tamanioValorRecibido);
        		memcpy(operacion->clave, bufferClave, strlen(bufferClave) + 1);
        		operacion->valor = bufferValor;
        		agregarEntrada(operacion, arrayEntradas, cantidadEntradas, tamanioValor, tablaEntradas, tamanioValorRecibido);
        		free(headerRecibido);
        		//free(operacion);
        		free(bufferClave);
        		free(bufferValor);
        	}

        	if (headerRecibido->tipo == OPERACION_GET){
        		longitudMaximaValorBuscado = calcularLongitudMaxValorBuscado(bufferClave,tablaEntradas);
        		valorGet = obtenerValor(longitudMaximaValorBuscado, tablaEntradas, bufferClave,arrayEntradas, tamanioValor);
        		printf("A ver que pija sdale: %s\n", valorGet);
        		enviarValorGet(socketCoordinador, valorGet);
        		free(valorGet);
        		free(headerRecibido);
        		//free(operacion);
        		free(bufferClave);
        	}

        	mostrarArray(arrayEntradas, cantidadEntradas);
         	printf("Cantidad de elementos en mi tabla de entradas: %d\n", list_size(tablaEntradas));

        }
    	printf("Tabla de entradas: %d\n", tablaEntradas->elements_count);

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
    	int tamanioValorGet = strlen(valorGet) + 1;
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

    	if(validarClaveExistente(claveRecibida, tablaEntradas) == true){
    	    puts("Entcontro clave repetyida");
    		eliminarNodosyValores(claveRecibida, tablaEntradas, arrayEntradas);
    	    	}

    	if(tamanioValorRecibido > tamanioValor){
    		int vecesAIterar = calcularIteracion(tamanioValorRecibido, tamanioValor);
    		printf("Veces  a iterar %d\n", vecesAIterar);
			int offset;

			for(int i = 0; i < vecesAIterar; i++){
				offset = i * tamanioValor;

				for(int j = 0; j < cantidadEntradas; j++){

					if(*(arrayEntradas[j]) == NULL){

						int bytesRestantes = tamanioValorRecibido - tamanioValor * i;

						if (i == vecesAIterar - 1){
							memcpy(arrayEntradas[j], valorRecibido + offset, bytesRestantes);
							agregarNodoAtabla(tablaEntradas, j, bytesRestantes, claveRecibida);
							break;
						}
						else{
							memcpy(arrayEntradas[j], valorRecibido + offset, tamanioValor);
							agregarNodoAtabla(tablaEntradas, j, tamanioValor, claveRecibida);
							break;
						}

					}

				}

			}

			free(valorRecibido);
			free(unaOperacion);
			return;
    	}

    	else{
    			for(int i = 0; i < cantidadEntradas; i++){
    				if(*(arrayEntradas[i]) == NULL){
    					memcpy(arrayEntradas[i], valorRecibido, tamanioValorRecibido);
    					break;
    				}
    			}
    			free(unaOperacion);
    			free(valorRecibido);
    		}

   	    return;
 }

    int calcularIteracion(int tamanioValorRecibido, int tamanioValor){
        if (tamanioValorRecibido % tamanioValor == 0){
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

    void mostrarArray(char **arrayEntradas, int cantidadEntradas){
    	for (int i = 0; i < cantidadEntradas; i++){
    		if(*(arrayEntradas[i]) != NULL){
    			printf("Valor de la primer entrada: %s\n", arrayEntradas[i]);
    		}
    	}
    return;
    }

    char *obtenerValor(int longitudMaximaValorBuscado, t_list *tablaEntradas, char *claveBuscada,char **arrayEntradas,int tamanioValor){
    	char *valor = malloc(longitudMaximaValorBuscado * tamanioValor + 1);
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
    		memcpy((valor + i * tamanioValor), arrayEntradas[index], tamanioValor);
    	}
    	valor[longitudMaximaValorBuscado * tamanioValor] = '\0';

    	list_destroy(tablaDuplicada);
    	free(bufferEntrada);
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
       	tEntrada *bufferEntrada = malloc(sizeof(tEntrada));

       	tablaDuplicada = list_duplicate(tablaEntradas);
       	tablaFiltrada = filtrarLista(nombreClave, tablaDuplicada);

       	printf("Elementos en la tabla filtrada %d\n", list_size(tablaFiltrada));


       	for(int i = 0; i < tablaFiltrada->elements_count; i++){
       	    		bufferEntrada = list_get(tablaFiltrada, i);
       	    		int index = bufferEntrada->numeroEntrada;
       	    		int bytes = bufferEntrada->tamanioAlmacenado;
       	    		memset(arrayEntradas[index], '\0', bytes);
       	    		printf("Itere %d en el for de tabla filtrada\n", i);

       	    	}

       	eliminarNodos(nombreClave, tablaEntradas);
       	list_destroy(tablaDuplicada);
       	list_destroy(tablaFiltrada);
       	free(bufferEntrada);

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

