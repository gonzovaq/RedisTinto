	#include "ESI.h"

    int main(int argc, char *argv[])
    {
        int socket_coordinador, socket_planificador;
        FILE * archivoConScripts;

        //Before
    	verificarParametrosAlEjecutar(argc, argv);
    	leerConfiguracion();
    	he = gethostbyname(IPCO);
    	archivoConScripts = leerArchivo(argv);


    	socket_coordinador = conectarmeYPresentarme(PORT_COORDINADOR);
	he = gethostbyname(IPPL);
    	socket_planificador = conectarmeYPresentarme(PORT_PLANIFICADOR);


    	manejarArchivoConScripts(archivoConScripts,socket_coordinador,socket_planificador);

        close(socket_coordinador);
        close(socket_planificador);
        return 0;
    }


	int  leerConfiguracion() {

			// Leer archivo de configuracion con las commons
			t_config* configuracion;

			configuracion = config_create(ARCHIVO_CONFIGURACION);

			PORT_COORDINADOR= config_get_int_value(configuracion, "PORTCO");
			PORT_PLANIFICADOR = config_get_int_value(configuracion, "PORTPL");

			IPPL = config_get_string_value(configuracion, "IPPL");

			IPCO =config_get_string_value(configuracion, "IPCO");

			return 1;

		}

    int verificarParametrosAlEjecutar(int argc, char *argv[]){

        if (argc != 2) {//argc es la cantidad de parametros que recibe el main.
        	puts("Error al ejecutar, para correr este proceso deberias ejecutar: ./ESI \"nombreArchivo\"");
            exit(1);
        }


     /*   if ((he=gethostbyname(argv[1])) == NULL) {  // obtener información de máquina
        	puts("Error al obtener el hostname, te faltan parametros.");
        	perror("gethostbyname");
            exit(1);
        }*/
        return EXIT_SUCCESS;
    }

    int conectarmeYPresentarme(int port){
        int sockfd;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        	puts("Error al crear el socket");
        	perror("socket");
            exit(1);
        }
        puts("El socket se creo correctamente\n");

        their_addr.sin_family = AF_INET;    // Ordenación de bytes de la máquina
        their_addr.sin_port = htons(port);  // short, Ordenación de bytes de la red
        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
        memset(&(their_addr.sin_zero),'\0', 8);  // poner a cero el resto de la estructura

        if (connect(sockfd, (struct sockaddr *)&their_addr,
                                              sizeof(struct sockaddr)) == -1) {
        	puts("Error al conectarme al servidor.");
        	perror("connect");
            exit(1);
        }
        puts("ESI conectado!\n");

        enviarHeader(sockfd); //Me presento

        return sockfd;

    }

    int enviarHeader(int sockfd){
    	int pid = getpid(); //Los procesos podrian pasarle sus PID al coordinador para que los tenga identificados
    	printf("Mi ID es %d \n",pid);
        tHeader *header = malloc(sizeof(tHeader));
               header->tipoProceso = ESI;
               header->tipoMensaje = CONECTARSE;
               header->idProceso = pid;
               strcpy(header->nombreProceso, "ESI DE PRUEBA" ); // El nombre se da en el archivo de configuracion --> MINOMBRE
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

    int recibirMensaje(int sockfd){
    	int numbytes;
        int buf;

        if ((numbytes=recv(sockfd, buf, sizeof(int), 0)) == -1) {
            perror("recv");
            exit(1);
        }
        return buf;
    }

    int recibirResultadoDelCoordinador(int sockfd, tResultado * resultado){
    	int numbytes;


        if ((numbytes=recv(sockfd, resultado, sizeof(tResultado), 0)) == -1) {
            perror("recv");
            exit(1);
        }
        puts("Recibi el resultado");
        return EXIT_SUCCESS;
    }

    int enviarOperaciontHeader(int sockfd, OperaciontHeader* header){

    	if ((send(sockfd, header, sizeof(OperaciontHeader), 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        puts("El header de la operacion se envio correctamente");
        return EXIT_SUCCESS;
    }

    int enviarMensaje(int sockfd, char* mensaje){

    	printf("Se envió el mensaje: %s \n",mensaje);

    	if ((send(sockfd, mensaje, TAMANIO_CLAVE, 0)) == -1) {
        	puts("Error al enviar el mensaje.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",mensaje);
        return EXIT_SUCCESS;
    }

    FILE *leerArchivo(char **argv){

    	//Esto lo copié literal del ejemplo del ParSI. A medida que avancemos, esto en vez de imprimir en pantalla,
    	//va a tener que enviarle los scripts al coordinador (siempre y cuando el planificador me lo indique)

    	FILE * file;
        file = fopen(argv[1], "r");
        if (file == NULL){
            perror("Error al abrir el archivo: ");
            exit(EXIT_FAILURE);
        }


        return file;
    }

    int manejarArchivoConScripts(FILE * file, int socket_coordinador, int socket_planificador){
        char * line = NULL;
        char * copiaLine = NULL;
        size_t len = 0;
        ssize_t read;

        OperacionAEnviar * operacion;
        OperaciontHeader * header;
        tValidezOperacion * validez = malloc(sizeof(tValidezOperacion));
        int tamanioValor;

        while(1){
        printf("volverALeer es %d \n",volverALeer);
        if(volverALeer == 1){
        	puts("Debo volver a leer la linea");
        	volverALeer=0;
        }
        else{
        	puts("Leo una linea nueva");
        	if(getline(&line, &len, file) == -1){
        		tResultado * resultado = malloc(sizeof(tResultado));
        		resultado->tipoResultado=CHAU;

        		enviarResultado(socket_planificador,resultado);
                free(resultado);
                fclose(file);
                if (line){
                    free(line);
                }
                //free(validez);
                puts("Chau!");
                return EXIT_SUCCESS;
        	}
        	copiaLine = malloc(strlen(line) + 1);
        	strcpy(copiaLine,line);
        }
            while (!recibirOrdenDeEjecucion(socket_planificador));

                t_esi_operacion parsed = parse(copiaLine);

				tResultado * resultado = malloc(sizeof(tResultado));
                if(parsed.valido){
                    switch(parsed.keyword){
                        case GET:
                        	validez=OPERACION_VALIDA;
                        	enviarValidez(socket_coordinador,&validez);
							puts("Manejo operacion GET");
							manejarOperacionGET(socket_coordinador, parsed.argumentos.GET.clave, &operacion, &header);
							puts("Se finalizo el manejo de la operacion GET");

							recibirResultado(socket_coordinador, resultado);
							resultado->tipoOperacion=OPERACION_GET;
							enviarResultado(socket_planificador,resultado);
                        	break;
                        case SET:
                        	validez=OPERACION_VALIDA;
                        	enviarValidez(socket_coordinador,&validez);
                        	puts("Manejo operacion SET");
							manejarOperacionSET(socket_coordinador, parsed.argumentos.SET.clave, parsed.argumentos.SET.valor, &operacion, &header);
							puts("Se finalizo el manejo de la operacion SET");

							recibirResultado(socket_coordinador, resultado);
							resultado->tipoOperacion=OPERACION_SET;
							enviarResultado(socket_planificador,resultado);
                        	break;
                        case STORE:
                        	validez=OPERACION_VALIDA;
                        	enviarValidez(socket_coordinador,&validez);
                        	puts("Manejo operacion STORE");
							manejarOperacionSTORE(socket_coordinador, parsed.argumentos.STORE.clave, &operacion, &header);
							puts("Se finalizo el manejo de la operacion STORE");

							recibirResultado(socket_coordinador, resultado);
							resultado->tipoOperacion=OPERACION_STORE;
							enviarResultado(socket_planificador,resultado);
                        	break;
                        default:
                        	validez = OPERACION_INVALIDA;
                            fprintf(stderr, "No pude interpretar <%s>\n", line);
                            enviarValidez(socket_coordinador, &validez);
                            exit(EXIT_FAILURE);
                    }

					/*
            		tResultado * resultado = malloc(sizeof(tResultado));
                    recibirResultado(socket_coordinador, resultado);
					enviarResultado(socket_planificador,resultado);
                    free(resultado);*/

					//TODO: REVISAR ESTO QUE SI SE DESCOMENTA NO FUNCIONA EL VOLVER A LEER
					//free(resultado);
                    //destruir_operacion(parsed);
                } else {
                	validez = OPERACION_INVALIDA;
                    fprintf(stderr, "La linea <%s> no es valida\n", line);
                    enviarValidez(socket_coordinador, &validez);
                    exit(EXIT_FAILURE);
                }
				
        }

		tResultado * resultado = malloc(sizeof(tResultado));
		resultado->tipoResultado=CHAU;
		enviarResultado(socket_planificador,resultado);
        free(resultado);
        fclose(file);
        if (line){
            free(line);
        }
        free(validez);
        puts("Chau!");
        return EXIT_SUCCESS;

    }

   int recibirOrdenDeEjecucion(int socket_planificador){
    	puts("Esperando orden de ejecución del Planificador ...");
    	char *buffer = malloc(strlen("EJECUTATE"));
    	int tengoPermiso = 0;
    	if(recv(socket_planificador,buffer,sizeof(buffer),0) <= 0){
    		perror("error al recibir orden de ejecucion");
    		exit(EXIT_FAILURE);
    	}
    	tengoPermiso = strcmp("EJECUTATE",buffer);
    	free(buffer);
    	return tengoPermiso;
    }

    int recibirResultado(int socket_coordinador, tResultado * resultado){
    	puts("Esperando que el Coordinador me envie el resultado");
    	recibirResultadoDelCoordinador(socket_coordinador,resultado);

    	switch(resultado->tipoResultado){
    		case OK:
    			puts("La operación salio OK"); //EN ESTOS CASE DEBERIA LOGGEAR O ALGO ASI, PREGUNTAR MAS ADELANTE
    			break;
    		case BLOQUEO:
    			volverALeer = 1;
				printf("La operación se BLOQUEO -- volverALeer es %d \n",volverALeer);
				break;
    		case ERROR:
				puts("La operación tiro ERROR");
				break;
    		default:
    			puts("ERROR AL RECIBIR EL RESULTADO");
    			EXIT_FAILURE;
    			break;
    	}
    	return EXIT_SUCCESS;
    }

	int manejarOperacionGET(int socket_coordinador, char clave[TAMANIO_CLAVE], OperacionAEnviar* operacion, OperaciontHeader * header) {

		header->tipo = OPERACION_GET;
		header->tamanioValor = 0;
		enviarOperaciontHeader(socket_coordinador, header);
		puts("Header de la Operacion GET enviado correctamente");

		clave[TAMANIO_CLAVE]='\0';
		printf("Voy a enviar la clave: %s \n",clave);
		enviarMensaje(socket_coordinador, clave);

		printf("Operacion GET con la clave: %s, enviada correctamente\n" ,clave);

		return EXIT_SUCCESS;
	}

	int manejarOperacionSET(int socket_coordinador, char clave[TAMANIO_CLAVE], char *valor, OperacionAEnviar* operacion, OperaciontHeader *header) {

		header->tipo = OPERACION_SET;
		header->tamanioValor = strlen(valor);

		enviarOperaciontHeader(socket_coordinador, header);
		puts("Header de la Operacion SET enviado correctamente");

		enviarMensaje(socket_coordinador, clave);
		enviarValor(socket_coordinador, valor);

		printf("Operacion SET con clave: %s, y valor %s, enviada correctamente\n" ,clave,valor);
		return EXIT_SUCCESS;
	}

	int manejarOperacionSTORE(int socket_coordinador, char clave[TAMANIO_CLAVE], OperacionAEnviar* operacion, OperaciontHeader *header) {

		header->tipo = OPERACION_STORE;
		header->tamanioValor = 0;
		enviarOperaciontHeader(socket_coordinador, header);
		puts("Header de la Operacion STORE enviado correctamente");

		enviarMensaje(socket_coordinador, clave);

		printf("Operacion STORE con la clave: %s, enviada correctamente\n" ,clave);

		return EXIT_SUCCESS;

	}

    int enviarValor(int sockfd, char* valor){

    	printf("Se envió el valor: %s \n",valor);

    	if ((send(sockfd, valor, strlen(valor), 0)) <= 0) {
        	puts("Error al enviar el valor.");
        	perror("send");
            exit(1);
        }

        printf("El mensaje: \"%s\", se ha enviado correctamente! \n\n",valor);
        return EXIT_SUCCESS;
    }

    int enviarValidez(int sockfd, tValidezOperacion *validez){
    	if ((send(sockfd, validez, sizeof(tValidezOperacion), 0)) <= 0) {
        	puts("Error al enviar la validez.");
        	perror("send");
            exit(1);
        }
    	puts("Se envió la validez");
        return EXIT_SUCCESS;

    }
	int enviarResultado(int socket,tResultado * resul)
	{
		if ((send(socket, resul, sizeof(tResultado), 0)) <= 0) {
        	puts("Error al enviar resultado al planificador");
        	perror("send");
            exit(1);
        }
		printf("Resultado es: %d \n",resul->tipoResultado);//Envio bien el resultado
		printf("Resultado clave: %s \n",resul->clave);
    	puts("Se envió el resultado al planificador");
        return EXIT_SUCCESS;
	}
