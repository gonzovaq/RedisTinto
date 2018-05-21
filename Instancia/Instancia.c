#include "Instancia.h"

    struct hostent *he;
    struct sockaddr_in their_addr; // información de la dirección de destino

    int main(int argc, char *argv[])
    {


        //**************************************************

    	int cantidadEntradas = 8;
    	int tamanioValor = 3;
    	t_list *tablaEntradas = list_create();
    	tablaEntradas->head = malloc(sizeof(t_link_element) * cantidadEntradas);

    	char **arrayEntradas = malloc(cantidadEntradas * sizeof(char*));

    	for (int i= 0; i < cantidadEntradas; i++){
    		arrayEntradas[i] = calloc(tamanioValor, sizeof(char));
    	}

    	int socketCoordinador;
        char *buffer_mensaje_recibido;
        char *mensaje_coordinador = "Hola Coordinador, como va?";

        tMensaje *mensajeSet = malloc(sizeof(tMensaje));
        strcpy(mensajeSet->instruccion.clave, "Probando claves\0");
        strcpy(mensajeSet->instruccion.valor, "bbbb\0");

        tMensaje *mensajeGet = malloc(sizeof(tMensaje));
        strcpy(mensajeGet->instruccion.clave, "Probando claves\0");
        strcpy(mensajeGet->instruccion.valor, "\0");
        int longitudMaximaValorBuscado;
        char *valorGet = calloc(longitudMaximaValorBuscado, 1);


        verificarParametrosAlEjecutar(argc, argv);
        leerConfiguracion();

        socketCoordinador = conectarmeYPresentarme(PORT);
    	buffer_mensaje_recibido = recibirMensaje(socketCoordinador);
        printf("Received: %s \n",buffer_mensaje_recibido);
        free(buffer_mensaje_recibido);
        enviarMensaje(socketCoordinador, mensaje_coordinador);

        //Pruebo el set
		agregarEntrada(mensajeSet, arrayEntradas, cantidadEntradas, tamanioValor, tablaEntradas);
		mostrarArray(arrayEntradas, cantidadEntradas);

		//Pruebo el get
		longitudMaximaValorBuscado = calcularLongitudMaxValorBuscado(mensajeGet->instruccion.clave,tablaEntradas);
		printf("El valor de la clave ocupa %d entradas en mi tabla \n", longitudMaximaValorBuscado);
		valorGet = obtenerValor(longitudMaximaValorBuscado, tablaEntradas, mensajeGet->instruccion.clave, arrayEntradas, tamanioValor);
		printf("Valor obtenido del GET: %s\n",valorGet);

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

    char *recibirMensaje(int socketCoordinador){
    	int numbytes;
        int tamanio_buffer=100;
        char* buf = malloc(tamanio_buffer) ; //Seteo el maximo del buffer en 100 para probar. Debe ser variable.

        if ((numbytes=recv(socketCoordinador, buf, tamanio_buffer-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        *(buf + numbytes) = '\0';
        return buf;
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

    tMensaje *recibirInstruccion(int socketCoordinador){
    	tMensaje *mensajeRecibido = malloc(sizeof(tMensaje));
    	int numBytes;

    	if ((numBytes = recv(socketCoordinador, mensajeRecibido, sizeof(tMensaje), 0)) == -1 ){
    		perror("Recv");
    		exit(1);
    	}
    	return mensajeRecibido;
    }

    void procesarInstruccion(tMensaje *unMensaje, char **arrayEntradas, int cantidadEntradas, int tamanioValor, t_list *tablaEntradas){
    	int *longitudMaximaValorBuscado = malloc(sizeof(int));
    	if (unMensaje->encabezado.tipoProceso != COORDINADOR){
    		puts("El mensaje no se recibio desde el tipo de proceso indicado");
    	}
    	else{
        	switch(unMensaje->encabezado.tipoMensaje){
        	case SET:
        		agregarEntrada(unMensaje, arrayEntradas, cantidadEntradas, tamanioValor, tablaEntradas);
        		break;
        	case GET:
        		longitudMaximaValorBuscado = calcularLongitudMaxValorBuscado(unMensaje->instruccion.clave,tablaEntradas);
        		printf("El valor de la clave ocupa %d entradas", longitudMaximaValorBuscado);
        		break;
        	default:
        		puts("Nada para hacer");
        	}

    	}

    	return;
    }


    void agregarEntrada(tMensaje *unMensaje, char ** arrayEntradas, int cantidadEntradas, int tamanioValor, t_list *tablaEntradas){
    	char *valorMensaje = calloc(40, 1);
    	memcpy(valorMensaje, unMensaje->instruccion.valor, strlen(unMensaje->instruccion.valor));
    	int tamanioBuffer = strlen(valorMensaje);
    	char *nombreClave = malloc(40);
    	strcpy(nombreClave, unMensaje->instruccion.clave);

    	if(tamanioBuffer > tamanioValor){
    		int vecesAIterar = calcularIteracion(tamanioBuffer, tamanioValor);
			int offset;

			for(int i = 0; i < vecesAIterar; i++){
				offset = i * tamanioValor;

				for(int j = 0; j < cantidadEntradas; j++){

					if(*(arrayEntradas[j]) == NULL){

						int bytesRestantes = tamanioBuffer - tamanioValor * i;

						if (i == vecesAIterar - 1){
							memcpy(arrayEntradas[j], valorMensaje + offset, bytesRestantes);
							agregarNodoAtabla(tablaEntradas, j, bytesRestantes, nombreClave);
							break;
						}
						else{
							memcpy(arrayEntradas[j], valorMensaje + offset, tamanioValor);
							agregarNodoAtabla(tablaEntradas, j, tamanioValor, nombreClave);
							break;
						}

					}

				}

			}
			free(unMensaje);
			free(valorMensaje);
			return;
    	}

    	else{
    			for(int i = 0; i < cantidadEntradas; i++){
    				if(*(arrayEntradas[i]) == NULL){
    					memcpy(arrayEntradas[i], valorMensaje, strlen(valorMensaje));
    					break;
    				}
    			}
    			free(unMensaje);
    			free(valorMensaje);
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
    	char *valor = malloc(longitudMaximaValorBuscado);
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

    	list_destroy(tablaDuplicada);
    	free(bufferEntrada);
    	return valor;
    }

    t_list* list_duplicate(t_list* self) {
    	t_list* duplicated = list_create();
    	list_add_all(duplicated, self);
    	return duplicated;
    }

