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
        char *buffer_mensaje_recibido;
        char *mensaje_coordinador = "Hola Coordinador, como va?";
        int longitudMaximaValorBuscado;


        verificarParametrosAlEjecutar(argc, argv);
        leerConfiguracion();

        socketCoordinador = conectarmeYPresentarme(PORT);
    	buffer_mensaje_recibido = recibirMensaje(socketCoordinador);
        printf("Received: %s \n",buffer_mensaje_recibido);
        free(buffer_mensaje_recibido);
        enviarMensaje(socketCoordinador, mensaje_coordinador);
        puts("Antes del while");

        while(1){
        	OperaciontHeader *headerRecibido;
        	headerRecibido = recibirHeader(socketCoordinador);
        	char *bufferClave;
        	char *valorGet;

        	puts("despues de recibir header");
        	int bytesValor = 0;
        	operacionRecibida *operacion;

        	printf("Tamanio en el header: %d\n", headerRecibido->tamanioValor);

        	if (headerRecibido->tipo == SET){
        		puts("Entra al if del while");
        		int bytesValor = headerRecibido->tamanioValor;
        		operacion = recibirOperacion(socketCoordinador, bytesValor);
        		agregarEntrada(operacion, arrayEntradas, cantidadEntradas, tamanioValor, tablaEntradas);
        	}

        	if (headerRecibido->tipo == GET){
        		bufferClave = recibirMensaje(socketCoordinador);
        		longitudMaximaValorBuscado = calcularLongitudMaxValorBuscado(bufferClave,tablaEntradas);
        		valorGet = obtenerValor(longitudMaximaValorBuscado, tablaEntradas, bufferClave,arrayEntradas, tamanioValor);
        		enviarValorGet(socketCoordinador, valorGet);
        		puts("Muere aca");
        		free(valorGet);

        	}

        	free(headerRecibido);
        	free(operacion);
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
    	if ((socketCoordinador = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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
			   if (send(socketCoordinador, header, sizeof(tHeader), 0) == -1){
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

    	if(send(socketCoordinador, header, sizeof(OperaciontHeader), 0) == -1){
    		puts("Error al enviar header de operacion");
    		perror("Send");
    		free(header);
    	}

    	if(send(socketCoordinador, valorGet, tamanioValorGet, 0) == -1){
    		puts("Error al enviar el valor buscado");
    		perror("Send");
    	}



    	return 1;
    }


    char *recibirMensaje(int socketCoordinador){
    	int numbytes;
        int tamanio_buffer=41;
        char* buf = malloc(tamanio_buffer) ; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(socketCoordinador, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        *(buf + numbytes) = '\0';
        return buf;
    }

    OperaciontHeader *recibirHeader(int socketCoordinador){
    	int bytesRecibidos;
    	OperaciontHeader *unHeader = malloc(sizeof(OperaciontHeader));

    	puts("Entre al recibir Header");

    	if ((bytesRecibidos = recv(socketCoordinador, unHeader, sizeof(OperaciontHeader), 0)) == -1 ){
    		perror("recv");
    		exit(1);
    	}

    	printf("ASDASDASD: %d \n", unHeader->tamanioValor);
    	return unHeader;
    }

    int enviarMensaje(int socketCoordinador, char* mensaje){
    	int longitud_mensaje, bytes_enviados;

        longitud_mensaje = strlen(mensaje);
        if ((bytes_enviados=send(socketCoordinador, mensaje, longitud_mensaje , 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",mensaje);
        return 1;
    }

    operacionRecibida *recibirOperacion(int socketCoordinador, int bytesValor){
    	operacionRecibida *operacion = malloc(sizeof(operacionRecibida) + bytesValor);
    	int numBytes;

    	if ((numBytes = recv(socketCoordinador, operacion, sizeof(operacionRecibida) + bytesValor, 0)) == -1 ){
    		perror("Recv");
    		exit(1);
    	}
    	return operacion;
    }


    void agregarEntrada(operacionRecibida *unaOperacion, char ** arrayEntradas, int cantidadEntradas, int tamanioValor, t_list *tablaEntradas){

    	int tamanioValorRecibido = strlen(unaOperacion->valor); // Sin contar el \0
    	char *valorRecibido = malloc(strlen(unaOperacion->valor));
    	memcpy(valorRecibido, unaOperacion->valor, strlen(unaOperacion->valor)); // Copio el valor sin el \0

    	char claveRecibida = malloc(40+1);
    	strcpy(claveRecibida, unaOperacion->clave); // Copio toda la clave

    	if(validarClaveExistente(claveRecibida, tablaEntradas) == true){
    	    		eliminarNodosyValores(claveRecibida, tablaEntradas, arrayEntradas);
    	    	}

    	if(tamanioValorRecibido > tamanioValor){
    		int vecesAIterar = calcularIteracion(tamanioValorRecibido, tamanioValor);
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
			free(unaOperacion);
			free(valorRecibido);
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

    int calcularIteracion(int valorSobrePasado, int tamanioValor){
        	return valorSobrePasado / tamanioValor + 1;
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
     	free(nombreClave);

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
    	printf("longitud lita duplicada: %d\n", tablaDuplicada->elements_count);

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

       	for(int i = 0; i < tablaDuplicada->elements_count; i++){
       	    		bufferEntrada = list_get(tablaDuplicada, i);
       	    		int index = bufferEntrada->numeroEntrada;
       	    		int bytes = bufferEntrada->tamanioAlmacenado;
       	    		memset(arrayEntradas[index], '\0', bytes);
       	    	}

       	list_destroy(tablaDuplicada);
       	list_destroy(tablaFiltrada);
       	free(bufferEntrada);
       	eliminarNodos(nombreClave, tablaEntradas);

       	return;
       }
       void destruirNodoDeTabla(tEntrada *unaEntrada){
       	free(unaEntrada);
       	return;
       }


       void eliminarNodos(char *nombreClave, t_list *tablaEntradas){

       	bool coincidir(tEntrada *unaEntrada){
       	    	    		return string_equals_ignore_case(unaEntrada->clave, nombreClave);
       	    	    	}
       	list_remove_and_destroy_by_condition(tablaEntradas, (void*) coincidir,(void*) destruirNodoDeTabla);
       	return;
       }

